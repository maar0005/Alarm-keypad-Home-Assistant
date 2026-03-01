# Alarm Keypad for Home Assistant

A physical alarm keypad built on an **ESP32-S3** running **ESPHome**, designed as an input and display peripheral for Home Assistant's built-in `alarm_control_panel`. All alarm logic — entry/exit delays, PIN validation, lockout, and notifications — lives in HA. The device handles only display and input.

> **[→ Flash pre-built firmware in your browser](https://maar0005.github.io/Alarm-keypad-Home-Assistant/flash/)**
> Chrome or Edge on desktop required.

---

## Features

- 4×4 matrix keypad with PIN arm/disarm and quick-arm shortcuts
- 1602 LCD showing live alarm state and masked code entry
- NeoPixel status LED with colour-coded alarm states
- Passive buzzer with distinct tones for each state transition
- PN532 NFC reader (13.56 MHz) — integrates with HA's native Tags system
- Parallax RFID reader (125 kHz) — integrates with HA's native Tags system
- PIR motion sensor for LCD backlight auto-wake
- BH1750 ambient light sensor for LCD auto-dim
- BME280 temperature, humidity and pressure sensor
- LiPo battery backup with voltage/percentage reporting
- Per-device random API encryption key (ESP32 hardware RNG)
- OTA firmware updates over WiFi

---

## Architecture

```
Home Assistant  ←── source of truth ──────────────────────────────────────────
alarm_control_panel                         ESPHome device (display + input)
        │                                              │
        │  alarm state (ESPHome API, encrypted)        │
        │ ─────────────────────────────────────────►  LCD + NeoPixel + Buzzer
        │                                              │
        │  ◄──────────────── alarm_control_panel.alarm_disarm(code)  keypad #
        │  ◄──────────────── alarm_control_panel.alarm_arm_away      key A
        │  ◄──────────────── alarm_control_panel.alarm_arm_home      key B
        │  ◄──────────────── alarm_control_panel.alarm_arm_night     key C
        │                                              │
        │  ◄──────────────── tag_scanned(uid)          NFC / RFID tap
        │         ↓
        │  HA Tags automation → arm / disarm
```

The device never validates codes locally and stores no alarm credentials. Entry/exit delays, wrong-code lockout, and push notifications are configured entirely in HA.

---

## Security

| Layer | Detail |
|-------|--------|
| **Per-device random key** | 32 bytes generated at first boot via `esp_fill_random()` (ESP32 hardware RNG). Stored in NVS. Every device has a unique key. |
| **Noise-protocol transport encryption** | The key is installed as the ESPHome API PSK before the server accepts connections. All traffic between the device and HA is ciphertext. |
| **HA-internal PIN verification** | Entered PINs are forwarded to HA over the encrypted channel. `alarm_control_panel` validates them against its own configured code. No PIN is stored on the device. |

---

## Hardware

### Bill of Materials

| Component | Interface | Notes |
|-----------|-----------|-------|
| Unexpected Maker FeatherS3 (ESP32-S3) | — | Main MCU |
| 1602 LCD + PCF8574 I2C backpack | I2C `0x27` | 16×2 character display |
| PN532 NFC reader (13.56 MHz) | I2C `0x24` | Bridge I0=VCC, I1=GND for I2C mode |
| Parallax RFID reader (125 kHz) | UART | RDM6300 protocol |
| 4×4 membrane keypad | 8× GPIO | |
| Passive buzzer ~85 dB | GPIO via MOSFET | |
| 2N7000 N-channel MOSFET | — | Buzzer gate driver |
| BH1750 ambient light sensor | I2C `0x23` | LCD auto-dim |
| HC-SR501 PIR motion sensor | GPIO | LCD backlight wake |
| BME280 temperature/humidity/pressure | I2C `0x76` | Environmental monitoring |
| LiPo 3.7 V 1500 mAh JST-PH 2.0 | — | Battery backup |
| 5 V USB-C power supply | — | |
| Proto PCB 5×7 cm | — | |

**Estimated component cost: ~$22–28 USD**

### GPIO Assignment

```
I2C SDA / SCL         GPIO8,  GPIO9          Shared bus (LCD, NFC, light, climate)
RFID UART RX          GPIO4                  Parallax RDM6300
RFID ENABLE           GPIO5                  Active low
Keypad rows           GPIO15, GPIO16, GPIO17, GPIO18
Keypad columns        GPIO11, GPIO12, GPIO13, GPIO14   (INPUT_PULLUP)
NeoPixel data         GPIO40                 Built-in on FeatherS3
Buzzer MOSFET gate    GPIO6                  LEDC PWM
PIR input             GPIO21
Battery ADC           GPIO2                  ÷2 voltage divider
```

### I2C Bus

```
SDA GPIO8 / SCL GPIO9
├── 0x23  BH1750 light sensor
├── 0x24  PN532 NFC reader
├── 0x27  LCD PCF8574 backpack
└── 0x76  BME280 climate sensor
```

Full wiring details: [`docs/wiring-diagram.md`](docs/wiring-diagram.md)

---

## Developer Install

For building and flashing your own device with a custom configuration.

### Prerequisites

- [ESPHome](https://esphome.io/guides/installing_esphome) ≥ 2024.x
- Python 3.9+
- USB-C cable (first flash only)

### 1. Configure Home Assistant alarm panel

Add to `configuration.yaml` if you do not already have an alarm panel:

```yaml
alarm_control_panel:
  - platform: manual
    name: "Home Alarm"
    code: "1234"
    code_arm_required: false
    arming_time: 45
    delay_time: 30
    trigger_time: 300
    disarmed:
      trigger_time: 0
    armed_home:
      arming_time: 0
      delay_time: 10
```

Restart HA and note the entity ID (e.g. `alarm_control_panel.home_alarm`).

### 2. Create secrets file

```bash
cd esphome/
cp secrets.yaml.template secrets.yaml
```

Edit `secrets.yaml`:

```yaml
wifi_ssid: "YourNetwork"
wifi_password: "YourPassword"
api_key: "<generate: python3 -c 'import secrets; print(secrets.token_urlsafe(32))'>"
ota_password: "yourpassword"
alarm_entity: "alarm_control_panel.home_alarm"
```

`secrets.yaml` is listed in `.gitignore` and is never committed.

### 3. Configure PN532 for I2C

Bridge the two solder pads on the PN532 board: **I0 = VCC (1), I1 = GND (0)**.

### 4. Wire the hardware

Follow [`docs/wiring-diagram.md`](docs/wiring-diagram.md).

### 5. First flash (USB)

Hold **BOOT**, press **RESET**, release **BOOT** to enter the FeatherS3 bootloader, then:

```bash
esphome run esphome/alarm-keypad.yaml
```

### 6. Add to Home Assistant

HA discovers the device automatically via mDNS. Go to **Settings → Devices & Services → ESPHome**, click Configure, and enter the `api_key` from `secrets.yaml`.

### 7. OTA updates

All subsequent updates can be delivered over WiFi:

```bash
esphome run esphome/alarm-keypad.yaml
```

---

## Distribution (Factory Firmware)

The factory firmware (`alarm-keypad-factory.yaml`) is designed for deploying pre-flashed devices to end users. It requires no compiler and no configuration files on the recipient's side.

### How the factory firmware differs

- **Per-device random API key** — generated at first boot via ESP32 hardware RNG; shown on the device web page as "Setup Key". No shared or hardcoded key.
- **Hotspot provisioning** — creates "Alarm Keypad Setup" open AP on first boot; captive portal for WiFi setup.
- **Improv Serial** — WiFi can also be provisioned through the browser immediately after USB flashing via [ESP Web Tools](https://esphome.github.io/esp-web-tools/).
- **Web server** — device hosts a status page at `http://alarm-keypad.local/` showing all sensor values including the Setup Key.
- **Factory reset button** — generates a new random key and clears the device state.

### Flashing

Use the browser-based flash tool (Chrome/Edge required):

**[→ https://maar0005.github.io/Alarm-keypad-Home-Assistant/flash/](https://maar0005.github.io/Alarm-keypad-Home-Assistant/flash/)**

Or flash locally:

```bash
esphome run esphome/alarm-keypad-factory.yaml
```

The CI pipeline (`build-firmware.yml`) builds and publishes the factory binary automatically on every push to `main`.

### End-user setup

The recipient performs the following steps without any programming knowledge:

**Step 1 — WiFi provisioning**

Power on the device. It creates a WiFi hotspot named **"Alarm Keypad Setup"** (open, no password). Connect a phone or laptop to that hotspot. A captive portal opens automatically — enter the home WiFi credentials and submit. The device connects and the hotspot disappears.

**Step 2 — Add to Home Assistant**

Home Assistant discovers the device automatically via mDNS. A notification appears: *"New device discovered: Alarm Keypad"*. Click **Configure**.

HA asks for the API encryption key. Open a browser and go to `http://alarm-keypad.local/` (or the device IP shown in the router). Find the **Setup Key** sensor value, copy it, and paste it into HA.

The device is now fully connected and operational.

**Step 3 — Set up NFC / RFID tags (optional)**

See [NFC and RFID Tags](#nfc-and-rfid-tags) below.

---

## Home Assistant Setup

### Alarm panel

The keypad reads the alarm panel state and forwards all arm/disarm requests to it. Ensure the `alarm_entity` substitution in the YAML matches your panel's entity ID. The default is `alarm_control_panel.home_alarm`.

To change the target entity on a deployed factory device without reflashing, update the ESPHome YAML substitution and push an OTA update.

### NFC and RFID tags

Both readers fire HA's native `tag_scanned` event directly, integrating with **Settings → Tags** without any Blueprint or custom automation template.

**Registering a tag:**

1. Tap the tag on the keypad.
2. HA shows a notification: *"Unknown tag scanned — do you want to create an automation?"*
3. Tap the notification. HA pre-fills the `platform: tag` trigger with the scanned UID.
4. Add your arm/disarm action and save.

Alternatively, register tags manually under **Settings → Tags → Add Tag** and create the automation from there.

**Example automation:**

```yaml
alias: "Keycard — toggle alarm"
trigger:
  - platform: tag
    tag_id: "04:AB:CD:EF:12:34:56"    # NFC UID  or decimal ID for RFID
action:
  - choose:
      - conditions:
          - condition: state
            entity_id: alarm_control_panel.home_alarm
            state: disarmed
        sequence:
          - service: alarm_control_panel.alarm_arm_away
            target:
              entity_id: alarm_control_panel.home_alarm
      - conditions:
          - condition: state
            entity_id: alarm_control_panel.home_alarm
            state:
              - armed_away
              - armed_home
              - armed_night
              - pending
        sequence:
          - service: alarm_control_panel.alarm_disarm
            target:
              entity_id: alarm_control_panel.home_alarm
            data:
              code: "1234"
mode: single
```

---

## Keypad Reference

### Key layout

```
[ 1 ][ 2 ][ 3 ][ A — Arm Away  ]
[ 4 ][ 5 ][ 6 ][ B — Arm Home  ]
[ 7 ][ 8 ][ 9 ][ C — Arm Night ]
[ * ][ 0 ][ # ][ D — spare     ]
```

### Key functions

| Key | Action |
|-----|--------|
| `0`–`9` | Enter digit (buffer up to 8 digits) |
| `#` | Submit — arm away if disarmed, disarm if armed |
| `*` | Clear buffer |
| `A` | Quick arm away (no PIN required unless `code_arm_required: true` in HA) |
| `B` | Quick arm home |
| `C` | Quick arm night |

Wrong codes are rejected by HA. Configure wrong-code lockout attempts and duration in the HA alarm panel, not in the device firmware.

### LCD display

| Condition | Line 1 | Line 2 |
|-----------|--------|--------|
| Not connected to HA | Device ID (6-char) | Status hint / WiFi status |
| Disarmed | `DISARMED` | Masked code entry |
| Armed | `ARMED AWAY` / `HOME` / `NIGHT` | Masked code entry |
| Exit delay | `ARMING...` | Masked code entry |
| Entry delay | `ENTRY DELAY` | Masked code entry |
| Triggered | `!! ALARM !!` | Masked code entry |
| HA message override | State | Custom HA message |

### NeoPixel status

| Alarm state | Colour / Effect |
|-------------|-----------------|
| `disarmed` | Solid green |
| `armed_away / home / night / vacation` | Solid red |
| `arming` (exit delay) | Pulsing orange |
| `pending` (entry delay) | Pulsing blue |
| `triggered` | Flashing red |
| Connecting / boot | Pulsing blue |

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| LCD shows `CONNECTING...` after HA pairing | Wrong API key | Re-enter the Setup Key from `http://alarm-keypad.local/` |
| Code entry does nothing | Wrong `alarm_entity` | Verify entity ID in ESPHome YAML matches HA panel |
| HA rejects valid code | PIN mismatch | Check `code:` in HA alarm panel `configuration.yaml` |
| NFC/RFID tap does nothing | Tag not registered | Check **Settings → Tags**; first scan triggers a setup notification |
| LCD stuck on device ID screen | WiFi not connected | Reconnect to hotspot and re-enter credentials |
| No buzzer during delays | State not transitioning | Confirm HA panel has `arming_time` and `delay_time` set |
| NeoPixel wrong colour | State mismatch | Inspect `ha_alarm_state` sensor in HA Developer Tools |

---

## Repository Structure

```
Alarm-keypad-Home-Assistant/
├── esphome/
│   ├── alarm-keypad.yaml            # Developer config — uses secrets.yaml
│   ├── alarm-keypad-factory.yaml    # Distribution firmware — no secrets required
│   ├── alarm_keypad_security.h      # Per-device key generation and NVS helpers (C++)
│   └── secrets.yaml.template        # Template — copy to secrets.yaml and fill in
├── .github/workflows/
│   └── build-firmware.yml           # CI: builds factory binary on push to main
├── docs/
│   ├── flash/
│   │   ├── index.html               # Browser flash page (GitHub Pages)
│   │   └── manifest.json            # ESP Web Tools firmware manifest
│   ├── setup-guide.md               # Extended step-by-step build guide
│   ├── wiring-diagram.md            # Full GPIO and wiring reference
│   └── bom.md                       # Bill of Materials with part numbers and prices
├── 3d-print/
│   ├── README.md                    # Print settings and assembly notes
│   ├── front-plate/                 # STL files
│   └── back-plate/                  # STL files
└── README.md
```

---

## Enclosure

3D-printed PETG sandwich design:

```
[ Front plate ] — LCD window, keypad cutout, NFC / PIR area, NeoPixel aperture
[ Components  ] — PCB, FeatherS3, wiring
[ Back plate  ] — wall-mount keyhole slots, USB-C opening, buzzer vents
```

Print settings and cutout dimensions: [`3d-print/README.md`](3d-print/README.md)

---

## License

MIT — see [LICENSE](LICENSE).
