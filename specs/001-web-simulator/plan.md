# Implementation Plan: Web Simulator

**Branch**: `001-web-simulator` | **Date**: 2026-07-10 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/001-web-simulator/spec.md`

## Summary

Build a zero-dependency static web page under `sim/` that simulates Firefly devices in the browser: all 17 firmware effects and 22 palettes ported to JavaScript with wire-index-faithful registries, the real device catalog, a pausable/scrubbable shared network clock, master-mode autoplay with firmware cadence and weighting, and SET_CONTROL override semantics. The rendering engine is written as plain ES modules with no DOM dependencies so the identical code runs in the page (canvas UI) and in Node's built-in test runner (headless verification). Correctness against firmware is enforced by reference color vectors generated from the existing C++ host-test infrastructure (`FakeLedManager` + FakeFastLED) and compared byte-for-byte in the test suite.

## Technical Context

**Language/Version**: JavaScript (ES2020 modules) + HTML/CSS for the page; C++17 for the reference-vector generator (existing host toolchain); Node ≥ 18 (built-in `node --test`) for headless tests

**Primary Dependencies**: None at runtime (no npm, no build step, no CDN). Dev-side reuse of existing CMake host build (googletest, ademuri/FakeFastLED fetched by CMake) for vector generation. Page served with `python3 -m http.server` (ES modules can't load over `file://`)

**Storage**: Committed JSON reference-vector files under `sim/test/vectors/`; UI state optionally mirrored in URL query params (shareable/deep-linkable states); no other persistence

**Testing**: Three layers — (1) `node --test "sim/test/cases/*.test.mjs"` headless suite (engine determinism, wire-index mappings, flag handling, tolerance fuzz, vector comparison); (2) in-page test runner `sim/test.html` reusing the same case modules (verifies engine works in the browser); (3) existing C++ gtest infra extended with a standalone `vectorgen` target that emits the reference JSON

**Target Platform**: Evergreen desktop browsers (Chrome/Firefox/Safari current); macOS/Linux dev machines for headless tests

**Project Type**: Static web application + headless-testable library core

**Performance Goals**: 60 fps animation with all catalog devices displayed simultaneously (~300 LEDs total — trivial for canvas); page interactive < 1 s after load

**Constraints**: Byte-identical per-LED output vs. the C++ host build for sampled (effect, palette, device, time) tuples; effect/palette wire indices must match `LedManager.cpp` registration exactly; no firmware code modified except additive test target; deterministic rendering (fixed PRNG seeds for the three constructor-seeded effects)

**Scale/Scope**: 17 unique effect implementations (35 wire indices incl. weighted duplicates), 22 palettes, ~9 catalog devices, ~15 FastLED math primitives, one page, one API surface, ~40 test cases

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

`.specify/memory/constitution.md` is the unfilled spec-kit template — no ratified project principles exist. Gate passes vacuously. In lieu of a constitution, this plan treats the **CLAUDE.md invariants** as governing constraints:

- ✅ `DisplayColorPaletteEffect`/`DarkEffect` last two indices — the JS registry mirrors this and the test suite asserts it (FR-004).
- ✅ Effect/palette indices are single wire bytes, < 256 total — JS registry asserts the same bound.
- ✅ Invalid packets must never crash — simulator mirrors the Arduino `index % total` tolerance path and fuzzes all 256 byte values (FR-014).
- ✅ No firmware behavior changes: the only firmware-tree change is an **additive** CMake target (`vectorgen`) reusing existing fakes; `smalltests`/`largetests`/CI are untouched.
- ✅ `DEBUG` macro stays commented; no changes near `RadioStateMachine::Tick()`.

**Post-Phase-1 re-check**: Design artifacts introduce no violations — no new firmware dependencies, no protocol changes, docs/ update planned as required by CLAUDE.md ("update them when you change documented behavior" — additive: new `docs/simulator` note + build-and-test mention).

## Project Structure

### Documentation (this feature)

```text
specs/001-web-simulator/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   ├── sim-api.md       # window.sim programmatic API contract
│   └── reference-vectors.md  # vector JSON schema + generation contract
└── tasks.md             # Phase 2 output (/speckit-tasks — NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
sim/                          # NEW — the entire web simulator
├── index.html                # single page: canvas stage + controls
├── style.css                 # visual design (dark stage, LED glow, clean control panel)
├── js/
│   ├── fastled.js            # byte-exact ports: sin16, sin8/cos8, scale8, qadd8,
│   │                         #   cubicwave8, ease8InOutApprox, lerp16by16,
│   │                         #   blend(SHORTEST_HUES), hsv2rgb_rainbow, random16 LCG
│   ├── perlin.js             # port of lib/math/Perlin.hpp (perlinNoise, perlinNoisePolar)
│   ├── palette.js            # ColorPalette (GetColor, GetGradient) + 22 palettes
│   ├── devices.js            # StripDescription flags + catalog from lib/device/Devices.hpp
│   ├── effects/
│   │   ├── <one file per unique effect>.js   # 17 files, each exports {name, getRGB}
│   │   └── registry.js       # wire-index-faithful registration (weights, order,
│   │                         #   non-random tail, ControlEffect) mirroring LedManager.cpp
│   ├── engine.js             # SimEngine: network clock, show state, SET_CONTROL
│   │                         #   override, RunEffect port (Reversed/Dim/Off), snapshots
│   ├── master.js             # master-mode autoplay: 60 s weighted change, delay honor
│   ├── api.js                # window.sim — documented programmatic surface
│   └── ui.js                 # canvas rendering (rings/lines), controls, clock UI
├── test.html                 # in-page test runner (same cases as Node)
└── test/
    ├── cases/                # environment-agnostic test case modules
    │   ├── registry.test.mjs     # index mappings, last-two invariant, weights
    │   ├── determinism.test.mjs  # identical state ⇒ identical colors
    │   ├── flags.test.mjs        # Reversed/Dim/Off central handling
    │   ├── tolerance.test.mjs    # all 256 effect/palette bytes, 0-LED strips
    │   ├── control.test.mjs      # SET_CONTROL override + delay expiry
    │   ├── master.test.mjs       # cadence + weighted selection distribution
    │   └── vectors.test.mjs      # byte-compare against reference vectors
    └── vectors/
        └── reference.json    # generated by C++ vectorgen (committed)

test/
└── VectorGen.cpp             # NEW additive C++ tool: renders sampled tuples through
                              #   FakeLedManager and prints reference.json
CMakeLists.txt                # MODIFIED (additive): vectorgen executable target

docs/
└── (additive note: simulator usage + regeneration workflow, linked from index)
```

**Structure Decision**: Single new top-level `sim/` directory keeps the web app fully separate from firmware source. The engine (everything under `sim/js/` except `ui.js`/`api.js`) is DOM-free ES modules, imported unchanged by both the page and `node --test` — this is what makes SC-009 (zero-human verification) cheap. The only firmware-tree touch is the additive `VectorGen.cpp` + CMake target, reusing `FakeLedManager` exactly as `LedManagerTest.cpp` already does.

## Complexity Tracking

No constitution violations to justify. One deliberate redundancy accepted by the spec (Assumptions): effect algorithms exist twice (C++ and JS). Mitigation is the committed reference vectors + `vectors.test.mjs`, which turn drift into a test failure rather than silent divergence.
