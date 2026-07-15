// Port of lib/effect/FireflyEffect.{hpp,cpp}.

import { getPalette, paletteGetGradient } from '../palette.js';
import { sin16, hsv2rgbRainbow } from '../fastled.js';

// The time it takes for the lights to go from randomly distributed to
// in-sync.
const PERIOD_MS = 20000; // kPeriodMs

// Smaller number means longer blinks.
const SIN_MULTIPLIER = 64; // kSinMultiplier

// kBlinkPeriod = (1 << 16) / kSinMultiplier
export const FIREFLY_BLINK_PERIOD = (1 << 16) / SIN_MULTIPLIER;

// Truncate a uint32 value to the int16 that FastLED's sin16 accepts (C's
// implicit conversion to int16_t: truncate to 16 bits, then sign-extend).
function toInt16(x) {
  return ((x & 0xffff) << 16) >> 16;
}

// offset is the constructor's `offset_ = random(0, kBlinkPeriod / 2)`,
// supplied by the caller instead of generated here.
export function makeFireflyEffect(offset) {
  return {
    name: 'Firefly',
    getRGB(ledIndex, timeMs, strip, show) {
      let effectiveOffset = offset;
      if (strip.hasFlag('Controller')) {
        // For the controller, blink the lights mostly in sync
        if (ledIndex % 2 === 1) {
          return { r: 0, g: 0, b: 0 };
        }

        effectiveOffset =
          (((FIREFLY_BLINK_PERIOD + 1234) << ledIndex) >>> 0) %
          (FIREFLY_BLINK_PERIOD / 2);
      }

      const phase = Math.trunc(timeMs / PERIOD_MS) % 3;

      let adjustedTime = 0;
      const periodStart = (Math.trunc(timeMs / PERIOD_MS) * PERIOD_MS) >>> 0;
      const elapsedInPeriod = (timeMs - periodStart) >>> 0;
      const remainingInPeriod = (PERIOD_MS - elapsedInPeriod) >>> 0;
      if (phase === 0) {
        // Out of sync -> in sync
        if (effectiveOffset < FIREFLY_BLINK_PERIOD / 4) {
          adjustedTime =
            (timeMs -
              Math.trunc(
                (effectiveOffset * remainingInPeriod) / PERIOD_MS,
              )) >>>
            0;
        } else {
          adjustedTime =
            (timeMs +
              Math.trunc(
                (effectiveOffset * remainingInPeriod) / PERIOD_MS,
              )) >>>
            0;
        }
      } else if (phase === 1) {
        // In sync
        adjustedTime = timeMs >>> 0;
      } else {
        // In sync -> out of sync
        if (effectiveOffset < FIREFLY_BLINK_PERIOD / 4) {
          adjustedTime =
            (timeMs -
              Math.trunc(
                (effectiveOffset * elapsedInPeriod) / PERIOD_MS,
              )) >>>
            0;
        } else {
          adjustedTime =
            (timeMs +
              Math.trunc(
                (effectiveOffset * elapsedInPeriod) / PERIOD_MS,
              )) >>>
            0;
        }
      }

      let curve = sin16(toInt16((adjustedTime * SIN_MULTIPLIER) >>> 0));
      if (curve < 0) {
        curve = 0;
      }
      const palette = getPalette(show.paletteIndex);
      const gradientPos =
        ((Math.trunc(timeMs / FIREFLY_BLINK_PERIOD) << 8) >>> 0) & 0xffff;
      // paletteGetGradient may return a direct reference into the shared
      // palette table (e.g. a solid color or an exact stop) -- copy before
      // mutating so we don't corrupt shared state, matching the C++
      // return-by-value CHSV semantics.
      const color = { ...paletteGetGradient(palette, gradientPos) };
      color.v = Math.trunc(curve / 256) & 0xff;
      return hsv2rgbRainbow(color);
    },
  };
}
