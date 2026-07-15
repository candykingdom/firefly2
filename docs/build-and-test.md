# Build, Test & CI

Two independent build systems:

- **PlatformIO** builds firmware for embedded targets (`platformio.ini`).
- **CMake** builds the platform-independent core against fakes and runs GoogleTest suites on the host.

## PlatformIO environments

Every env compiles one device folder from `src/devices/<x>` plus shared `src/arduino` + `src/generic` via `build_src_filter`, pins `mikem/RadioHead@^1.120`, and uses `monitor_speed = 115200`.

| Env | Board / MCU | Upload | Key flags |
|---|---|---|---|
| `node` | `rfboard` (custom SAMD, candykingdom platform) | bossac (SAM-BA bootloader) | UF2 post-build via `tools/create_uf2.py` |
| `node-arm64` | same | `./bossac_wrapper.sh` (custom, for Apple-silicon macOS) | same |
| `fancy-node` | `generic_stm32g030c8t` (STM32G030C8, M0+) | ST-Link (SWD) | `RADIO_SS=4 RADIO_DIO=8 WS2812_PIN=PA10`, `FLASH_BASE_ADDRESS=0x800F800` |
| `fancy-node-usb` | same | `stm32loader` (serial ROM bootloader) | needs the ademuri stm32loader fork (see platformio.ini comment) |
| `dmx` | `esp32thing_plus` (ESP32) | esptool default | `RADIO_SS=32 RADIO_DIO=14 WS2812_PIN=15`; DMX lib pinned to dead v1 (TODO: migrate to v2) |
| `controller` | `generic_stm32g070cbt` (STM32G070CB, M0+) | ST-Link | `RADIO_SS=16 RADIO_DIO=17 WS2812_PIN=4`, Serial2 on PA2/PA3, STM32RTC |
| `controller-usb` | same | `stm32loader ... -d` | same fork |
| `range_test`, `remote`, `trellis` | — | — | commented out (`; TODO`), source exists in `src/devices/` |

Gotchas:

- `[env:node]`'s `platform_packages` has three OS-specific bossac packages; **only the Linux one is uncommented** — switch it per OS (`platformio.ini:18-22`). `node-arm64` hardcodes the macOS package instead.
- The STM32G030 board def caps flash at 62 KB (`upload.maximum_size: 63488` in `boards/generic_stm32g030c8t.json`) — the last 2 KB page is deliberately reserved for EEPROM emulation at `FLASH_BASE_ADDRESS=0x800F800`.
- Pinned library forks are load-bearing: `candykingdom/FastLED` (SAMD board support), `ademuri/FakeFastLED` stm32g0 branch (real FastLED lacks STM32G0 support; **fancy-node and controller pin different commits**), `candykingdom/FlashStorage_STM32`, `ademuri/smart-input-filter`, `ademuri/arduino-timer`.

```bash
pio run -e node                 # build
pio run -e fancy-node -t upload # flash via ST-Link
pio run -e fancy-node-usb -t upload --upload-port /dev/ttyUSB0
pio device monitor              # 115200 baud
```

## Host tests (CMake + GoogleTest)

```bash
mkdir -p build && cd build
cmake ..            # add -DBUILD_SIMULATOR=false to skip the SDL simulator
make
make test           # or run ./smalltests / ./largetests directly
./smalltests --gtest_filter=RadioStateMachineTest*
```

- C++14, `-Wall -Wextra -pedantic`, **ASan + UBSan always on** (root `CMakeLists.txt:29-32`). The simulator subdir opts out of sanitizers.
- FetchContent pulls googletest, `ademuri/FakeFastLED` (host FastLED stub), `nlohmann/json` (pinned v3.11.3, test targets only — parses the reference corpus), and (simulator only) `ademuri/fast-led-simulator`.
- The `generic` static library = `src/generic/*.cpp` + all `lib/*/` sources compiled against fakes — this is the platform-independent core.
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON`; a checked-in symlink `src/generic/compile_commands.json → build/compile_commands.json` wires clangd/YCM after one build.

### Test binaries

- **`smalltests`** — all of `test/` except `InvalidPacketTest`: Battery, ColorPalette (incl. gradient degenerate inputs + palette-registry boundary), Debug, DeviceDescription, Effects (incl. golden output spot-checks), FireflyEffect, LedManager (incl. Off/Dim/Reversed/multi-strip/control-override render-loop semantics), Math, NetworkManager, Perlin, RadioPacket (incl. wire-codec round-trips), RadioStateIntegration, RadioStateMachine, ReferenceVector (re-renders all 1,380 corpus cases and compares RGB byte-exact — any effect-output change fails here until the corpus is regenerated; the 72 Firefly cases only byte-compare on the corpus-generating platform's libc, because Firefly's construction offset comes from `rand()` — elsewhere they are skipped, counted, and reported), StripDescription. New `test/*Test.cpp` files are picked up automatically by the CMake glob.
- **`largetests`** — only `InvalidPacketTest.cpp`, split out because it's a combinatorial fuzz (`testing::Combine`: 10 ids × 8 types × 20 lengths × 9 data vectors) and slow.

### Test doubles

- `FakeRadio` (`lib/fake-radio/`) — `Radio` impl with `setReceivedPacket()`/`getSentPacket()` hooks. The send path round-trips through the production wire codec (`sendPacket` → `RadioPacket::Serialize` → bytes → `Deserialize` in `getSentPacket`), so serialization defects fail every protocol/mesh test; receive-side injection stays raw-struct so the fuzz can inject frames the codec could never produce.
- `FakeLedManager` (`lib/fake-led-manager/`) — RAM-backed; `GetLed()` reads pixel output; `ClearEffects()`/`PublicAddEffect()` for registry tests.
- `FakeNetwork` (`test/FakeNetwork.{hpp,cpp}`) — 5 full nodes (radio + NetworkManager + state machine + LED manager), deterministic tick-based delivery, `setPacketLoss(n)` drops 1-in-n. This is how multi-node protocol behavior is tested on the host.

## Web simulator tests (Node + ESLint)

The browser simulator (`sim/`, see [simulator.md](simulator.md)) has its own zero-dependency test suite and linter:

```bash
node --test "sim/test/cases/*.test.mjs"   # or: npm test (Node >= 18; no npm install needed to run tests)
npm ci && npm run lint                    # ESLint (dev-only dependency; config in eslint.config.mjs)
```

- `sim/test/cases/vectors.test.mjs` compares the JS engine byte-for-byte against `sim/test/vectors/reference.json`, generated by the **`vectorgen`** CMake target (`test/VectorGen.cpp`, plain `main()`, excluded from `smalltests`). The case model (seeds, wire tables, device catalog, case enumeration) lives in `test/VectorGenCommon.{hpp,cpp}`, shared with `ReferenceVectorTest` — the corpus pins firmware and simulator in both directions. Regenerate after intentional firmware effect changes: `make vectorgen && ./vectorgen > ../sim/test/vectors/reference.json` (both the C++ and JS suites then gate against the new truth).
- The same cases run in-browser at `sim/test.html`.

## CI (`.github/workflows/`)

Four workflows, all `on: [push]`:

- `run-tests.yaml` → `./ci.sh`: cmake with `-DBUILD_SIMULATOR=false`, `make`, then runs `./smalltests` and `./largetests` directly.
- `run-lint.yaml` → `./lint.sh check` (clang-format `--dry-run --Werror`; style is `.clang-format`, Google-based).
- `build-platformio.yaml` → builds **`node`, `fancy-node`, and `controller`** (`dmx` commented out with a re-enable TODO).
- `sim.yaml` → `npm ci`, `npm run lint` (ESLint over `sim/`), `npm test` (the simulator suite).

`lint.sh` modes: `check`, `format` (in-place), `tidy` (clang-tidy; note there is **no `.clang-tidy` config file** in the repo, so it runs with defaults). Known latent bug: the `find` in `lint.sh` uses `-o` without grouping, so its `-not -path` prune only applies to the first `-iname` branch.

`test/DebugTest.cpp` fails the build if the `DEBUG` macro is left defined (`lib/debug/Debug.hpp:5`) — a guard against shipping debug output.

## Flashing & bootloaders

### SAMD node (rfboard / puck)

- Normal path: bossac over the SAM-BA UF2 bootloader. `bossac_wrapper.sh` does the 1200-baud touch reset, re-detects the renumbered port, then `bossac -e -w -v -b -R`.
- UF2: `tools/create_uf2.py` runs post-build (node envs only) and calls `tools/uf2conv.py` with family `0x68ed2b88` (SAMD21) and app base `0x2000`, producing a drag-and-drop `firmware.uf2`.
- First-time bootloader install: FT232H breakout as SWD probe + OpenOCD — see `bootloader/README.md` and `bootloader/program.sh`. Prebuilt bootloaders for `rfboard` and `firefly_v2` (puck) live in `bootloader/`, plus a self-update UF2. Linux users need the ModemManager-ignore udev rule (`bootloader/99-candy-kingdom.rules`, Atmel VID `0x03eb`) and `dialout` group membership.

### STM32 (fancy-node / controller)

ST-Link over SWD, or the `-usb` envs via the STM32 serial ROM bootloader using the pinned `ademuri/stm32loader` fork (`pip install 'stm32loader @ git+https://github.com/ademuri/stm32loader@4ef98d0'`). No UF2.
