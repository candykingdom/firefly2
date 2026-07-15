// Port of lib/effect/DisplayColorPaletteEffect.{hpp,cpp}.

import { flattenedGradientRGB } from '../fastled.js';
import { getPalette, paletteGetGradient } from '../palette.js';

export function makeDisplayColorPaletteEffect() {
  return {
    name: 'Display Color Palette',
    getRGB(ledIndex, timeMs, strip, show) {
      const palette = getPalette(show.paletteIndex);
      let position;
      if (
        strip.ledCount < palette.colors.length &&
        palette.colors.length <= 4
      ) {
        position = (Math.trunc(timeMs / 2) * 23) & 0xffff;
      } else {
        position = Math.trunc((ledIndex * 65536) / strip.ledCount) & 0xffff;
      }
      const color = { ...paletteGetGradient(palette, position) };
      if (!strip.hasFlag('Bright')) {
        color.v = Math.trunc(color.v / 2);
      }
      return flattenedGradientRGB(color);
    },
  };
}
