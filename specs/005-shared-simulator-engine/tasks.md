# Tasks: Shared Simulator Rendering Engine

**Input**: Design documents from `specs/005-shared-simulator-engine/`

**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `contracts/`

**Tests**: Required by FR-010 through FR-012. Each implementation phase starts with a failing contract/regression test and follows red-green-refactor order.

**Organization**: Tasks are grouped by user story. Execute task IDs in order unless `[P]` explicitly permits independent work.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel because it touches independent files and does not depend on another incomplete task.
- **[Story]**: Maps to the user story in `spec.md`.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Establish pinned build inputs and the Wasm project skeleton without changing simulator behavior.

- [X] T001 Pin the host FakeFastLED FetchContent dependency to `f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca` in `CMakeLists.txt` and verify the existing host corpus remains green
- [X] T002 Create the Emscripten 6.0.3 build target, fixed compiler/link flags, ordered production source list, and generated output paths in `sim/wasm/CMakeLists.txt`
- [X] T003 [P] Add generated-artifact and temporary Wasm build patterns to `.gitignore` while explicitly allowing `sim/generated/*`

---

## Phase 2: Foundational (Authoritative C++ Catalogs)

**Purpose**: Make C++ the sole source for metadata and deterministic construction before exposing it to a browser.

**⚠️ CRITICAL**: No browser migration starts until this phase and its host tests are green.

- [X] T004 Write failing effect registration/order/weight/final-two/seed-override tests in `test/SimulatorCatalogTest.cpp`
- [X] T005 [P] Write failing palette-name/color-order and named-device/strip-flag catalog tests in `test/SimulatorCatalogTest.cpp`
- [X] T006 Add explicit offset constructors used by simulator seed overrides while preserving default hardware randomness in `lib/effect/FireEffect.{hpp,cpp}`, `lib/effect/FireflyEffect.{hpp,cpp}`, and `lib/effect/RorschachEffect.{hpp,cpp}`
- [X] T007 Implement the authoritative effect declaration/factory table and seed model in `lib/effect/EffectRegistry.{hpp,cpp}`
- [X] T008 Refactor `LedManager` to consume `EffectRegistry` and optional seed overrides without changing its expanded wire table in `lib/led_manager/LedManager.{hpp,cpp}`
- [X] T009 Add authoritative palette names beside the existing palettes and size/uniqueness checks in `lib/effect/Effect.{hpp,cpp}`
- [X] T010 Add `StripDescription::GetFlags()` and the authoritative named device table in `lib/device/StripDescription.hpp`, `lib/device/DeviceCatalog.{hpp,cpp}`
- [X] T011 Run `smalltests`, `largetests`, `ReferenceVectorTest`, and regenerate-to-temp corpus comparison to prove the C++ refactor is behavior-neutral

**Checkpoint**: Firmware/host construction and every browser-needed catalog value now come from tested C++ sources.

---

## Phase 3: User Story 1 - Trust the simulator output (Priority: P1) 🎯 MVP

**Goal**: Render browser frames through production C++ and remove every handwritten JavaScript renderer.

**Independent Test**: The committed reference corpus, all effect bytes, all palette bytes, flags, seeded effects, and the yellow-gradient property pass through the Wasm adapter with the old rendering modules absent.

### Tests for User Story 1 ⚠️

- [X] T012 [US1] Write failing native renderer lifecycle, metadata, render-buffer, invalid-handle, custom-strip, and byte-wrap tests in `test/SimulatorRendererTest.cpp`
- [X] T013 [P] [US1] Write a failing Node ABI/metadata/reference-vector integration suite in `sim/test/cases/sharedRenderer.test.mjs`
- [X] T014 [P] [US1] Update `sim/test/cases/gradientPower.test.mjs`, `flags.test.mjs`, `tolerance.test.mjs`, and `registry.test.mjs` to target the adapter contract and prove they fail before the adapter exists

### Implementation for User Story 1

- [X] T015 [US1] Implement lazy per-device production rigs, seed-specific effect construction, packed output ownership, and custom-strip rendering in `sim/wasm/SimulatorRenderer.{hpp,cpp}`
- [X] T016 [US1] Implement ABI v1 metadata, lifecycle, bulk rendering, input validation, and keepalive exports in `sim/wasm/SimulatorBridge.cpp`
- [X] T017 [US1] Build the first modularized ES module and Wasm artifact into `sim/generated/firefly-renderer.{js,wasm}` and verify Node can initialize it
- [X] T018 [US1] Implement ABI validation, UTF-8 metadata decoding, flag-name expansion, seed-tuple handle caching, and copied RGB buffers in `sim/js/renderer.js`
- [X] T019 [US1] Refactor `SimEngine` to source catalogs from `renderer.js` and delegate `_renderDevice` to the C++ buffer while preserving public state/timer semantics in `sim/js/engine.js`
- [X] T020 [US1] Update URL device validation and UI catalog consumers to use `SimEngine` metadata rather than duplicated globals in `sim/js/api.js` and `sim/js/ui.js`
- [X] T021 [US1] Migrate determinism/vector/control/master suites to the shared metadata/seed surfaces in `sim/test/cases/determinism.test.mjs`, `vectors.test.mjs`, `control.test.mjs`, and `master.test.mjs`
- [X] T022 [US1] Delete `sim/js/effects/`, `sim/js/fastled.js`, `sim/js/perlin.js`, `sim/js/palette.js`, and `sim/js/devices.js`; add a no-renderer-copy assertion to `sim/test/cases/sharedRenderer.test.mjs`
- [X] T023 [US1] Run the complete quoted-glob Node suite and in-browser harness, fixing only adapter or browser-owned orchestration regressions until both are green

**Checkpoint**: The browser has one production rendering implementation and no JavaScript fallback.

---

## Phase 4: User Story 2 - Change rendering once (Priority: P2)

**Goal**: Make artifact refresh deterministic, one-command, reviewable, and CI-enforced.

**Independent Test**: Two clean builds byte-match; a temporary compiled-source change makes the freshness check fail; rebuilding refreshes all artifact files and restores green.

### Tests for User Story 2 ⚠️

- [X] T024 [US2] Write failing manifest schema/hash/source-fingerprint/determinism tests in `sim/test/cases/artifact.test.mjs`
- [X] T025 [P] [US2] Add a failing shell integration test for unchanged and deliberately stale artifacts in `scripts/test-simulator-wasm.sh`

### Implementation for User Story 2

- [X] T026 [US2] Implement clean temporary builds, toolchain/version checks, stable source fingerprinting, atomic replacement, and deterministic `manifest.json` generation in `scripts/build-simulator-wasm.sh`
- [X] T027 [US2] Implement non-mutating clean rebuild and per-file byte comparison in `scripts/check-simulator-wasm.sh`
- [X] T028 [US2] Regenerate `sim/generated/firefly-renderer.js`, `sim/generated/firefly-renderer.wasm`, and `sim/generated/manifest.json` twice and prove byte-identical output
- [X] T029 [US2] Add pinned Emscripten installation/cache and artifact freshness steps before simulator tests in `.github/workflows/sim.yaml`
- [X] T030 [US2] Add `build:sim-wasm`, `check:sim-wasm`, and artifact-test scripts without runtime dependencies in `package.json`

**Checkpoint**: Any authoritative source change requires one rebuild command and stale generated code cannot pass CI.

---

## Phase 5: User Story 3 - Keep the simulator easy to run and automate (Priority: P3)

**Goal**: Preserve the static-server UX and public automation contract after asynchronous shared-engine initialization.

**Independent Test**: A clean checkout starts with the existing Python server command; URL deep links, `window.sim`, controls, snapshots, and the browser test page behave as before without a compiler.

### Tests for User Story 3 ⚠️

- [X] T031 [US3] Add failing initialization-error, URL compatibility, and synchronous-post-import public API cases to `sim/test/cases/api.test.mjs` and `sim/test.html`

### Implementation for User Story 3

- [X] T032 [US3] Add explicit loading/fatal initialization states without a fallback renderer in `sim/index.html`, `sim/js/api.js`, `sim/js/ui.js`, and `sim/style.css`
- [X] T033 [US3] Preserve `setEffectSeed`, list/get state shapes, query parameters, multi-device rendering, control override, and master mode contracts in `sim/js/engine.js` and `specs/001-web-simulator/contracts/sim-api.md`
- [X] T034 [US3] Serve the checked-in artifact with `python3 -m http.server 8642 -d sim`, manually exercise `window.sim`, and run the in-browser `/test.html` suite with zero artifact build step

**Checkpoint**: Existing users and automation keep the same entry point and API, now backed by shared C++.

---

## Phase 6: Polish & Cross-Cutting Validation

**Purpose**: Remove stale maintenance guidance, verify performance, and run every repository gate.

- [X] T035 [P] Replace JavaScript-port instructions with shared-engine/artifact workflow and architecture in `CLAUDE.md`, `docs/simulator.md`, `docs/build-and-test.md`, and `docs/index.md`
- [X] T036 [P] Add a superseding architecture note to `specs/001-web-simulator/research.md` and update `specs/004-fix-yellow-spike/contracts/flattened-gradient.md` so future readers do not restore the JS mirror
- [X] T037 Measure initialization and representative/all-device frame time in `sim/test/cases/sharedRenderer.test.mjs`; document results and ensure normal frames fit the 16 ms goal
- [X] T038 Run `./scripts/check-simulator-wasm.sh`, `npm run lint`, quoted-glob simulator tests, in-browser tests, `./ci.sh`, `./lint.sh check`, and all three PlatformIO environments; fix every in-scope failure
- [X] T039 Run `git diff --check`, inspect the complete source/generated diff for accidental duplication or unrelated changes, and mark every completed task `[X]` in `specs/005-shared-simulator-engine/tasks.md`

---

## Phase 7: Automated End-to-End and Visual Validation

**Purpose**: Exercise the served application as a user and retain actionable browser evidence for local and CI diagnosis.

- [X] T040 Write failing real-browser journeys for production-Wasm startup, URL deep links, UI-to-API controls, API-to-UI reflection, deterministic canvas pixels, the full browser harness, and missing-artifact fatal behavior
- [X] T041 Add a cross-platform Playwright runner, isolated static test server, deterministic browser settings, and trace/screenshot capture without adding simulator runtime dependencies
- [X] T042 Wire the E2E suite and diagnostic artifact upload into simulator CI, document local headed/debug/report workflows, and preserve the Python static-server command
- [X] T043 Run the E2E suite in headless and headed modes, capture representative linear/circular/multi-strip/error screens, inspect them visually, then rerun simulator lint/unit/browser/freshness gates and review the complete diff

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: Starts immediately.
- **Foundational (Phase 2)**: Depends on T001 and blocks every browser-facing task.
- **US1 (Phase 3)**: Depends on the authoritative catalogs and is the functional MVP.
- **US2 (Phase 4)**: Depends on a working generated artifact from US1.
- **US3 (Phase 5)**: Depends on the asynchronous adapter and engine migration from US1; can overlap late US2 work.
- **Polish (Phase 6)**: Depends on all desired stories.

### User Story Dependencies

- **US1 (P1)**: Independent shared-renderer value after the C++ foundation.
- **US2 (P2)**: Requires US1's build output but can be tested independently as artifact freshness/reproducibility.
- **US3 (P3)**: Requires US1's adapter but is independently validated through the static-server/public-API journey.

### Within Each User Story

- Write and run the listed tests first; record the expected failure.
- Implement the smallest contract surface that turns them green.
- Run the whole phase's tests before proceeding.
- Do not delete the old JS port until shared-renderer parity is green.

### Parallel Opportunities

- T003 can run beside T001/T002.
- T004 and T005 describe independent catalog assertions before converging in one test file; execute sequentially when one editor owns the file.
- T013 and T014 can be written while T012 covers the native side.
- T024 and T025 cover independent Node and shell contracts.
- T035 and T036 update separate documentation sets after code stabilizes.

---

## Implementation Strategy

### MVP First (US1)

1. Pin inputs and centralize the C++ catalogs.
2. Prove the host renderer is behavior-neutral.
3. Build the narrow ABI and browser adapter.
4. Run reference parity through Wasm.
5. Remove the handwritten rendering port only after parity is green.

### Incremental Delivery

1. **C++ foundation**: no visible change; production metadata becomes authoritative.
2. **Shared renderer**: page and tests use C++/Wasm; duplicate JS is removed.
3. **Artifact discipline**: deterministic rebuild and stale checks land.
4. **UX compatibility**: loading/errors and all public surfaces are verified.
5. **Full gates**: docs, performance, host, browser, lint, and firmware validation.

## Notes

- Generated JS is compiler glue, not a handwritten rendering implementation; never edit it manually.
- The committed JS, Wasm, and manifest always change together.
- Keep `DisplayColorPaletteEffect` and `DarkEffect` last in the C++ registration table.
- The parent agent performs final architectural and code review; no delegated reviewer can approve this migration.
