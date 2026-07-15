# LED Management & Effects

## LedManager (`lib/led_manager/`)

Abstract orchestrator. Concrete backends implement three pure-virtual sinks — `SetGlobalColor(const CRGB&)`, `SetLed(uint8_t, const CRGB&)`, `WriteOutLeds()` (`LedManager.hpp:41-48`):

- `FastLedManager` (`src/arduino/`) — real hardware via FastLED
- `FakeLedManager` (`lib/fake-led-manager/`) — in-memory `CRGB` array for tests (and the controller's preview rendering)
- `SimulatorLedManager` (`lib/simulator/`) — SDL desktop window

### Effect registry and weighted randomness

`AddEffect(effect, proportion)` (`LedManager.cpp:128-142`) encodes occurrence probability **structurally**: an effect with weight N is pushed into the `effects` vector N times. Weight 0 puts it in `non_random_effects` instead — reachable only by explicit index, never by the master's random rotation. `uniqueEffectIndices` records the first copy of each distinct effect so UIs can enumerate distinct effects (`UniqueEffectNumberToIndex`, `LedManager.cpp:119-126`).

Registered in the constructor (`LedManager.cpp:12-38`):

- **Random pool** (weight): ColorCycle (2), ContrastBumps (2), Fire (1), Firefly (2), Lightning (1), Pride (1), RainbowBumps (4), Rainbow (4), Rorschach (2), Spark (4), SwingingLights (4) → 27 slots, 11 unique.
- **Non-random**: SwingingLights (as "formerly police lights"), StopLight, SimpleBlink ×4 (speeds 60/30/12/300 — strobes), then **DisplayColorPalette and Dark, which must stay the last two** — external code assumes "dark" is the final index.

After registration the counts are pushed to the radio: `SetNumEffects(27)`, `SetNumPalettes(22)` (`LedManager.cpp:37-38`). Effect and palette indices are each one wire byte; `AddEffect` asserts total < 256.

`GetCurrentEffect()` (`LedManager.cpp:51-59`): if the current packet is `SET_CONTROL`, returns the `ControlEffect` singleton (held outside both vectors); otherwise dispatches by the packet's effect index. On Arduino, out-of-range indices wrap mod total; on host they assert.

### RunEffect() — the render loop (`LedManager.cpp:76`)

Resolves the current effect, the set-effect packet, and `GetNetworkMillis()` **once per frame** (so every LED in a frame shares one timestamp), then for each strip, for each LED: compute a strip-local `virtual_index` (flipped if `Reversed`), call `effect->GetRGB(virtual_index, time_ms, strip, packet)`, apply `Dim` (÷8) or `Off` (black), write to a monotonically increasing global index, then `WriteOutLeds()`. Effects bind their palette from `Effect::palettes()` by const reference — never copy a `ColorPalette` in a `GetRGB` path (it heap-allocates per LED per frame).

Division of labor for strip flags:

- `Reversed`, `Dim`, `Off` — handled centrally in `RunEffect`.
- `Mirrored`, `Circular`, `Tiny`, `Bright`, `Controller` — handled inside each effect via `strip.FlagEnabled(...)`.

Power limiting is **not** done here — it's FastLED's `setMaxPowerInVoltsAndMilliamps` (see below).

## Effect contract (`lib/effect/Effect.hpp`)

One pure-virtual method — there is no Init/Calculate lifecycle (the old CLAUDE.md was wrong about this):

```cpp
virtual CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
                    const StripDescription &strip,
                    RadioPacket *setEffectPacket) const = 0;
```

`time_ms` is **network time** from `RadioStateMachine::GetNetworkMillis()` — the shared clock is what synchronizes animation across devices. Effects derive all motion from it (e.g. `time_ms/16`); per-device randomness (Fire, Firefly, Rorschach offsets) is seeded in constructors from `analogRead(A0)` on Arduino only, keeping host tests deterministic.

Shared helpers: `Effect::palettes()` (Meyers singleton, the global palette table), `GetThresholdSin(x, threshold)` (`Effect.cpp:5-13`) — a clamped/rescaled `sin16` used for discrete "bumps"; the negative half is always 0 — and `FlattenedGradientRGB(CHSV)` (`Effect.cpp:15-32`), the gradient-power flattener described next.

### Gradient power flattening (`FlattenedGradientRGB`)

FastLED's `hsv2rgb_rainbow` deliberately over-drives hues in the yellow band (32–95) by up to ~34% total RGB power, which looks fine for solid colors but renders a smooth palette gradient with a lone over-bright yellow LED wherever the interpolation crosses that band (e.g. the rainbow palette's red→green blend). The four palette-showcase effects — Rainbow, Color Cycle, Rainbow Bumps, Display Color Palette — convert their interpolated gradient colors through `FlattenedGradientRGB`, which rescales any converted color whose `r+g+b` exceeds the un-boosted (red-hue) baseline at the same saturation/value, so gradients drive uniform power. Noise/texture effects (Fire, Perlin-based, sparks) and exact palette stops via `GetColor` deliberately keep FastLED's boost — it's part of their look. The property is pinned on both sides by `test/GradientPowerTest.cpp` and `sim/test/cases/gradientPower.test.mjs` (specs/004-fix-yellow-spike).

## Effect catalog (`lib/effect/`)

| Effect | Visual | Notes |
|---|---|---|
| ColorCycle | Whole strip one color, cycling through the palette together | Solid palette → brightness pulse instead |
| ContrastBumps | Background color with traveling bumps of the palette-opposite color | Gradient offset `0x4000` |
| Fire | Flickering fire from 2-octave Perlin noise | Own hardcoded fire palette; `Circular` uses polar Perlin |
| Firefly | Blinking lights drifting between out-of-sync and in-sync | 20 s phase cycle |
| Lightning | Groups flash when Perlin noise crosses a threshold (160) | |
| Pride | Scrolling rainbow-flag stripes with soft fades | Own 6-color palette; `Tiny` → single band, 2× speed |
| RainbowBumps | Palette gradient scrolling, brightness in moving bumps | Highest weight (4) |
| Rainbow | Classic scrolling palette gradient | `Bright` → v=255 else 128 |
| Rorschach | Perlin ink-blot mirrored about strip center | |
| Spark | Bright dot bounces with fading comet tail | 8-LED tail profile; `Circular` wraps instead of bouncing |
| SwingingLights | Colored blobs swing sinusoidally, one per palette color, additively blended | 5 s period; also registered non-random as police-light replacement |
| Police | Red/blue strobe from both ends | **Compiled but not registered anywhere** — superseded by SwingingLights |
| StopLight | Red/amber/green segment cycle | Non-random |
| SimpleBlink(speed) | Strobe: on 1/3 of the cycle | Non-random, speeds 60/30/12/300 |
| DisplayColorPalette | Paints the palette across the strip (palette preview UI) | Must be second-to-last |
| Dark | All off | Must be last |
| Control | Solid RGB from a `SET_CONTROL` packet | Singleton, selected by packet type |

`test/EffectsTest.cpp` fuzzes every effect × every palette × LED counts 0–255 × multi-strip Tiny/Circular devices — new effects must survive that.

## ColorPalette (`lib/color/`)

Immutable `std::vector<CHSV>`. `GetColor(i)` wraps; `GetGradient(fract16 position, bool wrap = true)` interpolates across the 16-bit position space, with **hue taking the shorter arc** around the color wheel (red↔blue goes through violet, not green — pinned by `ColorPaletteTest`). Solid (size-1) palettes are the signal many effects use to switch from hue-cycling to brightness-cycling.

The global table in `Effect.cpp:18-67` has **22 palettes**: 8 solids, rainbow, warm, cool, 80s-Miami, vaporwave, candy-cane variants, fire, pastel rainbow, jazz-cup, double-rainbow (6 entries → count-sensitive effects like SwingingLights spawn 6 blobs), etc.

## Math & Perlin (`lib/math/`)

- `GetPosOnCircle(led_count, led_index, *angle, *radius)` — maps a linear strip onto a circle for polar effects.
- `MirrorIndex(*led_index, *led_count)` — folds the second half of a strip back onto the first for symmetric rendering.
- `Perlin.hpp` — header-only tileable gradient noise, `perlinNoise(x,y)` → [0,256), corner gradients seeded deterministically via `tileHash(x,y) = x + y*7919`; `perlinNoisePolar` offsets by a polar vector for circular strips. Contiguity and wraparound pinned in `PerlinTest`.

## FastLedManager (`src/arduino/FastLedManager.cpp`)

- Allocates `led_count + 1` CRGBs — **index 0 is the on-board LED**; `SetLed(i)` writes to `leds[i + 1]`. Single-LED devices also mirror index 0 to the onboard LED.
- FastLED setup: `addLeds<NEOPIXEL, WS2812_PIN>` with `TypicalLEDStrip` color correction; **power limiting** via `FastLED.setMaxPowerInVoltsAndMilliamps(5, device.milliamps_supported)` — FastLED scales global brightness each `show()` to stay under the device's budget. This is the only power math in the system.
- `PlayStartupAnimation()` — white dot sweep + hue cycle; `FatalErrorAnimation()` — infinite red blink, never returns (used when radio init fails).

## Simulator (`lib/simulator/`)

Runs the real effect/LedManager code on a desktop, rendering into an SDL window via `ademuri/fast-led-simulator` (fetched by CMake). `Simulator.cpp` wires a 20-LED strip + `FakeRadio` + `RadioStateMachine` and feeds wall-clock time into the fake `millis()`. Build and run:

```bash
cmake -B build && cmake --build build --target Simulator && ./build/lib/simulator/Simulator
```

Note `ci.sh` configures with `-DBUILD_SIMULATOR=false` to avoid the SDL dependency, and the simulator's CMakeLists drops the sanitizer flags.
