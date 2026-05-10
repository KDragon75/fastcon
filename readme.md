# FastConSimple

A simple Arduino / ESP32 library for pairing and controlling **FastCon / brMesh BLE advertisement lights**.

This library was built from practical reverse engineering of BroadLink/FastCon-style BLE lights using:

- Android `logcat`
- ESP32 BLE advertisement sniffing
- confirmed ESP32-based control packets
- confirmed known-device provisioning/pairing packets

The goal is not to be a perfect protocol implementation yet. The goal is to provide a small, usable library that can:

- Pair a known FastCon light during its boot/pairing window
- Save the mesh key and light ID
- Control power, brightness, RGB color, and basic white-like output
- Act as the FastCon control layer for a Zigbee/Hue proxy project

---

## Project Status

Working:

- Known-device pairing / provisioning
- On / off
- Brightness
- RGB color
- Basic white-like command
- Saved config using ESP32 Preferences/NVS
- Headless pairing button example
- BLE advertisement-based command sending

Partially mapped:

- Native white / CCT fields
- Native rainbow / effect packets
- Pairing success confirmation packet

Not currently supported:

- Automatic discovery-to-pairing workflow without a known device ID
- Reliable electronic pairing ACK detection
- Full native effect mapping
- Standard Bluetooth Mesh / brLight devices

---

## Compatibility

This library is for **FastCon / brMesh** lights.

It is **not** for:

- Standard Bluetooth Mesh bulbs
- `brLight` devices
- Zigbee bulbs
- Wi-Fi bulbs
- Normal BLE GATT lights

Some apps and device families look similar but use different protocols. FastCon / brMesh traffic appears as BLE manufacturer advertisements, commonly beginning with:

```text
F0 FF
```

---

## Repository Layout

```text
FastConSimple/
  library.properties
  README.md

  src/
    FastConSimple.h
    FastConSimple.cpp
    FastConProtocol.h
    FastConProtocol.cpp

  examples/
    BasicControl/
      BasicControl.ino

    PairKnownDevice/
      PairKnownDevice.ino

    HeadlessButtonPairing/
      HeadlessButtonPairing.ino
```

---

## Installation

Copy the `FastConSimple` folder into your Arduino libraries folder.

Typical location:

```text
Documents/Arduino/libraries/FastConSimple
```

Then restart Arduino IDE.

Include it in your sketch:

```cpp
#include <FastConSimple.h>
```

---

## Known Working Pairing Workflow

Pairing works during the bulb's short boot/pairing window.

Physical workflow:

```text
1. Configure the desired mesh key, device ID, light ID, and group ID.
2. Start pairKnownDevice().
3. Immediately power on or reset the bulb.
4. If the bulb blinks, provisioning was accepted.
5. When the pairing loop finishes, try normal control.
```

There is no decoded electronic ACK yet. The bulb blink is currently the practical success indicator.

---

## Confirmed Provision Payload

The working provision payload format is:

```text
device_id[6] light_id group_id new_mesh_key[4]
```

Example from a successful capture:

```text
E8 70 72 88 9D D4 01 01 32 39 30 37
```

Field breakdown:

| Bytes | Meaning |
|---|---|
| `E8 70 72 88 9D D4` | Device ID |
| `01` | Light ID |
| `01` | Group ID |
| `32 39 30 37` | New mesh key, ASCII `"2907"` |

Provision command metadata:

| Field | Value |
|---|---|
| Command type | `2` |
| Key | `5E 36 7B C4` |
| Forward flag | `false` |

---

## Basic Pairing Example

```cpp
#include <Arduino.h>
#include <FastConSimple.h>

FastConSimple light;

// From confirmed pairing capture.
uint8_t deviceId[6] = {
  0xE8, 0x70, 0x72, 0x88, 0x9D, 0xD4
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  light.begin("FastConPair");
  light.loadConfig();

  light.setDeviceId(deviceId);
  light.setLightId(1);
  light.setGroupId(1);

  // Four ASCII digits are known to work as a mesh key.
  light.setMeshKeyAscii("2907");

  light.saveConfig();

  Serial.println("Starting pairing. Power on/reset the bulb now.");

  // Sends the known-good provision packet repeatedly for 15 seconds.
  light.pairKnownDevice(15000, 250);

  Serial.println("Pairing loop finished. If the bulb blinked, try control.");
}

void loop() {
}
```

---

## Basic Control Example

```cpp
#include <Arduino.h>
#include <FastConSimple.h>

FastConSimple light;

void setup() {
  Serial.begin(115200);
  delay(1000);

  light.begin("FastConControl");
  light.loadConfig();

  // Use your saved/provisioned values.
  light.setMeshKeyAscii("2907");
  light.setLightId(1);

  light.on();
  delay(500);

  light.setBrightness(64);
  delay(500);

  light.setRGB(255, 0, 0); // red
  delay(1000);

  light.setRGB(0, 255, 0); // green
  delay(1000);

  light.setRGB(0, 0, 255); // blue
  delay(1000);

  light.off();
}

void loop() {
}
```

---

## API Reference

### Setup

```cpp
bool begin(const char* bleName = "FastConSimple");
void setDebug(bool enabled);
```

### Persistent Config

```cpp
bool loadConfig(const char* prefsNamespace = "fastcon");
bool saveConfig();
bool clearConfig();
```

Saved fields:

- Mesh key
- Device ID
- Light ID
- Group ID
- Brightness

### Config Setters

```cpp
void setMeshKey(const uint8_t key[4]);
bool setMeshKeyHex(const String& hex);
bool setMeshKeyAscii(const char ascii4[5]);

void setDeviceId(const uint8_t deviceId[6]);
bool setDeviceIdHex(const String& hex);

void setLightId(uint8_t lightId);
void setGroupId(uint8_t groupId);
```

Examples:

```cpp
light.setMeshKeyAscii("2907");
light.setMeshKeyHex("32393037");

light.setDeviceIdHex("e87072889dd4");

light.setLightId(1);
light.setGroupId(1);
```

### Pairing

```cpp
bool pairKnownDevice(uint32_t durationMs = 15000, uint32_t intervalMs = 250);
bool sendProvisionOnce();
bool sendWakeBurst(uint8_t count = 5, uint16_t gapMs = 150);
```

`pairKnownDevice()` sends provision packets repeatedly.

It returns `true` if provision packets were sent. It does **not** prove the bulb accepted pairing.

### Control

```cpp
bool on();
bool off();
bool setPower(bool enabled);

bool setBrightness(uint8_t brightness127);
bool setRGB(uint8_t red, uint8_t green, uint8_t blue);
bool setWhite(uint8_t whiteA = 0x7F, uint8_t whiteB = 0x7F);
bool setWhite(uint8_t brightness127, uint8_t whiteA, uint8_t whiteB);
```

Brightness range:

```text
0..127
```

RGB example:

```cpp
light.setRGB(255, 0, 0); // red
```

White-like command:

```cpp
light.setWhite();
```

White/CCT is not fully mapped yet. This sends the observed white-like payload using bytes 6 and 7 of the color command.

---

## Headless Pairing Example

The `HeadlessButtonPairing` example uses:

| Hardware | Pin |
|---|---:|
| Button | GPIO 9 |
| Status LED | GPIO 15 |
| Optional ARGB LED | GPIO 8 |

Button behavior:

| Action | Behavior |
|---|---|
| Hold 2 seconds, release | Start 15-second pairing loop |
| Hold 10 seconds, release | Clear saved config and reboot |

Pairing behavior:

```text
1. Hold button for 2 seconds and release.
2. Immediately power on/reset the bulb.
3. Status LED blinks during the provision loop.
4. If the bulb blinks, provisioning was accepted.
5. Try normal control after the loop ends.
```

---

## Confirmed Control Payloads

### Power / Brightness

Payload format:

```text
22 light_id brightness 00 00 00 00 00
```

Examples:

| Action | Payload |
|---|---|
| Off | `22 01 00 00 00 00 00 00` |
| App-style On | `22 01 80 00 00 00 00 00` |
| Lowest brightness | `22 01 01 00 00 00 00 00` |
| Highest brightness | `22 01 7F 00 00 00 00 00` |

### RGB

Payload format:

```text
72 light_id (80 | brightness) blue red green 00 00
```

Important: the color order is **blue, red, green**.

Examples:

| Color | Payload |
|---|---|
| Red | `72 01 E5 00 FF 00 00 00` |
| Blue | `72 01 E5 FF 00 00 00 00` |
| Green | `72 01 E5 00 00 FF 00 00` |

---

## White / CCT Notes

Observed white-like app payload:

```text
72 01 E5 00 00 00 7F 7F
```

Working field guess:

| Offset | Meaning |
|---:|---|
| 6 | White / CCT candidate A |
| 7 | White / CCT candidate B |

This is supported as:

```cpp
light.setWhite();
light.setWhite(100, 0x7F, 0x7F);
```

This is not a complete CCT implementation yet.

---

## Effects / Rainbow

Native FastCon rainbow/effect packets have been observed in the `type 5` packet family, but the byte fields are not decoded yet.

For now, use ESP-side RGB animation if you need a rainbow effect:

```cpp
for (int hue = 0; hue < 255; hue++) {
  // Convert HSV to RGB in your sketch, then:
  light.setRGB(r, g, b);
  delay(100);
}
```

Do not send animation updates too fast. BLE advertisement control can drop packets if spammed.

Recommended interval:

```text
100–250 ms
```

---

## Protocol Summary

FastCon commands are BLE manufacturer advertisements.

Observed BLE manufacturer prefix:

```text
F0 FF
```

Common encoded packet families:

| Pattern | Meaning |
|---|---|
| `F0FF6DB64368931D0D...` | Scan / wake |
| `F0FF6DB64368931D2D...` | Provision / add |
| `F0FF6DB64368931DDD...` | Type 5 command family |
| `F0FF4E...5E367BC4` | Unpaired identity advertisement |

Default pairing key:

```text
5E 36 7B C4
```

Typical app-style mesh key:

```text
ASCII four digits, such as "2907"
```

---

## Limitations

Current limitations:

- No automatic discovery-to-pairing workflow yet
- No decoded electronic pairing ACK yet
- Native CCT not fully mapped
- Native rainbow/effects not fully mapped
- Group behavior is not fully mapped
- Pairing requires known 6-byte device ID

Practical workaround:

- Use Android app / sniffer / logcat to get the device ID.
- Use `pairKnownDevice()`.
- Use the bulb blink as the human-visible pairing success indicator.

---

## Troubleshooting

### Pairing does nothing

Pairing only works during the bulb boot/pairing window.

Try:

```text
1. Start pairKnownDevice().
2. Immediately power on/reset the bulb.
3. Watch for bulb blink.
```

### Pairing seems to work, but control does not

Check:

- Mesh key matches the provisioned key
- Light ID is correct
- Bulb visibly blinked during pairing
- ESP is close enough to the bulb
- BLE and Zigbee/Wi-Fi coexistence is not overwhelming the radio

### RGB colors are wrong

FastCon uses byte order:

```text
blue, red, green
```

not RGB.

### Brightness is weird

FastCon brightness is `0..127`, not `0..255`.

RGB color brightness uses:

```text
0x80 | brightness
```

---

## Notes for Zigbee Proxy Use

Use this library as the FastCon side of a Zigbee color-light proxy.

Recommended mapping:

| Zigbee command | FastConSimple call |
|---|---|
| On | `light.on()` |
| Off | `light.off()` |
| Level | `light.setBrightness(mappedLevel)` |
| RGB / HSV / XY | Convert to RGB, then `light.setRGB(r, g, b)` |
| Color temperature | Approximate to RGB or use experimental `setWhite()` |

The Zigbee endpoint should remain separate. This library only controls the FastCon BLE side.
