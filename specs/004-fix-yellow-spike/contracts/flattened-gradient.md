# Contract: FlattenedGradientRGB (firmware ↔ simulator)

The two implementations below MUST be byte-exact for every input. The corpus (`sim/test/vectors/reference.json`) enforces this in CI from both sides; get the integer semantics right and it holds by construction.

## C++ (lib/effect/Effect.{hpp,cpp})

```cpp
// Effect.hpp — protected section, after GetThresholdSin:

/**
 * Converts an interpolated palette color to RGB with the extra power that
 * hsv2rgb_rainbow pumps into hues near yellow (hue 32-95) scaled back out,
 * so a smooth gradient drives uniform total power. Without this, a
 * gradient crossing the yellow band drives up to ~34% more power than its
 * endpoints, which reads as a lone bright yellow LED between two palette
 * colors (e.g. the rainbow palette's red->green blend). Exact palette
 * colors rendered via GetColor keep FastLED's solid-color boost.
 */
CRGB FlattenedGradientRGB(const CHSV &color) const;
```

```cpp
// Effect.cpp:
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

Notes: `hsv2rgb_rainbow` and `HUE_RED` come via `<FastLED.h>` (already included through `Types.hpp` on every platform; `SwingingLights.cpp` already calls `hsv2rgb_rainbow` directly with the same includes). The `(uint32_t)` cast is required: `rgb.r * reference_sum` can reach 255×765 = 195 075, overflowing `uint16_t`/`int16_t`.

## JavaScript (sim/js/fastled.js)

```js
// Port of Effect::FlattenedGradientRGB (lib/effect/Effect.cpp).
export function flattenedGradientRGB(color) {
  const rgb = hsv2rgbRainbow(color);
  const sum = rgb.r + rgb.g + rgb.b;
  const reference = hsv2rgbRainbow({ h: 0, s: color.s, v: color.v });
  const referenceSum = reference.r + reference.g + reference.b;
  if (sum > referenceSum) {
    rgb.r = Math.trunc((rgb.r * referenceSum) / sum);
    rgb.g = Math.trunc((rgb.g * referenceSum) / sum);
    rgb.b = Math.trunc((rgb.b * referenceSum) / sum);
  }
  return rgb;
}
```

Notes: `hsv2rgbRainbow` returns a fresh `{r,g,b}` object (safe to mutate in place). All operands are non-negative integers ≤ 195 075, exact in JS doubles; `Math.trunc(a/b)` matches C++ unsigned truncating division exactly. Do not use `| 0` shortcuts on the products (safe here, but `Math.trunc` matches the C++ reading).

## Call sites (must match 1:1 between C++ and the JS port of each effect)

| Effect | Site | C++ change |
|---|---|---|
| RainbowEffect | varying-palette Tiny branch | `return color;` → `return FlattenedGradientRGB(color);` |
| RainbowEffect | varying-palette normal branch | same |
| ColorCycleEffect | multi-color gradient branch (after the `Bright` v-halving) | same |
| RainbowBumpsEffect | final return (after `color.v = GetThresholdSin(...)`) | same |
| DisplayColorPaletteEffect | final return (after the `Bright` v-halving) | same |

Solid-color branches (`palette.Size() < 2`) in RainbowEffect/ColorCycleEffect are **not** changed. JS ports call `flattenedGradientRGB(color)` exactly where they currently call `hsv2rgbRainbow(color)` on those paths (import from `../fastled.js`).

## Behavioral guarantees (what the regression tests assert)

1. **Down-only**: output drive (r+g+b) ≤ un-flattened drive; equality outside the boosted region.
2. **Baseline-bounded**: output drive ≤ drive of `CHSV(HUE_RED, s, v)` (+0; truncation can only undershoot).
3. **Hue/saturation preserving**: channel ratios unchanged up to truncation (each channel scaled by the same factor).
4. **Safe degenerate inputs**: v=0 → black, no division by zero (guard is `sum > reference_sum`); s=0 → no-op (both conversions produce the same grey).
5. **Reference values** (rainbow palette 8, v=128, non-Tiny, t=0, `RainbowEffect`): pre-fix LED 7 of the 32-LED cycle = (44,43,0), drive 87; post-fix = (32,32,0), drive 64; all cycle LEDs post-fix in [63, 66].

## Uniform-drive regression test property

For `RainbowEffect` (palette 8, 60-LED non-Tiny strip, several times t, both Bright and not) and `DisplayColorPaletteEffect` (palette 8, 100-LED strip):

```text
for every rendered LED i:  drive(i) = r+g+b
endpoint_max = max drive over the palette's own stops rendered at the same
               brightness setting (red/green/blue at that v)
ASSERT drive(i) <= endpoint_max * 1.05
```

Pre-fix: fails (87 > 65×1.05 ≈ 68). Post-fix: passes (max 66). Implement firmware-side in `test/GradientPowerTest.cpp` (gtest, direct `GetRGB` calls — see `EffectsTest.cpp` for the driving pattern) and sim-side in `sim/test/cases/gradientPower.test.mjs` (harness: `import { test, assert } from '../harness.js'`, strip via `makeStrip(60, [])` from `sim/js/devices.js`, effect via `createRegistry()` from `sim/js/effects/registry.js` or `makeRainbowEffect()` directly, show object `{ paletteIndex: 8 }`).
