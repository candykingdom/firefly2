# SESSION HANDOFF — 2026-07-11 (RESOLVED: feature 003 implemented, merged, CI-verified same day)

> Historical. Feature 003 is COMPLETE: all 30 tasks in [tasks.md](tasks.md) done, 7 commits (dcc6101..5bab022) merged to master and pushed, all four CI workflows green at tip, SC-001/002/003 break-demonstrations recorded in the commit messages. One discovery during landing: the 72 Firefly corpus cases are libc-bound (glibc rand ≠ BSD rand) — platform-gated in dab7e31, rationale in [research.md](research.md) R7; the clean long-term fix (seed Firefly from random16) is a possible future feature. Still NOT proven on hardware: 002's D7 controller fix and D1 multi-hop over real radios. The original handoff below is kept for context.

**START HERE**: this file (plan below), then [spec.md](spec.md), then [research.md](research.md) — then `docs/index.md` + `CLAUDE.md` if you don't know the codebase.

**What this work is**: Feature 003 — close four audited regression-test coverage gaps (G1 corpus pin, G2 wire-path test radio, G3 render-loop flag semantics, G4 leaf utilities) and have everything run in existing CI. Spec/plan/research/data-model/contracts/quickstart are COMPLETE and committed; NO implementation has started.

**Where you are**: git worktree `.claude/worktrees/002-fix-audit-findings` (yes, named 002 — it is being reused for 003), branch `worktree-002-fix-audit-findings`, fast-forward-current with local `master`. Pattern so far: implement in this worktree, `git -C /Users/benryding/cuddlecult/firefly2 merge --ff-only worktree-002-fix-audit-findings` when the user says merge, push to origin when the user says push.

**DONE + committed (prior feature 002 + infra, all merged to master and pushed to origin as of 20dc6c8)**:
- 8 audit-defect fixes D1–D8 (one commit each, verifier-reviewed; see `specs/002-fix-audit-findings/`), incl. the mesh-breaking dataLength bug (D1) fixed via new `RadioPacket::Serialize/Deserialize` codec in `lib/radio/`.
- CI hardening: UBSan findings now FATAL (`-fno-sanitize-recover=undefined`, CMakeLists.txt); controller env added to `build-platformio.yaml`; controller lib_deps pinned by SHA (they had rotted).
- Regression evidence: regenerated `sim/test/vectors/reference.json` was byte-identical (1,380 cases) pre/post fixes; sim suite 59/59; host suite 69 tests + 14,400 fuzz green; README rewritten.
- Feature 003 spec artifacts (this directory).

**NOT done / NOT proven**:
- 003 implementation: nothing built yet. Next command in the speckit flow is `/speckit-tasks`, then implement.
- D7 (controller button LED fix) passes build only — NEVER verified on hardware (manual test procedure is in its commit message, `git log --grep "Fix D7"`).
- D1 multi-hop mesh fix proven by host tests only — no 3-node radio-range test was run.
- First remote CI run after the big push was NOT watched to completion — if something is red on GitHub (most likely: apt clang-format version drift vs our pinned 18.1.8, or a gcc-specific UBSan report now fatal), fix that before 003 work.

**NEXT STEP**: run `/speckit-tasks` for 003, then `/speckit-implement`. Honor the plan's Commit & Review Plan (5 commits, verifier review on commits 1–2, break-demonstrations for SC-001/002/003 recorded in commit messages).

**BLOCKERS / open questions**: none hard. Soft: confirm with the user whether to keep working in this (002-named) worktree — assume yes.

**KEY FACTS / infra (would cost you time to re-derive)**:
- Tooling on this Mac lives in `~/.local/bin` (installed via uv, NOT brew): `clang-format` pinned 18.1.8 (matches CI apt; brew's v22 flags the whole repo — do not use), `pio` (PlatformIO 6.1.19). Prefix commands: `PATH="$HOME/.local/bin:$PATH"`.
- Firmware builds on this Mac: use `-e node-arm64` (not `node`); gates: `pio run -e node-arm64 -e fancy-node` (+ `-e controller` when touching it).
- Host tests: `cd build && cmake .. -DBUILD_SIMULATOR=false && make -j8 && make test`. New `test/*Test.cpp` files auto-glob into smalltests; `.*VectorGen\.cpp$` is regex-excluded (name shared code `VectorGenCommon.cpp` so it is NOT excluded — deliberate, see plan).
- Corpus regen: `make vectorgen && ./vectorgen > ../sim/test/vectors/reference.json`; only the `firmwareGitDescribe` meta line should differ for behavior-neutral changes.
- Sim suite: `node --test "sim/test/cases/*.test.mjs"` (the bare-directory form fails — use the glob).
- Gotchas that bit this session: background Bash keeps its cwd (always `cd` absolute first); piping to `tail` masks exit codes; same-second edits can leave stale .o files (force-delete the .o or re-touch when a result looks impossible).
- Known latent defects deliberately NOT fixed (pinned/documented only — do not "helpfully" fix without the user): out-of-range palette index → OOB UB (`palettes()[byte]`); 255-LED count ceiling; `READ_FROM_FLASH` raw-read UB; millis() ~49.7-day rollover; header-only CLAIM_MASTER frames dropped on hardware receive.

---

# Implementation Plan: Regression Test Coverage for All Subsystems

**Branch**: `worktree-002-fix-audit-findings` (existing worktree, ff-current with master) | **Date**: 2026-07-11 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/003-regression-tests/spec.md`

## Summary

Close the four audited coverage gaps: (G1) pin the firmware's rendered output against the committed 1,380-case reference corpus by splitting the existing `vectorgen` generator into a shared case library plus a data-driven gtest comparison; (G2) route the shared test radio's send path through the production wire codec so serialization defects fail the whole protocol suite; (G3) pin the render loop's Off/Dim/Reversed/multi-strip/control-override semantics; (G4) cover the leaf utilities (LED-count aggregation, palette gradient edges, battery conversion). All new tests are auto-globbed into the existing CI entry points; each success criterion that demands a break-detection demonstration (SC-001..003) is actually performed and reverted during implementation.

## Technical Context

**Language/Version**: C++14 host tests (CMake, ASan+UBSan fatal since 002); firmware untouched

**Primary Dependencies**: googletest/gmock (existing), FakeFastLED (existing), **nlohmann/json (NEW, test-only, pinned via FetchContent)** for parsing the reference corpus in `ReferenceVectorTest`

**Storage**: N/A — the reference corpus `sim/test/vectors/reference.json` is the committed ground-truth artifact (schema: `specs/001-web-simulator/contracts/reference-vectors.md`)

**Testing**: `smalltests`/`largetests` via auto-glob (`test/*Test.cpp`); `vectorgen` stays a standalone target; sim suite unaffected

**Target Platform**: Host (macOS/Linux/WSL); CI = ubuntu-latest via `./ci.sh`

**Project Type**: Embedded firmware monorepo; this feature touches only `test/`, `lib/fake-radio/`, and CMakeLists

**Performance Goals**: SC-005 — host-suite wall time +<25% vs. baseline measured at implementation start on the same machine

**Constraints**: Deterministic (fixed seeds, `setMillis` fake clock); sanitizer-clean under fatal UBSan; must not weaken the invalid-packet fuzz (FR-005); no product-code changes outside test doubles

**Scale/Scope**: ~5 new/extended test files, 1 test-double change, 1 generator refactor, ~2 CMake edits; 5 commits

## Constitution Check

`.specify/memory/constitution.md` remains an unfilled template. De-facto gates are CLAUDE.md's invariants:

| Invariant | Impact | Status |
|-----------|--------|--------|
| Invalid/unknown packets never crash; `InvalidPacketTest` fuzz | G2 must preserve raw (non-codec) injection for invalid packets — FakeRadio's receive-side injection stays struct-based | PASS |
| `DEBUG` macro stays commented out | New tests must not define it | PASS |
| Last-two effect registry ordering; effects < 256 | G1's case library reuses the real `LedManager` registry; a registry-order regression *fails* the vector test (bonus coverage) | PASS |
| `Tick()` once-then-twice workaround | Untouched | PASS |
| `RunEffect` central flag handling | G3 pins it (that's the point) | PASS |
| New effects must pass `EffectsTest` fuzz | No new effects | PASS |

**Post-Phase-1 re-check**: PASS — the only product-adjacent change is `lib/fake-radio` (a test double compiled into host builds only).

## Project Structure

### Documentation (this feature)

```text
specs/003-regression-tests/
├── spec.md
├── plan.md              # This file
├── research.md          # Phase 0: decisions per gap
├── data-model.md        # Phase 1: corpus + test-double models
├── quickstart.md        # Phase 1: validation guide incl. break-demonstrations
├── contracts/
│   └── regression-suites.md  # What each suite guarantees to catch
└── tasks.md             # Phase 2 (/speckit-tasks)
```

### Source Code (repository root)

```text
CMakeLists.txt                     # + nlohmann/json FetchContent (test-only);
                                   #   + lib/battery include dir for host tests
test/
├── VectorGen.cpp                  # SHRINKS: keeps main() + JSON emission only
├── VectorGenCommon.hpp            # NEW: shared case model (seeds, tables,
├── VectorGenCommon.cpp            #   device catalog, RenderCase) — used by
│                                  #   vectorgen AND ReferenceVectorTest
├── ReferenceVectorTest.cpp        # NEW (G1): corpus vs firmware, case-by-case
├── LedManagerTest.cpp             # EXTENDED (G3): flag semantics, multi-strip
│                                  #   indexing, control-override rendering
├── DeviceDescriptionTest.cpp      # NEW (G4): LED-count aggregation
├── BatteryTest.cpp                # NEW (G4): voltage conversion round-trips
├── ColorPaletteTest.cpp           # EXTENDED (G4): gradient degenerate inputs
└── FakeNetwork.cpp                # UNCHANGED (wire path lives in FakeRadio)
lib/fake-radio/
├── FakeRadio.hpp                  # G2: wire-path send (Serialize→bytes→
└── FakeRadio.cpp                  #   Deserialize in getSentPacket); raw
                                   #   injection path preserved for fuzz
```

**Structure Decision**: The corpus comparison reuses `vectorgen`'s existing case enumeration by extraction, not duplication (QR-001) — `VectorGenCommon.{hpp,cpp}` is the single source of truth for "what the corpus covers", consumed by both the generator (JSON out) and the test (compare in). The CMake glob already excludes `VectorGen.cpp` from test targets by regex; `VectorGenCommon.cpp` is deliberately named so it is NOT excluded and lands in `testlib`.

## Commit & Review Plan

| # | Commit | Gap | Review |
|---|--------|-----|--------|
| 1 | Wire-path FakeRadio: sends round-trip through Serialize/Deserialize | G2 | verifier (touches shared test infra all suites depend on) |
| 2 | Split vectorgen into VectorGenCommon + add ReferenceVectorTest (nlohmann dep) | G1 | verifier (generator refactor must be output-identical: regenerated corpus byte-identical before/after) |
| 3 | Render-loop semantics tests (Off/Dim/Reversed/multi-strip/control) | G3 | self-review + SC-003 break demonstrations recorded in commit message |
| 4 | Leaf utility tests (device count, palette edges, battery) + CMake battery include | G4 | self-review (test-only additions) |
| 5 | Docs: build-and-test.md suite list, testing notes | — | — |

Per-commit gate (as in 002): `./lint.sh check` → full host suite → `pio run -e node-arm64 -e fancy-node` for commits touching `lib/` (commit 1 only; `lib/fake-radio` is host-only but the gate is cheap). SC-001/002/003 break-demonstrations are executed at their respective commits and documented (break → observe named failures → revert → green).

## Complexity Tracking

One new dependency (nlohmann/json) requires justification: parsing the committed corpus case-by-case is what makes FR-001's "failure names the diverging case" possible; the no-dependency alternative (byte-comparing regenerated JSON text) gives line-number failures, not case identification, and couples the test to JSON formatting. FetchContent-pinned, test-target-only, consistent with how googletest/FakeFastLED are already fetched. Recorded with alternatives in [research.md](research.md) R1.
