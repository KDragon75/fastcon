# FastCon / brMesh Protocol Notes

> Working notes from ESP32 BLE sniffing, Android `logcat`, and successful ESP-based control/pairing tests.
>
> This is a reverse-engineering reference, not an official protocol specification.

---

## 1. Scope

This document covers the parts of the FastCon / brMesh BLE advertisement protocol that have been confirmed or strongly inferred during testing:

- BLE advertisement wrapper
- FastCon RF payload encoding/decoding
- scan/wake packets
- unpaired bulb identity advertisements
- provision/add-light packets
- normal power/brightness/RGB control packets
- partial white/CCT observations
- pairing workflow
- current unknowns

The practical project goal is:

```text
Hue / Zigbee coordinator
  -> ESP Zigbee color-light endpoint
  -> FastCon proxy layer
  -> FastCon BLE advertisement commands
  -> FastCon / brMesh bulb
```

This is a **Zigbee color-light proxy**, not a real ZLL bridge.

---

## 2. BLE Advertisement Wrapper

FastCon packets are sent as BLE manufacturer advertisements.

Observed manufacturer data format:

```text
F0 FF <FastCon RF payload>
```

Where:

| Bytes | Meaning |
|---|---|
| `F0 FF` | FastCon/BroadLink manufacturer marker as exposed by ESP32 BLE stack |
| remaining bytes | FastCon RF payload |

When exposed as full raw BLE advertisement data, the packet often appears as:

```text
02 01 02 1B FF F0 FF <FastCon RF payload>
```

Typical raw BLE advertisement structure:

| Bytes | Meaning |
|---|---|
| `02 01 02` | BLE flags AD structure |
| `1B` | AD length |
| `FF` | Manufacturer-specific AD type |
| `F0 FF` | FastCon manufacturer marker |
| remaining | FastCon RF payload |

Example:

```text
Manufacturer data:
F0FF6DB64368931D0D8A5CF0C63E5F0F65CA0A67AA63130BB37C

Raw advertisement:
0201021BFFF0FF6DB64368931D0D8A5CF0C63E5F0F65CA0A67AA63130BB37C
```

---

## 3. RF Payload Prefix Families

Most encoded FastCon command packets observed so far begin with:

```text
F0 FF 6D B6 43 68 93 1D ...
```

After stripping `F0 FF`, the RF payload begins:

```text
6D B6 43 68 93 1D ...
```

The next bytes are useful for classifying packet families.

### Confirmed / Useful Classifiers

| Pattern | Classification | Notes |
|---|---|---|
| `F0FF6DB64368931D0D...` | scan/wake | Command type `0`, 12 zero bytes, no key |
| `F0FF6DB64368931D2D...` | provision/add-light | Command type `2`, default key `5E367BC4` |
| `F0FF6DB64368931DDD...` | type-5 command / validation / normal command family | Not reliable as pairing success |
| `F0FF4E...5E367BC4` | unpaired identity advertisement | Advertised by unpaired/default bulb |

### Important Finding

`DDD` is **not** a reliable pairing-success marker.

A failed-pair capture showed a `DDD` packet even though the pairing did not succeed. A good-pair capture showed the `D2D` provision packet as the meaningful difference. Therefore:

```text
D0D = scan/wake
D2D = provision/add
DDD = type-5 command family, not success by itself
```

---

## 4. Keys

### Default Pairing Key

The default key observed during provisioning is:

```text
5E 36 7B C4
```

Used as:

```cpp
uint8_t defaultKey[4] = {0x5E, 0x36, 0x7B, 0xC4};
```

This is used for the provision/add-light command.

### App-Generated Mesh Keys

The Android app commonly uses four ASCII digits as the mesh key.

Example:

```text
key: 31343832
```

Decoded ASCII:

```text
"1482"
```

As bytes:

```cpp
uint8_t meshKey[4] = {0x31, 0x34, 0x38, 0x32};
```

Another observed/provisioned key:

```text
32393037
```

ASCII:

```text
"2907"
```

As bytes:

```cpp
uint8_t meshKey[4] = {0x32, 0x39, 0x30, 0x37};
```

### Key Behavior

- New key usually does not become usable until the bulb is power-cycled after provisioning.
- ESP should save key immediately after provisioning.
- Resetting the FastCon send sequence after changing key helps avoid requiring an ESP reboot for normal control.

---

## 5. Scan / Wake Command

### Plain Payload

```text
000000000000000000000000
```

12 bytes of zero.

### Command Metadata

| Field | Value |
|---|---|
| Command type | `0` |
| Key | empty / no key mode |
| Forward | `false` |
| Payload length | 12 bytes |

### Android Log Example

```text
getPayloadWithInnerRetry---> payload:000000000000000000000000,  key:
send---> payload:5e4784b45e367bc45e367bc45e367bc4,
calculatedPayload:6db64368931d0d745c0ec63e5f0f65ca0a67aa63130bd3a4
```

### Encoded Advertisement Class

```text
F0FF6DB64368931D0D...
```

### Notes

- The app repeatedly sends scan/wake packets during discovery/pairing.
- These packets are common in both successful and failed pairing captures.
- They are not enough to indicate the bulb was found or provisioned.

---

## 6. Unpaired Bulb Identity Advertisement

Observed from default/unpaired bulb:

```text
F0FF4E5B7A99E87072889DD4A1A85E367BC4
```

Earlier equivalent capture:

```text
F0FF4E6C7A8EE87072889DD4A1A85E367BC4
```

### Working Field Map

After `F0 FF`, payload length is 16 bytes:

```text
4E 5B 7A 99 E8 70 72 88 9D D4 A1 A8 5E 36 7B C4
```

| Offset | Bytes | Meaning | Confidence |
|---:|---|---|---|
| 0–3 | `4E 5B 7A 99` | identity/header/random/checksum-ish bytes | Unknown |
| 4–9 | `E8 70 72 88 9D D4` | device ID used in provision payload | Confirmed |
| 10–11 | `A1 A8` | unknown tail | Unknown |
| 12–15 | `5E 36 7B C4` | default key tail / default-key marker | Strong |

### Device ID

The 6-byte device ID extracted from the identity advertisement:

```text
E8 70 72 88 9D D4
```

is used directly as the first 6 bytes of the provision payload.

---

## 7. Provision / Add-Light Command

### Plain Payload Format

```text
device_id[6] light_id group_id new_mesh_key[4]
```

Total length: 12 bytes.

### Confirmed Example

```text
E8 70 72 88 9D D4 01 01 32 39 30 37
```

| Offset | Bytes | Meaning |
|---:|---|---|
| 0–5 | `E8 70 72 88 9D D4` | device ID from identity advertisement |
| 6 | `01` | assigned light ID |
| 7 | `01` | group ID |
| 8–11 | `32 39 30 37` | new mesh key, ASCII `"2907"` |

### Command Metadata

| Field | Value |
|---|---|
| Command type | `2` |
| Key | default key `5E367BC4` |
| Forward | `false` |
| Plain payload length | 12 bytes |

### Android Log Example

```text
getPayloadWithInnerRetry---> payload:e87072889dd4010132393037,  key: 5e367bc4
send---> payload:7eb4bf39b646094cc3e27ac56c0f4bf3,
calculatedPayload:6db64368931d2d8767832e4e2d87f81e0b66985a233c6ff3
```

### Encoded Advertisement Class

```text
F0FF6DB64368931D2D...
```

### Good Pairing Capture

A successful pairing capture included a `D2D` packet:

```text
F0FF6DB64368931D2DAD675B2E4E2D87F81E08669956243F2D67
```

### Failed Pairing Capture

A failed pairing capture showed repeated `D0D` scan/wake packets and no `D2D` provision packet.

---

## 8. Pairing Workflow

### Manual / Serial Workflow

```text
keyascii 2907
id 1
group 1
device e87072889dd4
pair
```

Then immediately power on or reset the bulb while pairing packets are being sent.

After the pairing loop:

```text
power-cycle bulb
on
off
rgb 255 0 0
```

### Headless Workflow

Button behavior:

| Button Action | Behavior |
|---|---|
| hold 2 seconds, release | start pairing loop for 15 seconds |
| hold 10 seconds, release | clear saved settings and reboot |

Pairing LED behavior:

| State | LED |
|---|---|
| pairing active | pin 15 blinks rapidly |
| reset armed | ARGB/status red |
| timeout | amber blink pattern |
| success | not detected yet |

### Current Limitation

There is no confirmed reliable ACK packet for provisioning success.

Best current behavior:

```text
send provision packets for 15 seconds
blink until timeout
tell user to power-cycle bulb
test normal control with saved key
```

---

## 9. Normal Control: Power / Brightness

### Command `0x22`

Plain payload layout:

```text
22 light_id brightness 00 00 00 00 00
```

Only 8 bytes are used in the command helper, though decode often represents a 12-byte body with trailing zeros.

| Offset | Meaning |
|---:|---|
| 0 | command `0x22` |
| 1 | light ID |
| 2 | brightness / power byte |
| 3–7 | zero |

### Confirmed Values

| Payload | Meaning |
|---|---|
| `22 01 00 00 00 00 00 00` | off |
| `22 01 80 00 00 00 00 00` | app-style on |
| `22 01 01 00 00 00 00 00` | lowest brightness |
| `22 01 7F 00 00 00 00 00` | highest brightness |

### Brightness Range

```text
0x01 to 0x7F
```

ESP mapping from Zigbee level:

```text
Zigbee level 0–254 -> FastCon brightness 0–127
```

---

## 10. Normal Control: RGB Color

### Command `0x72`

Plain payload layout:

```text
72 light_id brightness blue red green 00 00
```

Important: color byte order is:

```text
blue, red, green
```

not RGB.

### Brightness Byte

The app sends RGB brightness as:

```text
0x80 | brightness
```

Example:

```text
E5 = 0x80 | 0x65
```

### Confirmed Color Payloads

| Action | Payload |
|---|---|
| red | `72 01 E5 00 FF 00 00 00` |
| blue | `72 01 E5 FF 00 00 00 00` |
| green | `72 01 E5 00 00 FF 00 00` |

### Field Map

| Offset | Meaning |
|---:|---|
| 0 | command `0x72` |
| 1 | light ID |
| 2 | `0x80 | brightness` |
| 3 | blue |
| 4 | red |
| 5 | green |
| 6 | white/CCT candidate A |
| 7 | white/CCT candidate B |

### ESP Helper

```cpp
uint8_t data[8] = {
  0x72,
  lightId,
  static_cast<uint8_t>(0x80 | brightness127),
  blue,
  red,
  green,
  0x00,
  0x00
};
```

---

## 11. White / CCT Candidate

The app white setting was observed as:

```text
72 01 E5 00 00 00 7F 7F
```

Working hypothesis:

| Offset | Meaning |
|---:|---|
| 6 | white/CCT candidate A |
| 7 | white/CCT candidate B |

Current interpretation:

```text
72 01 E5 00 00 00 7F 7F
```

means:

```text
color command
light ID 1
brightness/on byte E5
RGB channels zero
white/CCT channels full/full
```

### Current Proxy Behavior

For now, the Zigbee proxy converts color temperature to approximate RGB instead of using native FastCon white/CCT fields.

### Future Work

Map warm/cool CCT by sniffing:

```text
mark white_warm
mark white_cool
mark cct_min
mark cct_mid
mark cct_max
```

Then compare bytes 6 and 7.

---

## 12. Rainbow / Effects

Native FastCon rainbow/effects are not fully mapped.

Current recommended behavior for the Zigbee proxy:

```text
implement rainbow in ESP
send repeated RGB updates
do not chase native effect payloads until basic proxy is stable
```

Suggested ESP-side rainbow speeds:

| Mode | Interval |
|---|---:|
| slow | 500 ms |
| normal | 250 ms |
| fast | 100 ms |
| minimum sane | 50 ms |

### Future Native Effect Mapping

Capture with markers:

```text
mark rainbow
mark rainbow_faster
mark rainbow_slower
mark stop_effect
```

Then compare plaintext via logcat and `F0FF` packets via sniffer.

---

## 13. FastCon Encoded Body

The Android app logs two useful forms:

```text
getPayloadWithInnerRetry---> payload:<plain payload>, key:<key>
send---> payload:<encoded body>, calculatedPayload:<rf payload>, len:24
```

Example scan/wake:

```text
plain payload:
000000000000000000000000

encoded body:
5e4784b45e367bc45e367bc45e367bc4

RF payload:
6db64368931d0d745c0ec63e5f0f65ca0a67aa63130bd3a4
```

### Encoded Body Size

```text
16 bytes
```

### RF Payload Size

```text
24 bytes
```

### Manufacturer Data Size

```text
26 bytes
```

because:

```text
F0 FF + 24-byte RF payload
```

---

## 14. Encoding Model

### Body Layout Before XOR

```text
[0] command header
[1] sequence
[2] safe key byte
[3] checksum
[4..15] payload data / padded data
```

### Header Byte

```text
bits 0..3 = subtype
bits 4..6 = command type
bit 7     = forward flag
```

### Key Handling

Header bytes `[0..3]` are XORed with the default key:

```text
5E 36 7B C4
```

Data bytes `[4..15]` are XORed with the mesh key.

For no-key scan/wake packets, the data section appears as repeated default-key bytes.

### Checksum

Checksum is the 8-bit sum of:

```text
command header
sequence
safe key byte
payload bytes used
```

excluding the checksum byte itself.

### RF Layer

The encoded 16-byte body is wrapped into a custom RF-like payload with:

- fixed RF address bytes
- CRC16
- BLE whitening
- bit-reversed preamble/address bytes

The currently used RF address is:

```cpp
C1 C2 C3
```

---

## 15. Zigbee Proxy Mapping

### Zigbee On/Off

| Zigbee command | FastCon command |
|---|---|
| off | `22 id 00 00 00 00 00 00` |
| on | `22 id 80 00 00 00 00 00` |

### Zigbee Level

| Zigbee level | FastCon |
|---|---|
| `0` | off / brightness zero |
| `1–254` | map to `1–127` |

### Zigbee Color

| Zigbee command | Proxy behavior |
|---|---|
| Hue/Saturation | convert HSV to RGB, send `0x72` |
| XY color | convert XY to RGB, send `0x72` |
| Color temperature | currently approximate as RGB |
| Identify | blink via repeated FastCon power/RGB commands |

### FastCon RGB Send

```text
72 id (80 | brightness) blue red green 00 00
```

---

## 16. Sniffer Notes

Useful sniffer modes:

```text
mode f0ff
rssi -85
raw on
dedupe off
max 8000
sniff 120
```

For dense captures, `mode all` with dedupe can catch more context, but `mode f0ff` is easier to parse.

Useful markers:

```text
mark pair_start
mark pairing_done
mark red
mark blue
mark green
mark white
mark bright_1
mark bright_100
mark off
mark on
mark rainbow
```

---

## 17. Confirmed Implementation Commands

Current bridge command concepts:

```text
show
keyascii 2907
key 32393037
id 1
group 1
device e87072889dd4
pair
pair 15
paironce
wake
scan
rssi -65
on
off
b 64
rgb 255 0 0
clearconfig
reboot
```

Saved fields:

| Setting | Purpose |
|---|---|
| mesh key | used for normal control |
| device ID | used for provisioning |
| light ID | FastCon target ID |
| group ID | provisioning group byte |
| RSSI threshold | scan filtering |

---

## 18. Current Unknowns

### Reliable Pairing Success ACK

Not confirmed. Good pairing includes a `D2D` provision packet. Failed pairing does not. But no clean ACK has been identified.

### Native White/CCT

Partially observed:

```text
72 01 E5 00 00 00 7F 7F
```

Need more warm/cool captures.

### Native Rainbow / Effects

Not mapped.

### Identity Header Bytes

In identity advertisements:

```text
4E 5B 7A 99 ...
```

The first four bytes are not fully understood.

### Unknown Tail in Identity Advertisement

Bytes after device ID and before default key:

```text
A1 A8
```

unknown.

---

## 19. Working Summary

Confirmed enough for practical bridge operation:

```text
1. Pairing works with command type 2.
2. Provision payload is:
   device_id[6] light_id group_id new_mesh_key[4]

3. Normal power/brightness works with command 0x22.
4. Normal RGB works with command 0x72.
5. RGB byte order is blue, red, green.
6. RGB brightness byte should be 0x80 | brightness.
7. Scan/wake is command type 0 with 12 zero bytes and no key.
8. New key usually requires bulb power-cycle before normal control.
```

Recommended bridge behavior:

```text
Expose Zigbee color light endpoint.
Translate Zigbee commands to FastCon commands.
Keep pairing separate.
Use button/headless pairing mode for provisioning.
Blink until timeout, because success ACK is not confirmed.
```
