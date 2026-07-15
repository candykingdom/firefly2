# Feature Specification: Fix Confirmed Audit Findings

**Feature Branch**: `worktree-002-fix-audit-findings`

**Created**: 2026-07-10

**Status**: Draft

**Input**: User description: "open a worktree for this repo to work on this stuff, but write a spec to fix all these problems. but only fix problems you are sure about. make sure to include test cases. do this in a separate branch. do code reviews after each fix. each fix in a separate commit"

## Background

A read-only code audit (2026-07-10) of the Firefly2 codebase identified a set of defects and easy performance wins. This spec covers **only the findings confirmed with certainty** by reading the source — each one was verified to exist, has an unambiguous correct behavior, and has a low-risk fix. Findings that require a design decision or a redesign are explicitly out of scope (see Out of Scope).

### Defect Inventory (confirmed findings, in fix order)

| ID | Defect | Location | Kind |
|----|--------|----------|------|
| D1 | Received radio packets never get their payload length set; any re-send of a received packet (mesh flood rebroadcast, master's periodic effect rebroadcast) transmits only the 3 header bytes, and the next hop drops the truncated packet. Invisible to tests because the test radio copies whole structs and never serializes. | `src/arduino/RadioHeadRadio.cpp:18` (`readPacket`) | Correctness — breaks multi-hop mesh on hardware |
| D2 | Heartbeat time decoding shifts a byte value into the sign bit (`data[0] << 24` promotes to signed int) — undefined behavior once network time ≥ 0x80000000 (~24.9 days) or when a corrupt top byte ≥ 0x80 arrives. | `lib/radio/Radio.cpp:24` (`readTimeFromHeartbeat`) | Undefined behavior |
| D3 | Firefly effect computes `(kBlinkPeriod + 1234) << led_index` — undefined behavior for `led_index ≥ 32` on Controller-flagged strips. Latent today (largest controller strip is 12 LEDs). | `lib/effect/FireflyEffect.cpp:23` | Undefined behavior |
| D4 | Eleven effects copy a `ColorPalette` (which owns a heap-allocated color list) **by value on every per-LED color computation** — a heap allocate + copy + free per LED per frame on a microcontroller. | `lib/effect/{ColorCycle,ContrastBumps,DisplayColorPalette,Firefly,Lightning,RainbowBumps,Rainbow,Rorschach,SimpleBlink,Spark,SwingingLights}Effect.cpp` | Performance |
| D5 | The per-frame render loop re-resolves the current effect and re-reads the network clock **once per LED** instead of once per frame, and copies each strip descriptor by value. Also means a single frame renders across drifting timestamps. | `lib/led_manager/LedManager.cpp:76` (`RunEffect`) | Performance / frame consistency |
| D6 | Every hardware LED write re-computes the device's total LED count (a loop over all strips) just to check a single-LED special case. | `src/arduino/FastLedManager.cpp:64` (`SetLed`) | Performance |
| D7 | Right-button LED feedback tests the **left** buttons (copy-paste error): pressing right button 2 lights the wrong button's LED. Two sites. | `src/devices/controller/controller.cpp:179` and `controller.cpp:267-270` | Correctness — wrong visual feedback |
| D8 | Dead keep-alive block: a timer variable is updated every second but the only output statement is commented out. | `src/devices/node/node.cpp:149-152` | Dead code |

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Multi-hop mesh synchronization works (Priority: P1)

A participant's installation has nodes spread beyond direct radio range of the master (e.g., a long bike caravan or a large art piece). A node in the middle receives the master's heartbeat/effect packets and rebroadcasts them. Nodes that can only hear the rebroadcast must receive the **complete** packet and stay in sync — same effect, same palette, same network clock — as nodes that hear the master directly. (Fixes D1.)

**Why this priority**: This is the core promise of the product — a self-organizing mesh where every device animates in sync. Today, any rebroadcast is truncated to an empty payload and dropped by the next hop, so multi-hop topologies silently fail on real hardware.

**Independent Test**: Round-trip a packet through the real wire encode/decode path in a host unit test: a packet received from the air and then re-sent must be byte-identical (id, type, full payload) to the original transmission.

**Acceptance Scenarios**:

1. **Given** a packet with a non-empty payload arrives over the radio, **When** the node reads it, **Then** the in-memory packet reports the correct payload length (transmitted length minus the 3-byte header).
2. **Given** a received packet is rebroadcast by the mesh layer, **When** it is re-transmitted, **Then** the bytes on the wire are identical to the original transmission (same id, type, and full payload).
3. **Given** a heartbeat relayed through one intermediate node, **When** a third node receives only the relayed copy, **Then** it decodes the same network time as a node that heard the master directly.
4. **Given** a header-only wire frame (payload length 0, e.g., CLAIM_MASTER), **When** it is decoded and re-encoded at the codec level, **Then** it round-trips byte-identically with length 0. (The hardware receive path's pre-existing drop of header-only frames stays — out of scope per Assumptions.)

---

### User Story 2 - Devices render effects with headroom to spare (Priority: P2)

A device driving a large LED strip renders every frame without thousands of needless heap allocations and redundant recomputations, leaving CPU headroom under the SAMD node's ~128 ms watchdog and keeping animation timing consistent: every LED within one frame is rendered at the same network timestamp. (Fixes D4, D5, D6.)

**Why this priority**: Per-LED heap allocation on a microcontroller is the single largest avoidable cost in the hot loop and a fragmentation risk on long-running devices; frame-time drift across a strip is a visible artifact. No behavior change beyond timestamp consistency is intended.

**Independent Test**: The full existing effect fuzz suite (`EffectsTest`: every palette, 0–255 LEDs, multi-strip Tiny/Circular devices) and all other host tests pass unchanged under ASan/UBSan, and rendered output for a fixed timestamp is unchanged from before the fix.

**Acceptance Scenarios**:

1. **Given** any registered effect and any palette, **When** a single LED color is computed, **Then** no palette copy (and no heap allocation for palette data) occurs.
2. **Given** a frame render across a multi-strip device, **When** the frame is drawn, **Then** the current effect and network time are resolved once for the frame, and every LED in that frame is computed with the same timestamp.
3. **Given** the fixes are applied, **When** the existing effect fuzz and radio test suites run under ASan/UBSan, **Then** all pass with output identical to pre-fix behavior for identical inputs (same effect, palette, timestamp, strip).

---

### User Story 3 - Firmware is well-defined for all clock values and strip sizes (Priority: P3)

Devices run for weeks (network clock past ~25 days) and may be built with large Controller-flagged strips in the future. Decoding heartbeat times and rendering the Firefly effect must be well-defined behavior for **all** byte values and LED indices — no undefined behavior lurking behind today's typical configurations. (Fixes D2, D3.)

**Why this priority**: Both are certain UB, but latent — they need long uptimes or not-yet-built hardware to trigger. Fixing them is cheap insurance and keeps the UBSan-instrumented test suite meaningful.

**Independent Test**: New host unit tests exercising the exact boundary inputs (heartbeat times ≥ 0x80000000; Firefly on Controller strips with ≥ 32 LEDs) run clean under UBSan.

**Acceptance Scenarios**:

1. **Given** a heartbeat carrying a network time with the top bit set (e.g., 0x80000000, 0xFFFFFFFF), **When** it is encoded and decoded, **Then** the time round-trips exactly and UBSan reports nothing.
2. **Given** the Firefly effect on a Controller-flagged strip of up to 255 LEDs, **When** every LED index is rendered, **Then** no undefined behavior occurs (UBSan-clean) and output for existing hardware (indices 0–11) is bit-identical to current behavior.

---

### User Story 4 - Controller buttons light up correctly (Priority: P4)

A person holding the handheld controller presses a right-side button; the LED for **that** button shows the pressed state. (Fixes D7.)

**Why this priority**: Real but cosmetic — wrong LED feedback on two code paths of one device; no effect on the mesh or other devices.

**Independent Test**: The controller firmware has no host test harness (hardware-only translation unit with direct pin I/O), so this is verified by code inspection during review, a successful `pio run -e controller` build, and a documented manual test.

**Acceptance Scenarios**:

1. **Given** the controller in Effect mode, **When** right button 2 is pressed, **Then** right button 2's LED (not left button 2's) shows the pressed brightness.
2. **Given** the controller in DirectColor mode, **When** right button 2 or right button 3 is pressed, **Then** the corresponding right-button LED shows the pressed state.
3. **Given** the fix, **When** `pio run -e controller` is built, **Then** it compiles cleanly.

---

### User Story 5 - No dead code confusing the next reader (Priority: P5)

A developer reading the node's main loop does not stumble over a keep-alive block that updates state but can never produce output. (Fixes D8.)

**Why this priority**: Pure hygiene; zero user-visible impact.

**Independent Test**: `pio run -e node` builds cleanly; grep confirms the dead block is gone.

**Acceptance Scenarios**:

1. **Given** the node main loop, **When** the dead block is removed, **Then** `pio run -e node` compiles and no behavior changes (the variable and block had no observable effect).

### Edge Cases

- Rebroadcast of a maximum-size payload (58 bytes): must round-trip without truncation or overflow.
- Header-only packets (CLAIM_MASTER, payload length 0): must remain valid after the D1 fix — length 0 is legitimate, not an error.
- A transmission of exactly 3 bytes (header only, no payload) received from the air: current code drops it (`received_length > padding` check); decide and document whether that stays (it must at minimum not regress CLAIM_MASTER delivery — note CLAIM_MASTER sends 3 bytes today and is dropped by that same check; see Assumptions).
- Corrupt packets with a declared payload larger than the wire data: must never crash (`InvalidPacketTest` fuzz must keep passing).
- Heartbeat times at 0, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF.
- Firefly on Controller strips at LED indices 31, 32, 33, and 255.
- Single-color palettes (gradient degenerate case) must render identically after the by-reference change.
- Effects that keep a palette member (Fire, Pride) are unaffected by D4 and must stay unaffected.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001** (D1): A packet read from the radio MUST carry its correct payload length (received wire length minus the 3-byte header) so that any subsequent re-transmission of that packet is byte-identical to the original.
- **FR-002** (D1): The packet wire encode and decode logic MUST be exercisable in host unit tests (i.e., testable without radio hardware), and a round-trip test (encode → decode → encode) MUST prove payload preservation for lengths 0, 1, typical (3–4), and maximum (58).
- **FR-003** (D2): Heartbeat network-time decoding MUST be well-defined for all 2^32 time values and MUST round-trip encode → decode exactly.
- **FR-004** (D3): The Firefly effect MUST be well-defined for every LED index 0–255 on Controller-flagged strips, and MUST produce bit-identical output to current behavior for indices valid today (< 32).
- **FR-005** (D4): Computing one LED's color MUST NOT copy a palette or allocate heap memory for palette data in any of the 11 affected effects; rendered output MUST be unchanged for identical inputs.
- **FR-006** (D5): The frame render loop MUST resolve the current effect and the network timestamp exactly once per frame, use that single timestamp for every LED in the frame, and MUST NOT copy strip descriptors per strip iteration.
- **FR-007** (D6): Hardware LED writes MUST NOT recompute the device LED count per write; the count MUST be computed once at construction.
- **FR-008** (D7): Right-button LED feedback on the controller MUST reflect the state of the right buttons (both affected sites corrected).
- **FR-009** (D8): The dead keep-alive block in the node main loop MUST be removed.

### Process Requirements (from user request)

- **PR-001**: All work happens in the dedicated worktree on branch `worktree-002-fix-audit-findings` — never on `master`.
- **PR-002**: Each defect fix (D1–D8) lands as its **own commit**, with a message referencing the defect ID and this spec. Closely-coupled defects fixed by one coherent change (e.g., D5's two hoists) stay one commit; unrelated defects are never batched.
- **PR-003**: After each fix and **before its commit**, a fresh-context adversarial code review (verifier agent) reviews the diff against this spec and the CLAUDE.md invariants; findings are addressed before committing. Trivial doc/dead-code-only changes (D8) may skip review per the project's delegation policy.
- **PR-004**: Before each commit: `./lint.sh check` passes, the full host test suite (`smalltests` + `largetests`, ASan/UBSan) passes, and the PlatformIO envs affected by the change build (`node`, `fancy-node` for shared/lib code; `controller` for D7). Test-only or comment-only diffs still run the host suite.
- **PR-005**: New tests for a fix land in the same commit as the fix.

### Test Cases (minimum set)

| Defect | Test | Type |
|--------|------|------|
| D1 | Wire round-trip: encode packet (id, type, payload lengths 0/1/4/58) → decode → fields and payload length equal original | New host unit test |
| D1 | Decoded-then-re-encoded packet is byte-identical on the wire | New host unit test |
| D1 | Existing `RadioStateMachineTest` / `NetworkTest` suites still pass (FakeNetwork mesh behavior unchanged) | Regression |
| D2 | Heartbeat round-trip at 0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF under UBSan | New host unit test |
| D3 | Firefly `GetRGB` over all indices 0–255 on a Controller-flagged strip, UBSan-clean; output at indices 0–31 unchanged | New host unit test |
| D4 | `EffectsTest` full fuzz (every palette, 0–255 LEDs, multi-strip Tiny/Circular) passes unchanged under ASan/UBSan | Regression |
| D4 | Spot-check: for a fixed (effect, palette, time, strip), RGB output identical before/after (captured golden values in the test) | New host unit test |
| D5 | All existing host tests pass; frame renders use a single timestamp (verified in review; no host-observable hook exists) | Regression + review |
| D6 | `pio run -e node` and `-e fancy-node` build; no host test coverage exists for this hardware-only class | Build + review |
| D7 | `pio run -e controller` builds; manual test documented in commit message (press each right button, observe its own LED) | Build + manual |
| D8 | `pio run -e node` builds; block removed | Build |
| All | `./ci.sh` green; `./lint.sh check` clean; `InvalidPacketTest` fuzz passes | Regression, per commit |

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A packet relayed through an intermediate node arrives at a third node complete: 100% of rebroadcast packets are byte-identical to the original transmission (proven by round-trip tests).
- **SC-002**: Zero heap allocations occur during per-LED color computation in all registered effects (proven by construction: no palette value copies remain; verified by review and unchanged test output).
- **SC-003**: The entire host test suite, including new boundary tests, runs clean under ASan and UBSan.
- **SC-004**: Rendered LED output is unchanged for all existing hardware configurations and inputs, except that all LEDs within one frame now share one timestamp.
- **SC-005**: Every fix is one commit, reviewed before landing, on the feature branch — 8 defects → 7–8 commits (D5 may be one commit covering both hoists), each with tests/builds green at that commit.
- **SC-006**: Controller right-button presses light the correct LED in all three modes (manual verification).

## Out of Scope

Findings from the same audit that are **deliberately excluded** because the correct behavior is a design decision or the fix is a redesign, not a sure-thing repair:

- The 255-LED total ceiling (`GetLedCount` return width, 8-bit render indices) — may be an intentional constraint; widening touches wire-adjacent types.
- `READ_FROM_FLASH` device mode reading raw bytes into a structure containing heap-owning members — needs a serialization redesign; mode is not the default and is unused today.
- millis() rollover (~49.7 days) in protocol timers — accepted for the product's use pattern; fixing changes timing semantics and needs its own careful review.
- Structural deduplication of the controller's three mode functions — worthwhile but a refactor with manual-only verification; only the two concrete copy-paste bugs (D7) are fixed.
- The 3-byte header-only receive drop (`received_length > padding`) — interacts with CLAIM_MASTER delivery on hardware (see Assumptions); changing it alters radio behavior beyond the confirmed defect and is deferred.

## Assumptions

- The wire format (2-byte id, 1-byte type, payload) is correct as-is; D1 is fixed by populating the length on receive, not by changing the format. Making encode/decode host-testable may move that logic into shared platform-independent code, but the bytes on the wire do not change.
- CLAIM_MASTER packets are transmitted as 3 header bytes and are dropped by the current `received_length > kFrontPacketPadding` receive check — this pre-existing behavior is **out of scope** to change; the D1 fix must simply not make it worse. (Master conflicts still resolve via heartbeat-triggered elections.)
- "Sure about" is interpreted as: the defect was verified in source, the correct behavior is unambiguous, and the fix is local. All eight in-scope defects meet this bar; everything else from the audit is listed in Out of Scope.
- Bit-identical output comparisons are per-target-architecture and verified via the host build's deterministic fakes (FakeLedManager, FakeRadio, FakeNetwork).
- The existing CI definition (`./ci.sh`: cmake without simulator, smalltests, largetests; plus `node`/`fancy-node` PlatformIO builds) is the gate for "tests pass"; `controller` builds are added locally for D7 since CI does not build that env.
- The project constitution (`.specify/memory/constitution.md`) is an unfilled template; no additional governance constraints apply.
