# Research: Fix the Bright Yellow LED Artifact in Rainbow-Palette Gradients

**Date**: 2026-07-15 · All findings below were established empirically during diagnosis, on this machine, against the current `master`. Nothing here is speculative; re-verification commands are in [quickstart.md](quickstart.md).

## R1. Root cause

**Decision**: The artifact is the FastLED `hsv2rgb_rainbow` yellow-band power boost, not a gradient-math bug.

**Rationale / evidence**:

- `hsv2rgb_rainbow` (identical hue-band code in the real candykingdom FastLED fork, the host FakeFastLED, and the sim's `sim/js/fastled.js` port — verified by diff) hard-codes `Y1 = 1`: hue section 32–63 renders `r = 171, g = 85 + third`, section 64–95 renders `r = 171 - twothirds, g = 170 + third`. Total drive rises from the ~255 baseline (all other sections are power-flat at 255–256 per unit value) to a peak of ~341 (+34%) at hue 64.
- `ColorPalette::GetGradient` hue-lerps red(0) → green(96) straight through that band, so any smooth red→green palette blend (rainbow, fire, pastel/double rainbow) has exactly one LED near the boost peak.
- Measured, `RainbowEffect`, rainbow palette (index 8), 100-LED non-Tiny strip, v=128, t=0: LED 7 renders **(44,43,0) = drive 87** while every other LED in the 32-LED spatial cycle holds **64–66**. The pattern repeats every 32 LEDs (`led_index * 8` offset, `<<8`, mod 2^16).
- Exhaustive sweeps found **no** lone-pixel discontinuity in the gradient math itself: all reachable positions of `ColorPalette::GetGradient` (C++ and JS byte-identical), all strip sizes 2–255 for `DisplayColorPaletteEffect`, all 256 reachable time offsets × 255 LEDs for `RainbowEffect`, palettes 8/18/21, Bright on/off. The hue/index/segment-boundary arithmetic is correct and continuous (the only single-pixel extremes found were the *expected* exact-palette-color peaks in 2-color palettes).
- Why the report says "only on strips bigger than 50": small devices in `Devices.hpp` are mostly `Tiny`-flagged (whole strip renders one gradient color — no spatial gradient, so no adjacent-LED contrast). It is not a size-dependent code path.

**Alternatives considered**: off-by-one in `GetGradient` segment math (ruled out by sweep); fake-vs-real FastLED divergence (ruled out by diff — differences exist only in saturation-scaling paths, not hue bands); sim-only port bug (ruled out — sim output byte-matches firmware, both show the artifact).

## R2. Fix approach

**Decision**: Flatten gradient power at the HSV→RGB conversion inside the effects, via a shared protected helper `Effect::FlattenedGradientRGB(const CHSV&)`, rather than changing `GetGradient`, the FastLED forks, or the render loop.

**Rationale**:

- `GetGradient` cannot do it: effects overwrite `.v` *after* calling it (e.g. `RainbowEffect` sets `color.v = v`), so any value compensation there is destroyed. The flattening must happen at the final CHSV→CRGB conversion.
- `LedManager::RunEffect` cannot do it: effects return already-converted `CRGB`, and at that layer a deliberately bright solid-yellow palette is indistinguishable from a boosted gradient pixel.
- Patching `hsv2rgb_rainbow` itself would require coordinated changes to two external pinned forks (candykingdom/FastLED for hardware, ademuri/FakeFastLED for host/STM32) plus the JS port, and would change the look of *every* effect and solid palette. Out of scope.
- The prototype validated during diagnosis (same math as the contract):

  ```cpp
  CRGB Effect::FlattenedGradientRGB(const CHSV &color) const {
    CRGB rgb;
    hsv2rgb_rainbow(color, rgb);
    const uint16_t sum = rgb.r + rgb.g + rgb.b;
    // Reference drive: what an un-boosted hue (red) outputs at this
    // saturation and value.
    CRGB reference;
    hsv2rgb_rainbow(CHSV(HUE_RED, color.s, color.v), reference);
    const uint16_t reference_sum = reference.r + reference.g + reference.b;
    if (sum > reference_sum) {
      rgb.r = (uint32_t)rgb.r * reference_sum / sum;
      rgb.g = (uint32_t)rgb.g * reference_sum / sum;
      rgb.b = (uint32_t)rgb.b * reference_sum / sum;
    }
    return rgb;
  }
  ```

  Pixel-verified result: the artifact LED becomes (32,32,0) = drive 64, flat with the rest of the cycle; hue progression through orange/yellow preserved; all other pixels change by ≤1 count (some non-boosted sections sit at 256 vs red's 255, so they scale by 255/256 — visually invisible, but it is why the corpus must be regenerated rather than expecting a no-op outside the yellow band).
- Probing the reference at the *same* `s`/`v` makes the helper self-consistent for desaturated/dim palettes: at `s=0` both conversions yield the same grey (no-op); the `sum > reference_sum` guard makes `v=0` (black) safe with no divide-by-zero (0 > 0 is false).
- Integer truncating division (`uint32_t` math in C++, `Math.trunc` in JS) keeps the JS mirror byte-exact. Do **not** use FastLED `nscale8`/`scale8` here — their `>>8` semantics differ from true `/sum` scaling and are harder to mirror.

**Alternatives considered**: analytic per-hue compensation table (duplicates FastLED constants, drifts if the fork changes); `hsv2rgb_spectrum` (different hue geometry — palette hue constants like `HUE_GREEN=96` are rainbow-space, all colors would shift); scaling only the boosted band with a lookup (same as analytic; probe is simpler and self-consistent).

## R3. Scope of application

**Decision**: Apply in exactly four effects — `RainbowEffect` (both varying-palette branches), `ColorCycleEffect` (multi-color gradient branch), `RainbowBumpsEffect`, `DisplayColorPaletteEffect` — and nowhere else.

**Rationale**: These are the "palette showcase" effects that render smooth gradients where a power spike reads as a defect (spatially, or temporally for Color Cycle — the whole strip pulses brighter passing yellow). Noise/texture effects (Fire, Lightning, Rorschach, Firefly, Spark, SimpleBlink, ContrastBumps) are irregular by design and the boost arguably improves them (fire's bright yellow sparkles); `SwingingLights` uses exact palette colors via `GetColor` plus deliberate additive overlap (`qadd8` — red+green pulses crossing to yellow is a feature). Solid-color branches keep FastLED's solid-color boost. Broadening later is one line per effect.

## R4. Regression-test property

**Decision**: Pin "uniform gradient drive" semantically on both sides, in addition to the corpus bytes.

- **Firmware** (`test/GradientPowerTest.cpp`, auto-globs into `smalltests`): render `RainbowEffect` with palette 8 on a 60-LED non-Tiny strip (direct `GetRGB` calls, like `EffectsTest`), assert every LED's `r+g+b` ≤ max drive over the palette's own stops at the same brightness setting, + small rounding tolerance. Also cover `DisplayColorPaletteEffect` on 100 LEDs. Pre-fix this fails (87 > 65+tolerance); post-fix passes (≤66).
- **Sim** (`sim/test/cases/gradientPower.test.mjs`, using the existing `harness.js` `test`/`assert` API and `makeStrip` from `sim/js/devices.js`): same property against the JS ports.

**Rationale**: the corpus catches any byte change but a future regen could silently re-absorb a regression; the semantic test names the property. Threshold: endpoint drive + 5% (SC-001) — comfortably between post-fix rounding noise (≤2 counts) and the pre-fix spike (+34%).

## R5. Corpus regeneration workflow (traps)

**Decision**: Regenerate with the established workflow, same commit as the code change.

```bash
cd build && cmake .. -DBUILD_SIMULATOR=false && make vectorgen && \
  ./vectorgen > ../sim/test/vectors/reference.json
```

- The case model lives in `test/VectorGenCommon.{hpp,cpp}` shared by generator and `ReferenceVectorTest` — do not touch it; only the emitted bytes change.
- Determinism traps are already encoded in `test/VectorGen.cpp` (PRNG resets before `LedManager` construction; Firefly offset 423 = `meta.effectSeeds` = `DEFAULT_FIREFLY_OFFSET` in `sim/js/effects/registry.js`; Fire/Rorschach seeds are FastLED-LCG draws 1–2 from seed 1337). Nothing to re-derive — just run the target and commit the JSON.
- Expected corpus diff shape (review check): only cases for the four affected effects change; `meta.firmwareGitDescribe` changes; everything else byte-identical (SC-005).

## Invariants (from CLAUDE.md, restated as gates for this change)

- `DisplayColorPaletteEffect` and `DarkEffect` stay the last two registered effects; registry order/weights untouched (no registry edits in this change at all).
- Effect/palette indices remain single wire bytes; no radio-visible change.
- `EffectsTest` fuzz (every palette, 0–255 LEDs, multi-strip Tiny/Circular) must pass — the helper must be UB-free (host build has fatal UBSan; the prototype ran clean under ASan+UBSan).
- `sim/js/effects/registry.js` must continue to match `LedManager.cpp` registration — untouched.
- `ReferenceVectorTest` (host) and the sim suite gate the regenerated corpus — both must pass in the same commit.
- `DEBUG` macro in `lib/debug/Debug.hpp` stays commented out.
