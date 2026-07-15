// Port of lib/effect/LightningEffect.{hpp,cpp}.
//
// Blinks groups of LEDs in a vaguely lightning-light pattern.

import { getPalette, paletteGetGradient } from '../palette.js';
import { hsv2rgbRainbow } from '../fastled.js';
import { perlinNoise } from '../perlin.js';

export function makeLightningEffect() {
  return {
    name: 'Lightning',

    getRGB(ledIndex, timeMs, strip, show) {
      // UNUSED(led_index) in the C++ only suppresses the "declared but only
      // used in one branch" style of warning; led_index is still used below.
      // UNUSED(strip) — strip flags are not consulted by this effect.
      const paletteIndex = show.paletteIndex;
      const palette = getPalette(paletteIndex);

      // uint8_t noise = perlinNoise(led_index << 6, time_ms / 6);
      const x = (ledIndex << 6) >>> 0;
      const y = Math.trunc(timeMs / 6);
      const noise = perlinNoise(x, y);

      // CHSV color = palette.GetGradient(((noise - 160) * 512) + time_ms);
      const noiseTerm = (noise - 160) * 512;
      const sum = (noiseTerm + timeMs) >>> 0;
      const position = sum & 0xffff;
      const color = paletteGetGradient(palette, position);

      if (noise > 160) {
        color.v = noise;
      } else {
        color.v = 0;
      }

      return hsv2rgbRainbow(color);
    },
  };
}
