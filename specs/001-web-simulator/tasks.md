# Tasks: Web Simulator

**Input**: Design documents from `/specs/001-web-simulator/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/sim-api.md, contracts/reference-vectors.md, quickstart.md

**Tests**: INCLUDED — the spec explicitly requires a shipped automated test suite (FR-019/FR-020, SC-009).

**Organization**: Tasks grouped by user story. Foundational phase carries the ported engine because every story renders through it.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = preview page, US2 = device catalog & sync, US3 = programmatic API & verification, US4 = master mode & control

## Phase 1: Setup

- [X] T001 Create `sim/` skeleton: `sim/index.html` (placeholder), `sim/style.css` (empty), `sim/js/effects/`, `sim/test/cases/`, `sim/test/vectors/` directories with `.gitkeep` where needed
- [X] T002 Run `mkdir -p build && cd build && cmake .. -DBUILD_SIMULATOR=false` to fetch FakeFastLED into `build/_deps/fake-fast-led-src/`; read its `hsv2rgb_rainbow`, `sin16`, `sin8`, `cos8`, `scale8`, `qadd8`, `cubicwave8`, `ease8InOutApprox`, `lerp16by16`, `blend`, `random16`/`random16_set_seed`, `analogRead`, `random`/`randomSeed` implementations and record exact algorithms + the deterministic seed values the host build produces for Fire/Firefly/Rorschach construction in `specs/001-web-simulator/research-fastled-notes.md`

---

## Phase 2: Foundational (Blocking Prerequisites)

**⚠️ CRITICAL**: The DOM-free engine — everything below is required before any user story renders a pixel.

- [X] T003 [P] Port FastLED math to `sim/js/fastled.js` per T002 notes: byte/word-exact `sin16`, `sin8`, `cos8`, `scale8`, `qadd8`, `cubicwave8`, `ease8InOutApprox`, `lerp16by16`, `blend` (SHORTEST_HUES), `hsv2rgb_rainbow`, `random16`/`random16_set_seed`, CHSV/CRGB helpers, `MAX_UINT8`/`MAX_UINT16`; all integer math masked to firmware widths
- [X] T004 [P] Port `lib/math/Perlin.hpp` + `lib/math/Math.hpp` helpers to `sim/js/perlin.js` (`perlinNoise`, `perlinNoisePolar`, permutation table verbatim)
- [X] T005 [P] Port `lib/color/ColorPalette.cpp` and the 22 palettes from `lib/effect/Effect.cpp:19-65` to `sim/js/palette.js` (`getColor`, `getGradient` with hue-wrap lerp; palette names per spec; index = wire byte) plus `Effect::GetThresholdSin` from `lib/effect/Effect.cpp:5-13` as shared helper
- [X] T006 [P] Port `lib/device/StripDescription.hpp` flags and the full device catalog from `lib/device/Devices.hpp` to `sim/js/devices.js` (every device: name, strips with led counts/flags, milliamps)
- [X] T007 [P] Port simple effects to `sim/js/effects/`: `dark.js` (DarkEffect), `simpleBlink.js` (SimpleBlinkEffect, parameterized ms), `stopLight.js` (StopLightEffect), `displayColorPalette.js` (DisplayColorPaletteEffect), `control.js` (ControlEffect reading show-state RGB) from the corresponding `lib/effect/*.cpp`
- [X] T008 [P] Port gradient/wave effects to `sim/js/effects/`: `colorCycle.js`, `rainbow.js`, `rainbowBumps.js`, `contrastBumps.js` from `lib/effect/{ColorCycleEffect,RainbowEffect,RainbowBumpsEffect,ContrastBumpsEffect}.cpp`
- [X] T009 [P] Port sparkle/flash effects to `sim/js/effects/`: `spark.js`, `lightning.js`, `pride.js` from `lib/effect/{SparkEffect,LightningEffect,PrideEffect}.cpp` (Pride uses `blend` SHORTEST_HUES)
- [X] T010 [P] Port sine/threshold effects to `sim/js/effects/`: `swingingLights.js` (explicit `hsv2rgb_rainbow`), `firefly.js` (constructor seed offset per T002 notes) from `lib/effect/{SwingingLights,FireflyEffect}.cpp`
- [X] T011 [P] Port Perlin effects to `sim/js/effects/`: `fire.js`, `rorschach.js` (constructor `random16()` seed offsets per T002 notes) from `lib/effect/{FireEffect,RorschachEffect}.cpp`
- [X] T012 Create `sim/js/effects/registry.js`: `[EffectFactory, weight]` declarations mirroring `lib/led_manager/LedManager.cpp:12-35` order exactly; derive 35-entry wire table (0–26 weighted pool, 27–34 non-random, DisplayColorPalette=33, Dark=34), `randomPoolSize`, name↔index lookups, `index % 35` tolerant access, ControlEffect held separately; construct seeded effects in host-build order so default offsets match vectors (research R3)
- [X] T013 Create `sim/js/engine.js`: `SimEngine` per data-model.md — `SimClock` (uint32 wrap, pause/play/setTime/step/setSpeed, wall-clock deltas only, visibility-gap handling), `ShowState` (effect/palette/delay, SET_CONTROL override with expiry, control precedence per `LedManager::GetCurrentEffect`), `RunEffect` port (`LedManager.cpp:76-107`: Reversed inversion, Off short-circuit, post-effect Dim `/8`), `getSnapshot()`/`getState()`/`listEffects()`/`listPalettes()`/`listDevices()` per contracts/sim-api.md

**Checkpoint**: `node -e "…"` can construct SimEngine, render a snapshot — user stories can begin.

---

## Phase 3: User Story 1 - Preview any effect on a device without hardware (Priority: P1) 🎯 MVP

**Goal**: Open the page → immediately see a device animating; switch effect/palette/device live; clean, appealing UI.

**Independent Test**: `python3 -m http.server 8642 -d sim`, open the page: scarf animates Rainbow by default; changing effect/palette/device updates live without reload; animation smooth.

### Implementation for User Story 1

- [X] T014 [US1] Build `sim/index.html`: semantic layout — canvas stage, side control panel (device select, effect list showing wire index + name, palette swatch list), footer status bar (current time/effect/palette); module script bootstrapping engine + UI; default state scarf/Rainbow effect/Rainbow palette running (FR-017)
- [X] T015 [US1] Build `sim/style.css`: dark stage theme, LED-glow aesthetics, clean readable control panel, responsive flex layout (no horizontal page scroll), visually distinct paused-vs-dark states (spec edge case)
- [X] T016 [US1] Build `sim/js/ui.js` (core): requestAnimationFrame loop driving `SimEngine`, canvas renderer drawing linear strips as glowing dot rows with device/strip labels, control panel wiring (effect/palette/device selection → engine), clock transport UI (pause/play, scrub slider, speed selector, numeric time field)
- [X] T017 [US1] Manual+screenshot verification: serve `sim/`, load page, cycle through several effects/palettes/devices, capture screenshot, confirm US1 acceptance scenarios 1–5

**Checkpoint**: MVP — shows can be previewed without hardware.

---

## Phase 4: User Story 2 - Verify shows across real device layouts and strip behaviors (Priority: P2)

**Goal**: Multiple real devices side-by-side, in sync, with per-strip flags honored and circular strips drawn as rings.

**Independent Test**: Display ufo + puck simultaneously with one effect; ufo's Dim/Bright strips differ correctly, puck renders as a ring, both stay in sync.

### Implementation for User Story 2

- [X] T018 [US2] Extend `sim/js/ui.js` canvas layout: multi-device stage (device multi-select), Circular strips rendered as rings, multi-strip devices grouped with strip labels/flag badges, auto-layout for all catalog devices at once
- [X] T019 [US2] Visual verification of flags: render ufo (Dim/Bright), rainbow_cloak (Tiny/Circular ends), a Reversed test strip; confirm central behaviors visibly correct and devices synchronized; capture screenshot for US2 acceptance scenarios 1–4

**Checkpoint**: Whole-catalog preview with faithful strip semantics.

---

## Phase 5: User Story 3 - Programmatic driving and exact-color verification (Priority: P2)

**Goal**: `window.sim` API + Node/browser test suite + firmware reference vectors — the simulator becomes a self-verifying test instrument.

**Independent Test**: `node --test "sim/test/cases/*.test.mjs"` passes all suites with zero human involvement; console driving per quickstart works.

### Implementation for User Story 3

- [X] T020 [P] [US3] Build `sim/js/api.js`: expose engine as `window.sim` implementing every method/guarantee in contracts/sim-api.md (chainable setters, name-or-index resolution, `setEffectSeed`, synchronous pure `getSnapshot`); make `ui.js` consume the same API object; add URL query-param state init (`?device=&effect=&palette=&t=&paused=`)
- [X] T021 [P] [US3] Write `sim/test/cases/registry.test.mjs`: wire table length 35, pool 27, weights per `LedManager.cpp`, DisplayColorPalette=33/Dark=34 invariant, name↔index round-trip, `<256` bound, palette count 22 and palette contents vs `Effect.cpp`
- [X] T022 [P] [US3] Write `sim/test/cases/determinism.test.mjs`: identical (device,effect,palette,seed,time) ⇒ deep-equal snapshots across repeated calls and fresh engine instances; snapshot changes when time steps; `getSnapshot` purity (no clock advance)
- [X] T023 [P] [US3] Write `sim/test/cases/flags.test.mjs`: Reversed index inversion, Off short-circuit black, Dim integer `/8` post-effect, flag pass-through to effects (Tiny/Circular reach `getRGB`), multi-strip global LED ordering
- [X] T024 [P] [US3] Write `sim/test/cases/tolerance.test.mjs`: all 256 effect bytes × sample palettes render without throw (mod-35 equivalence), all 256 palette bytes tolerated, 0-LED strip renders empty, time 0 / 2^31 / 2^32−1 render, per FR-014 and spec edge cases
- [X] T025 [US3] Create Node runner glue so `node --test "sim/test/cases/*.test.mjs"` discovers `sim/test/cases/*.test.mjs`, and build `sim/test.html` in-page runner executing the same case modules against the browser engine with pass/fail summary banner
- [X] T026 [P] [US3] Write `test/VectorGen.cpp` + additive `vectorgen` CMake target in `CMakeLists.txt`: render the sampled grid from contracts/reference-vectors.md through `FakeLedManager` (pattern of `test/LedManagerTest.cpp`), emit byte-stable JSON incl. `meta.effectSeeds`; MUST NOT alter existing targets, `smalltests`, `largetests`, or `ci.sh` behavior; format per `./lint.sh check`
- [X] T027 [US3] Build vectorgen, generate and commit `sim/test/vectors/reference.json`; write `sim/test/cases/vectors.test.mjs` comparing JS snapshots byte-for-byte per the consumption contract (case id + LED index + expected/actual on failure); iterate on `sim/js/fastled.js`/effect ports until byte-exact (this is the fidelity gate — expect debugging)
- [X] T028 [US3] Full self-verification run: `node --test "sim/test/cases/*.test.mjs"` all green; browser `sim/test.html` all green; console-drive per quickstart.md and confirm US3 acceptance scenarios 1–5

**Checkpoint**: Simulator is provably faithful to firmware; agent can verify end-to-end unattended.

---

## Phase 6: User Story 4 - Master mode and control packets (Priority: P3)

**Goal**: Autoplay like a real master (60 s weighted changes) and SET_CONTROL solid-color overrides with delay expiry.

**Independent Test**: Enable master mode → effects rotate on cadence from weighted pool only; `sim.setControl([255,0,0],10)` → all devices red for 10 s then show resumes.

### Implementation for User Story 4

- [X] T029 [P] [US4] Build `sim/js/master.js`: master-mode scheduler on the network clock — change effect every 60 000 ms via seeded RNG `random(0,27)` + `random(0,22)` palette (delay 0), honor manual SET_EFFECT delay holds (`RadioStateMachine.cpp:51-53,201-208` semantics), reset timer on manual selection; integrate with engine + `setMasterMode(on, seed)`
- [X] T030 [P] [US4] Extend `sim/index.html`/`sim/js/ui.js`/`sim/style.css`: master-mode toggle with "next change" countdown, SET_CONTROL panel (color picker + delay + send/clear), override state indicator
- [X] T031 [P] [US4] Write `sim/test/cases/control.test.mjs`: override precedence over any effect index, exact solid RGB on all strips (Dim still applies), expiry at delay boundary returns to prior effect, delay 0 persists until clear, per US4 scenario 2
- [X] T032 [US4] Write `sim/test/cases/master.test.mjs`: with seeded RNG — change fires at exactly 60 000 ms steps, selections drawn only from indices 0–26 with distribution matching weights over many steps, delay hold postpones change, manual set resets timer; then verify US4 scenarios 1–3 in browser

**Checkpoint**: Full show-flow simulation.

---

## Phase 7: Polish & Cross-Cutting Concerns

- [X] T033 [P] Documentation: add `docs/simulator.md` (usage, API pointer, vector regeneration workflow) linked from `docs/index.md`; add simulator section to `docs/build-and-test.md`; add `sim/` + `node --test "sim/test/cases/*.test.mjs"` mention to `CLAUDE.md` build section
- [X] T034 [P] Visual polish pass on `sim/index.html`/`sim/style.css`/`sim/js/ui.js`: glow quality, spacing, empty states, keyboard focus styles; confirm SC-001 (first-time flow < 30 s) and SC-006 (all-devices smooth) with a screenshot
- [X] T035 Run `./lint.sh check` (fix formatting of `test/VectorGen.cpp` if flagged) and `./ci.sh` to prove firmware suites are untouched and green
- [X] T036 Execute every step of `specs/001-web-simulator/quickstart.md` end-to-end as written; fix any drift between docs and behavior
- [X] T037 Adversarial review (verifier agent) of the full diff against CLAUDE.md invariants + spec FRs before commit

---

## Dependencies & Execution Order

- **Phase 1 → Phase 2**: T002's FakeFastLED notes gate T003 (math port) and seed values in T010–T012.
- **Phase 2 internal**: T003–T011 all [P] after T002 (T004/T005/T006 don't strictly need T002 but are free to start immediately after T001); T012 needs T007–T011; T013 needs T012 + T005 + T006.
- **US1 (T014–T017)**: needs Phase 2. T014/T015 can draft in parallel; T016 needs both; T017 last.
- **US2 (T018–T019)**: needs US1's ui.js (extends it) — sequential after US1.
- **US3 (T020–T028)**: needs Phase 2 only — **can run in parallel with US1/US2** (api.js and test cases don't touch ui.js except T020's final wiring). T021–T024 and T026 all [P]; T025 after T021–T024; T027 after T026 + T003–T013; T028 last.
- **US4 (T029–T032)**: T029/T031 need Phase 2; T030 needs US1; T032 after T029.
- **Polish**: after all stories; T037 (verifier) strictly last before commit.

### Parallel Opportunities

- Biggest fan-out: T003–T011 (9 independent port tasks) after T002.
- Test-case authoring T021–T024 + T026 (5 tasks) in parallel once T013 exists.
- US3's C++ side (T026) is independent of all web UI work.

---

## Implementation Strategy

**MVP first**: Phases 1–3 deliver the P1 preview page. **But do not commit before Phase 5 (US3)**: the fidelity gate (T027) is what makes the effect ports trustworthy — a pretty page rendering wrong colors is worse than no page. Recommended execution: Phases 1–2, then US1 and US3 in parallel, byte-exactness converged in T027, then US2, US4, polish, verifier, single commit (or commit per checkpoint after T028 passes).
