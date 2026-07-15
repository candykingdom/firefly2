# Contract: What Each Regression Suite Guarantees to Catch

**Date**: 2026-07-11 | **Plan**: [../plan.md](../plan.md)

This contract states, per suite, the defect classes that MUST produce a CI failure and the failure-message quality required. Break-demonstrations (SC-001..003) prove each guarantee empirically during implementation.

## ReferenceVectorTest (G1)

**Guarantee**: any change to the rendered output of any registered effect, for any palette or catalog device covered by the corpus, fails `smalltests` in CI.

- Failure MUST identify: case id, effect name, device, LED index, expected vs actual RGB.
- MUST also fail on: registry drift (effect wire-index table changed, last-two invariant broken), palette table drift, seed-model drift, FastLED math primitive drift.
- MUST fail (not skip) if the corpus file is missing, unreadable, or schema-invalid.
- MUST ignore provenance metadata (`firmwareGitDescribe`).
- Escape hatch (intentional change): regenerate corpus in the same commit; both this suite and the JS sim suite then gate against the new truth.
- Proven by: SC-001 demonstration (one-line effect-math change → named failure → revert).

## Wire-path FakeRadio (G2)

**Guarantee**: a defect in `RadioPacket::Serialize`/`Deserialize` — or any code path that depends on correct wire framing — fails multiple existing protocol/mesh suites, not only `RadioPacketTest`.

- Every packet a test observes via `getSentPacket` (and everything FakeNetwork relays) has physically round-tripped through the production codec.
- Valid traffic MUST be codec-transparent: the entire pre-existing suite passes unchanged.
- Invalid-packet injection (`setReceivedPacket`) MUST remain codec-free so `InvalidPacketTest`'s 14,400-case fuzz keeps its full adversarial strength.
- Proven by: SC-002 demonstration (re-introduce D1: decode without setting payload length → ≥ 5 non-codec test failures → revert).

## Render-loop semantics tests (G3)

**Guarantee**: a regression in `RunEffect`'s centralized handling of `Off`, `Dim`, `Reversed`, multi-strip global indexing, or the `SET_CONTROL` override path fails a test whose name identifies the broken semantic.

- Assertions are exact (stub effect with computable expected values), each flag verified against an unflagged control strip rendered in the same frame.
- Proven by: SC-003 demonstrations (break each of Off/Dim/Reversed handling → named failure → revert).

## Leaf-utility tests (G4)

**Guarantee**: edge-case behavior changes in `DeviceDescription::GetLedCount`, `ColorPalette::GetGradient`, `BatteryVoltageToRawReading`, and the palette-registry boundary fail a named test.

- The out-of-range palette index path is documented, boundary-pinned (registry size == corpus table size; all valid indices render), and deliberately NOT executed (it is UB; see research R4).

## CI integration (FR-009)

- All C++ suites above run inside `smalltests` via the existing glob → `./ci.sh` → the `Run Tests` workflow, on every push. No workflow file changes expected; if any prove necessary they land in the same change.
- Determinism: fixed seeds + fake clock; suites MUST pass under fatal ASan/UBSan on macOS and ubuntu CI identically.
- Runtime budget: total host-suite wall time +<25% vs pre-change baseline (SC-005), measured warm on the same machine.
