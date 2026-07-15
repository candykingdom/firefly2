# Tasks: Regression Test Coverage for All Subsystems

**Input**: Design documents from `specs/003-regression-tests/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/regression-suites.md, quickstart.md

**Tests**: This feature IS tests — every user story delivers test code. No TDD split; each phase is a self-contained suite addition with its own gate.

**Organization**: Phases follow the plan's **Commit & Review Plan** (5 commits). That puts US2/G2 (commit 1) before US1/G1 (commit 2) — deliberate: both are verifier-reviewed, and landing the wire-path radio first means every later suite run (including G1's) exercises it. Each phase = one commit.

**Per-commit gate** (every phase): `PATH="$HOME/.local/bin:$PATH" ./lint.sh check` → `cd build && cmake .. -DBUILD_SIMULATOR=false && make -j8 && make test` → plus `PATH="$HOME/.local/bin:$PATH" pio run -e node-arm64 -e fancy-node` when `lib/` is touched (Phase 3 only).

## Format: `[ID] [P?] [Story] Description`

---

## Phase 1: Setup (Baseline)

**Purpose**: Record the SC-005 baseline and confirm a clean start.

- [X] T001 Verify worktree clean and current with master; confirm remote CI green for the latest master push (`gh run list`) — all 4 workflows green on master@0717e0e
- [X] T002 Build host suite warm and record SC-005 baseline wall time: `cd build && cmake .. -DBUILD_SIMULATOR=false && make -j8 && time make test` — **baseline: 23.30 s** (budget < ~29.1 s)

**Checkpoint**: Baseline recorded; all 69 tests green before any change.

---

## Phase 2: Foundational

**Purpose**: None required — no shared infrastructure precedes the stories (nlohmann/json FetchContent lands inside Phase 4 where it is consumed; QR-001 forbids speculative scaffolding).

*(no tasks)*

---

## Phase 3: User Story 2 — Wire-path test radio (Priority: P2) — **Commit 1**

**Goal**: Every packet observed via `FakeRadio::getSentPacket` (and therefore everything `FakeNetwork` relays) has round-tripped through the production `RadioPacket::Serialize`/`Deserialize` codec, so a serialization defect fails the whole protocol suite (FR-004), while raw invalid-packet injection stays codec-free (FR-005).

**Independent Test**: SC-002 — re-introduce the D1 defect (comment out the `dataLength` assignment in `Deserialize`); ≥ 5 existing non-codec tests fail; revert; green.

- [X] T003 [US2] Read `lib/fake-radio/FakeRadio.{hpp,cpp}`, `lib/radio/` codec API, and `test/FakeNetwork.cpp`'s use of `getSentPacket`/`setReceivedPacket` to confirm the R2 design maps onto current code
- [X] T004 [US2] Implement wire-path send in `lib/fake-radio/FakeRadio.{hpp,cpp}`: `sendPacket` serializes into an internal wire buffer (`uint8_t[61]` + length) via `RadioPacket::Serialize`; `getSentPacket` deserializes via `RadioPacket::Deserialize`, returning `nullptr` on rejection; `setReceivedPacket`/`readPacket` raw-struct path unchanged (FR-005)
- [X] T005 [US2] Run full gate incl. `pio run -e node-arm64 -e fancy-node` (touches `lib/`); entire pre-existing suite must pass unchanged (codec transparent for valid traffic) and `./largetests` fuzz must stay green
- [X] T006 [US2] Perform SC-002 break-demonstration: re-introduce D1 in `lib/radio` `Deserialize` (skip `dataLength` population) → `./smalltests` → record the ≥ 5 failing non-codec test names → revert → green — **11 non-codec failures** (see commit dcc6101)
- [X] T007 [US2] Launch **verifier** agent on the diff (shared test infra all suites depend on); address findings — no findings survived
- [X] T008 [US2] Commit 1 with SC-002 demonstration recorded in the commit message — dcc6101

**Checkpoint**: All existing suites now structurally cover the wire codec.

---

## Phase 4: User Story 1 — Corpus pin / ReferenceVectorTest (Priority: P1) — **Commit 2**

**Goal**: The firmware's rendered output is pinned case-by-case against the committed 1,380-case `sim/test/vectors/reference.json`; any divergence fails `smalltests` naming case id / effect / device / LED (FR-001/002/003).

**Independent Test**: SC-001 — perturb one constant in a covered effect's math; `ReferenceVectorTest` fails naming the case; revert; green. Plus: regenerated corpus byte-identical pre/post refactor (only `firmwareGitDescribe` may differ).

- [X] T009 [US1] Read `test/VectorGen.cpp` (503 lines) and the corpus schema (`specs/001-web-simulator/contracts/reference-vectors.md`); snapshot current generator output: `make vectorgen && ./vectorgen > /tmp/ref-before.json`
- [X] T010 [US1] Extract shared case model into `test/VectorGenCommon.hpp` + `test/VectorGenCommon.cpp`: seed prediction (`PredictEffectSeedsAndResetPrngs`), effect/palette wire tables, three-device catalog, `ForEachCase` case enumeration; shrink `test/VectorGen.cpp` to `main()` + JSON emission. Filename deliberately avoids the `.*VectorGen\.cpp$` glob exclusion so `VectorGenCommon.cpp` lands in testlib
- [X] T011 [US1] Verify extraction is output-identical: byte-identical pre/post refactor, and matches the committed corpus modulo `firmwareGitDescribe`
- [X] T012 [US1] Add nlohmann/json to `CMakeLists.txt` via FetchContent (pinned v3.11.3, test targets only) and define `REFERENCE_VECTORS_PATH` compile definition from `${CMAKE_CURRENT_SOURCE_DIR}` for the test
- [X] T013 [US1] Write `test/ReferenceVectorTest.cpp`: hard-fail if corpus missing/unparseable (FR-002); validate `meta.effectSeeds` against `VectorGenCommon` prediction; validate `effects[]`/`palettes[]` tables against live registry (order, size, last-two invariant) and `Effect::palettes()`; re-render all 1,380 `cases[]` and compare RGB byte-exact with failure messages naming case id/effect/device/LED index/expected/actual; re-evaluate `primitives` against FakeFastLED; ignore `firmwareGitDescribe`
- [X] T014 [US1] Run full gate (host-only; no `lib/` change) — suite 23.8 s vs 23.3 s baseline (+2.1%, SC-005 OK)
- [X] T015 [US1] Perform SC-001 break-demonstration: RainbowEffect hue step 8→9 → named per-case failures → revert → green
- [X] T016 [US1] Launch **verifier** agent on the diff; one non-blocking finding (VectorGenCommon compiled into 3 targets, pre-existing testlib/smalltests glob-overlap pattern) — addressed with a CMake comment
- [X] T017 [US1] Commit 2 with SC-001 demonstration and byte-identical-corpus evidence recorded in the commit message

**Checkpoint**: Rendered output is bidirectionally pinned (firmware ↔ corpus ↔ sim).

---

## Phase 5: User Story 3 — Render-loop flag semantics (Priority: P3) — **Commit 3**

**Goal**: `RunEffect`'s central Off/Dim/Reversed handling, multi-strip global indexing, and the SET_CONTROL override path are each pinned by an exact-value test (FR-006/007).

**Independent Test**: SC-003 — break each of Off/Dim/Reversed in `RunEffect` separately; at least one named LedManagerTest failure per flag; revert each; green.

- [X] T018 [US3] Read `test/LedManagerTest.cpp`, `lib/led_manager/LedManager.cpp` (`RunEffect`), and `lib/fake-led-manager/FakeLedManager.hpp` hooks (`ClearEffects`, `PublicAddEffect`, `GetLed`) to confirm the R3 design maps onto current code
- [X] T019 [US3] Extend `test/LedManagerTest.cpp` with a deterministic stub effect (`color = f(led_index)`) and table-driven cases: (a) four-strip device [plain, Off, Dim, Reversed] asserting each strip against the plain control strip (black / control÷8 / index-flipped); (b) multi-strip global indexing — contiguous offsets, no overlap/gap; (c) Dim+Reversed combined; (d) SET_CONTROL override via the state machine — all non-Off LEDs show commanded color, then override expires/replaced and effect rendering resumes
- [X] T020 [US3] Run full gate (host-only) — green, 24.8 s
- [X] T021 [US3] Perform SC-003 break-demonstrations: Off skipped → 2 failures; Dim skipped → 2 failures; Reversed skipped → 3 failures (incl. pre-existing callStripInReverse); each reverted → green
- [X] T022 [US3] Self-review the diff; commit 3 with all three SC-003 demonstrations recorded in the commit message

**Checkpoint**: The documented "RunEffect handles Reversed/Dim/Off centrally" invariant is executable.

---

## Phase 6: User Story 4 — Leaf utilities (Priority: P4) — **Commit 4**

**Goal**: Direct edge-case coverage for `DeviceDescription::GetLedCount`, `ColorPalette` gradient degenerate inputs, `BatteryVoltageToRawReading`, and the palette-registry boundary (FR-008, refined per research R4 — OOB is documented, boundary-pinned, NOT executed).

**Independent Test**: Each new test fails if its target's edge behavior changes; no latent defect gets "fixed".

- [X] T023 [P] [US4] Write `test/DeviceDescriptionTest.cpp`: `GetLedCount` for zero/one/many strips; pin the uint8_t return-width ceiling as documented current behavior (comment + boundary assertion; do NOT fix)
- [X] T024 [P] [US4] Write `test/BatteryTest.cpp`: `BatteryVoltageToRawReading` monotonicity + round-trip within one quantization step at `kBatteryEmpty`/`kBatteryFull`/midpoint; add `lib/battery` include dir for host tests in `CMakeLists.txt`
- [X] T025 [P] [US4] Extend `test/ColorPaletteTest.cpp`: empty palette → black, single-color palette everywhere, exact color boundaries + wrap-around at max; palette-registry boundary — `Effect::palettes().size() == 22`, no empty palettes; OOB hazard documented (not executed, it is UB)
- [X] T026 [US4] Run full gate — green, 88 smalltests total, 24.7 s (+5.8% vs baseline)
- [X] T027 [US4] Self-review the diff; commit 4

**Checkpoint**: All four gaps closed; suite complete.

---

## Phase 7: Polish — Docs & final verification — **Commit 5**

**Purpose**: Documentation of the new suites + end-to-end validation (FR-009, SC-004, SC-005).

- [X] T028 Update `docs/build-and-test.md` (suite list, wire-path FakeRadio, bidirectional corpus pin, nlohmann dep, stale controller-CI line), `docs/simulator.md` (shared case model), and `CLAUDE.md` (controller in CI; ReferenceVectorTest fails on corpus drift)
- [X] T029 Re-measure suite wall time vs T002 baseline — final 23.33 s vs 23.30 s baseline (+0.1%, SC-005 met); largetests 14,400/14,400; sim suite 59/59; lint clean
- [X] T030 Commit 5 (docs); merged/pushed per Ben's goal instruction. First CI run exposed a real gap — Firefly corpus cases are libc-bound (glibc rand ≠ BSD rand); fixed in follow-up commit dab7e31 (platform-gated, see research.md R7). Final CI on dab7e31: all 4 workflows green; logs show all new suites executing on ubuntu (88 smalltests + 14,400 fuzz PASSED, exactly 72 Firefly skips with loud NOTEs) — SC-004/FR-009 verified

**Checkpoint**: Feature complete; CI proof captured.

---

## Dependencies & Execution Order

- Phase 1 → Phase 3 (US2, commit 1) → Phase 4 (US1, commit 2) → Phase 5 (US3, commit 3) → Phase 6 (US4, commit 4) → Phase 7 (commit 5)
- Sequential by design: one logical change per commit, each gated, verifier on commits 1–2. Stories are independently testable but land in commit order.
- Within Phase 6, T023/T024/T025 are [P] (different files) and can be written concurrently (e.g., delegated to a worker agent as one batch), then gated together.

## Implementation Strategy

Single-developer, commit-by-commit; each phase ends in a green gate and a commit, so work can stop safely at any checkpoint. MVP = Phase 4 (US1 corpus pin) per spec priority, but commit order front-loads the shared-infra change (US2) while both verifier-reviewed commits are adjacent.
