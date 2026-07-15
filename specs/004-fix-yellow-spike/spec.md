# Feature Specification: Fix the Bright Yellow LED Artifact in Rainbow-Palette Gradients

**Feature Branch**: `004-fix-yellow-spike`

**Created**: 2026-07-15

**Status**: Draft

**Input**: User description: "on the rainbow effect palette between two of the colors there's a single yellow LED that lights up that might be only on strip sizes bigger than 50 but not sure, fix it"

## Background

**Symptom**: On the rainbow palette (red → green → blue), effects that render the palette as a smooth spatial gradient show one LED near the red→green crossover that is visibly brighter than its neighbors and reads as a stray yellow pixel. It repeats every 32 LEDs in the Rainbow effect, so it is most noticeable on longer strips; small devices are typically `Tiny`-flagged (whole strip renders one color, no spatial gradient), which is why short strips appear immune — the "bigger than 50" observation in the report.

**Root cause (verified empirically on the host build, 2026-07-15)**: The HSV→RGB conversion used throughout the firmware (`hsv2rgb_rainbow`, both the real FastLED fork and the host/sim fakes — their hue-band code is identical) deliberately over-drives the yellow hue band (hue 32–95, the "Y1 yellow boost"): total RGB drive rises from a ~255 baseline to ~341 (+34%), peaking at hue 64 (pure yellow). That boost is a reasonable choice for *solid* colors (yellow otherwise reads dim), but the palette gradient hue-lerps red(0)→green(96) straight through the band, so exactly one LED lands near peak boost. Measured on a 100-LED strip at half brightness: the artifact LED renders (44,43,0) — total drive 87 — while every other LED in the gradient cycle holds a flat 65–66.

**What is NOT the cause** (ruled out by exhaustive sweeps of every reachable input state, C++ and JS): the gradient interpolation math in `ColorPalette::GetGradient` and its simulator port are correct and continuous — no off-by-one, no wrap glitch, no segment-boundary discontinuity. The artifact is purely the power spike introduced at HSV→RGB conversion.

**Validated fix approach**: Rescale the converted RGB of *interpolated* gradient colors so total drive matches the palette's un-boosted baseline, applied only in the four "palette showcase" effects that render smooth gradients (Rainbow, Color Cycle, Rainbow Bumps, Display Color Palette). A prototype was implemented and pixel-verified on the host: the artifact LED becomes (32,32,0) — total drive 64, flat with the rest of the cycle — while the hue still sweeps through yellow as a rainbow should. Noise/texture effects (Fire, Lightning, Rorschach, Firefly, Spark, Swinging Lights, Simple Blink, Contrast Bumps) and exact palette colors intentionally keep the boost.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Rainbow gradients render with uniform brightness (Priority: P1)

A person watching a device (or the web simulator) running the Rainbow effect with the rainbow palette on a non-Tiny strip sees a smooth red→yellow→green→blue color sweep of even brightness. No single LED pops brighter than the band around it at the red→green crossover (or anywhere else in the cycle).

**Why this priority**: This is the reported bug. The artifact is visible on every long-strip device whenever the rainbow palette is active — at Burning Man scale, on every synced device simultaneously.

**Independent Test**: Render the Rainbow effect with the rainbow palette on a 60+ LED non-Tiny strip (simulator or host harness) and measure per-LED total RGB drive across one 32-LED spatial cycle; the maximum must not exceed the gradient endpoints' drive by more than a small tolerance.

**Acceptance Scenarios**:

1. **Given** the Rainbow effect with the rainbow palette on a 60-LED non-Tiny strip, **When** a frame is rendered, **Then** no LED's total RGB drive exceeds the maximum drive of the palette's own colors (red/green/blue at the same brightness setting) by more than a small tolerance (before the fix: 87 vs 65 — a 34% spike; after: flat 64–66).
2. **Given** the same setup, **When** a frame is rendered, **Then** the hue progression still passes through orange and yellow between red and green — the fix flattens brightness, it does not remove the rainbow's yellow.
3. **Given** the Display Color Palette, Color Cycle, and Rainbow Bumps effects with the rainbow palette, **When** frames are rendered, **Then** the same uniform-drive property holds for their gradient rendering (Color Cycle's is temporal: the whole strip must not pulse brighter as it sweeps past yellow).
4. **Given** any solid-color palette (including solid yellow) or an effect outside the four showcase effects, **When** frames are rendered, **Then** output is unchanged from today.

---

### User Story 2 - Simulator remains byte-exact with firmware (Priority: P1)

A developer testing shows in the web simulator sees exactly what the hardware will render: the simulator's ports of the four affected effects apply the identical brightness flattening, and the committed reference-vector corpus (regenerated for this intentional visual change) pins both sides.

**Why this priority**: The corpus is a bidirectional CI gate (firmware `ReferenceVectorTest` + sim suite). Changing firmware rendering without the mirrored sim change and corpus regen breaks CI by design; a half-done fix would either fail CI or silently desynchronize the simulator from hardware.

**Independent Test**: Run the firmware host suite and the sim suite against the regenerated corpus; both pass. Spot-check one affected case's bytes match between the C++ helper and the JS port.

**Acceptance Scenarios**:

1. **Given** the fix applied to firmware and simulator with the corpus regenerated in the same change, **When** the host test suite and sim test suite run, **Then** both pass byte-exact against the new corpus.
2. **Given** only one side changed (firmware but not sim, or vice versa), **When** the suites run, **Then** at least one suite fails naming the diverging case (this is the existing safety net working as intended — the change must land atomically).

---

### User Story 3 - The artifact can never silently return (Priority: P2)

A future refactor of the effects, palette code, or HSV→RGB conversion that re-introduces a gradient brightness spike fails CI with a test whose name and failure message point at the uniform-drive property, not just at "some vector byte changed".

**Why this priority**: The reference corpus would catch any output change, but a corpus regen after an unrelated intentional change could silently re-absorb a regression. A semantic test encodes *why* the property matters.

**Independent Test**: Check out the pre-fix code with only the new regression test added; the test fails. Apply the fix; it passes.

**Acceptance Scenarios**:

1. **Given** the pre-fix rendering code, **When** the new regression test runs, **Then** it fails, identifying the LED whose drive spikes above the gradient endpoints.
2. **Given** the fixed code, **When** the test runs, **Then** it passes, in both the firmware host suite and the simulator suite.

---

### Edge Cases

- Palettes with per-color value/saturation (pastel rainbow s=127, jazz cup v=200, candy-cane desaturated stops): flattening must reference the *same* saturation/value the gradient color carries, so desaturated and dimmed gradients are compared against an equally desaturated/dimmed baseline (at s=0 the boost vanishes and the fix must be a no-op).
- Fully dark colors (v=0, e.g. Rainbow Bumps between bumps): total drive 0 — the fix must not divide by zero and must return black.
- `Bright` vs. normal strips (v=255 vs v=128) and `Dim` strips (post-render ÷8): the uniform-drive property must hold at every brightness setting.
- Single-color palettes take the solid-color code paths in Rainbow/Color Cycle and must render exactly as today.
- Tiny strips render one gradient color across the whole strip: flattening applies (Color Cycle-like temporal uniformity) but there is no spatial neighbor to compare — covered by the temporal scenario.
- The four affected effects run on every device class (SAMD node, STM32 nodes, controller, ESP32 DMX): the per-LED cost of the flattening (a second HSV→RGB probe conversion and three integer divides) must stay well inside the SAMD ~128 ms watchdog budget for 100+ LED frames.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Gradient colors rendered by the Rainbow, Color Cycle, Rainbow Bumps, and Display Color Palette effects MUST have total RGB drive no greater than the drive of an un-boosted reference color at the same saturation and value (tolerance: rounding only, i.e. ≤ a few counts of 765).
- **FR-002**: The flattening MUST only ever scale drive down (never brighten), MUST preserve the color's hue and saturation (channel ratios), and MUST be a no-op when the converted color is not boosted (including v=0, s=0, and all hues outside the boosted band).
- **FR-003**: Effects other than the four named ones, exact (non-interpolated) palette colors, and the solid-color branches of Rainbow/Color Cycle MUST render byte-identically to today.
- **FR-004**: The web simulator's ports of the four effects MUST apply the identical flattening with byte-exact integer math (truncating division), verified by the reference-vector corpus.
- **FR-005**: The reference-vector corpus MUST be regenerated in the same change, and both the firmware-side vector test and the simulator suite MUST pass against it.
- **FR-006**: A regression test MUST exist on both the firmware and simulator sides asserting the uniform-drive property on a ≥60-LED non-Tiny strip with the rainbow palette — a test that fails on today's code and passes with the fix.
- **FR-007**: The effect/palette registries (order, weights, last-two invariant), the radio protocol, and all timing behavior MUST be unchanged.
- **FR-008**: The existing effect fuzz (`EffectsTest`: every palette, 0–255 LEDs, multi-strip Tiny/Circular devices) MUST pass unchanged, and the fix MUST NOT introduce undefined behavior (host suites run with fatal UBSan).

### Key Entities

- **Gradient color**: a palette color produced by interpolating between two palette stops (as opposed to an exact stop); the only kind of color the flattening applies to.
- **Total RGB drive**: the sum of a pixel's R+G+B output bytes — the proxy for LED power draw and perceived pop used by the measurements and the acceptance tolerance.
- **Reference (un-boosted) drive**: the drive of a hue outside the boosted band (red, hue 0) converted at the same saturation and value — the baseline gradients are flattened to.
- **Reference-vector corpus**: the committed byte-exact rendering corpus (`sim/test/vectors/reference.json`) that pins firmware and simulator to each other; regenerating it is the sanctioned way to land an intentional visual change.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On a 60+ LED non-Tiny strip with the rainbow palette, the brightest LED in a rendered Rainbow-effect frame exceeds the dimmest gradient-endpoint LED by less than 5% total drive (today: +34%).
- **SC-002**: The red→green transition still renders intermediate orange/yellow hues (the rainbow look is preserved — verified by hue inspection of the rendered bytes).
- **SC-003**: All existing suites (host smalltests including the vector test and effect fuzz, largetests, simulator suite) pass against the regenerated corpus on CI.
- **SC-004**: The new uniform-drive regression test fails when run against pre-fix rendering code and passes with the fix, on both firmware and simulator sides.
- **SC-005**: Rendered output for everything outside the four showcase effects' gradient paths is byte-identical to the pre-fix corpus (diff of old vs. new corpus shows changes only in cases for the four effects).

## Assumptions

- The flattening is intentionally scoped to the four palette-showcase effects; noise/texture effects keep FastLED's yellow boost as an aesthetic choice. Broadening later is a one-line change per effect.
- Solid (single-color) palettes keep the boost everywhere — changing the brightness of solid yellow is out of scope.
- The prototype approach (probe an un-boosted reference hue at the same s/v, scale channels by `reference_sum/sum` with truncating integer math, only when `sum > reference_sum`) is the validated implementation shape; the plan may refine details but the acceptance criteria above are shape-independent.
- "Strip sizes bigger than 50" in the report is explained by Tiny-flagging of small devices, not by a size-dependent code path; no size-specific fix is needed.
- Hardware-visual verification (flashing a physical strip) is out of scope for CI; the host-render measurements and simulator are accepted proxies, consistent with how the project verifies all rendering behavior.
