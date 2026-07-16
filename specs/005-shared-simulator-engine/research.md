# Research: Shared Simulator Rendering Engine

## R1. Share C++ with the browser through a committed WebAssembly artifact

**Decision**: Compile the existing portable C++ effect engine and host FastLED implementation to WebAssembly. Commit the generated module beside the static site.

**Rationale**: This removes handwritten effect, palette, device, color-math, noise, flag, and registry copies while preserving the current no-build viewing experience. The browser runs the same C++ methods that host tests and STM32 firmware compile. Emscripten supports modularized ES-module output for browser and Node consumers, and separate `.wasm` files are expected to be served over HTTP. See the official [modularized output](https://emscripten.org/docs/compiling/Modularized-Output.html) and [WebAssembly build](https://emscripten.org/docs/compiling/WebAssembly.html) documentation.

**Alternatives considered**:

- Keep the JavaScript port and rely on reference vectors: rejected because sampled parity detects drift after duplication; it does not remove the duplicate implementation or maintenance burden.
- Run a native C++ HTTP backend: rejected because it adds a service and requires every viewer to build or install it.
- Generate JavaScript source from C++: rejected because it introduces a bespoke transpiler and still produces difficult-to-review algorithm code.
- Share only `FlattenedGradientRGB`: rejected because Adam's observation applies to the whole port, not just the newest helper.

## R2. Use a narrow C ABI rather than binding C++ classes

**Decision**: Export versioned integer/string metadata accessors, renderer-handle lifecycle functions, and bulk RGB render calls through `extern "C"`. JavaScript copies packed RGB bytes from Wasm memory into snapshots.

**Rationale**: A C ABI is explicit, compact, and independent of C++ name mangling, class layout, RTTI, and exceptions. Bulk frame transfer avoids one JavaScript/Wasm crossing per LED. The adapter can fail immediately when the ABI version is incompatible.

**Alternatives considered**:

- Embind: rejected because it adds binding/runtime size and makes the generated JavaScript surface harder to audit.
- One exported function per effect or LED: rejected because it leaks registry structure and creates excessive call overhead.
- JSON-in/JSON-out: rejected because repeated parsing and allocation are unnecessary for every animation frame.

## R3. Make C++ catalogs authoritative

**Decision**: Add a C++ effect registration table that drives `LedManager` construction and browser metadata, expose palette names beside the existing C++ palettes, and add a named device catalog referencing the existing `Devices` definitions.

**Rationale**: Compiling effect bodies alone would leave registry weights, display names, palette order, and device layouts duplicated in JavaScript. One C++ table per concept lets firmware and the browser consume the same order and values. Tests preserve the final-two-effect invariant and `<256` wire limit.

**Alternatives considered**:

- Parse C++ headers at build time: rejected because a parser is fragile and generated metadata can still drift from runtime construction.
- Keep small JavaScript metadata tables: rejected because the current drift risk includes registry and device definitions, not only pixel algorithms.
- Macro-generate all existing device variables: deferred; a C++ catalog referencing those variables removes cross-language duplication without making embedded call sites less readable.

## R4. Preserve deterministic construction seeds without libc dependence

**Decision**: Add optional seed overrides to the three construction-seeded effects through the shared effect registry. Firmware uses the existing random defaults. The simulator uses the reference values Fire=6198, Firefly=423, and Rorschach=24359, and the public seed override API selects a cached renderer for each seed tuple.

**Rationale**: C library `rand()` sequences are platform-specific; Emscripten must not accidentally change Firefly output. Explicit simulator seeds preserve the current API and reference corpus without changing hardware randomness. Pooling renderer handles prevents hundreds of identical C++ registries from accumulating when tests create many `SimEngine` objects.

**Alternatives considered**:

- Depend on Emscripten's `rand()`: rejected because it would differ from the committed host corpus.
- Remove `setEffectSeed`: rejected because it is part of the documented simulator API.
- Add effect setters after construction: rejected because immutable construction parameters are easier to reason about and keep effect rendering const.

## R5. Pin and verify the build inputs

**Decision**: Pin Emscripten SDK 6.0.3 and FakeFastLED commit `f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca`. The build script emits fixed filenames and a deterministic manifest. CI performs a clean rebuild and byte-compares all generated files.

**Rationale**: Emscripten 6.0.3 is the current tagged release at design time and the SDK supports installing exact historical versions. Pinning the host FastLED commit also fixes a currently floating CMake dependency. A rebuild comparison catches source, flag, dependency, or generated-glue drift instead of sampling only output behavior. See the official [emsdk installation/versioning guidance](https://github.com/emscripten-core/emsdk).

**Alternatives considered**:

- Track `latest` or branch heads: rejected because artifact bytes would change independently of repository source.
- Store only a source hash: rejected because matching declared inputs does not prove the committed binary was actually produced from them.
- Download a prebuilt artifact in the browser: rejected because it adds a runtime network dependency and weakens source/review association.

## R6. Keep synchronous simulator calls behind asynchronous module startup

**Decision**: `sim/js/renderer.js` uses top-level await to initialize Emscripten's modularized ES module once. After module import resolves, `SimEngine` construction and its public methods remain synchronous.

**Rationale**: The browser and Node 22 support top-level await. Existing callers do not need to await every snapshot, and the page naturally waits for the imported module before constructing `window.sim`. Initialization failures reject import and are shown as a hard page error rather than falling back.

**Alternatives considered**:

- Make every engine method async: rejected because it breaks the public API and complicates animation loops.
- Synchronous Wasm instantiation: rejected because browser loading is naturally asynchronous and Emscripten's supported modularized output returns a promise.

## R7. Keep browser orchestration, remove rendering copies

**Decision**: Retain JavaScript clock transport, master-mode sequencing, URL parsing, public API composition, and canvas drawing. Move all frame rendering and catalog data behind the shared renderer. Delete the JavaScript effect/math/palette/device modules.

**Rationale**: These retained pieces are browser product behavior, not duplicated per-pixel firmware algorithms. The browser's deterministic autoplay RNG and scrub semantics intentionally model user-facing behavior rather than execute the radio state machine. Keeping that boundary avoids coupling the UI clock to hardware timers while satisfying the user's duplication concern.

**Alternatives considered**:

- Compile the full radio state machine and UI clock into Wasm: rejected because it would make pause/scrub and deterministic autoplay depend on hardware-oriented timers and networking without reducing rendering drift.
- Keep central strip handling in JavaScript: rejected because `Reversed`, `Dim`, and `Off` are production rendering responsibilities and already live in `LedManager::RunEffect`.

## R8. Test the artifact actually used by the page

**Decision**: Migrate all headless and browser tests to import the Wasm adapter. Keep the host reference corpus and pixel property tests, add ABI/catalog/freshness tests, and add an import guard that rejects production-renderer modules under `sim/js/`.

**Rationale**: Reference vectors still provide a reviewable behavior baseline and catch intentional changes, while shared execution eliminates cross-language algorithm drift. Running Node and browser harnesses against the committed artifact proves the actual user path. The import guard prevents a future fallback port from quietly returning.

**Alternatives considered**:

- Delete reference vectors because the source is shared: rejected because they pin intentional visual behavior and protect compiler/dependency changes.
- Test only the C++ host build: rejected because loader, ABI, memory transfer, and browser compatibility can still fail.
