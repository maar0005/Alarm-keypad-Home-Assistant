# 3D Print — Alarm Keypad Enclosure

Sandwich-style enclosure in two parts: **front plate** and **back plate**.
Designed for PETG at 0.2 mm layer height.

---

## File Index

| File | Description |
|------|-------------|
| `front-plate/front-plate.stl` | Front panel with all cutouts |
| `back-plate/back-plate.stl` | Back panel with wall-mount and vents |

> STL files will be added once the parts arrive and fit is verified.

---

## Print Settings

| Setting | Value |
|---------|-------|
| Material | PETG |
| Layer height | 0.2 mm |
| Perimeters / walls | 3 |
| Infill | 20 % (gyroid) |
| Supports | None — both parts designed to print flat |
| Bed temp | 70 °C |
| Nozzle temp | 235 °C |
| Cooling | 30 % fan |

---

## Front Plate Cutouts

```
┌────────────────────────────┐
│  ┌──────────────┐          │  ← LCD 1602 window  (37 × 16 mm)
│  │   LCD 1602   │ [NP]     │  ← NeoPixel dot     (5 mm hole)
│  └──────────────┘          │
│                            │
│  ┌──────┐  ┌────────────┐  │
│  │ PIR  │  │  NFC scan  │  │  ← PIR dome         (24 mm hole)
│  │ lens │  │    area    │  │  ← NFC/RFID zone    (40 × 40 mm recess, 1 mm deep)
│  └──────┘  └────────────┘  │
│                            │
│  ┌──────────────────────┐  │
│  │   4 × 4   KEYPAD     │  │  ← Keypad slot      (69 × 69 mm)
│  └──────────────────────┘  │
└────────────────────────────┘
   Overall: ~100 × 130 × 10 mm
```

---

## Back Plate Features

```
┌────────────────────────────┐
│  ╔══════╗                  │  ← USB-C access slot (12 × 7 mm)
│  ║ USB-C║                  │
│  ╚══════╝                  │
│                            │
│  ░░░░░░░░░░░░░░░░░░░░░░░   │  ← Buzzer vent holes (4 × 3 mm grid)
│  ░░░░░░░░░░░░░░░░░░░░░░░   │
│                            │
│  [=====] [=====]           │  ← Horizontal wall-mount keyhole slots
└────────────────────────────┘
   Overall: ~100 × 130 × 10 mm
```

---

## Assembly Order

1. Install LCD + I2C backpack → snap into front plate window
2. Solder / wire PCB → seat in enclosure channel
3. Thread keypad ribbon cable through keypad slot before seating front plate
4. Connect JST cables for PIR, NeoPixel, buzzer
5. Clip front + back plates together (M3 × 6 mm screws, 4 corners)
6. Mount on wall via keyhole slots; plug USB-C

---

## Hardware (non-electronic)

| Item | Qty | Note |
|------|-----|------|
| M3 × 6 mm screws | 4 | PH2 head |
| M3 heat-set inserts | 4 | For plastic bosses on back plate |
| Soldering iron tip (≈4 mm) | — | For heat-set inserts |
