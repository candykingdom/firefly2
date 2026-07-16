# Session Handoff — 2026-07-15

**Status**: Architecture/specification checkpoint complete; implementation has deliberately not started.

## START HERE

Read these files in order from a fresh session with zero chat context:

1. `CLAUDE.md` — repository commands and invariants.
2. `specs/005-shared-simulator-engine/spec.md` — required outcomes and scope.
3. `specs/005-shared-simulator-engine/research.md` — decisions, rejected alternatives, and toolchain pins.
4. `specs/005-shared-simulator-engine/contracts/renderer-abi.md` — planned C++/browser boundary.
5. `specs/005-shared-simulator-engine/contracts/artifact.md` — generated-artifact and freshness contract.
6. `specs/005-shared-simulator-engine/plan.md` — continue below this handoff for the implementation structure.
7. `specs/005-shared-simulator-engine/tasks.md` — execute T001–T039 in order.
8. `docs/simulator.md` and `specs/001-web-simulator/research.md` — current handwritten-JS architecture that this feature supersedes.

## Purpose

Replace the browser simulator's handwritten JavaScript copy of the C++ rendering engine with a committed WebAssembly build of the production C++ renderer, while preserving the static-server UX and public `window.sim` API.

## Done and committed in this checkpoint

This handoff commit contains the complete pre-implementation artifact set:

- `spec.md` plus a fully passing 16-item requirements checklist;
- `plan.md` with the chosen source/build structure;
- `research.md` documenting WebAssembly, C ABI, catalog, seed, build, and testing decisions;
- `data-model.md`;
- `contracts/renderer-abi.md` and `contracts/artifact.md`;
- `quickstart.md`;
- `tasks.md` with 39 TDD-ordered implementation and validation tasks;
- `.specify/feature.json` pointing at `specs/005-shared-simulator-engine`.

The branch was created from `origin/master` at `82ce715`. No production C++, JavaScript, workflow, generated artifact, or existing simulator documentation was changed before this checkpoint.

## Not done or not proven

- T001–T039 are all open. No implementation code exists yet.
- Emscripten is not installed/active on this machine (`emcc` and `emcmake` were absent when checked).
- It is not yet proven that the current FakeFastLED/core source set compiles under Emscripten 6.0.3.
- No Wasm module has been generated, loaded in Node, or loaded in a browser.
- Artifact byte reproducibility, source freshness checks, module size, and the 16 ms frame goal are design commitments, not measured results.
- No host, simulator, browser, lint, PlatformIO, or hardware tests were rerun because implementation has not begun. Existing `master` was green before this branch, but that is not evidence for feature 005.
- Hardware behavior is untouched and untested by this feature checkpoint.

## Next step

Start at T001 in `tasks.md`:

1. Pin top-level CMake's floating FakeFastLED fetch to `f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca` and prove the existing host/reference corpus remains green.
2. Add the Emscripten target skeleton and ignore rules (T002–T003).
3. Write `test/SimulatorCatalogTest.cpp` first and capture its compile/test failure before implementing C++ catalogs or seed overrides (T004–T005).
4. Complete and validate the C++-only foundation through T011 before touching the browser port.

Do not delete any JavaScript renderer file until the Wasm adapter matches the complete reference corpus at T021–T023.

## Blockers and open questions

- No user decision is currently blocked. The user explicitly chose shared code over the prior no-build-source architecture.
- Tooling prerequisite: install/activate the exact Emscripten SDK 6.0.3 for artifact work. Keep it local or otherwise avoid an unpinned global `latest` toolchain.
- The C ABI in `contracts/renderer-abi.md` is the planned contract, not validated code. If implementation reveals a necessary adjustment, update the contract and its tests before changing the adapter.
- The user relayed Adam's concern verbally; no matching recent GitHub comment was found. Do not claim a GitHub approval or quote that was not posted.

## Key facts and corrections

- The duplication is broader than the yellow-spike helper. Current production copies live in `sim/js/effects/`, `sim/js/fastled.js`, `sim/js/perlin.js`, `sim/js/palette.js`, and `sim/js/devices.js`.
- `specs/001-web-simulator/research.md` explicitly rejected Wasm because of the old no-toolchain/no-build requirement. Feature 005 supersedes that decision by committing generated Wasm + ES-module glue: viewers still run `python3 -m http.server 8642 -d sim` without compiling, while developers rebuild after C++ changes.
- Keep JavaScript only for browser-owned clock transport, deterministic autoplay orchestration, URL parsing, `window.sim`, snapshots, and canvas UI. `LedManager::RunEffect`, strip flags, effect bodies, palettes, devices, FastLED math, noise, and registry metadata move behind the C++ artifact.
- Planned pins: Emscripten `6.0.3`; FakeFastLED `f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca`. At handoff time FakeFastLED `master` resolved to that commit, but top-level `CMakeLists.txt` still follows the branch until T001.
- Reference simulator seeds are Fire `6198`, Firefly `423`, and Rorschach `24359`. Hardware keeps random construction defaults; only simulator construction gets explicit overrides.
- Emscripten modularized ES-module output supports browser and Node initialization through a promise. The plan uses top-level await in `sim/js/renderer.js`, then preserves synchronous `SimEngine` methods after module import resolves.
- Preserve every `CLAUDE.md` invariant, especially the final two effects, byte-sized indices, invalid-packet tolerance, central `Reversed`/`Dim`/`Off` handling, and the existing quoted simulator test glob.
- Environment observed at handoff: macOS arm64, Node `v26.5.0`, CMake `4.4.0`, Apple Clang `21.0.0`, Homebrew available, no Docker/Podman/emcc detected.
- The previous two controller PRs are unrelated to feature 005 and remain open/green: #65 at `ed63321` (base `master`) and stacked #66 at `bb60978` (base `adam-controller-config`). They require Adam's hardware review. This branch must not absorb their worktree changes.
- Review comments already left on #65 cover palette-preview indexing, wrong carousel preview, missing choose-slot cancel, and mismatched button LEDs. Comments on #66 cover the 1 MHz shared I2C bus and ignored FRAM save failures.

---

# Implementation Plan: Shared Simulator Rendering Engine

**Branch**: `codex/shared-simulator-engine` | **Date**: 2026-07-15 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/005-shared-simulator-engine/spec.md`

## Summary

Replace the browser's handwritten JavaScript rendering port with a WebAssembly build of the production C++ effect engine. Centralize effect metadata, palette names, device metadata, and deterministic simulator seeds in C++; expose a small versioned C ABI; keep only browser state/orchestration, the public API, and canvas UI in JavaScript. Commit the generated ES-module glue and Wasm binary so the existing static-server workflow stays build-free, and make CI rebuild with a pinned toolchain and reject any stale artifact.

## Technical Context

**Language/Version**: C++14 for shared firmware/renderer sources; ES2022 modules for the browser adapter, UI, and Node tests

**Primary Dependencies**: Emscripten SDK 6.0.3; ademuri/FakeFastLED pinned at `f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca`; existing Firefly generic libraries; Node.js 22 for headless tests

**Storage**: Committed generated files under `sim/generated/` plus a source/toolchain manifest; no runtime persistence

**Testing**: Existing CMake/gTest host suite, Node built-in test runner, in-browser harness, reference-vector corpus, PlatformIO firmware builds, ESLint, artifact rebuild-and-compare check

**Target Platform**: Modern evergreen browsers and Node.js 22 executing WebAssembly; existing SAMD/STM32/ESP32 firmware targets remain unchanged

**Project Type**: Embedded C++ library plus static browser application

**Performance Goals**: A normal simulator frame, including all LEDs for a selected catalog device, renders comfortably within a 16 ms frame budget on a typical laptop; initialization does not visibly delay first paint

**Constraints**: No runtime backend, CDN, npm runtime dependency, or firmware compilation required to view a clean checkout; generated artifact must be deterministic; public `window.sim` and URL contracts remain compatible; no JavaScript fallback renderer

**Scale/Scope**: 19 unique effect registrations / 35 wire indices, 22 palettes, 22 devices, up to 192 LEDs per catalog device, three construction-seeded effects, one versioned renderer ABI

## Constitution Check

*GATE: Passed before research and after design.*

The constitution file contains placeholders and establishes no enforceable project gates. The design follows the concrete repository guidance in `CLAUDE.md`: production effect ordering remains authoritative, the final-two-effect invariant stays enforced, wire indices remain bytes, invalid inputs remain non-crashing, `RunEffect` continues to own central strip flags, effect fuzz coverage remains, and simulator/firmware drift remains CI-gated. No hardware loop, radio protocol, watchdog, or debug-macro behavior changes.

## Project Structure

### Documentation (this feature)

```text
specs/005-shared-simulator-engine/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── artifact.md
│   └── renderer-abi.md
├── checklists/
│   └── requirements.md
└── tasks.md
```

### Source Code (repository root)

```text
lib/
├── device/
│   ├── DeviceCatalog.cpp              # authoritative named device table
│   └── DeviceCatalog.hpp
├── effect/
│   ├── Effect.cpp                     # palette colors + authoritative names
│   ├── Effect.hpp
│   ├── EffectRegistry.cpp             # authoritative effect order/weights/factories
│   └── EffectRegistry.hpp
└── led_manager/
    ├── LedManager.cpp                 # consumes EffectRegistry
    └── LedManager.hpp                 # optional deterministic seed overrides

sim/
├── generated/
│   ├── firefly-renderer.js             # committed generated ES-module glue
│   ├── firefly-renderer.wasm           # committed C++ renderer artifact
│   └── manifest.json                   # ABI/toolchain/source fingerprint
├── js/
│   ├── renderer.js                     # thin ABI adapter + metadata/cache
│   ├── engine.js                       # clock/show orchestration; delegates frames
│   ├── master.js
│   ├── api.js
│   └── ui.js
├── test/cases/
│   ├── artifact.test.mjs
│   ├── vectors.test.mjs
│   └── ...                             # existing API/behavior coverage via Wasm
└── wasm/
    ├── CMakeLists.txt
    ├── SimulatorBridge.cpp             # versioned C ABI
    └── SimulatorRenderer.{hpp,cpp}      # shared-engine rigs + render buffers

scripts/
├── build-simulator-wasm.sh             # pinned reproducible artifact refresh
└── check-simulator-wasm.sh             # clean rebuild + byte comparison

.github/workflows/sim.yaml               # pinned toolchain freshness gate
```

Obsolete `sim/js/effects/`, `sim/js/fastled.js`, `sim/js/perlin.js`, `sim/js/palette.js`, and `sim/js/devices.js` are deleted after the adapter is green.

**Structure Decision**: The portable C++ catalog and registration changes live beside the production classes they govern. Wasm-only ABI and lifecycle code stays under `sim/wasm/`. Generated browser files are isolated under `sim/generated/`, while the browser-owned engine/API/UI remain readable ES modules. This keeps the embedded build free of Emscripten dependencies and makes copied rendering code easy to detect by directory and import checks.

## Complexity Tracking

| Added complexity | Why needed | Simpler alternative rejected because |
|------------------|------------|--------------------------------------|
| Checked-in generated Wasm + JS glue | Users must run the static simulator from a clean checkout without a compiler | Requiring a local backend or build step breaks the current simulator workflow |
| Versioned C ABI and thin adapter | Browsers and Node need a stable, testable boundary to C++ | Embind increases generated surface and couples JavaScript to C++ class layout |
| Pinned Wasm toolchain freshness job | A generated artifact otherwise can silently lag its C++ source | Reference vectors detect sampled output drift but cannot prove the entire artifact is current |
| Deterministic seed overrides in effect construction | Wasm libc randomness differs by platform while the public API promises stable seeded effects | Reimplementing seed-sensitive effects in JavaScript recreates the duplication being removed |
