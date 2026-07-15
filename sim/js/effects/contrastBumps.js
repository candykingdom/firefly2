// Port of lib/effect/ContrastBumpsEffect.{hpp,cpp} (uses lib/math/Math.hpp
// MirrorIndex).

import { getPalette, paletteGetGradient, getThresholdSin } from '../palette.js';
import { qadd8, hsv2rgbRainbow } from '../fastled.js';
import { mirrorIndex } from '../perlin.js';

export function makeContrastBumpsEffect() {
  return {
    name: 'Contrast Bumps',
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
        offset = (ledIndexM * 24) & 0xff;
      }

      const t16 = Math.floor(timeMs / 16);
      const sum = (t16 + offset) >>> 0;
      const sinArg = (-sum << 8) >>> 0;
      const sin = getThresholdSin(sinArg, 0);

      const colorIndex = ((timeMs * 16) >>> 0) & 0xffff;

      if (sin === 0) {
        const color = { ...paletteGetGradient(palette, colorIndex) };
        color.v = 64;
        return hsv2rgbRainbow(color);
      } else {
        const color = {
          ...paletteGetGradient(palette, (colorIndex + 0x4000) & 0xffff),
        };
        color.v = qadd8(Math.trunc(sin / 2), 96);
        return hsv2rgbRainbow(color);
      }
    },
  };
}
