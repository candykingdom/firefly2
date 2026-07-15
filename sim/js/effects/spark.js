// Port of lib/effect/SparkEffect.{hpp,cpp}.
//
// Makes a bright light trace back and forth (or, on Circular strips, around),
// with a tail.

import { getPalette, paletteGetGradient } from '../palette.js';
import { hsv2rgbRainbow } from '../fastled.js';
import { mirrorIndex } from '../perlin.js';

// SparkEffect::brightnesses (lib/effect/SparkEffect.hpp) — constructor state,
// but it's a plain constant table so a module-level const replicates it
// faithfully.
const BRIGHTNESSES = [255, 255, 128, 96, 48, 32, 24, 16];

export function makeSparkEffect() {
  return {
    name: 'Spark',

    getRGB(ledIndex, timeMs, strip, show) {
      const pulseSize = BRIGHTNESSES.length;
      const paletteIndex = show.paletteIndex;
      const palette = getPalette(paletteIndex);

      let ledCount = strip.ledCount;
      if (strip.hasFlag('Mirrored')) {
        [ledIndex, ledCount] = mirrorIndex(ledIndex, ledCount);
      }

      let relativePos;
      let reverse = false;
      if (strip.hasFlag('Circular')) {
        // relative_pos = (time_ms * led_count / 3000 + led_index) % led_count;
        const product = (timeMs * ledCount) >>> 0;
        const quotient = Math.trunc(product / 3000);
        const sum = (quotient + ledIndex) >>> 0;
        relativePos = sum % ledCount;
      } else {
        // int16_t pos = ((time_ms * (led_count + pulse_size)) / 3000) %
        //               ((led_count + pulse_size) * 2);
        const width = ledCount + pulseSize;
        const product = (timeMs * width) >>> 0;
        const quotient = Math.trunc(product / 3000);
        let pos = quotient % (width * 2);

        if (pos > width) {
          reverse = true;
          pos = width - (pos - width);
        }
        relativePos = pos - ledIndex;
      }

      // CHSV hsv = palette.GetGradient(
      //     (time_ms / 24 + relative_pos * 14) << 8);
      const timeTerm = Math.trunc(timeMs / 24);
      const posTerm = relativePos * 14;
      const sum = (timeTerm + posTerm) >>> 0;
      const shifted = (sum << 8) >>> 0;
      const position = shifted & 0xffff;
      const hsv = paletteGetGradient(palette, position);

      if (relativePos < 0) {
        hsv.v = 0;
      } else if (relativePos < ledCount + pulseSize) {
        if (relativePos < pulseSize) {
          if (reverse) {
            hsv.v = BRIGHTNESSES[pulseSize - relativePos - 1];
          } else {
            hsv.v = BRIGHTNESSES[relativePos];
          }
        } else {
          hsv.v = 0;
        }
      } else {
        hsv.v = 0;
      }

      return hsv2rgbRainbow(hsv);
    },
  };
}
