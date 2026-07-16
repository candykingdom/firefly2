# Quickstart: Shared Simulator Rendering Engine

## Prerequisites

- Node.js 22 for the headless simulator suite.
- Python 3 for the existing static server.
- Emscripten SDK 6.0.3 only when rebuilding or freshness-checking the committed artifact.
- The existing CMake and PlatformIO prerequisites for full repository validation.

## 1. Run the committed simulator without compiling

```bash
python3 -m http.server 8642 -d sim
```

Open `http://localhost:8642/`. Expected: the scarf starts in Rainbow/Rainbow, effect/palette/device controls populate from C++ metadata, and `window.sim.getSnapshot()` returns structured LED bytes.

## 2. Run the headless shared-engine suite

```bash
npm ci
node --test "sim/test/cases/*.test.mjs"
```

Expected: all existing API, timing, master, flag, tolerance, gradient-power, registry, and reference-vector cases pass through the Wasm renderer.

## 3. Rebuild the browser artifact

Activate Emscripten SDK 6.0.3, then:

```bash
./scripts/build-simulator-wasm.sh
git diff -- sim/generated
```

Expected with unchanged sources: no diff. After an intentional production rendering change: generated files change together and `manifest.json` records new hashes/fingerprint.

## 4. Prove stale artifacts are rejected

```bash
./scripts/check-simulator-wasm.sh
```

Expected: success on a clean refreshed branch. Temporarily changing a compiled rendering source without rebuilding makes this command name the stale artifact and fail.

## 5. Verify there is no browser rendering copy

```bash
test ! -e sim/js/effects
test ! -e sim/js/fastled.js
test ! -e sim/js/perlin.js
test ! -e sim/js/palette.js
test ! -e sim/js/devices.js
rg "Port of lib/|Transcribed EXACTLY|hsv2rgbRainbow|paletteGetGradient" sim/js
```

Expected: the files are absent and the search has no rendering-port matches.

## 6. Full repository validation

```bash
./ci.sh
./lint.sh check
npm run lint
node --test "sim/test/cases/*.test.mjs"
uv run pio run -e node-arm64
uv run pio run -e fancy-node
uv run pio run -e controller
```

Expected: all commands pass. The firmware effect registry and final-two-effect invariant remain unchanged, while the browser consumes them through the shared artifact.
