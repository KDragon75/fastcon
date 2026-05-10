# FastCon / brMesh Byte Field Table

Working reverse-engineering notes for FastCon / brMesh BLE advertisement packets.

This table separates confirmed fields from fields that are still hypotheses.

---

## BLE Manufacturer Advertisement

| Offset | Size | Example | Field | Notes |
|---:|---:|---|---|---|
| 0 | 1 | `F0` | Manufacturer marker low byte | Seen by ESP BLE stack |
| 1 | 1 | `FF` | Manufacturer marker high byte | `F0FF` |
| 2..25 | 24 | `6D B6 43 ...` | FastCon RF payload | Encoded / whitened packet |

---

## FastCon RF Payload Classifier

| Manufacturer Pattern | Packet Class | Meaning |
|---|---|---|
| `F0 FF 6D B6 43 68 93 1D 0D ...` | Scan / wake | Discovery packet |
| `F0 FF 6D B6 43 68 93 1D 2D ...` | Provision / add | Add bulb to mesh |
| `F0 FF 6D B6 43 68 93 1D DD ...` | Type 5 command | Normal control / effects / validation |
| `F0 FF 4E ... 5E 36 7B C4` | Identity advert | Unpaired bulb identity |

---

## Encoded Body — 16 Bytes

| Offset | Size | Field | Notes |
|---:|---:|---|---|
| 0 | 1 | Encoded command header | XORed with default key byte `5E` |
| 1 | 1 | Encoded sequence | XORed with default key byte `36` |
| 2 | 1 | Encoded safe-key byte | XORed with default key byte `7B` |
| 3 | 1 | Encoded checksum | XORed with default key byte `C4` |
| 4..15 | 12 | Encoded payload data | XORed with mesh key, or default-key pattern for no-key scan |

---

## Decoded Body Header

| Offset | Size | Field | Bit Layout / Meaning |
|---:|---:|---|---|
| 0 | 1 | Command header | Bits `0..3` subtype, bits `4..6` command type, bit `7` forward flag |
| 1 | 1 | Sequence | Increments per send |
| 2 | 1 | Safe-key byte | Usually last key byte; `FF` for no-key scan |
| 3 | 1 | Checksum | 8-bit sum of header, sequence, safe-key byte, and used payload bytes |
| 4..15 | 12 | Plain payload | Command-specific data |

---

## Scan / Wake Plain Payload

Command metadata:

| Field | Value |
|---|---|
| Command type | `0` |
| Forward | `false` |
| Key | none / no-key mode |
| Payload length | 12 |

Payload:

| Offset | Size | Example | Field | Notes |
|---:|---:|---|---|---|
| 0..11 | 12 | `00 00 00 00 00 00 00 00 00 00 00 00` | Empty scan payload | Discovery / wake |

---

## Unpaired Identity Advertisement

Example:

```text
F0 FF 4E 5B 7A 99 E8 70 72 88 9D D4 A1 A8 5E 36 7B C4
```

Payload after `F0 FF`:

| Offset | Size | Example | Field | Confidence |
|---:|---:|---|---|---|
| 0 | 1 | `4E` | Identity header byte 0 | Unknown |
| 1 | 1 | `5B` | Identity header byte 1 | Unknown |
| 2 | 1 | `7A` | Identity header byte 2 | Unknown |
| 3 | 1 | `99` | Identity header byte 3 | Unknown |
| 4 | 1 | `E8` | Device ID byte 0 | Confirmed |
| 5 | 1 | `70` | Device ID byte 1 | Confirmed |
| 6 | 1 | `72` | Device ID byte 2 | Confirmed |
| 7 | 1 | `88` | Device ID byte 3 | Confirmed |
| 8 | 1 | `9D` | Device ID byte 4 | Confirmed |
| 9 | 1 | `D4` | Device ID byte 5 | Confirmed |
| 10 | 1 | `A1` | Unknown tail byte 0 | Unknown |
| 11 | 1 | `A8` | Unknown tail byte 1 | Unknown |
| 12 | 1 | `5E` | Default key byte 0 | Strong |
| 13 | 1 | `36` | Default key byte 1 | Strong |
| 14 | 1 | `7B` | Default key byte 2 | Strong |
| 15 | 1 | `C4` | Default key byte 3 | Strong |

---

## Provision / Add-Light Payload

Example:

```text
E8 70 72 88 9D D4 01 01 32 39 30 37
```

Command metadata:

| Field | Value |
|---|---|
| Command type | `2` |
| Forward | `false` |
| Key | `5E 36 7B C4` |
| Payload length | 12 |

Payload:

| Offset | Size | Example | Field | Notes |
|---:|---:|---|---|---|
| 0 | 1 | `E8` | Device ID byte 0 | From identity advert |
| 1 | 1 | `70` | Device ID byte 1 | From identity advert |
| 2 | 1 | `72` | Device ID byte 2 | From identity advert |
| 3 | 1 | `88` | Device ID byte 3 | From identity advert |
| 4 | 1 | `9D` | Device ID byte 4 | From identity advert |
| 5 | 1 | `D4` | Device ID byte 5 | From identity advert |
| 6 | 1 | `01` | Light ID | Assigned target ID |
| 7 | 1 | `01` | Group ID | Usually `01` |
| 8 | 1 | `32` | New mesh key byte 0 | ASCII `"2"` |
| 9 | 1 | `39` | New mesh key byte 1 | ASCII `"9"` |
| 10 | 1 | `30` | New mesh key byte 2 | ASCII `"0"` |
| 11 | 1 | `37` | New mesh key byte 3 | ASCII `"7"` |

---

## Power / Brightness Payload

Command metadata:

| Field | Value |
|---|---|
| Command type | `5` |
| Forward | `true` |
| Key | Active mesh key |
| Payload length used | 8 |

Payload:

| Offset | Size | Example | Field | Notes |
|---:|---:|---|---|---|
| 0 | 1 | `22` | Command ID | Power / brightness |
| 1 | 1 | `01` | Light ID | Target light |
| 2 | 1 | `00` / `01` / `7F` / `80` | Power / brightness byte | See value table below |
| 3 | 1 | `00` | Reserved | Zero |
| 4 | 1 | `00` | Reserved | Zero |
| 5 | 1 | `00` | Reserved | Zero |
| 6 | 1 | `00` | Reserved | Zero |
| 7 | 1 | `00` | Reserved | Zero |
| 8..11 | 4 | `00 00 00 00` | Padding | Only in 12-byte decoded body |

Brightness byte values:

| Value | Meaning |
|---|---|
| `00` | Off |
| `01` | Lowest brightness |
| `7F` | Highest brightness |
| `80` | App-style on |

---

## RGB Payload

Command metadata:

| Field | Value |
|---|---|
| Command type | `5` |
| Forward | `true` |
| Key | Active mesh key |
| Payload length used | 8 |

Payload:

| Offset | Size | Example Red | Field | Notes |
|---:|---:|---|---|---|
| 0 | 1 | `72` | Command ID | Color command |
| 1 | 1 | `01` | Light ID | Target light |
| 2 | 1 | `E5` | Brightness / on byte | `0x80 | brightness` |
| 3 | 1 | `00` | Blue | Blue channel |
| 4 | 1 | `FF` | Red | Red channel |
| 5 | 1 | `00` | Green | Green channel |
| 6 | 1 | `00` | White / CCT candidate A | Usually zero for RGB |
| 7 | 1 | `00` | White / CCT candidate B | Usually zero for RGB |
| 8..11 | 4 | `00 00 00 00` | Padding | Only in 12-byte decoded body |

Known RGB examples:

| Color | Payload |
|---|---|
| Red | `72 01 E5 00 FF 00 00 00` |
| Blue | `72 01 E5 FF 00 00 00 00` |
| Green | `72 01 E5 00 00 FF 00 00` |

---

## White / CCT Candidate Payload

Example:

```text
72 01 E5 00 00 00 7F 7F
```

| Offset | Size | Example | Field | Notes |
|---:|---:|---|---|---|
| 0 | 1 | `72` | Command ID | Color / white command |
| 1 | 1 | `01` | Light ID | Target light |
| 2 | 1 | `E5` | Brightness / on byte | `0x80 | brightness` |
| 3 | 1 | `00` | Blue | Zero in white mode |
| 4 | 1 | `00` | Red | Zero in white mode |
| 5 | 1 | `00` | Green | Zero in white mode |
| 6 | 1 | `7F` | White / CCT candidate A | Not fully mapped |
| 7 | 1 | `7F` | White / CCT candidate B | Not fully mapped |
| 8..11 | 4 | `00 00 00 00` | Padding | Only in decoded 12-byte body |

---

## Rainbow / Native Effect Candidate

Observed packets:

```text
F0FF6DB64368931DDDCB97CC233F12BD0BC84393C1605FFB40DA
F0FF6DB64368931DDDCA97CA233F11BD0BC84393C1605FFBEDFB
F0FF6DB64368931DDDD597C8233F10BD0BC84393C1605FFBC1DF
F0FF6DB64368931DDDD497C6233F17BD0BC84393C1605FFB9863
```

Known only at packet-family level:

| Offset / Pattern | Field | Notes |
|---|---|---|
| `F0 FF` | Manufacturer marker | Confirmed |
| `6D B6 43 68 93 1D` | Encoded FastCon RF prefix | Confirmed |
| `DD` region | Type-5 command family | Confirmed family, not decoded |
| Remaining bytes | Effect payload | Unknown |

Native rainbow fields are not decoded yet. Current reliable approach is ESP-side simulated rainbow using repeated RGB commands.


---

## Public-Source Additions / Cross-Checks

These rows come from public reverse-engineering projects and community notes. They mostly confirm behavior and implementation constraints rather than adding fully decoded byte offsets.

| Area | Public Source Finding | How It Affects This Table | Confidence |
|---|---|---|---|
| Protocol family | brMesh / Broadlink BLE lights are based on the Broadlink FastCon protocol | Confirms naming and that this is not standard BLE Mesh | Strong |
| Not brLight | `brLight` can look similar in the app UI, but is a different protocol and advertises as full BLE Mesh | Add a compatibility warning: this table applies to FastCon / brMesh, not brLight | Strong |
| ESP32 support | Public ESPHome component targets ESP32 and controls FastCon/brMesh lights | Confirms ESP32 BLE advertisement approach is practical | Strong |
| Supported features | Public ESPHome component lists On/Off, Brightness, RGB color, and White mode | Confirms our command classes cover the same practical feature set | Strong |
| Mesh key config | ESPHome FastCon component uses a required `mesh_key` in hex format | Confirms mesh-key-based control model | Strong |
| Advertisement timing | ESPHome FastCon config exposes `adv_interval_min`, `adv_interval_max`, `adv_duration`, `adv_gap`, and queue size | Add implementation fields for reliability/tuning, not byte fields | Strong |
| Dropped commands / transitions | Community notes mention command queueing and dropped messages when many transitional commands are sent | Supports debouncing Zigbee color/level bursts before sending FastCon commands | Strong |
| Pairing flow | BRmesh ESP32 MQTT project says: turn lights off, start ESP, turn lights on, ESP sends alive/wake, receives response, sends new key, lights respond | Confirms our observed pairing window and repeated provision-loop behavior | Strong |
| Pairing flakiness | Public project notes adding lights can be flaky and BLE advertising timing may need adjustment | Supports 15-second retry loop and configurable advertisement timing | Strong |
| App-first mesh key | brMeshMQTT notes first device added by Android app generates a unique mesh key and later devices use it | Confirms app-created mesh-key model | Strong |
| QR code key extraction | HA community thread provides AES-CBC constants for decrypting BRMesh QR export into mesh key + devices | Potential future import path for saved mesh key/device list | Medium/strong |
| White mode | ESPHome component claims White mode support, and community notes discuss RGBW/CCT behavior issues | Confirms native white mode likely exists, but byte fields remain incomplete | Medium |
| Group control | Community reports group control may act serially and miss commands with multiple lights | Supports queue/gap tuning and suggests group/broadcast behavior needs mapping | Medium |
| Native effects | ESPHome light framework supports effects generally, but FastCon native effects are not documented in public sources found | Keep rainbow/native effect bytes as observed-only until more captures are decoded | Weak |

---

## Implementation Parameters Worth Tracking

These are not protocol byte fields, but public FastCon implementations expose or discuss them and they matter for reliability.

| Parameter | Example / Default | Meaning | Source Use |
|---|---|---|---|
| `mesh_key` | `"31343832"` / `"32393037"` | Active 4-byte mesh key in hex | Required by ESPHome FastCon component |
| `adv_duration` | `50 ms`, community example `15 ms` | How long each command advertisement burst runs | Tuning reliability / speed |
| `adv_gap` | `10 ms`, community example `2 ms` | Gap between advertisement bursts | Tuning reliability / speed |
| `adv_interval_min` | `0x20`, community example `0x06` | BLE advertisement minimum interval | Tuning reliability / speed |
| `adv_interval_max` | `0x40`, community example `0x08` | BLE advertisement maximum interval | Tuning reliability / speed |
| `max_queue_size` | `100`, community example `16` | Command queue depth | Prevents flooding / dropped commands |
| transition suppression | `default_transition_length: 0s` | Avoids many intermediate color/brightness packets | Recommended by community users for reliability |
| pairing retry window | 10–15 seconds practical | Bulb only accepts provisioning immediately after power-on/reset | Confirmed by our tests and public auto-add flow |

---

## Compatibility Warning

| Device/App Type | Compatibility With This Table | Notes |
|---|---|---|
| brMesh app lights | Yes | Broadlink FastCon / brMesh family |
| BroadLink BLE mobile app lights | Likely yes | If they use FastCon/brMesh protocol |
| brLight app lights | No | Similar-looking UI, different underlying BLE Mesh protocol |
| Standard Bluetooth Mesh bulbs | No | Not FastCon/brMesh |
| Broadlink GW4C hub path | Different integration path | Can bridge BLE to cloud/Alexa/Google, but not this local ESP packet format |

---

## Public-Source-Backed To-Do Items

| To-Do | Why |
|---|---|
| Add configurable `adv_duration`, `adv_gap`, `adv_interval_min`, and `adv_interval_max` to `FastConBridge` | Public ESPHome users report advertisement timing impacts speed/reliability |
| Keep a command queue but avoid huge transitional bursts | Community reports too many transition packets cause flicker/wrong end states |
| Add optional QR import/decode utility later | Public thread shows BRMesh QR export can expose mesh key and devices |
| Keep white/CCT as its own mapping target | Public implementations support White mode, and our bytes `[6]` / `[7]` support that direction |
| Keep native effects separate from RGB proxy effects | No public byte-field mapping found for native rainbow/effects |
