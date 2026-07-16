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

## Web simulator (production C++ via Wasm)

The browser simulator (`sim/`, see [simulator.md](simulator.md)) loads a committed Emscripten build of the production C++ renderer. Viewing it and running its functional tests require no C++ toolchain:

```bash
python3 -m http.server 8642 -d sim
node --test "sim/test/cases/*.test.mjs"   # or: npm test (no npm install needed)
npm ci && npm run lint                    # ESLint (dev-only dependency; config in eslint.config.mjs)
npx playwright install chromium firefox  # one-time managed browser install
npm run test:e2e                          # Chromium + Firefox served-page journeys
```

- `sim/test/cases/vectors.test.mjs` sends every committed reference case through the C++ Wasm adapter and byte-compares the result with `sim/test/vectors/reference.json`. The corpus comes from the **`vectorgen`** CMake target (`test/VectorGen.cpp`); its case model is shared with host `ReferenceVectorTest`. Regenerate it after an intentional production rendering change.
- `sim/test/cases/artifact.test.mjs` checks the manifest schema, generated-file hashes/sizes, normalized source fingerprint, and absence of machine-specific paths or timestamps.
- The same cases run in-browser at `sim/test.html`.
- Playwright drives the actual served page through deep links, UI and `window.sim` changes, deterministic canvas pixels, reloads, the complete browser harness, and missing-Wasm failure handling. `npm run test:e2e:headed`, `test:e2e:debug`, and `test:e2e:report` provide interactive and retained visual diagnosis.

Rebuilding or freshness-checking the committed artifact additionally requires Emscripten SDK **6.0.3** active in the current environment:

```bash
npm run build:sim-wasm             # portable macOS/Windows/Linux command
npm run check:sim-wasm             # clean, non-mutating byte comparison
npm run test:sim-wasm-integration  # current + deliberately stale cases
```

FakeFastLED is pinned to `f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca`. The shell wrappers under `scripts/` provide the documented POSIX commands, while their Node implementation keeps the developer workflow native on all three operating systems.

## CI (`.github/workflows/`)

Five workflows. The build and test workflows run on every push; CodeQL also
runs for pull requests targeting `master`, pushes to `master`, a weekly
schedule, and manual dispatch:

- `run-tests.yaml` → `./ci.sh`: cmake with `-DBUILD_SIMULATOR=false`, `make`, then runs `./smalltests` and `./largetests` directly.
- `run-lint.yaml` → `./lint.sh check` (clang-format `--dry-run --Werror`; style is `.clang-format`, Google-based).
- `build-platformio.yaml` → builds **`node`, `fancy-node`, and `controller`** (`dmx` commented out with a re-enable TODO).
- `sim.yaml` → macOS/Windows/Linux matrix; caches and activates Emscripten 6.0.3, rejects stale generated artifacts before tests, exercises deliberate staleness, runs Node plus Chromium/Firefox E2E suites on every OS, and uploads browser traces/screenshots/reports (ESLint and agent-bridge checks run once on Linux).
- `codeql.yml` → CodeQL v4 scans C/C++ under `lib/` and `src/` with the
  security-extended and security-and-quality query suites. Its no-build mode
  includes embedded target sources that the host CMake build does not compile.

`lint.sh` modes: `check`, `format` (in-place), `tidy` (clang-tidy; note there is **no `.clang-tidy` config file** in the repo, so it runs with defaults). Known latent bug: the `find` in `lint.sh` uses `-o` without grouping, so its `-not -path` prune only applies to the first `-iname` branch.

`test/DebugTest.cpp` fails the build if the `DEBUG` macro is left defined (`lib/debug/Debug.hpp:5`) — a guard against shipping debug output.

## Flashing & bootloaders

### SAMD node (rfboard / puck)

- Normal path: bossac over the SAM-BA UF2 bootloader. `bossac_wrapper.sh` does the 1200-baud touch reset, re-detects the renumbered port, then `bossac -e -w -v -b -R`.
- UF2: `tools/create_uf2.py` runs post-build (node envs only) and calls `tools/uf2conv.py` with family `0x68ed2b88` (SAMD21) and app base `0x2000`, producing a drag-and-drop `firmware.uf2`.
- First-time bootloader install: FT232H breakout as SWD probe + OpenOCD — see `bootloader/README.md` and `bootloader/program.sh`. Prebuilt bootloaders for `rfboard` and `firefly_v2` (puck) live in `bootloader/`, plus a self-update UF2. Linux users need the ModemManager-ignore udev rule (`bootloader/99-candy-kingdom.rules`, Atmel VID `0x03eb`) and `dialout` group membership.

### STM32 (fancy-node / controller)

ST-Link over SWD, or the `-usb` envs via the STM32 serial ROM bootloader using the pinned `ademuri/stm32loader` fork (`pip install 'stm32loader @ git+https://github.com/ademuri/stm32loader@4ef98d0'`). No UF2.
