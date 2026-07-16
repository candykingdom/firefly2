# Feature Specification: Shared Simulator Rendering Engine

**Feature Branch**: `codex/shared-simulator-engine`

**Created**: 2026-07-15

**Status**: Draft

**Input**: User description: "The simulator website should share the production rendering code instead of copying the C++ implementation into JavaScript."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Trust the simulator output (Priority: P1)

A show designer opens the browser simulator and sees output produced by the same rendering implementation used by Firefly firmware, so the website cannot silently behave differently because a handwritten copy drifted.

**Why this priority**: The simulator is the project's hardware-free test bench. Its main value depends on its colors and effect behavior representing production rather than a second implementation.

**Independent Test**: Render every committed reference case through the browser-facing engine and the host firmware engine and confirm every LED byte, effect index, palette index, device layout, and strip behavior agrees without maintaining a second rendering algorithm.

**Acceptance Scenarios**:

1. **Given** any registered effect, palette, device, and network time, **When** the simulator renders a frame, **Then** the colors are computed by the production rendering implementation.
2. **Given** a production effect bug fix, **When** its browser artifact is refreshed, **Then** the simulator reflects the fix without a corresponding handwritten rendering change in another language.
3. **Given** the simulator repository, **When** a reviewer inspects the browser code, **Then** it contains no independent copies of production effect, palette, device, color-conversion, noise, or registry logic.

---

### User Story 2 - Change rendering once (Priority: P2)

A firmware developer changes an effect, palette, device definition, or shared rendering primitive in its authoritative source and uses one documented command to refresh the browser artifact. Automated checks reject a stale artifact.

**Why this priority**: One source of truth only helps if the generated browser artifact is reproducible and drift is caught before merge.

**Independent Test**: Change one known output byte in a temporary working copy, run the freshness check and observe failure, refresh the browser artifact, then observe the freshness check and simulator tests pass.

**Acceptance Scenarios**:

1. **Given** unchanged production sources, **When** the browser artifact is rebuilt twice with the pinned toolchain, **Then** both builds are byte-identical.
2. **Given** a rendering source change without a refreshed browser artifact, **When** continuous integration runs, **Then** it fails with a direct stale-artifact message.
3. **Given** a new production effect or palette, **When** the browser artifact is refreshed, **Then** the simulator registry and user interface expose it without a manually duplicated registry entry.

---

### User Story 3 - Keep the simulator easy to run and automate (Priority: P3)

A contributor can still serve the checked-out simulator with the existing lightweight local server, use the existing controls, and drive it through the documented programmatic API without installing a firmware toolchain just to view it.

**Why this priority**: Sharing production code must not turn the simulator into a hardware-toolchain-only developer experience.

**Independent Test**: From a clean checkout containing the committed browser artifact, start the existing static server, load the page, exercise the public simulator API, and run the headless tests with the documented runtime prerequisites only.

**Acceptance Scenarios**:

1. **Given** a clean checkout, **When** a user starts the documented static server, **Then** the page loads and animates without first compiling firmware code.
2. **Given** existing simulator bookmarks and automation, **When** they use the documented query parameters and public API, **Then** their behavior remains compatible.
3. **Given** an unsupported or missing browser artifact, **When** the page initializes, **Then** it reports a clear error instead of displaying plausible but incorrect output.

### Edge Cases

- A production source changes while the committed browser artifact is stale.
- A browser cannot initialize the shared rendering artifact or blocks its loading.
- Effect or palette indices cover the full wire-byte range, including invalid values.
- Devices contain zero-length, reversed, dimmed, disabled, mirrored, circular, tiny, bright, or multiple strips.
- Seeded effects are instantiated repeatedly or rendered at network-time wrap boundaries.
- Several simulated devices render in one frame and must not leak state into one another.
- A user opens the page without the development toolchain used to produce the artifact.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The simulator MUST use the production implementation as the sole authoritative source for effect rendering, palettes, device definitions, strip behavior, color conversion, procedural math, and effect registration.
- **FR-002**: The browser application MUST NOT contain independently maintained copies of production rendering algorithms or registries.
- **FR-003**: Browser-specific code MAY adapt data, manage time and controls, draw the user interface, and expose the public API, but MUST delegate production rendering decisions to the shared engine.
- **FR-004**: The shared engine MUST expose enough metadata and rendering operations for the existing simulator to list effects, palettes, and devices and to obtain every rendered LED color.
- **FR-005**: The simulator MUST preserve the existing wire-index mappings, weighted effect registration, final-two-effect invariant, palette order, device catalog, strip flags, show semantics, deterministic seeds, invalid-index tolerance, and network-time wrapping behavior.
- **FR-006**: The existing public simulator API, URL parameters, clock controls, multi-device display, master mode, control overrides, and structured snapshots MUST remain compatible unless a documented correctness issue requires a change.
- **FR-007**: A clean checkout MUST include everything required to run the browser simulator without first compiling production code.
- **FR-008**: The project MUST provide one documented, repeatable command for refreshing the browser rendering artifact after authoritative sources change.
- **FR-009**: The browser rendering artifact MUST be reproducible with a pinned toolchain and MUST be committed so reviewers can associate it with the source change that produced it.
- **FR-010**: Continuous integration MUST rebuild or otherwise verify the committed artifact and fail when it is stale or generated from incompatible sources.
- **FR-011**: Automated tests MUST exercise the actual shared engine used by the page in both a headless environment and a browser-compatible environment.
- **FR-012**: Automated parity coverage MUST compare the shared browser engine with the existing host renderer across representative effects, palettes, devices, strip flags, control overrides, seeded effects, and time boundaries.
- **FR-013**: Initialization or rendering failures MUST be explicit and MUST NOT fall back to the removed independent implementation.
- **FR-014**: The migration MUST remove obsolete duplicated rendering files, tests, documentation, and maintenance instructions once equivalent shared-engine coverage exists.

### Key Entities

- **Authoritative rendering source**: The production definitions and algorithms that determine effect registration, palettes, devices, strip behavior, and RGB output.
- **Browser rendering artifact**: A reproducible, checked-in representation of the authoritative rendering source that browsers and headless tests can execute directly.
- **Browser adapter**: The small browser-specific layer that initializes the shared engine, translates simulator state into rendering requests, and returns structured metadata and LED snapshots.
- **Artifact manifest**: Metadata tying the browser artifact to its source inputs and generator version so freshness can be verified.
- **Simulator state**: Devices, show selection, control override, network time, playback controls, and master-mode state retained by the browser application.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Zero independently maintained production rendering algorithms, palettes, device catalogs, or effect registries remain in browser application source.
- **SC-002**: Every committed firmware-reference case matches the browser engine byte-for-byte with zero LED color differences.
- **SC-003**: A deliberately stale browser artifact is detected by automated checks on every supported development platform.
- **SC-004**: Two clean artifact builds from the same revision and pinned toolchain produce byte-identical committed files.
- **SC-005**: A contributor can open and use the simulator from a clean checkout with the same single local-server command and no production compilation step.
- **SC-006**: Existing simulator automation passes without public API or URL-contract regressions.
- **SC-007**: A production rendering change requires one authoritative code edit plus one artifact-refresh command, with no handwritten browser rendering edit.
- **SC-008**: The full host, simulator, lint, and firmware continuous-integration suites pass after migration.

## Assumptions

- Modern evergreen browsers can execute a checked-in portable compiled artifact.
- Rebuilding the artifact may require a development toolchain, but viewing and testing the committed simulator should not require the firmware toolchain.
- The simulator remains a static local web application; adding a long-running backend service is out of scope.
- Browser UI, canvas drawing, clock transport, master-mode orchestration, and the public JavaScript API remain browser-owned because they do not duplicate firmware rendering logic.
- Production rendering is the correctness authority; the browser must report a hard initialization error rather than keep a handwritten fallback.
- Prototype-only browser effects that have no production counterpart are out of scope for the shared production registry; experiments should become production effect sources before claiming firmware fidelity.
