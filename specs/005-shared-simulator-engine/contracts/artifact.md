# Browser Artifact Contract

## Committed files

The following files are generated together and reviewed as one unit:

- `sim/generated/firefly-renderer.js`
- `sim/generated/firefly-renderer.wasm`
- `sim/generated/manifest.json`

The page imports only the committed artifact. No runtime download or generated-code fallback is allowed.

## Rebuild command

From the repository root, with Emscripten SDK 6.0.3 active:

```bash
./scripts/build-simulator-wasm.sh
```

The command performs a clean release build into a temporary directory and atomically replaces all three generated files only after successful compilation and manifest generation.

## Determinism

- Emscripten version and FakeFastLED commit are exact pins.
- Output names, optimization flags, export order, source order, and manifest key order are fixed.
- No timestamp, hostname, username, absolute checkout path, or dirty-tree marker appears in generated output.
- Two clean builds from identical inputs must byte-match all committed files.

## Freshness check

```bash
./scripts/check-simulator-wasm.sh
```

The check rebuilds into an isolated temporary directory, compares the JS, Wasm, and manifest bytes, and exits nonzero with the stale filenames. It never modifies the working tree.

CI installs/activates the pinned SDK, runs the freshness check before simulator tests, then runs Node and browser-compatible tests against the committed artifact.

## Source fingerprint

The manifest fingerprint covers, in stable path order:

- every compiled `lib/` and `src/generic/` rendering source/header;
- `sim/wasm/` bridge/build sources;
- the build script and exact compiler/link flags;
- pinned Emscripten and FakeFastLED identifiers.

Documentation, UI-only JavaScript, CSS, tests, and reference vectors are excluded because they do not determine the artifact bytes.

## Failure behavior

- Missing artifact: module import fails with a clear initialization message.
- ABI mismatch: adapter refuses to construct `SimEngine`.
- Stale artifact: CI fails before functional simulator tests.
- Generation failure: existing committed files remain untouched.
