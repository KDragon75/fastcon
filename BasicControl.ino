# FastConSimple

Simple Arduino library for FastCon / brMesh BLE advertisement lights.

## What works

- Known-device provisioning / pairing
- On / off
- Brightness
- RGB color
- White-like command candidate
- Preferences/NVS save/load

## Known working pairing workflow

Pairing only works during the bulb's short boot/pairing window.

```cpp
FastConSimple light;

uint8_t deviceId[6] = {0xE8, 0x70, 0x72, 0x88, 0x9D, 0xD4};
uint8_t newKey[4]  = {'2', '9', '0', '7'};

light.begin();
light.setDeviceId(deviceId);
light.setMeshKey(newKey);
light.setLightId(1);
light.setGroupId(1);
light.saveConfig();

light.pairKnownDevice(15000);
```

Physical workflow:

```text
1. Set desired key/device/light/group.
2. Start pairKnownDevice().
3. Immediately power on/reset the bulb while pairing loop is running.
4. If the bulb blinks, it accepted provisioning.
5. Control using the saved/new mesh key.
```

There is no confirmed electronic ACK decoded in the protocol yet. `pairKnownDevice()` means "sent the known-good provision packets for the requested window," not "confirmed success." The bulb's visible blink is currently the practical success indicator.

## Minimal control

```cpp
light.on();
light.setBrightness(100);       // 0-127
light.setRGB(255, 0, 0);        // red
light.off();
```

## Packet model

Provision payload:

```text
device_id[6] light_id group_id new_mesh_key[4]
```

Provision command:

```text
commandType = 2
key = 5E367BC4
forward = false
```

Control RGB payload:

```text
72 light_id (80 | brightness) blue red green 00 00
```
