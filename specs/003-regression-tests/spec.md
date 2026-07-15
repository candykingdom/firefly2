# Feature Specification: Regression Test Coverage for All Subsystems

**Feature Branch**: `worktree-002-fix-audit-findings` (continues in the existing worktree; merges to `master`)

**Created**: 2026-07-11

**Status**: Draft

**Input**: User description: "i want you to write regression tests for all the parts of this software and make sure they run with CI. tests should be meaningful, concise, and useful"

## Background

The project already has substantial test assets: 69 host tests (protocol state machine, mesh dedup, wire codec, effect fuzzing, utilities), a 14,400-case invalid-packet fuzz, a browser-simulator suite (59 tests), and — since the simulator landed — a committed corpus of 1,380 firmware-generated reference vectors covering every effect × palette × three device layouts. All of these already run in CI on every push.

"Regression tests for all the parts" is therefore a **gap-closing** feature, not a green-field one. A coverage audit (2026-07-11, test-case inventory vs. subsystem list) found four meaningful gaps, each of which corresponds to a real bug class that has already occurred or nearly occurred in this codebase:

| Gap | Bug class it would catch | Evidence it matters |
|-----|--------------------------|---------------------|
| G1. The firmware's rendered output is not pinned by any firmware test — the 1,380 reference vectors are only checked against the *JS simulator*, and the firmware side is merely the generator. An unintended change to any effect's output passes CI today. | Silent visual regressions in any effect/palette/device | The D4/D5 perf fixes were only provable harmless by hand-built golden tests; the vector corpus already exists and covers ~150× more cases |
| G2. Mesh/protocol tests exchange in-memory packet structs — the real wire encode/decode path is bypassed by the test radio. | Serialization bugs invisible to every protocol test | Defect D1 (mesh-breaking `dataLength` bug) lived for years precisely because of this |
| G3. No test asserts the render loop's central flag semantics (Off → black, Dim → ÷8, Reversed+multi-strip global LED indexing) or the control-override rendering path. | Regressions in `RunEffect`'s centralized flag handling | Flagged as a coverage gap by the D5 code review; those branches were moved as untested code |
| G4. Assorted small units have no coverage: device LED-count aggregation, palette gradient degenerate inputs (empty palette, out-of-range palette index — a known latent out-of-bounds), battery voltage conversion. | Edge-case arithmetic errors in leaf utilities | The out-of-range palette index was flagged by the D4 review as a real latent defect path |

Everything else in the test-worthy core is already covered; hardware-only translation units (RadioHead driver, FastLED backend, device `main`s) cannot run on the host and stay covered by CI firmware builds only.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Any unintended change to rendered output fails CI (Priority: P1)

A developer (or agent) modifies effect code, palette data, or the render pipeline — deliberately or as a side effect of a refactor. If any LED's rendered color changes for any registered effect, palette, or catalog device relative to the committed reference corpus, the test suite fails on the next push, naming the exact case that diverged. Intentional visual changes are accommodated by regenerating the corpus in the same change, making the visual diff explicit and reviewable.

**Why this priority**: Rendered output is the product. The reference corpus already exists and is maintained (the simulator depends on it); pinning the firmware against it is the single highest-leverage regression net available, and it converts the corpus from one-directional (JS checked against firmware) to bidirectional.

**Independent Test**: Introduce a one-line change to any effect's math; the suite must fail identifying that effect/case. Revert; suite green.

**Acceptance Scenarios**:

1. **Given** the committed reference corpus, **When** the host suite runs on unmodified code, **Then** every reference case matches the firmware's rendered output exactly and the suite passes.
2. **Given** a change that alters any effect's output for any covered case, **When** the suite runs, **Then** it fails and the failure message identifies the effect, device, and case that diverged.
3. **Given** an intentional effect change accompanied by a regenerated corpus, **When** the suite runs, **Then** it passes (and the corpus diff is visible in review alongside the code change).
4. **Given** the corpus file is missing or malformed, **When** the suite runs, **Then** the test fails with a clear message (it must not silently skip).

---

### User Story 2 - Protocol tests exercise the real wire path (Priority: P2)

A developer changes anything in the packet path — the codec, the mesh layer, the state machine's packet handling. The existing protocol and multi-node mesh tests now move every packet through the real wire encoding (serialize → bytes → deserialize) rather than copying in-memory structs, so a serialization bug of the class that silently broke multi-hop sync (D1) fails many tests immediately.

**Why this priority**: The costliest historical bug in this codebase was invisible precisely because tests bypassed serialization. Closing that structural blind spot protects the entire protocol suite at once.

**Independent Test**: Re-introduce the D1 bug (stop setting the payload length on decode); a broad swath of existing mesh/state-machine tests must fail. Restore; all green.

**Acceptance Scenarios**:

1. **Given** the test radio routes packets through the wire codec, **When** the full existing protocol/mesh/fuzz suites run, **Then** all pass unchanged (the codec is transparent for valid packets).
2. **Given** a re-introduced D1-class defect (payload length not populated on receive), **When** the suite runs, **Then** multiple existing tests fail — not only the codec unit tests.
3. **Given** the invalid-packet fuzz corpus (oversized lengths, unknown types), **When** packets flow through the wire path, **Then** nothing crashes and invalid frames are dropped or tolerated per the existing invariants.

---

### User Story 3 - Render-loop semantics are pinned (Priority: P3)

A developer touches the central render loop. Tests now assert its documented per-flag semantics — `Off` strips render black, `Dim` strips render at one-eighth brightness, `Reversed` flips strip-local order, multi-strip devices map each strip's LEDs to the correct contiguous global range — and the control-override path (a control packet renders the commanded solid color instead of the current effect).

**Why this priority**: These semantics are documented invariants ("RunEffect handles Reversed/Dim/Off centrally") with zero direct assertions today; the D5 review called this out. They're cheap to pin and every device depends on them.

**Independent Test**: Break any one flag branch (e.g., skip the Dim divide); at least one test fails naming that flag.

**Acceptance Scenarios**:

1. **Given** a device with strips flagged Off / Dim / Reversed / unflagged, **When** a frame renders, **Then** each strip's output obeys its flag exactly (black / ÷8 of the unflagged output / index-reversed / unchanged), verified against the same effect rendered on an unflagged control strip.
2. **Given** a multi-strip device, **When** a frame renders, **Then** every strip's pixels land at the correct global LED offsets with no overlap or gap.
3. **Given** a control packet commanding a solid color, **When** a frame renders, **Then** all non-Off LEDs show that color regardless of the current effect index.

---

### User Story 4 - Leaf utilities stop being blind spots (Priority: P4)

Small, pure functions that everything else leans on get direct edge-case tests: device LED-count aggregation across strips, palette gradient behavior at degenerate inputs (empty, single-color, wrap vs. no-wrap boundaries), battery voltage/reading conversion round-trips, and the documented behavior for the known latent defect of out-of-range palette indices (pinned as "documented current behavior", not fixed — fixing it is a separate decision).

**Why this priority**: Cheap insurance; lowest impact of the four because these functions change rarely. Included to honor "all the parts" honestly rather than by inflating test count.

**Independent Test**: Each new test fails if its target function's edge behavior changes.

**Acceptance Scenarios**:

1. **Given** devices with zero, one, and many strips, **When** the LED count is computed, **Then** it equals the sum of strip counts (within the type's documented range).
2. **Given** degenerate palette inputs (single color; positions at exact color boundaries; wrap vs. no-wrap), **When** a gradient color is computed, **Then** results match the documented interpolation behavior.
3. **Given** battery voltages at and beyond the calibration range, **When** converted to raw readings and back, **Then** values round-trip within one quantization step.

### Edge Cases

- Reference corpus and firmware disagree only in metadata (e.g., generator version string): the comparison must ignore provenance metadata and compare only behavioral content.
- Effects with per-boot random state: the corpus generator already fixes seeds; the firmware-side comparison must use the same seeding so runs are deterministic everywhere (CI containers, macOS, WSL).
- Wire-path routing must preserve the existing tests' ability to inject deliberately invalid packets (the fuzz suite constructs packets that could never be produced by the codec) — invalid injection must bypass or extend the codec path without weakening it for valid traffic.
- A strip with both Dim and Reversed set: both semantics apply.
- Suite runtime: additions must not meaningfully slow the developer loop (see SC-005); the vector comparison is large but must stay a small fraction of the existing fuzz cost.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001** (G1): The host test suite MUST verify the firmware's rendered output against the committed reference corpus — every case, exact match — and fail with the diverging case identified (effect, device, position) on any mismatch.
- **FR-002** (G1): The comparison MUST fail loudly if the corpus is missing, unreadable, or structurally invalid.
- **FR-003** (G1): Regenerating the corpus MUST remain a single documented command, so intentional visual changes are a code-plus-corpus diff in one review.
- **FR-004** (G2): The shared test radio MUST route every valid packet through the production wire encode/decode path, such that a serialization defect fails existing protocol/mesh tests, not just codec unit tests.
- **FR-005** (G2): Tests MUST retain the ability to inject invalid/corrupt packets (the fuzz invariants stay intact and unweakened).
- **FR-006** (G3): Tests MUST assert the render loop's central flag semantics — Off, Dim, Reversed, and multi-strip global index mapping — each against an unflagged control rendering.
- **FR-007** (G3): Tests MUST assert the control-override rendering path (solid color replaces the current effect for the override's duration).
- **FR-008** (G4): Tests MUST cover device LED-count aggregation, palette gradient degenerate inputs, battery conversion round-trips, and MUST pin (as documented current behavior) the out-of-range palette index path.
- **FR-009**: All new tests MUST run in the existing CI pipelines on every push with no manual steps; if any new suite falls outside current CI entry points, CI MUST be extended in the same change.
- **FR-010**: All new tests MUST be deterministic (fixed seeds, no wall-clock dependence) and pass under the enforcing sanitizers.

### Test-quality bar (from user request: meaningful, concise, useful)

- **QR-001**: No new test may duplicate an existing assertion; prefer extending existing suites/fixtures over parallel structures.
- **QR-002**: Prefer data-driven/table-driven cases over copy-pasted test bodies.
- **QR-003**: Every new test must be traceable to a gap (G1–G4) — no coverage theater; if a planned test turns out to be redundant during implementation, drop it and note why.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A one-line change to any registered effect's output math causes a CI failure that names the diverging case (demonstrated during implementation for at least one effect, then reverted).
- **SC-002**: Re-introducing the D1 defect causes ≥ 5 existing (non-codec) tests to fail (demonstrated during implementation, then reverted).
- **SC-003**: Breaking any single render-loop flag branch causes at least one test failure naming that flag (demonstrated for Off, Dim, and Reversed, then reverted).
- **SC-004**: 100% of the new tests run in CI on every push (verified in the CI logs of the landing change) and pass sanitizer-clean.
- **SC-005**: Total host-suite wall time increases by less than 25% relative to the pre-change baseline on the same machine.
- **SC-006**: All 1,380 reference cases pass on unmodified code across the three covered device layouts.

## Out of Scope

- Hardware-only translation units (`RadioHeadRadio`, `FastLedManager`, device `main`s, controller UI logic) — not host-runnable; they remain covered by CI firmware builds. No new mocking layer for them (a hardware-abstraction test harness is a separate, larger decision).
- Fixing latent defects encountered while writing tests (e.g., out-of-range palette index OOB) — behavior is *pinned*, fixes are separate features; a test may document current behavior but must not change product code beyond what FR-004 requires in the test doubles.
- Performance benchmarks/regression timing gates (beyond SC-005's suite-runtime bound).
- Browser-simulator test expansion (its 59-test suite + vector cross-check already run in CI and were built to spec in 001).
- millis() rollover behavior (accepted product limitation, documented in 002's Out of Scope).

## Assumptions

- The committed reference corpus (1,380 cases, three device layouts) is the agreed-upon definition of "correct rendered output"; broadening the corpus (more devices/timestamps) is a corpus-generator concern and can be done later without changing the comparison mechanism.
- "All the parts" means all host-runnable subsystems; the user accepts build-only coverage for hardware-only code (consistent with 002's reviewed-and-accepted approach).
- Routing test-double traffic through the production codec (FR-004) is a test-infrastructure change to `lib/fake-radio`/`test/FakeNetwork`, permitted product-code-adjacent change; the production codec itself must not need changes (it was built and reviewed for exactly this in 002).
- CI entry points remain `./ci.sh` (host suites, auto-globbed) and the existing workflows; new C++ test files are picked up automatically, so FR-009 is expected to be satisfiable with zero or minimal workflow edits.
- Battery conversion helpers are host-compilable pure functions; if any turn out to be hardware-bound, they move to Out of Scope with a note rather than gaining a mock.
