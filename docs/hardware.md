# Hardware

KiCad projects under `hardware/`; newer boards are KiCad 6+ (`.kicad_pro`), older ones legacy KiCad ≤5 (`.pro`).

| Directory | Board | Firmware target | Format | Notes |
|---|---|---|---|---|
| `rfboard/` | SAMD RF node board | `node` (`board = rfboard`) | KiCad 6+ | Gerbers, STEP model, JLC fabrication-toolkit options; PCB v2.0 per recent commits |
| `fancy-node/` | STM32G030C8T node | `fancy-node` | KiCad 6+ | JLC BOM/CPL files |
| `controller/` | STM32G070CBT handheld controller | `controller` | KiCad 6+ (migrated from v5; rescue libs) | JLC BOM/CPL |
| `puck/` | Original "firefly2" puck (`firefly_v2`) | `node`-class | Legacy (2018) | Uses ATSAMD11C14A per `assembly/firefly_v2.0_bom.csv`; the `firefly_v2` bootloader binary matches this board |
| `ring-40mm/`, `ring-50mm/` | WS2812 LED rings | none (passive LED boards) | KiCad 6+ | round-tracks config |
| `magnet-connector/` | Magnetic interconnect | none | KiCad 6+ | |
| `remote/` | `rf_remote` single-button remote | `remote` (env commented out) | Legacy | Bundles `homebrew_button` footprint lib |
| `rescue-backup/` | 2022-04-03 rescue snapshot of the puck schematic | — | Legacy | backup only |

Common electronics per device: MCU (SAMD11/SAMD21, STM32G0, or ESP32), RFM69 radio at 915 MHz, WS2812B strip connectors, 5 V power path with a per-device current budget (`milliamps_supported`), and on battery-powered boards a divider into an ADC pin for voltage monitoring (see `lib/battery/Battery.hpp`: empty 3.7 V, full 4.2 V, divider 62/180).

## Bootloader (`bootloader/`)

- Prebuilt UF2/SAM-BA bootloaders (uf2-samdx1 v3.6.0 lineage): `bootloader-rfboard-*.bin`, `bootloader-firefly_v2-*.bin`, plus `update-bootloader-rfboard-*.uf2` for self-update over an existing bootloader.
- First-time install uses an Adafruit FT232H as an SWD probe (D0→CLK, D1→DIO, 470 Ω D1–D2) driving OpenOCD (`ft232h_swd.cfg`); `program.sh` pipes the flash commands into a running OpenOCD via `nc localhost 4444`. Full walkthrough in `bootloader/README.md`.
- `99-candy-kingdom.rules` stops ModemManager from grabbing the boards' USB-CDC port (Atmel VID `0x03eb`); install to `/etc/udev/rules.d/` and be in `dialout`.

Application firmware on SAMD starts at `0x2000` (the bootloader owns the first 8 KB) — this is the base address baked into UF2 generation (`tools/create_uf2.py`).
