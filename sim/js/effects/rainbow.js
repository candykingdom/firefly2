// Port of lib/effect/RainbowEffect.{hpp,cpp}.

import { getPalette, paletteGetColor, paletteGetGradient } from '../palette.js';
import {
  cubicwave8,
  flattenedGradientRGB,
  hsv2rgbRainbow,
} from '../fastled.js';

export function makeRainbowEffect() {
  return {
    name: 'Rainbow',
    getRGB(ledIndex, timeMs, strip, show) {
      const v = strip.hasFlag('Bright') ? 255 : 128;

      const palette = getPalette(show.paletteIndex & 0xff);
      const t16 = Math.floor(timeMs / 16);

      // Check for whether the entire palette is the same color - if so,
      // change the brightness rather than the hue.
      if (palette.colors.length < 2) {
        // Solid color palette
        if (strip.hasFlag('Tiny')) {
          const cw = cubicwave8(t16 & 0xff);
          return hsv2rgbRainbow(
            paletteGetGradient(palette, (cw << 8) & 0xffff),
          );
        } else {
          const color = { ...paletteGetColor(palette, 0) };
          const cw = cubicwave8((t16 + ledIndex * 8) & 0xff);
          if (strip.hasFlag('Bright')) {
            color.v = cw & 0xff;
          } else {
            color.v = Math.trunc((cw * 2) / 3) & 0xff;
          }
          return hsv2rgbRainbow(color);
        }
      } else {
        // Varying color palette
        if (strip.hasFlag('Tiny')) {
          const color = { ...paletteGetGradient(palette, (t16 << 8) & 0xffff) };
          color.v = v & 0xff;
          return flattenedGradientRGB(color);
        } else {
          const color = {
            ...paletteGetGradient(
              palette,
              ((t16 + ledIndex * 8) << 8) & 0xffff,
            ),
          };
          color.v = v & 0xff;
          return flattenedGradientRGB(color);
        }
      }
    },
  };
}
