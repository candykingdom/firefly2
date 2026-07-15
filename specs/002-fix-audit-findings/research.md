# Research: Fix Confirmed Audit Findings

**Date**: 2026-07-10 | **Plan**: [plan.md](plan.md)

No NEEDS CLARIFICATION markers existed in the Technical Context; the research questions here are the per-defect fix-approach decisions. Each was resolved by reading the affected source (all findings were re-verified at head `f7547e1`).

## R1: How to fix D1 (dataLength never set on receive) testably

**Decision**: Add a wire codec to `RadioPacket` in `lib/radio/` — `uint8_t Serialize(uint8_t* buf) const` (writes id big-endian, type, payload; returns total wire length) and `bool Deserialize(const uint8_t* buf, uint8_t len)` (parses header, sets `dataLength = len - 3`, copies payload; returns false if `len < 3` or `len - 3 > PACKET_DATA_LENGTH`). `RadioHeadRadio::sendPacket`/`readPacket` delegate to these, preserving their existing radio-mode bookkeeping (`radio.available()` calls) and the existing `received_length > kFrontPacketPadding` drop check at the call site.

**Rationale**:
- The defect lives in Arduino-only code that host tests can't compile, but the encode/decode *logic* is platform-independent. `lib/radio/Radio.cpp` is already host-compiled and already owns the packet field helpers (`writeHeartbeat`, etc.), so it's the natural home. FR-002 requires host round-trip tests; this makes them possible with zero test-infrastructure changes (test glob picks up the existing `RadioPacketTest.cpp`).
- Bounding `dataLength` at 58 on decode upholds the "invalid packets never crash" invariant defensively. (Today RH_RF69 caps received payloads below that, so the check can't fire in practice — it guards future radio backends and keeps `Deserialize` total.)
- Bytes on the wire are unchanged: same 3-byte header, same payload placement.

**Alternatives considered**:
- *One-line fix in `RadioHeadRadio::readPacket`* (`packet.dataLength = received_length - kFrontPacketPadding`): smallest diff, but untestable on host — rejected by FR-002, and this exact class of bug survived precisely because this file has no test coverage.
- *Free functions in a new `lib/wire/`*: no benefit over members; RadioPacket already carries its field codecs.
- *Route `FakeRadio` through Serialize/Deserialize so all mesh tests exercise the wire path*: attractive (would have caught D1), but changes shared test infrastructure and risks masking behavior differences across ~all existing tests in the same commit as the fix. Deferred; noted as possible follow-up outside this feature.
- *Also fix the 3-byte header-only drop (`received_length > kFrontPacketPadding`) so CLAIM_MASTER propagates*: real issue, but changes on-air protocol behavior — explicitly out of scope per spec.

## R2: How to fix D2 (heartbeat decode shifts into sign bit)

**Decision**: Cast each byte to `uint32_t` before shifting in `readTimeFromHeartbeat` (`time |= (uint32_t)this->data[0] << 24;` — cast all four lanes for symmetry). Add round-trip tests at 0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF.

**Rationale**: In C++14, `uint8_t` promotes to `int`; `<< 24` of a value ≥ 0x80 sets the sign bit — UB. The cast makes it defined with identical intended results. UBSan in the host build verifies.

**Alternatives considered**: `memcpy`-based decode (obscures the big-endian intent); changing wire byte order (forbidden — wire format frozen).

## R3: How to fix D3 (Firefly `<< led_index` for index ≥ 32) behavior-preservingly

**Decision**: `offset = ((kBlinkPeriod + 1234) << (led_index & 31)) % (kBlinkPeriod / 2);` with the shifted operand explicitly `uint32_t`.

**Rationale**: For `led_index < 32` (all current hardware; largest Controller strip is 12) the mask is a no-op → bit-identical output, which is the spec's FR-004 requirement. For 32–255 the result becomes well-defined (wrapped) instead of UB. `kBlinkPeriod = (1<<16)/64 = 1024`, so the shifted value overflows `uint32_t` for `led_index ≥ 22` — that's *defined* unsigned wraparound and already the effect's behavior today for indices 22–31; the visual result is an arbitrary-but-stable offset, which is all this effect needs.

**Alternatives considered**: widening to `uint64_t` (still UB at shift ≥ 64, just moves the cliff; larger codegen on Cortex-M0); redesigning the per-LED offset hash (behavior change — out of "sure fix" scope).

## R4: How to fix D4 (11 effects copy `ColorPalette` by value per LED)

**Decision**: Change local bindings to `const ColorPalette&` in the 11 affected files (`ColorCycle`, `ContrastBumps`, `DisplayColorPalette`, `Firefly`, `Lightning`, `RainbowBumps`, `Rainbow`, `Rorschach`, `SimpleBlink`, `Spark`, `SwingingLights`). Add a golden-value spot check to `EffectsTest` pinning exact RGB output for a fixed (effect, palette, time, strip) tuple.

**Rationale**: `Effect::palettes()` returns `const std::vector<ColorPalette>&` to a function-local static — references into it are valid for program lifetime, and `GetColor`/`GetGradient`/`Size` are const. The copy (and its `std::vector<CHSV>` heap allocation) per LED per frame is pure waste. Reference binding is provably output-identical.

**Alternatives considered**: caching the palette per frame in `LedManager` and passing it down (API change to `GetRGB` — touches every effect signature, higher risk); leaving member-palette effects (`Fire`, `Pride`) alone (they already don't copy — confirmed untouched).

## R5: How to fix D5 (per-LED re-resolution in `RunEffect`)

**Decision**: In `LedManager::RunEffect`, hoist `Effect* effect = GetCurrentEffect()`, `RadioPacket* packet = radio_state->GetSetEffect()`, and `uint32_t time_ms = radio_state->GetNetworkMillis()` above the loops; iterate strips with `const StripDescription&` (range-for). Flag handling (Reversed/Dim/Off) stays byte-for-byte identical.

**Rationale**: The main loop is single-threaded (`state_machine.Tick(); led_manager->RunEffect();`) — no packet can arrive mid-frame, so hoisting `GetCurrentEffect`/`GetSetEffect` is semantics-preserving. Hoisting `GetNetworkMillis()` is the one *intended* behavior change: every LED in a frame renders at the same timestamp (spec SC-004). The strip copy (`const StripDescription strip = *it;`) becomes a reference — `StripDescription` stores `const uint8_t` members only, so the copy was cheap but still pointless.

**Alternatives considered**: none serious; this is the textbook hoist. A per-frame render test was considered and rejected — no host-observable hook exposes the timestamp used per LED without adding test-only instrumentation; review + existing suites cover it (spec's D5 test row says exactly this).

## R6: How to fix D6 (per-write `GetLedCount()` in `FastLedManager::SetLed`)

**Decision**: Store `const uint16_t led_count_` in `FastLedManager`, initialized in the constructor (which already computes it), and use it in `SetLed` for the single-LED special case.

**Rationale**: `DeviceDescription::strips` is const — the count cannot change at runtime. `SetLed` runs once per LED per frame; removing a loop-over-strips per call is free performance. Hardware-only class → verified by `pio run -e node -e fancy-node` builds + review (no host harness exists; spec acknowledges).

**Alternatives considered**: memoizing inside `DeviceDescription::GetLedCount()` (mutable member or static cache — worse: hides cost, invites misuse elsewhere); fixing the uint8_t return-width truncation at the same time (out of scope — 255-LED ceiling is a design decision per spec).

## R7: How to fix D7 (controller right-button LED copy-paste)

**Decision**: `controller.cpp:179` — `left_buttons[1]` → `right_buttons[1]` in `RunEffectMode`'s right-button chain. `controller.cpp:267,270` — `left_buttons[1]`/`left_buttons[2]` → `right_buttons[1]`/`right_buttons[2]` in `RunColorMode`'s right-button chain. Nothing else.

**Rationale**: `RunPaletteMode` (lines 336–348) contains the correct pattern — the intent is unambiguous. Behavior verified by `pio run -e controller` build, review, and a documented manual test (press each right button; its own LED shows pressed state).

**Alternatives considered**: deduplicating the three `Run*Mode` LED blocks into a helper (the refactor that would prevent recurrence) — explicitly out of scope per spec; manual-only verification makes a structural refactor a poor fit for "sure fixes".

## R8: How to fix D8 (dead keep-alive block in node.cpp)

**Decision**: Delete `node.cpp:149-152` (the `if (millis() > print_alive_at)` block) and the `print_alive_at` global (line 142). Keep `watchdog_counter` logic untouched.

**Rationale**: The block's only effect is updating its own variable; the `Serial.println` inside is commented out. Anyone wanting the debug print back has git history.

**Alternatives considered**: re-enabling the print (adds serial traffic in the hot loop for no requested benefit); leaving it (fails FR-009).

## Cross-cutting: review & verification workflow

**Decision**: After each fix (commits 1–7), launch the **verifier** agent (fresh context, opus) on the staged diff with: the defect ID + spec/plan/research references, the relevant CLAUDE.md invariants, and the instruction to construct concrete failure scenarios. Address findings before committing. D8 skips review (trivial dead-code removal, per CLAUDE.local.md policy). Every commit runs the full gate in [quickstart.md](quickstart.md) first.

**Rationale**: Direct user requirement (PR-003) matching the project's standing delegation policy.
