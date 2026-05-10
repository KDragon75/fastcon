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
