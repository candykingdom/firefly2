# Quickstart: Web Simulator

**Feature**: 001-web-simulator

## Prerequisites

- Any evergreen browser; `python3` (page serving); Node ≥ 18 (headless tests). No installs, no npm.
- Only for regenerating firmware reference vectors: the existing CMake host toolchain (same as `./ci.sh`).

## Run the simulator

```bash
python3 -m http.server 8642 -d sim
# open http://localhost:8642/
```

Expected: page loads showing the scarf animating the Rainbow effect immediately; picking any device/effect/palette updates live; clock transport (pause/scrub/speed) works; enabling Master mode auto-changes effects; the Control panel sends a solid-color override that expires after its delay.

## Headless verification (agent-runnable, zero human)

```bash
node --test "sim/test/cases/*.test.mjs"
```

Expected: all suites pass — registry wire-index mappings ([contracts/reference-vectors.md](contracts/reference-vectors.md) cross-check + last-two invariant), determinism, central flag handling, 0–255 index fuzz + 0-LED strips, SET_CONTROL semantics, master cadence/weighting, and byte-exact comparison against `sim/test/vectors/reference.json`.

## Browser-side verification

1. Open `http://localhost:8642/test.html` — same case modules run in-page; expected: all green, summary banner shows 0 failures.
2. Console driving (API contract: [contracts/sim-api.md](contracts/sim-api.md)):

```js
sim.setDevices(['scarf']).setEffect('Rainbow').setPalette(8).pause().setTime(5000)
sim.getSnapshot().devices[0].strips[0].leds[0]   // deterministic [r,g,b]
sim.setControl([255,0,0], 10)                    // all LEDs solid red for 10 s
```

## Regenerate firmware reference vectors (after intentional effect changes)

```bash
mkdir -p build && cd build && cmake .. -DBUILD_SIMULATOR=false && make vectorgen
./vectorgen > ../sim/test/vectors/reference.json
cd .. && node --test "sim/test/cases/*.test.mjs"   # confirm simulator matches new firmware behavior
```

Expected: nonempty `reference.json` diff **only** when firmware effect behavior actually changed; `vectors.test.mjs` failures pinpoint (case, LED, expected, actual).

## Full pre-commit check

```bash
./ci.sh                 # unchanged firmware suites still pass (additive vectorgen only)
npm test                # simulator suite (= node --test "sim/test/cases/*.test.mjs")
npm run lint            # ESLint over sim/
clang-format --dry-run --Werror test/VectorGen.cpp   # lint.sh only scans lib/ and src/
```
