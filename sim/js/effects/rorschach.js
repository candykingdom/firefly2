// Port of lib/effect/RorschachEffect.{hpp,cpp}.

import { hsv2rgbRainbow, MAX_UINT8 } from '../fastled.js';
import { getPalette, paletteGetColor, paletteGetGradient } from '../palette.js';
import { perlinNoise } from '../perlin.js';

export function makeRorschachEffect(offset) {
  return {
    name: 'Rorschach',
    getRGB(ledIndex, timeMs, strip, show) {
      const palette = getPalette(show.paletteIndex);

      // LEDs at the center of the strip have a lower position.
      const ledPos =
        -Math.abs((ledIndex & 0xff) - ((strip.ledCount & 0xff) >> 1)) & 0xffff;

      const wrappedTime = (timeMs + offset) >>> 0;
      const noise = perlinNoise(
        ((ledPos << 5) + (wrappedTime >>> 3)) >>> 0,
        ((wrappedTime >>> 3) + (ledPos << 2)) >>> 0,
      ); // Skew coordinates to avoid moire patterns.

      // If the palette is only one color, change the value instead of the
      // hue.
      if (palette.colors.length < 2) {
        const v = noise > MAX_UINT8 ? MAX_UINT8 : noise;
        const color = { ...paletteGetColor(palette, 0) };
        color.v = v & 0xff;
        return hsv2rgbRainbow(color);
      } else {
        const color = paletteGetGradient(palette, (noise << 8) & 0xffff, false);
        if (!strip.hasFlag('Bright')) {
          color.v = Math.trunc(color.v / 2);
        }
        return hsv2rgbRainbow(color);
      }
    },
  };
}
