# Setup Guide

Step-by-step instructions from parts to a running alarm keypad.

---

## Prerequisites

- ESPHome installed (`pip install esphome` or via Home Assistant add-on)
- Home Assistant instance on your network
- USB-C cable for initial flash
- Parts from the BOM

---

## 1. Prepare ESPHome Secrets

```bash
cd esphome/
cp secrets.yaml.template secrets.yaml
```

Edit `secrets.yaml` with your real values:

```yaml
wifi_ssid: "YourNetwork"
wifi_password: "YourPassword"
fallback_password: "changeme123"
api_key: "<generate with: python3 -c \"import secrets; print(secrets.token_urlsafe(32))\">"
ota_password: "yourpassword"
alarm_pin: "1234"      # Change this to your desired PIN
```

> **`secrets.yaml` is in `.gitignore` — it will never be committed.**

---

## 2. Configure PN532 for I2C Mode

The PN532 breakout has two jumper pads to select the interface:

| Pad | Set to | Result |
|-----|--------|--------|
| I0  | 1 (high / VCC) | I2C mode |
| I1  | 0 (low / GND)  | I2C mode |

Solder or bridge accordingly before wiring.

---

## 3. Wire Everything Up

Follow [`wiring-diagram.md`](wiring-diagram.md) for exact connections.

Quick checklist:
- [ ] I2C bus: SDA=GPIO8, SCL=GPIO9 shared by LCD, PN532, BH1750, BME280
- [ ] RFID: RX=GPIO4, ENABLE=GPIO5
- [ ] Keypad: rows GPIO15–18, columns GPIO11–14
- [ ] PIR: GPIO21
- [ ] Buzzer: MOSFET gate=GPIO6
- [ ] NeoPixel: GPIO40 (FeatherS3 built-in)
- [ ] Battery: JST-PH 2.0 connector on FeatherS3

---

## 4. First Flash (USB)

Connect FeatherS3 via USB-C. Hold BOOT button, press RESET, release BOOT.

```bash
esphome run esphome/alarm-keypad.yaml
```

ESPHome will compile and flash. On first boot the device will appear in Home
Assistant's **Integrations → ESPHome** automatically via mDNS.

---

## 5. Add to Home Assistant

1. Open HA → **Settings → Devices & Services**
2. You should see **"Alarm Keypad"** discovered — click **Configure**
3. Enter the API key from your `secrets.yaml`
4. Done — all entities appear under the device

---

## 6. Set Up the Alarm Panel in HA

The keypad exposes an arm-state text sensor and arm/disarm buttons.
To use it with the HA Alarm Control Panel:

1. Import the blueprint from `ha-blueprints/alarm-keypad-automation.yaml`
2. Create an **Alarm Control Panel** (Manual or alarm_control_panel integration)
3. Map the keypad's arm/disarm events to the alarm panel via the blueprint

---

## 7. Register RFID and NFC Tags

When a tag is tapped, the device fires a Home Assistant event:

```
esphome.alarm_keypad_nfc_tag   (NFC — PN532)
esphome.alarm_keypad_rfid_tag  (RFID — Parallax)
```

Create an automation in HA:
- **Trigger:** Event `esphome.alarm_keypad_nfc_tag` where `uid == "YOURTAG"`
- **Action:** Call `button.press` on the Disarm button entity

Use the HA logbook to find your tag's UID after first tap.

---

## 8. OTA Updates

After the first USB flash, all subsequent updates go over WiFi:

```bash
esphome run esphome/alarm-keypad.yaml
```

ESPHome will detect the device on the network and upload automatically.

---

## 9. Adjust Timings and PIN

All configurable from HA or by editing `substitutions` in `alarm-keypad.yaml`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `alarm_pin` | from secrets | Keypad disarm code |
| `lockout_attempts` | 3 | Wrong codes before lockout |
| `lockout_duration` | 30 s | Lockout duration |
| `entry_delay` | 30 s | Time to disarm after entry |
| `exit_delay` | 45 s | Time to leave after arming |

Change `alarm_pin` in `secrets.yaml` and re-flash (OTA) to update.

---

## 10. Print and Assemble Enclosure

See [`../3d-print/README.md`](../3d-print/README.md) for print settings and
assembly instructions.

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| I2C devices not found at startup | Check SDA/SCL wiring; enable I2C scan in log |
| LCD shows garbage | Verify PCF8574 address is `0x27`; check 5V supply |
| RFID tags not reading | Ensure ENABLE pin is driven LOW; check UART baud 2400 |
| NFC tags not reading | Verify PN532 I2C jumpers; check address `0x24` |
| Keypad keys wrong | Swap row/column pin assignments in YAML |
| No buzzer sound | Check MOSFET orientation and gate resistor; verify PWM output |
| Battery % jumps around | Increase ADC averaging; ensure stable 3.3V reference |
