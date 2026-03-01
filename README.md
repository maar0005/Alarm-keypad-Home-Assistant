# Alarm Keypad — Home Assistant

A DIY alarm keypad built on an **ESP32-S3 (FeatherS3)** running **ESPHome**, with native Home Assistant integration, OTA updates, and a 3D-printed PETG enclosure.

Supports arm/disarm via keypad code, RFID fob, NFC tag, and phone NFC (HA Companion app).

---

## Repository Structure

```
Alarm-keypad-Home-Assistant/
├── esphome/
│   ├── alarm-keypad.yaml          # Main ESPHome configuration
│   └── secrets.yaml.template      # Template — copy to secrets.yaml and fill in
├── 3d-print/
│   ├── README.md                  # Print settings, material notes
│   ├── front-plate/               # Front panel STL files
│   └── back-plate/                # Back panel STL files
├── docs/
│   ├── wiring-diagram.md          # Pin assignments and wiring guide
│   ├── bom.md                     # Full Bill of Materials with links
│   └── setup-guide.md             # Step-by-step build and flash guide
├── ha-blueprints/
│   └── alarm-keypad-automation.yaml  # HA automation blueprint
└── README.md
```

---

## Platform

| Item | Detail |
|------|--------|
| MCU | Unexpected Maker FeatherS3 (ESP32-S3) |
| Firmware | ESPHome |
| HA integration | Native API (no MQTT needed) |
| Updates | OTA over WiFi |
| Power | USB-C 5V + LiPo 3.7V 1500mAh backup |

---

## Components

| Component | Interface | Status |
|-----------|-----------|--------|
| FeatherS3 (ESP32-S3) | — | Have |
| Parallax RFID reader (125 kHz) | UART | Have |
| 1602 LCD + I2C backpack (PCF8574) | I2C `0x27` | Have |
| NeoPixel (built-in or external) | 1× GPIO | Have |
| PN532 NFC reader (13.56 MHz) | I2C `0x24` | Buy |
| NTAG215 NFC stickers (10-pack) | — | Buy |
| EM4100 RFID key fobs (10-pack) | — | Buy |
| 4×4 membrane keypad | 8× GPIO | Buy |
| Passive buzzer (~85 dB) | 1× GPIO via MOSFET | Buy |
| 2N7000 MOSFET | — | Buy |
| BH1750 light sensor | I2C `0x23` | Buy |
| HC-SR501 PIR motion sensor | 1× GPIO | Buy |
| BME280 temp/humidity/pressure | I2C `0x76` | Buy |
| LiPo battery 3.7V 1500mAh (JST-PH 2.0) | — | Buy |
| 5V USB-C power supply | — | Buy |
| Proto PCB 5×7 cm | — | Buy |
| Dupont wires + connectors | — | Buy |

**Estimated cost for parts to buy: ~$22–28 USD**

---

## Arm / Disarm Methods

1. **Keypad code** — 4×4 matrix; A/B/C/D act as function keys
2. **RFID fob tap** — Parallax 125 kHz reader on the panel
3. **NFC tag tap** — PN532 13.56 MHz reader on the panel
4. **Phone NFC** — HA Companion app; no extra hardware needed

---

## I2C Bus

```
SDA / SCL shared bus
├── LCD PCF8574    0x27
├── PN532 NFC      0x24
├── BH1750 light   0x23
└── BME280 climate 0x76
```

---

## GPIO Usage Summary

```
├── I2C SDA / SCL         2 pins
├── UART RX + ENABLE      2 pins  (Parallax RFID)
├── 4×4 keypad matrix     8 pins
├── NeoPixel data         1 pin
├── Buzzer (MOSFET gate)  1 pin
├── PIR sensor input      1 pin
└── Total                15 pins  — FeatherS3 handles it fine
```

See [`docs/wiring-diagram.md`](docs/wiring-diagram.md) for exact pin numbers.

---

## Features

### Hardware-driven
- Keypad entry with audible feedback per key press
- Dual-reader NFC (13.56 MHz) + RFID (125 kHz) tag scanning
- NeoPixel status LED — red / green / blue / pulsing per arm state
- 1602 LCD — shows status, countdown, prompts; text settable from HA
- Passive buzzer with distinct tones per event
- PIR motion detection
- Temperature, humidity, pressure (BME280)
- Ambient light level (BH1750)
- Battery backup with auto USB-C charging

### Software (ESPHome / Home Assistant — no reflashing needed)
- Entry / exit countdown with accelerating beeps
- Wrong-code lockout + HA push notification
- Battery percentage monitoring
- WiFi signal strength reporting
- LCD auto-dim based on ambient light (BH1750)
- LCD wake-on-motion (PIR)
- Uptime and boot status sensors
- All thresholds and messages configurable from HA UI

---

## Enclosure

**3D-printed PETG**, sandwich design:

```
[ Front plate ]  — cutouts: LCD, keypad, NFC scan area, PIR lens, NeoPixel window
[ Components  ]  — PCB, wiring
[ Back plate  ]  — wall mount slots, USB-C access port, buzzer vents
```

See [`3d-print/README.md`](3d-print/README.md) for print settings and assembly notes.

---

## Quick Start

1. Clone this repo
2. Copy `esphome/secrets.yaml.template` → `esphome/secrets.yaml` and fill in your credentials
3. Flash with `esphome run esphome/alarm-keypad.yaml`
4. Add the device in Home Assistant (auto-discovered via mDNS)
5. Import the blueprint from `ha-blueprints/` to set up automations

Full instructions in [`docs/setup-guide.md`](docs/setup-guide.md).

---

## License

MIT — see [LICENSE](LICENSE).
