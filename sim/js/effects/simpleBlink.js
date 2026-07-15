// Port of lib/effect/SimpleBlinkEffect.{hpp,cpp}.

import { hsv2rgbRainbow } from '../fastled.js';
import { getPalette, paletteGetGradient } from '../palette.js';

export function makeSimpleBlinkEffect(speed) {
  return {
    name: 'Simple Blink',
    getRGB(ledIndex, timeMs, strip, show) {
      const chunk = Math.trunc(timeMs / speed) % 3;
      if (chunk === 0) {
        const palette = getPalette(show.paletteIndex);
        const position = (Math.trunc(timeMs / 4) * 23) & 0xffff;
        const color = { ...paletteGetGradient(palette, position) };
        if (!strip.hasFlag('Bright')) {
          color.v = Math.trunc(color.v / 2);
        }
        return hsv2rgbRainbow(color);
      } else {
        return { r: 0, g: 0, b: 0 };
      }
    },
  };
}
