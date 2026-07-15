// Port of lib/effect/RainbowBumpsEffect.{hpp,cpp} (uses lib/math/Math.hpp
// MirrorIndex).

import { getPalette, paletteGetGradient, getThresholdSin } from '../palette.js';
import { flattenedGradientRGB } from '../fastled.js';
import { mirrorIndex } from '../perlin.js';

export function makeRainbowBumpsEffect() {
  return {
    name: 'Rainbow Bumps',
    getRGB(ledIndex, timeMs, strip, show) {
      const palette = getPalette(show.paletteIndex & 0xff);

      let ledIndexM = ledIndex;
      let ledCountM = strip.ledCount & 0xff;
      if (strip.hasFlag('Mirrored')) {
        [ledIndexM, ledCountM] = mirrorIndex(ledIndexM, ledCountM);
      }

      // NB: C++ divides by led_count here with no zero-check (UB if
      // led_count is 0); the engine never calls getRGB on a 0-LED strip, so
      // we replicate the bare division rather than adding a guard C++ lacks.
      let offset;
      if (strip.hasFlag('Circular')) {
        offset = Math.trunc((ledIndexM * 255) / ledCountM) & 0xff;
      } else {
        offset = (ledIndexM * 8) & 0xff;
      }

      const gradientPos = ((Math.floor(timeMs / 10) + offset) << 8) & 0xffff;
      const color = { ...paletteGetGradient(palette, gradientPos) };

      const t16 = Math.floor(timeMs / 16);
      const diff = (t16 - offset * 3) >>> 0;
      const sinArg = (-diff << 8) >>> 0;
      color.v = getThresholdSin(sinArg, 0);

      return flattenedGradientRGB(color);
    },
  };
}
