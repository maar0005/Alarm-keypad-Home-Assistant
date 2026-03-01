# Wiring Diagram — Alarm Keypad

All connections to the **Unexpected Maker FeatherS3** (ESP32-S3).

---

## FeatherS3 Pinout Used

```
FeatherS3
┌─────────────────────────────────────────┐
│  GPIO8   SDA ─── I2C bus (shared)       │
│  GPIO9   SCL ─── I2C bus (shared)       │
│                                         │
│  GPIO4   RX  ─── Parallax RFID TX       │
│  GPIO5   EN  ─── Parallax RFID ENABLE   │
│                                         │
│  GPIO15  R0  ─── Keypad Row 0           │
│  GPIO16  R1  ─── Keypad Row 1           │
│  GPIO17  R2  ─── Keypad Row 2           │
│  GPIO18  R3  ─── Keypad Row 3           │
│  GPIO11  C0  ─── Keypad Col 0           │
│  GPIO12  C1  ─── Keypad Col 1           │
│  GPIO13  C2  ─── Keypad Col 2           │
│  GPIO14  C3  ─── Keypad Col 3           │
│                                         │
│  GPIO40  NP  ─── NeoPixel Data          │
│  GPIO6   BZ  ─── 2N7000 Gate (Buzzer)  │
│  GPIO21  PIR ─── HC-SR501 OUT           │
│  GPIO2   VBAT── Battery ADC (1:2 div)   │
│                                         │
│  3.3V    ─── logic power for sensors    │
│  5V/USB  ─── Parallax RFID VCC, LCD VCC │
│  GND     ─── common ground              │
└─────────────────────────────────────────┘
```

---

## I2C Bus (shared SDA=GPIO8, SCL=GPIO9)

| Device | Address | VCC |
|--------|---------|-----|
| 1602 LCD + PCF8574 backpack | `0x27` | 5V |
| PN532 NFC reader | `0x24` | 3.3V |
| BH1750 light sensor | `0x23` | 3.3V |
| BME280 temp/humidity | `0x76` | 3.3V |

> All SDA/SCL lines share the same bus. 4.7 kΩ pull-up resistors on SDA and SCL
> (to 3.3V) are recommended if not already on the breakout boards.
> LCD backpack typically runs on 5V; logic level is compatible with 3.3V GPIO.

---

## Parallax RFID Reader (125 kHz)

```
Parallax RFID                FeatherS3
┌─────────────┐
│  VCC  ──────┼──────────── 5V
│  GND  ──────┼──────────── GND
│  TX   ──────┼──────────── GPIO4  (RX)
│  /ENABLE ───┼──────────── GPIO5  (drive LOW to read)
└─────────────┘
```

> Drive ENABLE LOW to activate reading. The ESPHome `rdm6300` component
> handles this automatically. RFID TX is 5V-level but the ESP32-S3 RX pins
> are 5V-tolerant via their internal ESD diodes; use a 1 kΩ series resistor
> for safety.

---

## Passive Buzzer (via 2N7000 MOSFET)

```
3.3V ──┬── 10 kΩ ── 2N7000 Gate ── GPIO6
       │
     Buzzer (+)
       │
     Buzzer (−) ── 2N7000 Drain
                        │
                       GND ── 2N7000 Source
```

```
2N7000 pinout (TO-92, flat face toward you):
  Source | Gate | Drain
```

> The MOSFET allows 5V buzzer drive without loading the 3.3V GPIO.
> Add a 100 Ω resistor in series with the gate to suppress ringing.

---

## 4×4 Membrane Keypad

```
Keypad Pin  Function    FeatherS3
──────────────────────────────────
  1 (R0)    Row 0       GPIO15
  2 (R1)    Row 1       GPIO16
  3 (R2)    Row 2       GPIO17
  4 (R3)    Row 3       GPIO18
  5 (C0)    Col 0       GPIO11  ← 10 kΩ pull-up to 3.3V (or INPUT_PULLUP)
  6 (C1)    Col 1       GPIO12
  7 (C2)    Col 2       GPIO13
  8 (C3)    Col 3       GPIO14
```

Key mapping:
```
[ 1 ][ 2 ][ 3 ][ A ]
[ 4 ][ 5 ][ 6 ][ B ]
[ 7 ][ 8 ][ 9 ][ C ]
[ * ][ 0 ][ # ][ D ]
```
- `#` = Enter/Submit code
- `*` = Clear entry
- `A` = Arm Away shortcut
- `B` = Arm Home shortcut

---

## HC-SR501 PIR Sensor

```
HC-SR501              FeatherS3
┌───────────┐
│  VCC ─────┼── 5V
│  OUT ─────┼── GPIO21
│  GND ─────┼── GND
└───────────┘
```

> HC-SR501 output is 3.3V even when powered at 5V — safe for direct GPIO.
> Set jumper to single-trigger mode; sensitivity and delay pots to minimum.

---

## BME280 (0x76)

```
BME280           FeatherS3
VCC  ──────────  3.3V
GND  ──────────  GND
SDA  ──────────  GPIO8
SCL  ──────────  GPIO9
```

> If your module has SDO → GND it uses `0x76`. If SDO → VCC it's `0x77`.
> Update `address` in the YAML if different.

---

## BH1750 (0x23)

```
BH1750           FeatherS3
VCC  ──────────  3.3V
GND  ──────────  GND
SDA  ──────────  GPIO8
SCL  ──────────  GPIO9
ADDR ──────────  GND   (sets address to 0x23)
```

---

## PN532 NFC (0x24)

```
PN532            FeatherS3
VCC  ──────────  3.3V
GND  ──────────  GND
SDA  ──────────  GPIO8
SCL  ──────────  GPIO9
```

> Set the I2C mode jumpers on the PN532 board: I0=1, I1=0.

---

## Battery Monitoring

The FeatherS3 has an on-board LiPo charger (JST-PH 2.0) and a battery voltage
divider connected to GPIO2 (using a 1:2 resistor divider to keep the voltage
within ADC range). The ESPHome config reads this and applies a ×2 multiplier.

---

## Power Rail Summary

| Rail | Source | Consumers |
|------|--------|-----------|
| 5V | USB-C or LiPo (boosted) | LCD backpack, Parallax RFID, HC-SR501 VCC, NeoPixel |
| 3.3V (on-board LDO) | FeatherS3 | ESP32-S3 core, PN532, BME280, BH1750 |
| GND | Common | All |
