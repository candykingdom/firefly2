# Research: Web Simulator

**Feature**: 001-web-simulator | **Date**: 2026-07-10

All Technical Context unknowns resolved. Facts below were verified directly against the repository (file:line refs) unless noted.

## R1. Runtime architecture: plain ES modules, engine/UI split

**Decision**: Vanilla JavaScript ES2020 modules, no build step, no npm, no CDN. The rendering engine (`fastled.js`, `perlin.js`, `palette.js`, `devices.js`, `effects/*`, `engine.js`, `master.js`) is DOM-free; `ui.js`/`api.js` are the only browser-coupled files. Page is served with `python3 -m http.server` (documented in quickstart); the same engine modules are imported by Node's built-in test runner (`node --test`, Node ≥ 18) for headless verification.

**Rationale**: FR-001 forbids installs/build steps. The engine/UI split gives SC-009 (agent verifies with zero human involvement) for free: `node --test "sim/test/cases/*.test.mjs"` exercises the exact modules the page runs, no browser automation stack needed. ES modules require HTTP (not `file://`), and `python3 -m http.server` is present on every macOS/Linux dev machine — an acceptable one-liner, already the pattern used for similar in-repo tools.

**Alternatives considered**: (a) Classic `<script>` tags + globals to support `file://` — rejected: loses Node importability, the single most valuable property for self-verification. (b) WASM-compiling the C++ effects (Emscripten) — rejected: perfect fidelity but adds a heavyweight toolchain and build step, violating FR-001; also makes "add a prototype effect in 15 min" (SC-008) much worse. (c) npm + bundler + Playwright — rejected: violates the zero-dependency constraint outright.

## R2. Fidelity strategy: port FakeFastLED's math, byte-exact

**Decision**: Port the FastLED math primitives from **ademuri/FakeFastLED** — the exact implementation the host tests link (`CMakeLists.txt:23-27`, fetched into `build/_deps/fake-fast-led-src/` at configure time) — using integer-exact JS (`Uint8`/`Uint16` semantics via masking). Primitives needed (verified by sweeping `lib/effect/*`): `sin16`, `sin8`, `cos8`, `scale8`, `qadd8`, `cubicwave8`, `ease8InOutApprox`, `lerp16by16`, `blend` with `SHORTEST_HUES`, `hsv2rgb_rainbow`, `random16`/`random16_set_seed`, plus `lib/math/Perlin.hpp` (`perlinNoise`, `perlinNoisePolar`) and `Effect::GetThresholdSin` (`lib/effect/Effect.cpp:5-13`: `sin16(x)/128`, threshold cut, `(v-t)*256/(256-t)` rescale).

**Rationale**: Reference vectors (R5) are produced by the host build, which uses FakeFastLED — so FakeFastLED, not upstream FastLED, is the ground truth to match byte-for-byte (SC-003). CHSV→CRGB conversion happens implicitly via `CHSV::operator CRGB()` → `hsv2rgb_rainbow` everywhere except `SwingingLights` (explicit call); the JS engine converts every effect's CHSV result through the same ported `hsv2rgb_rainbow`.

**Alternatives considered**: Porting upstream FastLED's versions — rejected where they differ from FakeFastLED, since the comparison target is the host build. (Implementation must diff the two when the fetched source is available; any intentional FakeFastLED simplifications become the spec for the JS port.)

## R3. Determinism: constructor-seeded effects get fixed seeds

**Facts**: No effect uses randomness at render time — `GetRGB` is pure in `(index, time, strip, packet)`. Three effects seed a per-device offset **at construction** from `analogRead(A0)`-derived seeds: `FireEffect.cpp:8-10` (`random16_set_seed(...); offset = random16()`), `FireflyEffect.cpp:7-9` (`randomSeed(...); offset_ = random(0, kBlinkPeriod/2)`), `RorschachEffect.cpp:8-10` (same pattern as Fire). On real hardware this de-syncs those effects slightly per device (intentional aesthetic); on the host build the fake `analogRead` is deterministic.

**Decision**: The JS registry constructs effects with the same deterministic PRNG sequence the host build produces (port FakeFastLED's `analogRead` stub + `random16` LCG and call them in the same construction order). Result: identical offsets to the vector generator, satisfying FR-013 across page loads. Additionally expose `sim.setEffectSeed(effectName, offset)` (or seed param in the API) so per-device variation can be demonstrated deliberately; default is always the host-matching seed.

**Rationale**: Makes vectors match without exporting private C++ members; keeps FR-013 (cross-reload determinism) exact. Fallback if the fake's sequence proves awkward to replicate: `VectorGen.cpp` prints the effective offsets (derivable by construction-order replay in C++) into `reference.json`, and JS consumes them as data. Either way the contract is "offsets live in one place and both sides agree."

## R4. Wire-index registry: exact mirror of LedManager.cpp

**Facts** (verified at `lib/led_manager/LedManager.cpp:12-38, 51-74, 109-142`):
- Weighted pool (`effects` vector, duplicates = weight): ColorCycle×2, ContrastBumps×2, Fire×1, Firefly×2, Lightning×1, Pride×1, RainbowBumps×4, Rainbow×4, Rorschach×2, Spark×4, SwingingLights×4 → **27 entries, wire indices 0–26**.
- Non-random tail (weight 0): SwingingLights (ex-police), StopLight, SimpleBlink(60), SimpleBlink(30), SimpleBlink(12), SimpleBlink(300), DisplayColorPalette, Dark → **indices 27–34**; Dark is 34 (last), DisplayColorPalette 33 (second-to-last).
- `GetEffect`: Arduino path does `index % 35`; host path asserts. **Simulator adopts the Arduino modulo path** (FR-014 tolerance; the assert is a test-harness aid, the wire behavior is modulo).
- Palettes: **22** (counted at `lib/effect/Effect.cpp:19-65`): 8 solids (Red, Orange, Yellow, Green, Aqua, Blue, Purple, Pink) + Rainbow, Warm, Cool, Yellow-Green, 80s Miami, Vaporwave, Cool (ex-Popo), Candy-Cane, Winter-Mint Candy-Cane, Fire, Pastel Rainbow, Jazz Cup, Yellow & Double Purp, Double Rainbow.
- `ControlEffect` is **outside** the registry (`LedManager.cpp:11,51-58`): when the current packet is SET_CONTROL, it renders the packet's RGB regardless of effect index.
- Master cadence (`RadioStateMachine.hpp:71-91`, `.cpp:201-208`): heartbeat 1000 ms; effect change every **60000 ms** picking `random(0, 27)` — i.e. only the weighted pool, weights via duplicates — and `random(0, 22)` palettes, delay always 0; current effect rebroadcast every 2000 ms; a received SET_EFFECT with `delay > 0` holds the effect for `delay` seconds (slave logic, `.cpp:51-53`).

**Decision**: `registry.js` declares `[EffectClass, weight]` pairs in the same order and derives the flat wire table + weighted pool from it; a test asserts index↔name mapping, pool size 27, total 35, last-two invariant, and `< 256`.

## R5. Reference vectors: additive C++ `vectorgen` target

**Decision**: Add `test/VectorGen.cpp`, a standalone executable (new CMake target, not part of `smalltests`/`largetests`) that reuses `FakeLedManager` (`lib/fake-led-manager/`, per-LED readback via `GetLed(i)`, pattern proven in `test/LedManagerTest.cpp:92`) and `FakeRadio`/fake state machine plumbing to render a sampled grid — every unique effect × representative palettes (a solid, a 2-color, a 3+-color, Double Rainbow) × representative devices (scarf, puck [Tiny+Circular], ufo [multi-strip Dim/Bright]) × times {0, 1, 1000, 60000, 2^31, 2^32−1 ms} — and print `sim/test/vectors/reference.json`. The JSON is committed; `vectors.test.mjs` compares JS output byte-for-byte. Regeneration is documented (`cmake .. && make vectorgen && ./vectorgen > ../sim/test/vectors/reference.json`) so drift after intentional firmware effect changes is a one-command refresh with a reviewable diff.

**Rationale**: SC-003/FR-020 need firmware ground truth without running C++ in the browser. Committing vectors keeps the web suite zero-dependency and makes firmware↔simulator drift visible in review. Sampling covers the time-boundary edge cases from the spec.

**Alternatives considered**: (a) gtest that loads JS via an embedded engine — rejected, heavyweight. (b) Golden screenshots — rejected, tests pixels not data, fragile. (c) Generating vectors in CI only — rejected: committed vectors let `node --test` run with no C++ toolchain present.

## R6. Central flag handling + strip semantics

**Facts** (`LedManager.cpp:76-107`): per strip, per LED: `virtual_index = Reversed ? led_count-1-i : i`; `Off → CRGB::Black` (short-circuit, effect not called); else effect renders, then `Dim → rgb / 8` (integer division per channel). `Tiny`/`Bright`/`Circular`/`Mirrored`/`Controller` pass through in the `StripDescription` for effects to interpret.

**Decision**: `engine.js` ports this loop verbatim, including the `Off` short-circuit and the *post-effect* Dim division. `flags.test.mjs` covers each behavior.

## R7. Time & clock model

**Decision**: `SimClock` holds `networkMillis` as an unsigned-32 value (wrap at 2^32, matching firmware `uint32_t`); modes: running (advance by wall-clock delta × speed, via `performance.now()` deltas — never absolute wall time), paused, or pinned (exact value set programmatically). Backgrounded tabs: on `visibilitychange`/first frame after a gap, the clock advances by the real elapsed delta (show continues from the correct position, per spec edge case) with a capped single-frame render (no burst of catch-up frames).

**Rationale**: FR-010/FR-011/FR-013; wall-clock never enters `GetRGB`, only the controllable clock does, which is what makes pinned-time rendering deterministic.

## R8. UI & rendering approach

**Decision**: Single `<canvas>` stage rendering all displayed devices — linear strips as rows of glowing dots, Circular strips as rings — with device/strip labels; controls in a side panel (device multi-select, effect list with wire indices, palette swatches, clock transport: pause/play/scrub/speed, master-mode toggle, SET_CONTROL color+delay picker). Dark UI theme so LED colors read like real installations; LED glow via radial gradients or shadow blur. Default on load: scarf + RainbowEffect + Rainbow palette, clock running (FR-017). URL query params mirror UI state for shareable deep links.

**Rationale**: Canvas comfortably renders ~300 LEDs at 60 fps (SC-006); one stage makes the multi-device sync story (US2) visually obvious. Aesthetic direction per FR-016: LEDs are the centerpiece, controls self-explanatory.

## R9. Programmatic API surface

**Decision**: `window.sim` (documented in `contracts/sim-api.md`): state setters (`setDevices`, `setEffect` by index or name, `setPalette`, `setControl(rgb, delaySeconds)`, `clearControl`), clock control (`pause`, `play`, `setTime`, `step(ms)`, `setSpeed`), master mode (`setMasterMode(bool)`, seeded RNG for reproducible autoplay), and readback (`getState()`, `getSnapshot()` → `{devices: [{name, strips: [{leds: [[r,g,b], …]}]}], time, effectIndex, paletteIndex, override}`). The same API is the engine's own interface — `ui.js` is just another client — so Node tests and the browser console exercise identical code paths.

**Rationale**: FR-011/FR-012/US3. Making the UI a client of the API guarantees the programmatic surface can do everything the UI can.
