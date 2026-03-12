# Setup Guide

Step-by-step instructions from parts to a running alarm keypad integrated
with Home Assistant's built-in alarm panel.

---

## Architecture

```
HA Alarm Control Panel  ←─── source of truth ───→  ESPHome keypad
   (all logic lives here)                           (display + input only)
         │                                                  │
         │  state push (API)      ──────────────────────►  LCD + NeoPixel
         │                                                  │
         │  ◄── alarm_control_panel.alarm_disarm ──────────  keypad code entry
         │  ◄── alarm_control_panel.alarm_arm_away ─────────  A key / code
         │  ◄── alarm_control_panel.alarm_arm_home ─────────  B key / code
         │  ◄── alarm_control_panel.alarm_arm_night ────────  C key / code
         │                                                  │
         │  ◄── HA automation (blueprint) ─────────────────  RFID / NFC tap
```

The ESP32 never validates codes locally. It forwards everything to HA and reacts
to whatever state HA reports back. Entry/exit delays, lockout, and notifications
are all configured in the HA alarm panel — no reflashing needed.

---

## 1. Create the HA Alarm Control Panel

If you don't already have one, add a manual alarm panel to `configuration.yaml`:

```yaml
alarm_control_panel:
  - platform: manual
    name: "Home Alarm"
    # Arm/disarm code — also set in esphome/secrets.yaml as alarm_entity
    code: "1234"
    code_arm_required: false   # set true to require code for arming too
    # Entry/exit delays (seconds)
    arming_time: 45
    delay_time: 30
    # Trigger duration before auto-disarm (0 = never)
    trigger_time: 300
    disarmed:
      trigger_time: 0
    armed_home:
      arming_time: 0
      delay_time: 10
```

Restart HA, then find the entity ID:
**Settings → Devices & Services → Entities** — search "alarm".
Copy the entity ID (e.g. `alarm_control_panel.home_alarm`).

---

## 2. Prepare ESPHome Secrets

```bash
cd esphome/
cp secrets.yaml.template secrets.yaml
```

Edit `secrets.yaml`:

```yaml
wifi_ssid: "YourNetwork"
wifi_password: "YourPassword"
fallback_password: "changeme123"
api_key: "<generate: python3 -c \"import secrets; print(secrets.token_urlsafe(32))\">"
ota_password: "yourpassword"
alarm_entity: "alarm_control_panel.home_alarm"   # ← your entity ID here
```

> **`secrets.yaml` is in `.gitignore` — never committed.**

---

## 3. Configure PN532 for I2C Mode

Bridge the jumper pads on the PN532 board before wiring:

| Pad | Set to | Result |
|-----|--------|--------|
| I0  | 1 (VCC) | I2C selected |
| I1  | 0 (GND) | I2C selected |

---

## 4. Wire Everything Up

Follow [`wiring-diagram.md`](wiring-diagram.md). Quick checklist:

- [ ] I2C: SDA=GPIO8, SCL=GPIO9 — shared by LCD, PN532, BH1750, BME280
- [ ] RFID: RX=GPIO4, ENABLE=GPIO5
- [ ] Keypad: rows GPIO15–18, cols GPIO11–14
- [ ] PIR: GPIO21
- [ ] Buzzer MOSFET gate: GPIO6
- [ ] NeoPixel: GPIO40 (FeatherS3 built-in)
- [ ] Battery: JST-PH 2.0 on FeatherS3

---

## 5. First Flash (USB)

Hold BOOT, press RESET, release BOOT on the FeatherS3 to enter bootloader.

```bash
esphome run esphome/alarm-keypad.yaml
```

---

## 6. Add to Home Assistant

HA discovers the device automatically via mDNS:

1. **Settings → Devices & Services → ESPHome** — click Configure
2. Enter the `api_key` from your `secrets.yaml`
3. Done — all entities appear under "Alarm Keypad"

---

## 7. Verify State Sync

- In HA, manually arm the alarm panel
- The keypad LCD should change to `ARMED AWAY` and NeoPixel turns **solid red**
- Disarm from HA — LCD shows `DISARMED`, NeoPixel turns **solid green**

---

## 8. Register RFID and NFC Tags

### Find your tag UID / ID

Tap a tag on the keypad. In HA go to **Settings → Logbook** and filter for
`esphome.alarm_keypad_nfc_tag` or `esphome.alarm_keypad_rfid_tag`. Copy the
`uid` or `tag_id` value from the event data.

### Import the blueprint

1. **Settings → Automations → Blueprints → Import Blueprint**
2. The blueprint file is at `ha-blueprints/alarm-keypad-automation.yaml`
   — import it from your local repo or paste the raw content
3. Create an automation from the blueprint:
   - Select your alarm panel entity
   - Enter your disarm code
   - Paste the allowed UIDs/tag IDs
   - Choose arm mode (away / home / night)

Each tag toggles: tap when disarmed → arms; tap when armed → disarms.

---

## 9. Keypad Usage

| Key | Action |
|-----|--------|
| `0–9` | Enter digits |
| `#` | Submit code → arm away (if disarmed) or disarm (if armed) |
| `*` | Clear entered digits |
| `A` | Quick arm away (no code; respects HA `code_arm_required`) |
| `B` | Quick arm home |
| `C` | Quick arm night |

Wrong codes are rejected by HA — configure lockout attempts and duration in the
HA alarm panel config, not in the ESPHome YAML.

---

## 10. NeoPixel Colour via HA Template Sensor

The keypad subscribes to `sensor.alarm_led_farve` in HA.  Whenever that
sensor's state changes to an `"R,G,B"` string the NeoPixel is updated
immediately (any active pulse/flash effect is cancelled first).

Add the following to your `configuration.yaml` (or an included file):

```yaml
template:
  - trigger:
      # Re-evaluate every second — drives the 1 Hz blink for pending/arming.
      - platform: time_pattern
        seconds: "/1"
    sensor:
      - name: "Alarm LED Farve"
        unique_id: alarm_led_farve
        state: >-
          {# ── 1. Check for low battery across all door/window sensors ── #}
          {% set ns = namespace(lav_batteri=false) %}
          {% set alle_sensorer = expand('binary_sensor.vinduer', 'binary_sensor.dorene') %}
          {% for entity in alle_sensorer %}
            {% set bat_id = entity.entity_id
                | replace('binary_sensor.', 'sensor.')
                | replace('_contact', '_battery') %}
            {% if states(bat_id) | float(100) < 30 %}
              {% set ns.lav_batteri = true %}
            {% endif %}
          {% endfor %}
          {# ── 2. Map alarm state → R,G,B ── #}
          {# Note: 'triggered' is intentionally omitted — ESPHome handles   #}
          {# the fast Alarm Flash effect (150 ms) directly on the device.    #}
          {% set status = states('alarm_control_panel.home_alarm') %}
          {% if status == 'pending' %}
            {% if now().second % 2 == 0 %} 0,0,0 {% else %} 255,192,0 {% endif %}
          {% elif status == 'arming' %}
            {% if now().second % 2 == 0 %} 0,0,0 {% else %} 0,0,255 {% endif %}
          {% elif status in ['armed_away', 'armed_home', 'armed_night', 'armed_vacation'] %}
            255,0,0
          {% elif ns.lav_batteri %}
            255,191,0
          {% elif status == 'disarmed' %}
            0,255,0
          {% else %}
            255,255,255
          {% endif %}
```

**Colour map:**

| State | Effect | Colours |
|---|---|---|
| `triggered` | Fast blink ~7 Hz (ESPHome) | red ↔ off |
| `pending` (entry delay) | 1 Hz blink (HA) | off ↔ amber |
| `arming` (exit delay) | 1 Hz blink (HA) | off ↔ blue |
| `armed_*` | Solid | red |
| Disarmed + low battery (<30 %) | Solid | amber |
| `disarmed` | Solid | green |
| Unknown | Solid | white |

> `triggered` is handled entirely by ESPHome's built-in "Alarm Flash" effect
> (150 ms on/off ≈ 7 Hz) — HA's 1 Hz sensor update is ignored by the device
> while in this state, so the fast flash is never interrupted.

> **Note:** replace `binary_sensor.vinduer` and `binary_sensor.dorene` with
> your own door/window group entity IDs, and adjust the `_contact` →
> `_battery` name pattern to match your sensor naming convention.

After adding the sensor, restart HA to create the entity.

---

## 11. OTA Updates

After the first USB flash, all future updates go over WiFi:

```bash
git pull origin main          # get latest changes
# edit esphome/alarm-keypad.yaml or secrets.yaml as needed
esphome run esphome/alarm-keypad.yaml   # compiles + uploads OTA
```

The NeoPixel pulses blue on boot to confirm the update landed.

---

## 12. Print and Assemble Enclosure

See [`../3d-print/README.md`](../3d-print/README.md).

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| LCD stuck on `CONNECTING...` | HA API not connected; check `api_key` in secrets |
| LCD shows state but keypad does nothing | Check `alarm_entity` in secrets matches real entity ID |
| HA rejects code | Code mismatch; check `code:` in HA alarm panel config |
| RFID/NFC tap does nothing | Check logbook for event; add UID to blueprint allowlist |
| NeoPixel wrong colour | State mismatch — check `ha_alarm_state` sensor in HA dev tools |
| No buzzer during delays | Check `arming_time`/`delay_time` in HA alarm config; state must be `arming` or `pending` |
