# Feature Specification: Web Simulator

**Feature Branch**: `001-web-simulator`

**Created**: 2026-07-10

**Status**: Draft

**Input**: User description: "i want you to create an easy to use simple web page in this repo that is for testing firefly lighting shows without having to use the actual hardware. design a nice, clean, visually appealing site. make sure to understand how firefly works and create a good UX for us to be able to test out the lights and stuff. also make it so you can easily interact with the page yourself so you can verify both the websites functionality and its correctness with the firefly protocal and testing new shows" — later clarified: "this site should also just be used for general testing of firefly. its basically a simulator so we dont need hardware"

## Overview

Firefly devices render lighting effects as pure functions of a shared network clock: every effect computes each LED's color from `(led position, network time, strip characteristics, current show parameters)`. Today, seeing what Firefly actually does requires flashing physical hardware (or building the desktop SDL simulator, which renders a single 20-LED line). This feature adds a browser-based simulator — a web page living in this repository — that becomes the project's standard hardware-free way to test Firefly: it reproduces the effect library, the device catalog, and the protocol's show semantics on simulated devices, so anyone on the project can preview shows, exercise protocol behavior, and validate changes from a laptop with no hardware, no toolchain, and no install.

The page serves two audiences equally: humans exploring shows visually, and automated agents (e.g. Claude working in this repo) that need to drive the page programmatically and read back exact rendered colors to verify the simulator against the firmware's behavior and to test new shows before they're written into firmware. Beyond show design, it is the general visual test bench for Firefly work: checking how an effect change looks across the device catalog, exercising protocol semantics (effect/palette wire indices, control overrides, master cadence), and demonstrating behavior without assembling hardware.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Preview any effect on a device without hardware (Priority: P1)

A show designer opens the simulator page in a browser, picks a device layout (e.g. scarf, puck, UFO, bike), picks an effect and a color palette, and immediately sees the LEDs animating exactly as that device would render them. The page is clean and visually appealing, with the LED rendering as the centerpiece and controls that are self-explanatory without documentation.

**Why this priority**: This is the core value — replacing "flash hardware to see anything" with "open a page." Every other capability builds on being able to render an effect on a simulated device.

**Independent Test**: Open the page, select the scarf device, select the Rainbow effect with the Fire palette, and observe a smooth animation on 46 simulated LEDs. Delivers standalone value even if nothing else is built.

**Acceptance Scenarios**:

1. **Given** the page is freshly opened, **When** no selection has been made yet, **Then** a device with a sensible default effect and palette is already animating (no blank screen, no mandatory setup).
2. **Given** an animating device, **When** the user selects a different effect, **Then** the rendering switches to the new effect without reloading the page.
3. **Given** an animating device, **When** the user selects a different palette, **Then** effects that use palettes visibly change their colors accordingly.
4. **Given** the full effect catalog, **When** the user browses the effect list, **Then** every effect registered in the firmware (including the blink variants, color-palette display, and dark) is available by its human-readable name, and every firmware palette is available.
5. **Given** any effect/palette/device combination, **When** it renders, **Then** animation is smooth (no visible stutter) on a typical laptop.

---

### User Story 2 - Verify shows across real device layouts and strip behaviors (Priority: P2)

A developer testing a show wants confidence it looks right on *all* the real hardware shapes, not just one strip. They select from the project's actual device catalog — multi-strip devices with different LED counts and per-strip flags (Tiny, Bright, Circular, Mirrored, Reversed, Dim, Off) — and view several devices side by side, all animating from the same shared clock, confirming they stay in sync just like a real mesh.

**Why this priority**: Firefly's whole point is many heterogeneous devices animating in sync. A single-strip preview can't answer "will the UFO's four rings and the cloak's three strips look right together?"

**Independent Test**: Display the UFO (4 strips, mixed Dim/Bright flags) and the puck (Tiny + Circular) simultaneously with the same effect; verify each strip honors its flags and both devices show the same moment of the animation.

**Acceptance Scenarios**:

1. **Given** the device picker, **When** the user browses it, **Then** the project's real device catalog (scarf, puck, lantern, bike, UFO, rainbow cloak, etc.) is available, each with its true strip counts and flags.
2. **Given** a multi-strip device, **When** an effect renders, **Then** each strip is rendered separately with its own LED count and flags, and strips with `Reversed`, `Dim`, or `Off` show the centrally-applied behavior (index inversion, ÷8 brightness, black) while `Tiny`/`Circular`/`Mirrored`/`Bright` show the effect's own adaptation.
3. **Given** two or more devices displayed at once, **When** they animate, **Then** they render from the same network time and remain visually synchronized indefinitely.
4. **Given** a circular strip (e.g. puck), **When** it renders, **Then** it is drawn as a ring, not a line, so circular effects read correctly.

---

### User Story 3 - Programmatic driving and exact-color verification (Priority: P2)

An automated agent (or a developer in the browser console) drives the simulator without clicking: it sets the device, effect, palette, and — critically — an exact network time, then reads back the precise color of every LED as data. Rendering is deterministic: the same (effect, palette, device, time) always yields the same colors. This lets the agent verify the simulator's output against the firmware's reference behavior, validate protocol semantics (effect/palette indices as wire bytes, packet field meanings), and regression-test new shows.

**Why this priority**: Explicitly requested — the page must be verifiable by the agent that builds and maintains it. Determinism and state readback are what turn a demo into a test instrument; without them, "correctness with the Firefly protocol" is unfalsifiable. Ranked with US2 because both are required before the simulator can be trusted.

**Independent Test**: From an automated script, set effect index 0, palette index 8, network time 5000 ms on the scarf; read back all 46 LED colors; set the same state again and confirm byte-identical colors; advance time by one frame and confirm the colors change.

**Acceptance Scenarios**:

1. **Given** the page is loaded, **When** an automated agent sets device, effect, palette, and network time through a documented programmatic interface, **Then** the display updates to exactly that state with no human interaction.
2. **Given** a fixed (device, effect, palette, time) tuple, **When** the frame is rendered twice — including across page reloads — **Then** every LED's color is identical both times.
3. **Given** any rendered frame, **When** the agent queries the LED state, **Then** it receives every LED's color as structured data (per strip, per index), not just pixels.
4. **Given** the time controls, **When** a human uses them, **Then** they can pause, resume, scrub to a specific time, and change playback speed — the same clock the programmatic interface controls.
5. **Given** an effect selected by wire index, **When** compared to selection by name, **Then** both resolve to the same effect, and the index↔name mapping matches the firmware's registration order (including weighted duplicates), with the color-palette display and dark effects as the final two indices.

---

### User Story 4 - Simulate show flow: master behavior and control packets (Priority: P3)

A user wants to experience a show as the mesh actually plays it, not just one hand-picked effect. They enable an "autoplay / master mode" where the simulator behaves like a Firefly master: picking random weighted effects on the master's cadence with random palettes, exactly like an unattended installation. They can also exercise the control path — sending a solid-color command (as `SET_CONTROL` does) with a delay, and observing devices hold that color, then return to the ambient show when the delay expires.

**Why this priority**: Valuable for judging how a night of shows feels and for testing protocol semantics beyond single effects, but the simulator is already useful for design and verification without it.

**Independent Test**: Enable master mode, observe effects changing automatically on the master cadence with weighted randomness; send a red control command with a 10-second delay and observe all devices go solid red, then resume the show after 10 seconds.

**Acceptance Scenarios**:

1. **Given** master mode is enabled, **When** the show runs, **Then** effects change automatically at the firmware master's cadence, drawn with the same weighting the firmware uses for random selection (weight-0 effects never chosen).
2. **Given** a control command with an RGB color and a delay in seconds, **When** it is issued, **Then** all displayed devices render that solid color for the delay duration and then resume the current effect.
3. **Given** a manually selected effect with a delay, **When** the delay expires in master mode, **Then** the master picks the next effect automatically.

---

### Edge Cases

- **Out-of-range indices**: Effect and palette indices are single wire bytes; the firmware must tolerate any byte value without crashing. The simulator must do the same — an out-of-range index requested programmatically is handled gracefully (mirroring firmware behavior for invalid packets), never a frozen or broken page.
- **Extreme LED counts**: Firmware effects are fuzzed from 0 to 255 LEDs per strip. The simulator must render (or benignly no-op) a 0-LED strip and stay smooth at the large end (e.g. the cloak's 94-LED strip, or all catalog devices displayed at once).
- **Time boundaries**: Network time is a 32-bit millisecond counter; scrubbing to very large times or time 0 must render correctly, not glitch.
- **Backgrounded tab**: When the browser tab is hidden and later refocused, the show continues from the correct clock position rather than freezing or bursting.
- **Paused vs. dark**: Pausing time and selecting the Dark effect must be visually distinguishable states (paused shows a frozen frame; dark shows black LEDs with the clock still advancing).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST be a web page stored in this repository that a user can open locally and use immediately, with no hardware, firmware toolchain, package installation, or network services required.
- **FR-002**: The system MUST reproduce the firmware's full registered effect catalog — every distinct effect including the four blink-interval variants, the color-palette display effect, and the dark effect — each selectable by human-readable name.
- **FR-003**: The system MUST reproduce all of the firmware's color palettes, selectable by name, and effects that consume palettes MUST use the selected palette's colors.
- **FR-004**: The effect↔index and palette↔index mappings MUST match the firmware's wire-protocol byte indices exactly, including registration order, weighted duplicate entries, and the invariant that the color-palette display and dark effects are the last two indices.
- **FR-005**: The system MUST render each LED's color as a function of (LED index, network time, strip description, show parameters) with the same semantics as the firmware's per-LED rendering contract, and MUST visually match the firmware's output for the same inputs.
- **FR-006**: The system MUST provide the project's real device catalog as selectable layouts, each with its actual strips, LED counts, and per-strip flags.
- **FR-007**: The system MUST honor the centrally-applied strip behaviors — `Reversed` (index inversion), `Dim` (brightness ÷ 8), `Off` (black) — outside the effects, and pass `Tiny`/`Bright`/`Circular`/`Mirrored`/`Controller` flags through to effects, mirroring the firmware's division of responsibility.
- **FR-008**: The system MUST render circular strips as rings and linear strips as lines, with each strip of a multi-strip device shown distinctly and labeled or visually identifiable.
- **FR-009**: The system MUST support displaying multiple devices simultaneously, all rendering from one shared network clock so they remain in sync.
- **FR-010**: Users MUST be able to control the clock: pause, resume, scrub to an arbitrary network time, and adjust playback speed.
- **FR-011**: The system MUST expose a documented programmatic interface through which an automated agent can set device(s), effect, palette, show parameters, and exact network time, and step or freeze the clock, without simulated mouse interaction.
- **FR-012**: The system MUST expose, programmatically, the current rendered state: every LED's color as structured data (device → strip → LED index → color), plus the current effect, palette, and network time.
- **FR-013**: Rendering MUST be deterministic: identical (device, effect, palette, parameters, time) inputs MUST produce identical LED colors, across frames and across page loads.
- **FR-014**: The system MUST tolerate out-of-range effect and palette indices and degenerate strip configurations (including 0 LEDs) without crashing, freezing, or corrupting state, consistent with the firmware's invalid-packet tolerance.
- **FR-015**: The system MUST offer an autoplay ("master") mode that changes effects automatically at the firmware master's cadence using the firmware's weighted random selection, and MUST support issuing a solid-color control command with a delay that overrides the show for that duration on all displayed devices.
- **FR-016**: The page MUST present a clean, visually appealing, self-explanatory interface where the simulated LEDs are the visual centerpiece; a first-time user MUST be able to change device, effect, and palette without instructions.
- **FR-017**: The page MUST begin animating a sensible default (device, effect, palette) immediately on load.
- **FR-018**: Adding a newly written effect to the simulator MUST be possible by adding one self-contained effect definition and one registry entry, without modifying unrelated parts of the page — so new shows can be prototyped in the simulator before or alongside firmware implementation.
- **FR-019**: The feature MUST ship with an automated, repeatable test suite that drives the simulator through its programmatic interface (FR-011/FR-012) and verifies, at minimum: every effect and palette renders without error; the wire-index mappings match the firmware registration order (FR-004); rendering determinism (FR-013); out-of-range-input tolerance (FR-014); central strip-flag behavior (FR-007); and control-override semantics (FR-015). The suite MUST be runnable on demand — by a developer or by an automated agent — and report pass/fail per case.
- **FR-020**: The test suite MUST include rendering-correctness cases that compare the simulator's per-LED colors against firmware reference values for a sampled set of (effect, palette, device, time) tuples, so simulator/firmware drift is detectable whenever either side changes.

### Key Entities

- **Effect**: A named, pure per-LED color function of (LED index, time, strip, show parameters). Identified on the wire by a single-byte index into a registration list that includes weighted duplicates; the final two indices are fixed by invariant.
- **Palette**: A named ordered list of colors (hue/saturation/value) that palette-aware effects draw from; identified on the wire by a single-byte index.
- **Device**: A named hardware layout from the project catalog: an ordered list of strips plus a power budget. The unit a user selects to preview on.
- **Strip**: A run of LEDs with a count and a set of flags (Tiny, Bright, Circular, Mirrored, Reversed, Controller, Dim, Off) that alter rendering either centrally or inside effects.
- **Network clock**: The single shared millisecond time source all devices render from; the simulator makes it pausable, scrubbable, and settable exactly.
- **Show state**: The currently active effect, palette, and parameters (equivalent to the firmware's set-effect message: effect index, palette index, delay), plus any temporary solid-color override (equivalent to the control message: RGB + delay).
- **LED state snapshot**: The structured readout of every rendered LED color at the current time, keyed by device, strip, and LED index — the verification surface for automated testing.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A first-time user can go from opening the page to watching a chosen effect on a chosen device in under 30 seconds, without reading any documentation.
- **SC-002**: 100% of the firmware's registered effects and 100% of its palettes are selectable and render animated output in the simulator.
- **SC-003**: For a sampled set of (effect, palette, device, time) tuples, the simulator's per-LED colors match the firmware reference implementation's output for the same inputs.
- **SC-004**: An automated agent can, without any human action, set an exact simulator state, read back every LED color, and get byte-identical results on repeated renders of the same state — demonstrated by a repeatable verification run included with the feature.
- **SC-005**: Multiple simultaneously displayed devices remain visually synchronized for at least 10 minutes of continuous playback (no drift between devices).
- **SC-006**: Animation of the largest catalog device (and of all catalog devices shown together) is smooth on a typical laptop — no visible stutter at normal playback speed.
- **SC-007**: Feeding every possible effect index and palette index byte value (0–255) programmatically never crashes or freezes the page.
- **SC-008**: A developer can add a new prototype effect and see it animating in the simulator in under 15 minutes of work.
- **SC-009**: The full automated test suite (functionality, protocol mappings, determinism, firmware-comparison samples) runs to completion and reports results with zero human involvement — an automated agent can build, verify, and demonstrate the simulator end-to-end on its own.

## Assumptions

- **Local, developer-facing tool**: The page is for project contributors and runs from the repository on their machines; public hosting, authentication, and multi-user features are out of scope.
- **Rendering fidelity target**: "Correct" means matching the firmware's per-LED output for the same inputs (the effect functions are deterministic given time and parameters), verified by comparison against the firmware reference (e.g. via the existing host-test fakes) for sampled states. Sub-pixel monitor color calibration and LED gamma/physical-brightness modeling are out of scope — colors are compared as RGB data, displayed as-is.
- **Mesh simulation depth**: The simulator models the *rendering* consequences of the protocol (shared clock, show state, control overrides, master cadence and weighted effect selection). It does not simulate radio-layer mechanics — packet loss, rebroadcast dedup, master election timing — which are already covered by the existing host tests (`FakeNetwork`, `RadioStateMachineTest`). As the project's general hardware-free test bench, the simulator may grow radio-layer visualization later; this feature deliberately excludes it.
- **Power limiting out of scope**: The firmware's milliamp-budget power limiting alters physical brightness on hardware; the simulator displays unconstrained colors and does not model the power limiter.
- **Effect duplication is acceptable**: The effect algorithms will exist in a second (web) implementation alongside the C++ firmware; keeping them in lockstep is a maintenance duty, mitigated by the comparison verification in SC-003/SC-004. Firmware code is not modified by this feature.
- **Modern evergreen browser**: Users run a current desktop browser; legacy browser support and mobile-first layout are out of scope (the page should merely not break on a tablet).
- **Controller flag**: The `Controller` strip flag exists for the handheld controller's status LEDs; catalog devices using it render like any other strip, with the flag passed through to effects.
