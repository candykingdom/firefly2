# Contract: Firmware Reference Vectors

**Feature**: 001-web-simulator

Ground truth for simulator↔firmware fidelity (FR-005, FR-020, SC-003). Produced by the additive C++ target `vectorgen` (`test/VectorGen.cpp`) using the same fakes as the existing host tests (`FakeLedManager`, FakeFastLED); committed at `sim/test/vectors/reference.json`; consumed by `sim/test/cases/vectors.test.mjs` for byte-exact comparison.

## Generation

```bash
mkdir -p build && cd build
cmake .. -DBUILD_SIMULATOR=false
make vectorgen
./vectorgen > ../sim/test/vectors/reference.json
```

- `vectorgen` is **not** part of `smalltests`/`largetests` and does not change CI behavior.
- Output is deterministic (fixed fake `analogRead` seeds); regenerating without firmware changes yields an identical file (byte-stable JSON: fixed key order, no floats, LF newlines) — a nonempty diff always means firmware behavior changed.

## Sampled grid

Every **unique** effect (17) × palettes {0 (Red, solid), 9 (Warm, 2-color), 8 (Rainbow, 3-color), 21 (Double Rainbow, 6-color)} × devices {scarf, puck, ufo} × times {0, 1, 1000, 60000, 2147483648, 4294967295} ms, plus SET_CONTROL cases (rgb {255,0,0} and {12,34,56}) on each device at times {0, 1000}. Dim/Off/Reversed coverage comes via the ufo (flagged strips).

## Schema

```json
{
  "meta": {
    "generator": "test/VectorGen.cpp",
    "firmwareGitDescribe": "<git describe --always --dirty>",
    "effectSeeds": { "Fire": 12345, "Firefly": 678, "Rorschach": 9012 }
  },
  "effects": [ { "index": 0, "name": "Color Cycle" } ],
  "palettes": [ { "index": 0, "name": "Red" } ],
  "cases": [
    {
      "effectIndex": 14,
      "paletteIndex": 8,
      "device": "puck",
      "timeMs": 1000,
      "leds": [ [255, 0, 0], [254, 2, 0] ]
    },
    {
      "control": { "rgb": [255, 0, 0] },
      "device": "scarf",
      "timeMs": 0,
      "leds": [ [255, 0, 0] ]
    }
  ]
}
```

- `leds` is the flat global-index order `LedManager::RunEffect` writes (strip concatenation, post Reversed/Dim/Off) — i.e. exactly what `FakeLedManager::GetLed(i)` returns after `RunEffect()`.
- `effects`/`palettes` tables let `registry.test.mjs` cross-check the JS wire mappings against firmware, not just against this spec.
- `meta.effectSeeds` records the constructor offsets in effect in the generating build; the JS engine's defaults must reproduce them (or load them from this file — either satisfies the contract, see research R3).

## Consumption contract

For every case: construct the JS engine with the case's device, effect/palette (or control), seeds from `meta`, pin time to `timeMs`, `getSnapshot()`, flatten to global order, and assert **strict equality** of every channel byte. Any mismatch fails with (case id, LED index, expected, actual).
