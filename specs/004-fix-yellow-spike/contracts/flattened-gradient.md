# Contract: FlattenedGradientRGB (production C++ and browser Wasm)

`Effect::FlattenedGradientRGB` below is the single implementation. Firmware, host tests, and the browser simulator all compile this production C++ method. The corpus (`sim/test/vectors/reference.json`) and `GradientPowerTest` still pin its behavior, but there is no JavaScript implementation to keep in sync.

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

## Production call sites

| Effect | Site | C++ change |
|---|---|---|
| RainbowEffect | varying-palette Tiny branch | `return color;` → `return FlattenedGradientRGB(color);` |
| RainbowEffect | varying-palette normal branch | same |
| ColorCycleEffect | multi-color gradient branch (after the `Bright` v-halving) | same |
| RainbowBumpsEffect | final return (after `color.v = GetThresholdSin(...)`) | same |
| DisplayColorPaletteEffect | final return (after the `Bright` v-halving) | same |

Solid-color branches (`palette.Size() < 2`) in RainbowEffect/ColorCycleEffect are **not** changed. The Emscripten target compiles these same effect source files, so browser behavior changes only when the C++ changes and the committed artifact is rebuilt.

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

Pre-fix: fails (87 > 65×1.05 ≈ 68). Post-fix: passes (max 66). `test/GradientPowerTest.cpp` exercises direct production `GetRGB` calls. `sim/test/cases/gradientPower.test.mjs` renders custom strips through `sim/js/renderer.js`, which calls the production C++ Wasm ABI; it is an independent execution surface, not a second algorithm.
