# Firefly2

Wireless LED control for Burning Man art installations, bikes, and wearables. Devices driving WS2812 strips self-organize over 915 MHz RFM69 radios into a flood mesh with exactly one master, share a network clock via heartbeats, and render effects as pure functions of network time — so every device animates in sync without streaming frames.

New to the project? Start with [GETTING_STARTED.md](GETTING_STARTED.md). Working in the code (human or agent)? The mental model lives in [docs/index.md](docs/index.md) and the shared Claude Code/Codex ground rules in [CLAUDE.md](CLAUDE.md).

## Web simulator (test without hardware)

```bash
python3 -m http.server 8642 -d sim
# open http://localhost:8642/
```

A zero-dependency browser simulator of the whole system: every effect and palette on the real device catalog, synced to a scrubbable network clock, with master-mode autoplay and control overrides. Byte-exact against the firmware (enforced by tests: `npm test`). See [docs/simulator.md](docs/simulator.md).

## Building firmware

Firmware builds with [PlatformIO](https://platformio.org/) (`pip install platformio`). One env per device target:

```bash
pio run -e node                  # SAMD21 rfboard node (node-arm64 on Apple silicon)
pio run -e fancy-node            # STM32G030 node
pio run -e controller            # STM32G070 handheld controller
pio run -e dmx                   # ESP32 DMX bridge
pio run -e fancy-node -t upload  # flash via ST-Link
pio device monitor               # 115200 baud
```

Note: `[env:node]`'s bossac `platform_packages` entry is OS-specific — uncomment the right line in [platformio.ini](platformio.ini) for your OS. Full env/pin/flag details: [docs/build-and-test.md](docs/build-and-test.md).

### Programming over USB

The `fancy-node` device can be programmed either using an STLink, or via USB port. To program via USB, you must first install stm32loader using pip (`pip install stm32loader`). You may also need to pass the port (typically `/dev/ttyUSB0`), e.g. `pio run -e fancy-node-usb -t upload --upload-port /dev/ttyUSB0`.

### Saving device description in flash

Both `node` and `fancy-node` support reading the [`DeviceDescription`](lib/device/DeviceDescription.hpp) from flash. The modes are defined by the `DeviceMode` enum within [`DeviceDescription.hpp`](lib/device/DeviceDescription.hpp). They work as follows:

- `CURRENT_FROM_HEADER`: use the `current` device defined in [`Devices.hpp`](lib/device/Devices.hpp)
- `READ_FROM_FLASH`: read the device saved in flash, if present. If not present (determined by the validity of `check_value`), fall back to `current`.
- `WRITE_TO_FLASH`: write `current` to flash, and then use it. This will only write the device to flash if it is different, to avoid causing flash wear.

## Tests & linting

The platform-independent core (`lib/`, `src/generic/`) builds on the host against fakes, with ASan + UBSan on (and fatal). Requires cmake and a GCC/Clang toolchain (on Windows, use WSL or similar):

```bash
mkdir -p build && cd build
cmake .. -DBUILD_SIMULATOR=false   # true builds the SDL desktop simulator too
make && make test
./smalltests --gtest_filter=RadioStateMachineTest*
```

```bash
./lint.sh check     # clang-format, Google style — CI enforces this
./lint.sh format    # format in place
./ci.sh             # exactly what CI runs: cmake + smalltests + largetests
node --test "sim/test/cases/*.test.mjs"   # simulator suite, incl. firmware byte-exactness
```

CI (GitHub Actions, on every push): host tests, lint, `node`/`fancy-node`/`controller` firmware builds, and the simulator suite.

## Repo layout

| Path | Contents |
|---|---|
| `lib/`, `src/generic/` | Platform-independent core: radio protocol, mesh, effects, LED management |
| `src/arduino/` | Hardware backends (FastLED, RadioHead RFM69) |
| `src/devices/` | One `main` per device target (node, fancy-node, controller, dmx) |
| `test/` | Host GoogleTest suites + `FakeNetwork` multi-node mesh simulation |
| `sim/` | Browser simulator (JS port of the effect engine, vector-tested against firmware) |
| `docs/` | Architecture and per-subsystem notes — start at [docs/index.md](docs/index.md) |
| `hardware/`, `boards/`, `bootloader/` | PCB designs, PlatformIO board definitions, bootloaders |
| `specs/` | Spec-kit feature specs (spec → plan → tasks workflow, see CLAUDE.md) |

## For agents & contributors

- [CLAUDE.md](CLAUDE.md) — the canonical guidance for Claude Code and Codex, including build commands, architecture, and the **invariants you must not break**. Codex reaches the same file through `AGENTS.md`; shared skills remain canonical under `.claude/skills/` and are exposed to Codex through `.agents/skills/`.
- [docs/](docs/index.md) — per-subsystem research notes (radio/network, LED/effects, devices, build/test, hardware). Update them when you change documented behavior.
- Substantial features go through the spec-kit workflow (`speckit-specify` → `plan` →
  `tasks` → `implement`; use `/` in Claude Code or `$` in Codex); specs live in `specs/`.
