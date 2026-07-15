# Research: Regression Test Coverage for All Subsystems

**Date**: 2026-07-11 | **Plan**: [plan.md](plan.md)

All decisions below were made after reading the artifacts involved (`test/VectorGen.cpp`, `test/FakeNetwork.{hpp,cpp}`, `lib/fake-radio/FakeRadio.{hpp,cpp}`, `lib/fake-led-manager/FakeLedManager.hpp`, `lib/battery/Battery.hpp`, `CMakeLists.txt` test targets, and the full existing test-case inventory).

## R1: How the firmware-vs-corpus comparison works (G1)

**Decision**: Split `test/VectorGen.cpp` (503 lines, self-contained) into `test/VectorGenCommon.{hpp,cpp}` — the case model (seed prediction, effect/palette wire tables, device catalog, per-case rendering into RGB triples) — plus a thin `VectorGen.cpp` keeping `main()` and JSON emission. New `test/ReferenceVectorTest.cpp` parses `sim/test/vectors/reference.json` with **nlohmann/json** (FetchContent, pinned tag, linked into test targets only) and, for every case, re-renders via `VectorGenCommon` and compares RGB-exact, failing with `(case id, effect, device, led index, expected, actual)`. Provenance metadata (`meta.firmwareGitDescribe`) is ignored. Missing/unparseable corpus = hard test failure (FR-002).

**Rationale**: The generator already encodes the hard parts (deterministic seed prediction for the three construction-time-random effects, registry order, device catalog); extracting it guarantees the test and the generator can never drift apart (QR-001). nlohmann is the standard header-only C++ JSON library, fetched the same way the repo already fetches googletest and FakeFastLED.

**Alternatives considered**:
- *Byte-compare regenerated JSON text against the committed file* (zero new deps): fails FR-001's case-identification requirement (line-number diffs), and couples correctness to JSON formatting.
- *Hand-rolled minimal JSON parser*: more code to maintain than the dependency it avoids; fragile against corpus formatting changes.
- *Have the sim's Node runner do the comparison* (already parses JSON): runs in a different CI job with no firmware toolchain; the point is a *firmware-side* pin that fails `./ci.sh`.
- *File path resolution*: the test locates the corpus relative to the source tree via a compile definition (`-DREFERENCE_VECTORS_PATH=...` set in CMake from `${CMAKE_CURRENT_SOURCE_DIR}`), the same technique as hardcoding but robust to build-dir location. Runtime cost: 1,380 cases ≈ what `vectorgen` itself does in well under a second — comfortably inside SC-005.

## R2: How the wire path is threaded through the test radio (G2)

**Decision**: Change `FakeRadio` only (FakeNetwork stays untouched): `sendPacket` serializes the packet into an internal wire buffer via `RadioPacket::Serialize`; `getSentPacket` deserializes that buffer via `RadioPacket::Deserialize` into the returned packet (returning `nullptr` if deserialization rejects it). The receive-side injection API (`setReceivedPacket`, raw struct memcpy in `readPacket`) is **unchanged**, preserving the fuzz suite's ability to inject packets the codec could never produce (FR-005).

**Rationale**: Every packet that flows across the simulated mesh goes `sendPacket → FakeNetwork picks it up via getSentPacket → setReceivedPacket` on peers, so putting Serialize→Deserialize inside the send/pick-up hop makes **all** existing protocol, mesh, and integration tests exercise the production codec with zero changes to those tests. A D1-class regression (decode not populating `dataLength`) then propagates `dataLength = 0` packets into every peer, tripping the host-build asserts in the payload readers and failing many suites (SC-002's ≥ 5 requirement — verified by demonstration during implementation). Keeping injection raw-struct means the fuzz path is not filtered by codec validation.

**Alternatives considered**:
- *Route inside `FakeNetwork::Tick`*: covers only multi-node tests; single-node `RadioStateMachineTest`/`NetworkManagerTest` (which use FakeRadio directly) would stay blind.
- *Serialize on `setReceivedPacket` too*: would clamp/reject the fuzz suite's deliberately invalid packets — weakens FR-005; rejected.
- *A new `WireFakeRadio` subclass used selectively*: splits the suites into covered/uncovered halves; the point is structural coverage everywhere.

## R3: Render-loop semantics tests (G3)

**Decision**: Extend `test/LedManagerTest.cpp` using existing hooks: `FakeLedManager::ClearEffects()` + `PublicAddEffect()` with a tiny deterministic stub effect (color = f(led_index), defined in the test) so expected values are trivially computable. Table-driven cases: (a) four-strip device [unflagged, Off, Dim, Reversed] — assert per-strip output against the unflagged control strip (Off → black, Dim → control÷8, Reversed → control index-flipped); (b) multi-strip global indexing — each strip's pixels land at the correct contiguous offsets (uses `GetLed` across the whole device); (c) Dim+Reversed combined; (d) control override — inject a `SET_CONTROL` packet via the state machine, assert all LEDs show the commanded color, then expire/replace it and assert effect rendering resumes.

**Rationale**: A stub effect makes flag assertions exact (no dependence on real effect math), which is what "pins the semantics" means; the real-effect interactions are already covered by `EffectsTest` fuzz + G1's corpus. Uses only existing public test hooks — no product changes.

**Alternatives considered**: asserting against real effects (couples flag tests to effect internals — rejected); new fixture file (unnecessary — LedManagerTest already has the right includes/fixtures, QR-001).

## R4: Leaf utilities (G4)

**Decision**:
- `DeviceDescriptionTest.cpp`: `GetLedCount` for zero/one/many strips; document (in a comment + assertion at the current type's boundary) the known uint8_t return-width ceiling as pinned current behavior.
- `ColorPaletteTest.cpp` (extend): gradient at exact color boundaries, wrap vs. no-wrap, single-color palette, position 0 and max — table-driven.
- `BatteryTest.cpp`: `BatteryVoltageToRawReading` monotonicity and round-trip within one quantization step at `kBatteryEmpty`, `kBatteryFull`, and midpoints. Requires adding `lib/battery` to the host include dirs (one CMake line; header-only constexpr, no new sources).
- **FR-008's "pin the out-of-range palette index path" is refined**: the OOB path is undefined behavior (vector `operator[]` past end) and cannot be executed under fatal sanitizers. Instead we pin the *boundary contract*: a test asserts `Effect::palettes().size()` equals the wire-table constant the corpus records, and asserts every valid index renders (extending what `EffectsTest::allColorPalettes` covers to make the boundary explicit). The OOB hazard stays documented in the test with a reference to the 002 review finding. This is recorded as a deliberate deviation from the spec's letter (executing UB to "pin" it would be worse than the gap).

**Rationale**: Cheap, table-driven, each traceable to G4; nothing duplicates existing assertions (checked against the inventory).

## R5: CI integration (FR-009)

**Decision**: No workflow changes. All new C++ tests are `test/*Test.cpp` → auto-globbed into `smalltests` → run by `./ci.sh` → `Run Tests` workflow. `VectorGenCommon.cpp` intentionally does not match the `.*VectorGen\.cpp$` exclusion regex and lands in `testlib`. The nlohmann FetchContent happens at CMake configure, same network posture as existing fetches. Verified assumption: the CMake glob picks up new files after a `cmake ..` re-run, which `./ci.sh` performs fresh every time.

## R6: Determinism & runtime (FR-010, SC-005)

**Decision**: All new tests use `setMillis()` (fake clock) and the seed-prediction pattern already established by `VectorGenCommon` (`srand(1)` + `random16_set_seed(1337)` before LedManager construction). Baseline suite wall time is measured at implementation start (`make test` on this machine, warm build) and re-measured at the end; the reference comparison is bounded (~1,380 × ~50 LED renders) and expected to add low single-digit seconds.

## R7: Firefly cases are libc-bound (discovered on first CI run, 2026-07-11)

**Finding**: the first CI run of `ReferenceVectorTest` failed on ubuntu — `effectSeedsMatchPrediction` (corpus Firefly seed 423 vs glibc prediction) and 60 of the 72 Firefly cases. Root cause: `FireflyEffect`'s construction offset comes from libc `rand()` after `srand(1)`, and `rand()` is implementation-defined — the committed corpus encodes the macOS/BSD sequence. This was invisible before because nothing on CI compared *firmware* rendering to the corpus (the JS sim hardcodes `DEFAULT_FIREFLY_OFFSET=423` rather than calling `rand()`; docs/simulator.md documents the same non-portability for the JS port).

**Decision**: platform-gate exactly the Firefly comparisons. `effectSeedsMatchPrediction` asserts Fire/Rorschach (FakeFastLED LCG — portable) and reports, without failing, a Firefly seed mismatch; `allCasesMatchFirmwareRendering` skips the Firefly RGB comparison only when the local prediction differs from the recorded seed, counts the skips, asserts the count is exactly the Firefly grid size (4 palettes × 6 times × 3 devices = 72), and prints a NOTE. On the corpus-generating platform all 1,380 cases stay byte-pinned; elsewhere 1,308 are. This narrows FR-001's "every case" to "every case whose inputs are platform-reproducible" — recorded here as a deliberate deviation.

**Alternatives considered**: making `FireflyEffect` seed from `random16` (portable) — the correct long-term fix, but a product-code + corpus + JS-port change, out of this feature's scope (spec: no product changes beyond test doubles); per-platform seed constants in the test (fragile against libc updates); skipping silently (violates the no-silent-caps principle).
