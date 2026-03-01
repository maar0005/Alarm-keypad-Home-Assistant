# Alarm Keypad — Home Assistant

A DIY alarm keypad built on an **ESP32-S3 (FeatherS3)** running **ESPHome**,
designed as a physical input/display peripheral for **Home Assistant's built-in
alarm control panel**.

The ESP32 owns no alarm logic. It mirrors HA's alarm state on the LCD and
NeoPixel, forwards keypad codes and RFID/NFC scans to HA, and reacts to
whatever state HA sends back — entry/exit delays, lockout, and notifications
are all configured in HA.

---

## How It Works

```
HA Alarm Control Panel  ←── source of truth
         │
         │  state pushed to ESP (ESPHome API)
         ▼
   LCD + NeoPixel + Buzzer   (display)
         │
   Keypad / RFID / NFC       (input)
         │
         └──► alarm_control_panel.alarm_disarm / arm_away / arm_home …
                        sent back to HA as service calls
```

Arm/disarm delays, wrong-code lockout, push notifications — all in HA, zero
reflashing needed.

---

## Install (for distribution — friends never touch a compiler)

> **[→ Flash firmware in your browser](https://maar0005.github.io/Alarm-keypad-Home-Assistant/flash/)**
> Requires Chrome or Edge on desktop.

After flashing, friends set up WiFi via the captive portal hotspot, then add the device in HA. See the [Friend Setup](#friend-setup) section below.

---

## Security Model

| Layer | What it does |
|-------|-------------|
| **Per-device random API key** | 32 bytes from ESP32 hardware RNG, generated at first boot and stored in NVS. Every device has a different key. |
| **Noise-protocol transport encryption** | The key is installed as the ESPHome API PSK before the server starts. All HA ↔ device traffic is ciphertext on the LAN. |
| **HA-internal PIN verification** | The entered PIN is sent over the encrypted channel; HA's `alarm_control_panel` verifies it against its own configured code. The PIN is never stored on the device. |

```
First boot:
  esp_fill_random() → 32 bytes → base64 → stored in NVS
  → shown as "Setup Key" on http://alarm-keypad.local/

Keypad entry:  1 2 3 4 #
  → alarm_control_panel.alarm_disarm(code="1234")   ← over encrypted API
  → HA verifies code internally → disarm or reject

NFC / RFID scan:
  → homeassistant.tag_scanned(uid)
  → HA Tags system handles it — no Blueprint needed
```

---

## Friend Setup

Friends receive a pre-flashed device. No programming skills required.

```
1. Power on → connects to "Alarm Keypad Setup" WiFi hotspot
2. Phone captive portal opens → enter home WiFi → device connects
3. HA discovers device → asks for API encryption key
   → open http://alarm-keypad.local/
   → copy the "Setup Key" value → paste into HA → done
```

The keypad is immediately operational. No Blueprint, no automation, no pairing code.

---

## Repository Structure

```
Alarm-keypad-Home-Assistant/
├── esphome/
│   ├── alarm-keypad.yaml            # ESPHome config (your own install, uses secrets.yaml)
│   ├── alarm-keypad-factory.yaml    # Pre-built distribution firmware
│   ├── alarm_keypad_security.h      # Key generation + NVS helpers (C++, used by factory fw)
│   └── secrets.yaml.template        # Copy → secrets.yaml and fill in
├── .github/workflows/
│   └── build-firmware.yml           # Auto-builds factory binary on push to main
├── docs/
│   ├── flash/
│   │   ├── index.html               # Web flash page (GitHub Pages)
│   │   └── manifest.json            # ESP Web Tools firmware manifest
│   ├── wiring-diagram.md            # Full pin reference with ASCII diagrams
│   ├── bom.md                       # Bill of Materials with prices
│   └── setup-guide.md               # Step-by-step build and integration guide
├── 3d-print/
│   ├── README.md                    # Print settings, cutout dimensions
│   ├── front-plate/                 # STL files (added when designed)
│   └── back-plate/                  # STL files (added when designed)
└── README.md
```

---

## Platform

| Item | Detail |
|------|--------|
| MCU | Unexpected Maker FeatherS3 (ESP32-S3) |
| Firmware | ESPHome |
| HA integration | Native API — no MQTT needed |
| Alarm logic | HA `alarm_control_panel` (manual or any platform) |
| Updates | OTA over WiFi |
| Power | USB-C 5V + LiPo 3.7V 1500mAh backup |

---

## Components

| Component | Interface | Status |
|-----------|-----------|--------|
| FeatherS3 (ESP32-S3) | — | Have |
| Parallax RFID reader (125 kHz) | UART | Have |
| 1602 LCD + I2C backpack (PCF8574) | I2C `0x27` | Have |
| NeoPixel (built-in, GPIO40) | GPIO | Have |
| PN532 NFC reader (13.56 MHz) | I2C `0x24` | Buy |
| NTAG215 NFC stickers (10-pack) | — | Buy |
| EM4100 RFID key fobs (10-pack) | — | Buy |
| 4×4 membrane keypad | 8× GPIO | Buy |
| Passive buzzer (~85 dB) | GPIO via MOSFET | Buy |
| 2N7000 MOSFET | — | Buy |
| BH1750 light sensor | I2C `0x23` | Buy |
| HC-SR501 PIR motion sensor | GPIO | Buy |
| BME280 temp/humidity/pressure | I2C `0x76` | Buy |
| LiPo 3.7V 1500mAh JST-PH 2.0 | — | Buy |
| 5V USB-C power supply | — | Buy |
| Proto PCB 5×7 cm | — | Buy |
| Dupont wires | — | Buy |

**Estimated cost for parts to buy: ~$22–28 USD**

---

## Arm / Disarm Methods

| Method | How |
|--------|-----|
| Keypad code + `#` | Sends code to HA; HA validates and arms/disarms |
| `A` key | Quick arm away (no code if `code_arm_required: false` in HA) |
| `B` key | Quick arm home |
| `C` key | Quick arm night |
| RFID fob tap | HA native tag_scanned → automation → arm/disarm |
| NFC tag tap | HA native tag_scanned → automation → arm/disarm |
| Phone NFC | HA Companion app — no extra hardware |

---

## NeoPixel State Colours

| HA Alarm State | Colour / Effect |
|----------------|-----------------|
| `disarmed` | Solid green |
| `armed_away / home / night` | Solid red |
| `arming` (exit delay) | Pulsing orange |
| `pending` (entry delay) | Pulsing blue |
| `triggered` | Flashing red |
| Connecting / unavailable | Pulsing blue (boot) |

---

## I2C Bus

```
SDA GPIO8 / SCL GPIO9 (shared)
├── LCD PCF8574    0x27
├── PN532 NFC      0x24
├── BH1750 light   0x23
└── BME280 climate 0x76
```

---

## GPIO Summary

```
I2C SDA / SCL         2 pins  (GPIO8, GPIO9)
UART RX + ENABLE      2 pins  (GPIO4, GPIO5)  — Parallax RFID
4×4 keypad matrix     8 pins  (GPIO11–18)
NeoPixel data         1 pin   (GPIO40)
Buzzer MOSFET gate    1 pin   (GPIO6)
PIR sensor input      1 pin   (GPIO21)
Battery ADC           1 pin   (GPIO2)
──────────────────────────────────────────
Total                15 GPIO  — FeatherS3 handles it fine
```

See [`docs/wiring-diagram.md`](docs/wiring-diagram.md) for full details.

---

## Quick Start

```bash
# 1. Clone
git clone https://github.com/maar0005/Alarm-keypad-Home-Assistant
cd Alarm-keypad-Home-Assistant

# 2. Configure secrets
cp esphome/secrets.yaml.template esphome/secrets.yaml
# Edit secrets.yaml — set alarm_entity to your HA alarm panel entity ID

# 3. Flash (USB first time)
esphome run esphome/alarm-keypad.yaml

# 4. Add to HA — auto-discovered via mDNS

# 5. Future updates — OTA
esphome run esphome/alarm-keypad.yaml
```

Full instructions: [`docs/setup-guide.md`](docs/setup-guide.md)

---

## Enclosure

3D-printed PETG, sandwich design:

```
[ Front plate ] — LCD window, keypad slot, NFC/PIR area, NeoPixel dot
[ Components  ] — PCB, wiring
[ Back plate  ] — wall-mount keyholes, USB-C slot, buzzer vents
```

See [`3d-print/README.md`](3d-print/README.md).

---

## License

MIT — see [LICENSE](LICENSE).
