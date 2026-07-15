# Implementation Plan: Fix Confirmed Audit Findings

**Branch**: `worktree-002-fix-audit-findings` | **Date**: 2026-07-10 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/002-fix-audit-findings/spec.md`

## Summary

Fix the eight confirmed audit findings (D1–D8): restore multi-hop mesh rebroadcast by populating the payload length on packet receive (via a host-testable wire codec on `RadioPacket`), remove two undefined-behavior shifts, eliminate per-LED heap allocation and redundant per-LED work in the render path, correct two controller LED copy-paste bugs, and delete a dead code block. One commit per defect, adversarial review before each commit, full sanitized host suite + affected PlatformIO builds green at every commit.

## Technical Context

**Language/Version**: C++14 (`CMAKE_CXX_STANDARD 14`, PlatformIO GCC toolchains for ARM)

**Primary Dependencies**: FastLED (hardware) / FakeFastLED (host), RadioHead RH_RF69 (pinned fork), googletest/gmock (host)

**Storage**: N/A (flash device-description mode is out of scope)

**Testing**: CMake host build (`build/`), `smalltests` + `largetests` with ASan+UBSan always on; test files auto-globbed from `test/*.cpp` (`*Test.cpp` → smalltests, `InvalidPacketTest.cpp` → largetests). Firmware verified by `pio run -e node|fancy-node|controller`.

**Target Platform**: SAMD21 / STM32G030 / STM32G070 / ESP32 firmware + host (macOS/Linux) test build

**Project Type**: Embedded firmware monorepo with platform-independent core (`lib/`, `src/generic/`) and per-device mains (`src/devices/`)

**Performance Goals**: Render loop must keep comfortable headroom under the SAMD ~128 ms watchdog; zero heap allocation per LED per frame after D4

**Constraints**: Wire format must not change (D1 fixes length bookkeeping only); rendered output must be bit-identical for existing hardware configs (except intended single-timestamp-per-frame from D5); CLAUDE.md invariants (below) must hold

**Scale/Scope**: 8 defects, ~15 source files touched, 3 new test files, 8 commits

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

`.specify/memory/constitution.md` is an unfilled template — no formal constitution gates. The de-facto constitution is CLAUDE.md's **Invariants** section; the ones this feature can touch:

| Invariant | Impact | Status |
|-----------|--------|--------|
| Invalid/unknown radio packets must never crash (`InvalidPacketTest` fuzz) | D1 codec must bound-check payload length on decode | PASS — decode rejects payload > 58 bytes (defensive; today's radio can physically deliver ≤ 57) |
| Keep packet-type `switch`es tolerant of unknown values | D1 does not touch switches | PASS |
| `Tick()` once-then-twice `RadioTick` workaround | Untouched | PASS |
| `DisplayColorPaletteEffect`/`DarkEffect` last two in registry; effects < 256 | D4 only changes palette binding inside effects | PASS |
| `DEBUG` macro stays commented out | Untouched | PASS |
| `RunEffect` handles Reversed/Dim/Off centrally | D5 hoists but keeps flag handling identical | PASS |
| New/changed effects must pass `EffectsTest` fuzz | D3, D4 gated on full fuzz | PASS |
| SAMD watchdog ~128 ms | D4–D6 strictly reduce loop work | PASS |

**Post-Phase-1 re-check**: PASS — the codec design adds two members to `RadioPacket` (no wire change, no new dependency); everything else is local edits.

## Project Structure

### Documentation (this feature)

```text
specs/002-fix-audit-findings/
├── spec.md              # Feature specification
├── plan.md              # This file
├── research.md          # Phase 0: per-defect fix decisions
├── data-model.md        # Phase 1: RadioPacket wire format + touched entities
├── quickstart.md        # Phase 1: per-commit validation gate + manual tests
├── contracts/
│   └── wire-format.md   # Phase 1: on-air byte layout + codec contract
└── tasks.md             # Phase 2 (/speckit-tasks — not created here)
```

### Source Code (repository root)

```text
lib/
├── radio/
│   ├── Radio.hpp                 # D1: declare Serialize/Deserialize on RadioPacket
│   └── Radio.cpp                 # D1: codec impl; D2: cast in readTimeFromHeartbeat
├── effect/
│   ├── FireflyEffect.cpp         # D3: mask shift count; D4: palette by ref
│   ├── ColorCycleEffect.cpp      # D4 (×11 files total)
│   ├── ContrastBumpsEffect.cpp   # D4
│   ├── DisplayColorPaletteEffect.cpp # D4
│   ├── LightningEffect.cpp       # D4
│   ├── RainbowBumpsEffect.cpp    # D4
│   ├── RainbowEffect.cpp         # D4
│   ├── RorschachEffect.cpp       # D4
│   ├── SimpleBlinkEffect.cpp     # D4
│   ├── SparkEffect.cpp           # D4
│   └── SwingingLights.cpp        # D4
└── led_manager/
    └── LedManager.cpp            # D5: hoist effect/time/packet, strip by ref

src/
├── arduino/
│   ├── RadioHeadRadio.cpp        # D1: delegate to codec (sets dataLength)
│   ├── FastLedManager.hpp        # D6: cached led_count_ member
│   └── FastLedManager.cpp        # D6
└── devices/
    ├── controller/controller.cpp # D7: two right-button fixes
    └── node/node.cpp             # D8: remove dead block

test/
├── RadioPacketTest.cpp           # D1 round-trip cases + D2 boundary times (existing file, extended)
└── FireflyEffectTest.cpp         # D3 UBSan coverage + behavior-preservation (new)
                                  # D4 golden-value spot check → EffectsTest.cpp (existing, extended)
```

**Structure Decision**: No new directories. The only structural change is moving wire encode/decode from `RadioHeadRadio` (Arduino-only) into `RadioPacket` (in `lib/radio/`, already host-compiled and already home to the packet read/write helpers) so FR-002's host round-trip tests are possible.

## Fix Order & Commit Plan

One commit per defect, ordered so the riskiest/most valuable changes get the most review runway, and so unrelated diffs never mix:

| # | Commit | Files | Review gate |
|---|--------|-------|-------------|
| 1 | D1 — packet wire codec + set `dataLength` on receive | `lib/radio/Radio.{hpp,cpp}`, `src/arduino/RadioHeadRadio.cpp`, `test/RadioPacketTest.cpp` | verifier |
| 2 | D2 — heartbeat decode UB (cast before shift) | `lib/radio/Radio.cpp`, `test/RadioPacketTest.cpp` | verifier |
| 3 | D3 — Firefly shift-count UB (mask, behavior-preserving) | `lib/effect/FireflyEffect.cpp`, `test/FireflyEffectTest.cpp` | verifier |
| 4 | D4 — palette by const reference (11 effects) | 11 × `lib/effect/*.cpp`, `test/EffectsTest.cpp` (golden spot-check) | verifier |
| 5 | D5 — hoist per-frame work in `RunEffect` | `lib/led_manager/LedManager.cpp` | verifier |
| 6 | D6 — cache LED count in `FastLedManager` | `src/arduino/FastLedManager.{hpp,cpp}` | verifier |
| 7 | D7 — controller right-button LED feedback | `src/devices/controller/controller.cpp` | verifier |
| 8 | D8 — remove dead keep-alive block | `src/devices/node/node.cpp` | skipped (trivial, per delegation policy) |

Per-commit gate (PR-004): `./lint.sh check` → host suite (`smalltests`, `largetests`) → `pio run -e node -e fancy-node` for shared-code commits (1–5), plus `-e controller` for commit 7, `node` alone for 6* and 8. Details in [quickstart.md](quickstart.md).

*Commit 6 touches `src/arduino/`, compiled by both `node` and `fancy-node` — build both.

## Complexity Tracking

No constitution violations; table not needed. The one scope-shaped decision — introducing a codec instead of a one-line `dataLength` assignment — is required by FR-002 (host-testable wire logic) and is recorded with alternatives in [research.md](research.md).
