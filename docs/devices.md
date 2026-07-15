# Device Targets & Configuration

Per-device firmware lives in `src/devices/<target>/`; each PlatformIO environment selects exactly one device folder via `build_src_filter`. Shared code is in `lib/` and `src/{arduino,generic}/`.

Active environments: `node` (+ `node-arm64`), `fancy-node` (+ `fancy-node-usb`), `dmx`, `controller` (+ `controller-usb`). The `range_test`, `remote`, and `trellis` environments are commented out in `platformio.ini` (TODO).

## StripDescription (`lib/device/StripDescription.hpp`)

One contiguous LED strip: `led_count` plus a bitmask of flags (folded from a `std::vector<StripFlag>` at construction; test with `FlagEnabled(flag)`).

| Flag | Bit | Meaning |
|---|---|---|
| `Tiny` | 0x01 | Small strip; effects treat it as a compact pixel run |
| `Bright` | 0x02 | Drive brighter than default |
| `Circular` | 0x04 | Loop/ring; effects wrap around |
| `Mirrored` | 0x08 | Render half, mirror to the other half (e.g. two-sided puck) |
| `Reversed` | 0x10 | Physical wiring runs backwards |
| `Controller` | 0x20 | Part of a controller UI grid, not a wearable output |
| `Dim` | 0x40 | Drive dimmer than default |
| `Off` | 0x80 | Strip disabled |

## DeviceDescription (`lib/device/DeviceDescription.hpp`)

A whole device: `milliamps_supported` (power budget at 5 V) + ordered `std::vector<StripDescription> strips` + `check_value` (flash-validity sentinel, `kCheckValue = 0x12345678`). `kMaxSize = 128` bytes is the flash budget for a serialized description; a `static_assert` in `Devices.hpp` enforces it.

Gotcha: `GetLedCount()` (`DeviceDescription.cpp:11-17`) accumulates into a `uint16_t` but returns `uint8_t` — a device with more than 255 total LEDs silently truncates.

### Flash storage (DeviceMode)

`enum class DeviceMode { CURRENT_FROM_HEADER, READ_FROM_FLASH, WRITE_TO_FLASH }` (`DeviceDescription.hpp:9-13`). Each firmware sets a compile-time `kDeviceMode`; all committed sources use `CURRENT_FROM_HEADER`. Only `node` and `fancy-node` implement the flash paths:

- **node (SAMD21)**: `FlashStorage_SAMD` writing the top 8 KB of flash at `0x1E000` (`node.cpp:28-30`).
- **fancy-node (STM32G0)**: `FlashStorage_STM32` EEPROM emulation at `FLASH_BASE_ADDRESS=0x800F800` — the last 2 KB page, reserved by capping the board's flash to 62 KB.

Both serialize the `DeviceDescription` as raw bytes — including a `std::vector` with heap pointers — so the mechanism is fragile and guarded by `check_value`. Writes are skipped when flash already matches, to limit wear.

## Devices.hpp catalog

`Devices::RF_BOARD_MA_SUPPORTED = 2350` (2400 mA supply minus 50 mA margin). `SimpleRfBoardDescription(led_count, flags)` builds single-strip devices.

Defined devices include single-strip wearables (`bike`, `ben_s_bike`, `will_bike`, `scarf`, `lantern`, `puck`, `two_side_puck`, `dan_jacket`, `will_jacket`, `will_top_hat`, `hex_light`, …) and multi-strip builds:

- `rainbow_cloak` — 2 tiny circular rings + a 94-LED run
- `half_matrix_panel` — 8×16 serpentine matrix (alternating `Reversed`)
- `backpack_rope`, `will_backpack`, `ross_backpack` — paired forward/reversed ropes
- `ufo` — 4 concentric circular strips with per-ring Bright/Dim
- `brooke_bike` — 4 mixed strips

**`Devices::current`** (`Devices.hpp:89`) selects which device a build targets — currently `scarf`. Changing this one reference is the normal way to build for different hardware.

## Device entry points

### node (`src/devices/node/node.cpp`) — SAMD21 rfboard

The pure receiver/player: no buttons, no battery monitoring. Radio + `FastLedManager`, startup animation, `FatalErrorAnimation()` if the radio fails. Runs a hardware watchdog off the 32 kHz oscillator with a ~128 ms timeout (`WDT->CONFIG.bit.PER = 9`, `node.cpp:128`), fed every ~5 loop iterations. Loop: `state_machine.Tick()` → `led_manager->RunEffect()`.

### fancy-node (`src/devices/fancy-node/fancy-node.cc`) — STM32G030C8

Node plus battery management. Debug serial is **Serial2** (PA2/PA3). Notable behavior:

- Clears the `nBOOT_SEL` option bit at boot (flash-unlock key sequence) so the board boots application code from USB power-up (`fancy-node.cc:113-131`).
- Battery on PA0 through a divider, median(5) + EMA filtering; low-battery detection suppressed for the first 3 s while the RC filter settles.
- **Low battery**: cutoff 3.6 V under load → clear LEDs, dim red indicator, `radio->sleep()`, idle until voltage recovers past 3.85 V (~50% SoC hysteresis).
- Hold either button at boot → battery gauge display (white flash, 2 s cap-charge wait, then red→green hue on the onboard LED for 20 s).

### controller (`src/devices/controller/`) — STM32G070CB

The handheld network remote: 42 WS2812 UI LEDs (`kLedCount`, `leds.h:6`), 9 buttons in 3 columns, a 3-position analog mode switch (PA6), battery on PA7. Uses `FakeLedManager` — the effect engine renders *previews* on the UI LEDs rather than driving a wearable strip.

Three modes selected by the switch (`controller.cpp:557-563`):

1. **Effect** — left/right buttons step effect (row 0) and palette (row 1); the top preview row renders the selected effect live and the middle row the palette; bottom button broadcasts `SetEffect` with `kSetEffectDelay = 60` s lock. Status LEDs: index 7 red/green = Slave/Master, index 9 blue = broadcasting.
2. **DirectColor** — 6 side buttons map to 6 evenly-spaced hues; a press sends a `SET_CONTROL` (solid color network-wide).
3. **DirectPalette** — 6 side buttons map to palette indices {8..13}; a press keeps the current effect but swaps the palette.

Battery cutoff is 3.4 V (higher load device), resume 3.85 V. Also implements a double-tap-reset → system-bootloader jump using RTC backup state (`controller.cpp:363-460`), needed because the CH340X USB chip toggles reset on power-up. Note `analog-button.{cpp,h}` (`AnalogButton`) is compiled but currently unreferenced — the mode switch is read inline with a median filter.

### dmx (`src/devices/dmx/dmx.cpp`) — ESP32 Thing Plus

Bridges a DMX console onto the radio network. Reads DMX (192 channels, SparkFun DMX Shield v1 — pinned; v2 is a breaking change, see TODO in `platformio.ini`), takes RGB from channels 3/4/5 (off-by-channel mystery noted in a TODO), and sends a `SET_CONTROL` packet whenever the color changes or every 1 s. The 5-second delay field acts as a rolling reservation so the master's effect rotation doesn't override the console while it's connected. DMX drives one global color, not per-pixel data.

### Disabled targets

- **range_test** — RSSI diagnostic; ping-pongs packets and colors one LED by `LastRssi()`.
- **remote** — single-button transmitter; sends a fixed `SetEffect` on press, sleeps the radio otherwise.
- **trellis** — Adafruit NeoTrellis 4×4 controller, predecessor of `controller`. Uses an obsolete `FastLedManager`/effect API and no longer compiles against the current library — likely why its env is commented out.

## Battery (`lib/battery/Battery.hpp`)

`kBatteryEmpty = 3.7 V`, `kBatteryFull = 4.2 V`; divider constants 62/180; `BatteryVoltageToRawReading()` converts a voltage to the expected 10-bit ADC reading.

## Debug (`lib/debug/`)

`debug_printf(fmt, ...)`, gated on the `DEBUG` macro which is **commented out by default** (`Debug.hpp:5`). With `DEBUG` + Arduino it formats into a static buffer and writes to `Serial`; on host builds it goes to stdout. `test/DebugTest.cpp` fails CI if `DEBUG` is left defined — don't ship debug builds. Device firmwares also log directly: node uses `Serial`, fancy-node/controller use `Serial2`, all at 115200.
