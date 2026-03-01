# Bill of Materials

Full parts list for the Alarm Keypad build.

---

## Parts You Already Have

| Component | Notes |
|-----------|-------|
| Unexpected Maker FeatherS3 (ESP32-S3) | Main MCU |
| Parallax RFID reader module (125 kHz) | UART, 5V, reads EM4100 fobs |
| 1602 LCD + I2C PCF8574 backpack | I2C 0x27, 5V |
| NeoPixel (1 LED, built-in on FeatherS3 GPIO40) | Status indicator |

---

## Parts to Buy

| Component | Qty | Est. Price | Notes |
|-----------|-----|-----------|-------|
| PN532 NFC module (13.56 MHz, I2C mode) | 1 | ~$4 | Set I2C jumpers: I0=1, I1=0; address 0x24 |
| NTAG215 NFC stickers (10-pack) | 1 pack | ~$3 | Compatible with phone NFC & PN532 |
| EM4100 RFID key fobs 125 kHz (10-pack) | 1 pack | ~$3 | Compatible with Parallax reader |
| 4×4 membrane keypad | 1 | ~$2 | 8-pin ribbon cable |
| Passive buzzer 5V ~85 dB | 1 | ~$1 | Do NOT use an active buzzer |
| 2N7000 MOSFET (N-channel TO-92) | 2 | <$1 | 1 for buzzer, 1 spare |
| BH1750 ambient light sensor module | 1 | ~$2 | I2C, ADDR pin → GND for 0x23 |
| HC-SR501 PIR motion sensor | 1 | ~$2 | Set to single-trigger mode |
| BME280 temp/humidity/pressure module | 1 | ~$3 | I2C 0x76 (SDO → GND) |
| LiPo battery 3.7V 1500mAh JST-PH 2.0 | 1 | ~$5 | Must be JST-PH 2.0 for FeatherS3 |
| USB-C power supply 5V ≥1A | 1 | ~$4 | Wall adapter |
| Proto PCB 5×7 cm | 1 | ~$1 | For neat soldering |
| Dupont wires male-female 20 cm (40-pack) | 1 pack | ~$2 | For sensor connections |
| 100 Ω resistor | 2 | <$1 | Gate resistor for MOSFET |
| 4.7 kΩ resistor | 2 | <$1 | I2C pull-ups (if not on breakouts) |
| 1 kΩ resistor | 1 | <$1 | RFID TX series resistor |
| M3 × 6 mm screws + heat-set inserts | 4 each | ~$1 | Enclosure assembly |

**Total estimate: ~$22–28 USD** (prices vary by supplier/region)

---

## Recommended Suppliers

| Supplier | Good for |
|----------|---------|
| AliExpress | Sensors, NFC modules, keypad, fobs (slow shipping) |
| Adafruit | FeatherS3, quality sensors, NeoPixels |
| Amazon | Quick shipping on most items |
| LCSC / DigiKey | Discrete components (resistors, MOSFETs) |

---

## Optional / Nice-to-Have

| Component | Purpose |
|-----------|---------|
| 10 kΩ resistors (10-pack) | Keypad column pull-ups (firmware uses internal pull-ups, external is cleaner) |
| JST-PH 2.0 connector kit | Neat connectors for battery & sensors |
| Female pin headers (2.54 mm) | Socket the FeatherS3 on the proto PCB for removability |
| 3D printer filament — PETG 1 kg | Enclosure printing |
