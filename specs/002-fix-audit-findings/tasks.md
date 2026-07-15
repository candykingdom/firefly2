# Tasks: Fix Confirmed Audit Findings

**Input**: Design documents from `specs/002-fix-audit-findings/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/wire-format.md, quickstart.md

**Tests**: REQUIRED — the spec explicitly demands test cases (FR-002, FR-003, FR-004, Test Cases table). Tests land in the same commit as their fix (PR-005).

**Organization**: Grouped by user story. Each defect is its own commit (PR-002) with the per-commit gate from quickstart.md (PR-004) and a verifier review before committing (PR-003; D8 exempt). The stories are independent — no fix depends on another — so phases execute in priority order but any story could be pulled forward alone.

**Commit order note**: Phases below follow spec priority (US1 → US5), so the resulting commit sequence is D1, D4, D5, D6, D2, D3, D7, D8. plan.md's table listed defect-ID order; both are valid since the fixes are independent — priority order delivers the highest-value fixes first and is authoritative.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to
- The per-commit gate = `./lint.sh check` + `cd build && make && make test` (smalltests + largetests, ASan/UBSan) + the PlatformIO builds named in the task

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Confirm a green baseline in the worktree so every later failure is attributable to a fix.

- [x] T001 Configure host build in the worktree: `mkdir -p build && cd build && cmake .. -DBUILD_SIMULATOR=false && make && make test` — all existing tests must pass before any change
- [x] T002 [P] Baseline firmware builds: `pio run -e node -e fancy-node -e controller` succeed at branch point (note: `[env:node]`'s bossac `platform_packages` entry is OS-specific per CLAUDE.md — use the Apple-silicon variant)
- [x] T003 [P] Baseline lint: `./lint.sh check` is clean at branch point

**Checkpoint**: Baseline green. Record the baseline commit hash (`f7547e1`) in your notes; all golden values captured later are captured at pre-fix state.

---

## Phase 2: Foundational (Blocking Prerequisites)

**No foundational tasks.** All eight fixes are local and share no new infrastructure. (The D1 codec is story work, not shared scaffolding — no other story uses it.)

**Checkpoint**: Proceed directly to user stories.

---

## Phase 3: User Story 1 - Multi-hop mesh synchronization works (Priority: P1) 🎯 MVP — Defect D1

**Goal**: Received packets carry their true payload length so rebroadcasts are byte-identical to the original transmission; wire codec becomes host-testable.

**Independent Test**: `./build/smalltests --gtest_filter='RadioPacketTest*'` proves decode→re-encode losslessness; `InvalidPacketTest` (largetests) still green.

- [x] T004 [US1] Write conformance tests in `test/RadioPacketTest.cpp` per the table in `contracts/wire-format.md`: encode→decode round-trip for payload lengths 0/1/4/58 across all four packet types; decode a hand-built wire frame → re-encode → byte-compare; `Deserialize` returns false for len 0/1/2 and for payload > 58; garbage type byte is tolerated (returns true); relay scenario (decode master heartbeat frame → re-encode → decode as third node → same network time). These won't compile yet (codec doesn't exist) — that's expected TDD red.
- [x] T005 [US1] Declare `uint8_t Serialize(uint8_t* buf) const` and `bool Deserialize(const uint8_t* buf, uint8_t len)` on `RadioPacket` in `lib/radio/Radio.hpp`, with doc comments stating the contract from `contracts/wire-format.md`
- [x] T006 [US1] Implement both methods in `lib/radio/Radio.cpp`: Serialize writes packet_id big-endian (buf[0]=high), type byte, payload, returns `3 + dataLength`; Deserialize validates `len >= 3 && len - 3 <= PACKET_DATA_LENGTH`, sets all fields including `dataLength = len - 3`. Re-run `cmake ..` in `build/` if needed; make T004 tests pass
- [x] T007 [US1] Delegate in `src/arduino/RadioHeadRadio.cpp`: `sendPacket` serializes via `packet.Serialize(buffer.data())`; `readPacket` calls `packet.Deserialize(buffer.data(), received_length)` after the existing `received_length > kFrontPacketPadding` check (which stays — CLAIM_MASTER drop behavior is out of scope). Keep all `radio.available()` mode-management calls exactly where they are
- [x] T008 [US1] Run per-commit gate: `./lint.sh check`; full host suite (smalltests + largetests — InvalidPacketTest must stay green); `pio run -e node -e fancy-node`
- [x] T009 [US1] Verifier review of the staged D1 diff (fresh context): provide defect description, `contracts/wire-format.md`, and the CLAUDE.md invariants (invalid packets never crash; switch tolerance). Address all findings, re-running T008 after any change
- [x] T010 [US1] Commit: `Fix D1: set dataLength on radio receive so rebroadcasts carry payload` (message references specs/002-fix-audit-findings and the review)

**Checkpoint**: MVP delivered — the mesh-breaking bug is fixed and provably lossless on the wire.

---

## Phase 4: User Story 2 - Devices render with headroom to spare (Priority: P2) — Defects D4, D5, D6

**Goal**: Zero per-LED heap allocation, once-per-frame effect/timestamp resolution, no per-write LED-count recomputation. Output unchanged except single-timestamp-per-frame.

**Independent Test**: `./build/smalltests --gtest_filter='EffectsTest*:LedManagerTest*'` green with golden spot-check; `pio run -e node -e fancy-node` builds.

### D4 — palette copies (commit 2)

- [x] T011 [US2] At pre-fix state, add a golden spot-check to `test/EffectsTest.cpp`: for a fixed tuple (e.g., RainbowEffect + SparkEffect, palette 8, time_ms 123456, 36-LED plain strip), assert exact RGB values at LED 0/17/35 — capture the actual pre-fix outputs as the expected constants by running the test once and pinning values. Verify it PASSES before the fix
- [x] T012 [US2] Change palette bindings to `const ColorPalette&` in all 11 files: `lib/effect/ColorCycleEffect.cpp`, `ContrastBumpsEffect.cpp`, `DisplayColorPaletteEffect.cpp`, `FireflyEffect.cpp`, `LightningEffect.cpp`, `RainbowBumpsEffect.cpp`, `RainbowEffect.cpp`, `RorschachEffect.cpp`, `SimpleBlinkEffect.cpp`, `SparkEffect.cpp`, `SwingingLights.cpp`. Do NOT touch FireEffect/PrideEffect (member palettes). Golden test and full EffectsTest fuzz must still pass
- [x] T013 [US2] Run per-commit gate: `./lint.sh check`; full host suite; `pio run -e node -e fancy-node`
- [x] T014 [US2] Verifier review of staged D4 diff (reference: R4 in research.md; check every binding actually binds to `Effect::palettes()`'s static storage, not a temporary). Address findings
- [x] T015 [US2] Commit: `Fix D4: bind palettes by const reference in effects (no per-LED heap alloc)`

### D5 — per-LED re-resolution in RunEffect (commit 3)

- [x] T016 [US2] In `lib/led_manager/LedManager.cpp` `RunEffect`: hoist `Effect* effect = GetCurrentEffect()`, `RadioPacket* packet = radio_state->GetSetEffect()`, `uint32_t time_ms = radio_state->GetNetworkMillis()` above the strip loop; iterate `for (const StripDescription& strip : device.strips)`; keep Reversed/Dim/Off handling byte-identical. All existing tests must pass
- [x] T017 [US2] Run per-commit gate: `./lint.sh check`; full host suite; `pio run -e node -e fancy-node`
- [x] T018 [US2] Verifier review of staged D5 diff (reference: R5 in research.md; confirm no mid-frame packet mutation is possible in the single-threaded loop and flag handling is unchanged). Address findings
- [x] T019 [US2] Commit: `Fix D5: resolve effect, packet, and network time once per frame in RunEffect`

### D6 — per-write LED-count recomputation (commit 4)

- [x] T020 [US2] Add `const uint16_t led_count_` member to `FastLedManager` in `src/arduino/FastLedManager.hpp`, initialize from `device.GetLedCount()` in the constructor in `src/arduino/FastLedManager.cpp`, and use it in `SetLed`'s single-LED special case (and anywhere else the class calls `GetLedCount()` repeatedly, e.g. `PlayStartupAnimation` may keep its local)
- [x] T021 [US2] Run per-commit gate: `./lint.sh check`; full host suite (unaffected but cheap); `pio run -e node -e fancy-node` (this file compiles in both)
- [x] T022 [US2] Verifier review of staged D6 diff (reference: R6 in research.md; hardware-only class — review is the primary check). Address findings
- [x] T023 [US2] Commit: `Fix D6: cache device LED count in FastLedManager instead of recomputing per write`

**Checkpoint**: Render path allocation-free per LED; frames time-consistent; all suites green.

---

## Phase 5: User Story 3 - Firmware well-defined for all clock values and strip sizes (Priority: P3) — Defects D2, D3

**Goal**: No undefined behavior in heartbeat time decoding (any 2^32 value) or Firefly rendering (any LED index 0–255 on Controller strips); behavior unchanged for all currently-valid inputs.

**Independent Test**: `./build/smalltests --gtest_filter='RadioPacketTest*:FireflyEffectTest*'` — UBSan-clean at the exact boundary inputs that trip pre-fix.

### D2 — heartbeat decode UB (commit 5)

- [x] T024 [US3] Add heartbeat boundary round-trip tests to `test/RadioPacketTest.cpp`: `writeHeartbeat`/`readTimeFromHeartbeat` at 0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF. Run pre-fix and confirm UBSan reports a shift error for the top-bit-set cases (TDD red — the runtime error is the failure)
- [x] T025 [US3] Fix `readTimeFromHeartbeat` in `lib/radio/Radio.cpp`: cast each byte to `uint32_t` before shifting (all four lanes for symmetry). T024 tests now pass UBSan-clean
- [x] T026 [US3] Run per-commit gate: `./lint.sh check`; full host suite; `pio run -e node -e fancy-node`
- [x] T027 [US3] Verifier review of staged D2 diff (reference: R2 in research.md; confirm big-endian decode unchanged for values < 0x80000000). Address findings
- [x] T028 [US3] Commit: `Fix D2: UB-free heartbeat time decode for all 32-bit values`

### D3 — Firefly shift-count UB (commit 6)

- [x] T029 [US3] Create `test/FireflyEffectTest.cpp`: (a) at pre-fix state, capture golden CRGB outputs for FireflyEffect on a Controller-flagged strip at indices 0–31 (fixed time/palette) and pin them as expected constants — verify green pre-fix; (b) add a case rendering every index 0–255 on a 255-LED Controller-flagged strip — confirm UBSan reports shift-count overflow pre-fix (TDD red). Re-run `cmake ..` to pick up the new file
- [x] T030 [US3] Fix `lib/effect/FireflyEffect.cpp:23`: `offset = ((uint32_t)(kBlinkPeriod + 1234) << (led_index & 31)) % (kBlinkPeriod / 2);` — mask is a no-op for indices < 32 (golden test proves bit-identical); full-range case now UBSan-clean
- [x] T031 [US3] Run per-commit gate: `./lint.sh check`; full host suite (EffectsTest fuzz included); `pio run -e node -e fancy-node`
- [x] T032 [US3] Verifier review of staged D3 diff (reference: R3 in research.md; confirm behavior preservation argument and that unsigned wraparound for indices ≥ 22 was already present pre-fix). Address findings
- [x] T033 [US3] Commit: `Fix D3: well-defined Firefly offset shift for all LED indices`

**Checkpoint**: Host suite exercises the exact former-UB boundaries and is sanitizer-clean.

---

## Phase 6: User Story 4 - Controller buttons light up correctly (Priority: P4) — Defect D7

**Goal**: Right-button LED feedback reflects right-button state in Effect and DirectColor modes.

**Independent Test**: `pio run -e controller` builds; code inspection confirms both chains test `right_buttons`; manual hardware test documented.

- [x] T034 [US4] Fix `src/devices/controller/controller.cpp`: line 179 `left_buttons[1]` → `right_buttons[1]` (RunEffectMode right-button chain); lines 267/270 `left_buttons[1]`/`left_buttons[2]` → `right_buttons[1]`/`right_buttons[2]` (RunColorMode right-button chain). Touch nothing else — the Run*Mode dedup refactor is explicitly out of scope
- [x] T035 [US4] Run per-commit gate: `./lint.sh check`; full host suite (unaffected but cheap); `pio run -e controller` (CI doesn't build this env — local build is the gate)
- [x] T036 [US4] Verifier review of staged D7 diff (reference: R7 in research.md; cross-check against RunPaletteMode's correct pattern at lines 336–348). Address findings
- [x] T037 [US4] Commit: `Fix D7: right-button LED feedback tests right buttons (copy-paste fix)` — include the manual test procedure in the commit message (flash controller; Effect mode: press right button 2, its own LED shows pressed state; repeat right buttons in DirectColor mode)

**Checkpoint**: Both copy-paste sites corrected; manual procedure recorded for next hardware session.

---

## Phase 7: User Story 5 - No dead code confusing the next reader (Priority: P5) — Defect D8

**Goal**: Dead keep-alive block removed from the node main loop.

**Independent Test**: `pio run -e node` builds; `grep -n print_alive_at src/devices/node/node.cpp` returns nothing.

- [x] T038 [US5] Remove the `print_alive_at` global (line 142) and the `if (millis() > print_alive_at)` block (lines 149–152) from `src/devices/node/node.cpp`; leave `watchdog_counter` logic untouched
- [x] T039 [US5] Run per-commit gate: `./lint.sh check`; full host suite (unaffected but cheap); `pio run -e node`
- [x] T040 [US5] Commit: `Fix D8: remove dead keep-alive block from node loop` — verifier review skipped per delegation policy (trivial dead-code removal, PR-003 exemption)

**Checkpoint**: All eight defects fixed, eight commits on the branch.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Final validation of the whole branch and documentation debt.

- [x] T041 Run `./ci.sh` at the branch tip (exact CI pipeline: cmake without simulator + smalltests + largetests) — must be green
- [x] T042 [P] Update `docs/` radio/network notes for the D1 behavior change (documented mesh rebroadcast behavior was wrong on hardware; CLAUDE.md requires docs updates when documented behavior changes) — folds into a final docs commit
- [x] T043 [P] Verify branch hygiene: exactly 8 fix commits (one per defect, D-ID in each message), no unrelated diffs, spec artifacts committed; reconcile `specs/002-fix-audit-findings/` into a docs/spec commit if not yet committed
- [x] T044 Re-run quickstart.md "Done when" checklist end-to-end; note the two hardware-only validations (D7 manual button test, 3-node mesh relay smoke test) as pending-hardware if no boards are attached

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately
- **Foundational (Phase 2)**: empty — no blockers
- **User Stories (Phases 3–7)**: each depends only on Setup. Stories are mutually independent; priority order (US1 → US2 → US3 → US4 → US5) is the execution order, but any story could be pulled forward or dropped without breaking the others
- **Polish (Phase 8)**: after all stories

### Within-story ordering (strict, enforced by the commit workflow)

Write/capture tests (pre-fix state where goldens are needed) → implement fix → per-commit gate → verifier review → commit. Never start the next defect's source changes with an uncommitted diff pending.

### File-collision notes (why cross-story parallelism is NOT used)

- `lib/radio/Radio.cpp` + `test/RadioPacketTest.cpp`: touched by both D1 (US1) and D2 (US3) — sequential commits handle it
- `lib/effect/FireflyEffect.cpp`: touched by both D4 (US2) and D3 (US3) — sequential commits handle it
- The one-commit-per-defect + review-per-commit workflow is inherently serial; [P] markers appear only where tasks genuinely share no files and no commit ordering (Setup baselines, Polish docs tasks)

### Parallel opportunities

- T002 + T003 (baseline firmware builds and lint) run in parallel with each other while T001's cmake build runs
- T042 + T043 (docs update, branch hygiene audit) are parallel within Polish
- Within T012, the 11 file edits are mechanically independent (single task/commit regardless)

---

## Implementation Strategy

**MVP first**: Phase 1 + Phase 3 (D1) alone is a shippable increment — it fixes the product's core promise (multi-hop sync). Stop and validate there if time-boxed.

**Incremental delivery**: each subsequent phase is one to three self-contained commits, each independently revertable, each green at its commit. After every checkpoint the branch is mergeable.

**Suggested single-session order**: T001–T003 → US1 → US2 → US3 → US4 → US5 → Polish. Estimated 8 fix commits + 1 docs/spec commit.
