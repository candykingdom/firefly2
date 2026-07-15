// Port of lib/effect/PrideEffect.{hpp,cpp}.

import { paletteGetColor } from '../palette.js';
import { hsv2rgbRainbow, blendHsvShortestHues } from '../fastled.js';

// Integer division truncating toward zero, matching the C++ compile-time
// `deg * 255 / 360` (etc.) expressions used to build the private pride
// palette below.
function t(a, b, c) {
  return Math.trunc((a * b) / c);
}

function chsv(h, s, v) {
  return { h, s, v };
}

// PrideEffect::palette (lib/effect/PrideEffect.hpp) — constructor state, but
// it's an immutable constant table so a module-level const replicates it
// faithfully.
const PRIDE_PALETTE = {
  colors: [
    chsv(t(0, 255, 360), t(100, 255, 100), t(100, 255, 100)),
    chsv(t(34, 255, 360), t(100, 255, 100), t(99, 255, 100)),
    chsv(t(54, 255, 360), t(100, 255, 100), t(100, 255, 100)),
    chsv(t(118, 255, 360), t(93, 255, 100), t(63, 255, 100)),
    chsv(t(218, 255, 360), t(97, 255, 100), t(69, 255, 100)),
    chsv(t(292, 255, 360), t(79, 255, 100), t(86, 255, 100)),
  ],
};

// The minimum number of steps of "temporal resolution" per pixel. Is
// multiplied by the strip LED count to ensure the result is a round number.
const DEPTH_MULTIPLIER = 8;

// The reciprocal of the amount of fade per stripe width.
const FADE_FRACT = 8;

export function makePrideEffect() {
  return {
    name: 'Pride',

    getRGB(ledIndex, timeMs, strip) {
      const paletteSize = PRIDE_PALETTE.colors.length;
      const stripeWidth = strip.ledCount * DEPTH_MULTIPLIER;

      let fadeWidth = Math.trunc(stripeWidth / FADE_FRACT);
      if (strip.hasFlag('Tiny')) {
        ledIndex = 0;
        timeMs = (timeMs * 2) >>> 0;
        fadeWidth = Math.trunc(fadeWidth / 2);
      }

      // const uint16_t v_index =
      //     ((led_index * depth_multiplier * palette.Size()) + (time_ms / 8)) %
      //     (stripe_width * palette.Size());
      const smallPart = ledIndex * DEPTH_MULTIPLIER * paletteSize;
      const timeDiv8 = Math.trunc(timeMs / 8);
      const sum = (smallPart + timeDiv8) >>> 0;
      const divisor = stripeWidth * paletteSize;
      const vIndex = sum % divisor;

      const colorIndex = Math.trunc(vIndex / stripeWidth) % paletteSize;
      const colorAmount = vIndex % stripeWidth;

      if (colorAmount < fadeWidth) {
        // palette.GetColor(color_index - 1) — color_index is uint8_t, so
        // `color_index - 1` truncates to uint8_t (wrapping 0 -> 255) before
        // GetColor's internal `% colors.size()`.
        const prevIndex = (colorIndex - 1) & 0xff;
        const amount = Math.trunc((colorAmount * 255) / fadeWidth) & 0xff;
        return hsv2rgbRainbow(
          blendHsvShortestHues(
            paletteGetColor(PRIDE_PALETTE, prevIndex),
            paletteGetColor(PRIDE_PALETTE, colorIndex),
            amount,
          ),
        );
      } else {
        return hsv2rgbRainbow(paletteGetColor(PRIDE_PALETTE, colorIndex));
      }
    },
  };
}
