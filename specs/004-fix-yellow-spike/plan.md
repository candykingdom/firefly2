# Implementation Plan: Fix the Bright Yellow LED Artifact in Rainbow-Palette Gradients

**Branch**: `004-fix-yellow-spike` | **Date**: 2026-07-15 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/004-fix-yellow-spike/spec.md`

## Summary

Effects that render smooth palette gradients show a lone over-bright yellow LED wherever the hue interpolation crosses FastLED's boosted yellow band (hue 32–95, up to +34% total RGB drive). Fix: add one shared `Effect` helper that converts an interpolated `CHSV` to `CRGB` and rescales it to the palette's un-boosted power baseline, call it from the four palette-showcase effects (Rainbow, Color Cycle, Rainbow Bumps, Display Color Palette), mirror the identical integer math in the simulator's JS ports, regenerate the reference-vector corpus, and pin the property with a new firmware + sim regression test. The approach was prototyped and pixel-verified on the host during diagnosis (see research.md for the measurements).

## Technical Context

**Language/Version**: C++14/17 (PlatformIO firmware + CMake host build), vanilla ES-module JavaScript (zero-dependency web simulator)

**Primary Dependencies**: FastLED (candykingdom fork, pinned `#12dee8f`) on hardware; ademuri FakeFastLED via CMake FetchContent on host; no new dependencies

**Storage**: N/A (committed reference corpus `sim/test/vectors/reference.json` is regenerated, not schema-changed)

**Testing**: googletest host suite (`smalltests`, ASan+UBSan fatal), `node --test "sim/test/cases/*.test.mjs"` sim suite, `./ci.sh`, `./lint.sh check`

**Target Platform**: SAMD21 / STM32G030 / STM32G070 / ESP32 firmware + browser simulator + host test build (macOS/Linux)

**Project Type**: Embedded firmware with a byte-exact JS simulator twin

**Performance Goals**: Per-LED render cost increase ≤ one extra `hsv2rgb_rainbow` call + 3 integer divides; a 100-LED frame must stay far inside the SAMD ~128 ms watchdog budget (it will: `hsv2rgb_rainbow` is branch-only 8-bit math)

**Constraints**: Byte-exact firmware↔sim parity (corpus-gated in CI both directions); integer-truncating math only (no float) so JS mirrors exactly; fatal UBSan on host; effect registry order and radio protocol untouched

**Scale/Scope**: ~6 firmware files touched, ~5 sim files, 1 corpus regen, 2 new test files. No API/wire changes.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

`.specify/memory/constitution.md` is the unfilled template — no project constitution is established. Gate passes vacuously. The binding constraints are CLAUDE.md's "Invariants — do not break these", restated in research.md §Invariants.

## Project Structure

### Documentation (this feature)

```text
specs/004-fix-yellow-spike/
├── spec.md              # Feature specification
├── plan.md              # This file
├── research.md          # Root-cause evidence, measurements, decisions
├── data-model.md        # Entities: gradient color, drive, corpus case
├── quickstart.md        # How to reproduce the bug and validate the fix
├── contracts/
│   └── flattened-gradient.md   # Helper contract (C++ + JS), regression-test property
└── tasks.md             # Phase 2 output (/speckit-tasks)
```

### Source Code (repository root)

```text
lib/effect/
├── Effect.hpp                    # + FlattenedGradientRGB declaration (protected)
├── Effect.cpp                    # + FlattenedGradientRGB implementation
├── RainbowEffect.cpp             # call helper in both varying-palette branches
├── ColorCycleEffect.cpp          # call helper in the gradient (multi-color) branch
├── RainbowBumpsEffect.cpp        # call helper at return
└── DisplayColorPaletteEffect.cpp # call helper at return (after Bright halving)

sim/js/
├── fastled.js                    # + flattenedGradientRGB export (byte-exact mirror)
└── effects/
    ├── rainbow.js                # mirror call sites
    ├── colorCycle.js             # mirror call sites
    ├── rainbowBumps.js           # mirror call sites
    └── displayColorPalette.js    # mirror call sites

test/
└── GradientPowerTest.cpp         # NEW: firmware-side uniform-drive regression test
                                  # (auto-globs into smalltests)

sim/test/
├── cases/gradientPower.test.mjs  # NEW: sim-side uniform-drive regression test
└── vectors/reference.json        # REGENERATED (vectorgen)
```

**Structure Decision**: Existing layout; no new directories beyond the two test files. Firmware change is confined to `lib/effect/` (platform-independent core compiled by every PlatformIO env and the host build — no per-device code involved).

## Complexity Tracking

No constitution violations; table not needed.
