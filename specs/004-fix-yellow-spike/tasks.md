# Tasks: Fix the Bright Yellow LED Artifact in Rainbow-Palette Gradients

**Input**: Design documents from `specs/004-fix-yellow-spike/`

**Prerequisites**: plan.md, spec.md, research.md, contracts/flattened-gradient.md, quickstart.md

**Tests**: Explicitly requested by the spec (FR-006, SC-004): regression tests MUST be written first and demonstrably FAIL against pre-fix code (red), then pass after the fix (green). Do not skip the red step — it is an acceptance criterion.

**Organization**: Grouped by user story. Note one deliberate coupling: this repo's reference-vector corpus gates firmware and simulator against each other in CI, so US1 (firmware) and US2 (simulator + corpus regen) MUST land in the same commit — the tree is only fully green after Phase 4. The story phases are still independently *testable* via their own suites as described in each phase.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = uniform gradient brightness (firmware), US2 = simulator parity + corpus, US3 = regression protection (red/green tests)

## Path Conventions

Single repo, existing layout: firmware core in `lib/`, host tests in `test/`, simulator in `sim/`. All paths below are repo-relative. Host build dir is `build/` (create with `mkdir -p build && cd build && cmake .. -DBUILD_SIMULATOR=false`).

---

## Phase 1: Setup (baseline must be green before touching anything)

**Purpose**: Confirm the toolchain works and the pre-change tree passes, so every later red/green signal is attributable to this feature.

- [x] T001 Build the host test suite and confirm baseline green: `mkdir -p build && cd build && cmake .. -DBUILD_SIMULATOR=false && make -j8 && ./smalltests && ./largetests` (repo root: `./ci.sh` does the same). All existing tests must pass before any edit.
- [x] T002 [P] Confirm the sim suite baseline is green: `node --test "sim/test/cases/*.test.mjs"` (quoted glob required). Also `npm ci && npm run lint` once so later lint runs are meaningful.
- [x] T003 [P] Reproduce the artifact pre-fix and record the numbers, per quickstart.md §1: RainbowEffect, palette 8, 100-LED non-Tiny strip, v=128, t=0 must show LED 7 = (44,43,0) drive 87 vs neighbors 64–66. This is the ground truth the tests encode. (A throwaway host harness is fine; see quickstart for the compile line.)

**Checkpoint**: Baseline green + artifact reproduced and measured.

---

## Phase 2: User Story 3 — regression tests written and RED (Priority: P2, but TDD-first by design)

**Goal**: The uniform-drive property is encoded as tests on both sides, and both tests FAIL against the current rendering code — proving they can catch this bug class (SC-004, acceptance scenario US3-1).

**Independent Test**: Run each new test against the unmodified tree; each fails naming the offending LED/drive. (US3's "passes post-fix" half is validated in Phase 5.)

- [x] T004 [P] [US3] Write `test/GradientPowerTest.cpp` (gtest; auto-globs into `smalltests`, no CMake edit needed): implement the uniform-drive property from contracts/flattened-gradient.md §"Uniform-drive regression test property" — RainbowEffect with `writeSetEffect(0, 0, 8)` on `StripDescription(60, {})` and `StripDescription(60, {Bright})` across several `time_ms` values (e.g. 0, 1000, 5000), plus DisplayColorPaletteEffect on `StripDescription(100, {})`: for every LED, `r+g+b` ≤ 1.05 × (max drive over the palette's three stops rendered at the same brightness). Derive the endpoint baseline in-test (render `CHSV(HUE_RED/GREEN/BLUE, 255, v)` through the same conversion), don't hardcode 65. Failure message must print the LED index, its RGB, its drive, and the allowed max. Follow `test/EffectsTest.cpp` for the driving pattern and includes.
- [x] T005 [P] [US3] Write `sim/test/cases/gradientPower.test.mjs`: same property against the JS ports — `import { test, assert } from '../harness.js'`, `makeStrip(60, [])` / `makeStrip(60, ['Bright'])` from `../../js/devices.js`, `makeRainbowEffect()` from `../../js/effects/rainbow.js` and `makeDisplayColorPaletteEffect()` from `../../js/effects/displayColorPalette.js`, show object `{ paletteIndex: 8 }`. Same 1.05 threshold and endpoint-baseline derivation (use `hsv2rgbRainbow` from `../../js/fastled.js`).
- [x] T006 [US3] Prove RED: `cd build && make smalltests && ./smalltests --gtest_filter=GradientPowerTest*` MUST FAIL (expect a drive-87 LED vs ~68 allowed), and `node --test "sim/test/cases/gradientPower.test.mjs"` MUST FAIL identically. Capture both failure outputs (paste into the PR/commit description). If either passes here, the test is wrong — fix the test, not the threshold.

**Checkpoint**: Two failing tests demonstrating the bug on both sides. Commit nothing yet (or commit to the feature branch knowing CI is red until Phase 4).

---

## Phase 3: User Story 1 — firmware fix (Priority: P1) 🎯 core of the MVP

**Goal**: Gradient-rendering effects drive uniform power; the lone yellow LED is gone in firmware rendering.

**Independent Test**: `./smalltests --gtest_filter=GradientPowerTest*:EffectsTest*` passes; the quickstart §1 harness now shows LED 7 = (32,32,0) drive 64, all cycle LEDs in 63–66. (`ReferenceVectorTest` is EXPECTED red until Phase 4 regenerates the corpus — that is the safety net working.)

- [x] T007 [US1] Add `FlattenedGradientRGB` to `lib/effect/Effect.hpp` (protected, after `GetThresholdSin`) and `lib/effect/Effect.cpp`, copying the exact signature, doc comment, and body from contracts/flattened-gradient.md §C++. Preserve the `(uint32_t)` casts (overflow) and the `sum > reference_sum` guard (v=0 safety) exactly.
- [x] T008 [P] [US1] `lib/effect/RainbowEffect.cpp`: in the varying-color-palette else-branch, change both `return color;` statements (Tiny and normal sub-branches) to `return FlattenedGradientRGB(color);`. Do NOT touch the solid-color (`palette.Size() < 2`) branch.
- [x] T009 [P] [US1] `lib/effect/ColorCycleEffect.cpp`: in the multi-color gradient else-branch, change `return color;` to `return FlattenedGradientRGB(color);` (after the `Bright` v-halving). Do NOT touch the solid-color branch.
- [x] T010 [P] [US1] `lib/effect/RainbowBumpsEffect.cpp`: change the final `return color;` to `return FlattenedGradientRGB(color);` (after `color.v = GetThresholdSin(...)`).
- [x] T011 [P] [US1] `lib/effect/DisplayColorPaletteEffect.cpp`: change the final `return color;` to `return FlattenedGradientRGB(color);` (after the `Bright` v-halving; applies to both gradient branches since they share the return).
- [x] T012 [US1] Build and verify firmware-side green-except-vectors: `cd build && make -j8 && ./smalltests --gtest_filter=GradientPowerTest*` now PASSES; `./smalltests --gtest_filter=EffectsTest*` passes (fuzz, fatal UBSan); full `./smalltests` fails ONLY in `ReferenceVectorTest` (corpus intentionally stale — confirm no other failures). Re-run the quickstart §1 harness: LED 7 must be (32,32,0) drive 64.

**Checkpoint**: Firmware renders flat gradients; only the corpus test is (expectedly) red.

---

## Phase 4: User Story 2 — simulator parity + corpus regen (Priority: P1) 🎯 completes the MVP

**Goal**: The simulator applies identical math byte-for-byte and the regenerated corpus pins both sides; the whole tree is green.

**Independent Test**: Full `./smalltests` and full `node --test "sim/test/cases/*.test.mjs"` both pass against the regenerated corpus.

- [x] T013 [US2] Add `flattenedGradientRGB` to `sim/js/fastled.js`, copying the exact body from contracts/flattened-gradient.md §JavaScript (uses the existing `hsv2rgbRainbow`; `Math.trunc` division; mutate-and-return the fresh object `hsv2rgbRainbow` produces).
- [x] T014 [P] [US2] `sim/js/effects/rainbow.js`: import `flattenedGradientRGB` from `../fastled.js`; in the varying-color-palette branch, replace both `return hsv2rgbRainbow(color);` calls (Tiny and normal) with `return flattenedGradientRGB(color);`. Solid-color paths unchanged.
- [x] T015 [P] [US2] `sim/js/effects/colorCycle.js`: same substitution in the multi-color gradient branch only.
- [x] T016 [P] [US2] `sim/js/effects/rainbowBumps.js`: same substitution at the final conversion.
- [x] T017 [P] [US2] `sim/js/effects/displayColorPalette.js`: same substitution at the final conversion (covers both gradient branches).
- [x] T018 [US2] Regenerate the corpus in the same working tree: `cd build && make vectorgen && ./vectorgen > ../sim/test/vectors/reference.json`. Do not edit `test/VectorGen*.cpp` — determinism traps (PRNG reset order, Firefly offset 423, Fire/Rorschach LCG seeds) are already encoded there.
- [x] T019 [US2] Verify full green both sides: `cd build && ./smalltests && ./largetests` and `node --test "sim/test/cases/*.test.mjs"` all pass (vectors test now green against regenerated corpus; gradientPower green on both sides).
- [x] T020 [US2] Verify corpus diff scope (SC-005), per quickstart.md §5: only cases belonging to Rainbow / Color Cycle / Rainbow Bumps / Display Color Palette changed (plus `meta.firmwareGitDescribe`). If any other effect's case changed, a call site is misplaced — stop and fix.

**Checkpoint**: MVP complete — bug fixed, sim byte-exact, corpus regenerated, both regression tests green.

---

## Phase 5: Polish & Cross-Cutting

**Purpose**: CI parity, style, docs, and the red→green proof assembled for review.

- [x] T021 [P] Firmware target builds (what CI builds): `pio run -e node && pio run -e fancy-node && pio run -e controller`. (On this machine `pio` runs via `uv`; `[env:node]` needs the Apple-silicon `node-arm64` bossac package variant — see CLAUDE.md Build System notes.)
- [x] T022 [P] Lint both sides: `./lint.sh check` (clang-format 18.1.8 from `~/.local/bin`, NOT the brew 22 one) and `npm run lint`. Fix any formatting the new files introduce.
- [x] T023 [P] Update `docs/led-effects.md` (and `docs/simulator.md` if it describes effect rendering): document that the four palette-showcase effects flatten the hsv2rgb_rainbow yellow-band power boost for interpolated gradient colors, and why noise/texture effects deliberately keep it. Keep it to a short paragraph in each.
- [x] T024 Assemble the red→green evidence in the commit/PR description: T006 failure output (both sides), T012 post-fix pixel dump (LED 7: 87→64), T020 corpus-diff scope listing. Single commit containing firmware + sim + corpus + tests + docs (the corpus gate makes splitting commits leave CI red mid-stack).
- [x] T025 Run the full acceptance sweep once more from a clean state: `./ci.sh && node --test "sim/test/cases/*.test.mjs"` — everything green; spec SC-001..SC-005 each verifiably satisfied (map each SC to its evidence from T006/T012/T019/T020).

---

## Dependencies & Execution Order

```text
Phase 1 (Setup)          T001 → {T002, T003} in parallel
Phase 2 (US3, red)       {T004, T005} in parallel → T006   (MUST precede Phase 3 — red is unprovable after the fix)
Phase 3 (US1, firmware)  T007 → {T008..T011} in parallel → T012
Phase 4 (US2, sim)       T013 → {T014..T017} in parallel → T018 → T019 → T020   (T018 requires Phase 3 complete)
Phase 5 (Polish)         {T021, T022, T023} in parallel → T024 → T025
```

- US3's tests must exist and fail BEFORE US1/US2 change rendering (SC-004's "fails pre-fix" half).
- US1 must complete before T018 (the corpus is generated from firmware output).
- US1 + US2 + the regenerated corpus land in ONE commit; the tree is not fully green between them by design (bidirectional corpus gate).

## Parallel Opportunities

- T002 ∥ T003 (independent checks); T004 ∥ T005 (different files/sides); T008–T011 (four independent effect files); T014–T017 (four independent JS files); T021–T023 (build/lint/docs).

## Implementation Strategy

MVP = Phases 1–4 (the fix, proven and pinned). Phase 5 is finishing work required before merge but not before demoing. If anything in Phase 4 diverges byte-wise between C++ and JS, diff a single failing corpus case's bytes first — the mismatch is almost always integer-division or a call site applied on a solid-color path (see contracts/flattened-gradient.md notes).
