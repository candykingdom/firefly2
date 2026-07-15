// Port of lib/effect/ColorCycleEffect.{hpp,cpp}.

import { getPalette, paletteGetColor, paletteGetGradient } from '../palette.js';
import {
  cubicwave8,
  flattenedGradientRGB,
  hsv2rgbRainbow,
} from '../fastled.js';

export function makeColorCycleEffect() {
  return {
    name: 'Color Cycle',
    getRGB(ledIndex, timeMs, strip, show) {
      const palette = getPalette(show.paletteIndex & 0xff);
      const t16 = Math.floor(timeMs / 16);

      // Check for whether the entire palette is the same color - if so,
      // change the brightness rather than the hue.
      if (palette.colors.length < 2) {
        // Solid color palette
        const color = { ...paletteGetColor(palette, 0) };
        const cw = cubicwave8(t16 & 0xff);
        if (strip.hasFlag('Bright')) {
          color.v = cw & 0xff;
        } else {
          color.v = Math.trunc((cw * 2) / 3) & 0xff;
        }
        return hsv2rgbRainbow(color);
      } else {
        const color = { ...paletteGetGradient(palette, (t16 << 8) & 0xffff) };
        if (!strip.hasFlag('Bright')) {
          color.v = Math.trunc(color.v / 2) & 0xff;
        }
        return flattenedGradientRGB(color);
      }
    },
  };
}
