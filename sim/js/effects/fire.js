// Port of lib/effect/FireEffect.{hpp,cpp}, plus the GetPosOnCircle half of
// lib/math/Math.{hpp,cpp} (MirrorIndex itself lives in ../perlin.js).

import { hsv2rgbRainbow } from '../fastled.js';
import { paletteGetGradient } from '../palette.js';
import { perlinNoise, perlinNoisePolar, mirrorIndex } from '../perlin.js';

// Fire color palette (FireEffect.hpp: {h, s, v} triples).
const FIRE_PALETTE = {
  colors: [
    { h: 0, s: 255, v: 8 },
    { h: 23, s: 249, v: 45 },
    { h: 30, s: 246, v: 113 },
    { h: 35, s: 200, v: 192 },
  ],
};

// Port of GetPosOnCircle (lib/math/Math.cpp).
function getPosOnCircle(ledCount, ledIndex) {
  const circum = (ledCount * 32) >>> 0;
  const angle = Math.trunc((ledIndex * 255) / ledCount) & 0xff;
  const radius = Math.trunc((circum * 1000) / 6283) & 0xff;
  return { angle, radius };
}

export function makeFireEffect(offset) {
  return {
    name: 'Fire',
    getRGB(ledIndex, timeMs, strip, show) {
      void show;
      let sideDifferentiator = 0;
      let ledCount = strip.ledCount & 0xff;
      let ledIdx = ledIndex & 0xff;

      if (strip.hasFlag('Mirrored')) {
        if (ledIdx > Math.trunc(ledCount / 2)) {
          sideDifferentiator = 6789;
        }
        [ledIdx, ledCount] = mirrorIndex(ledIdx, ledCount);
      }

      const wrappedTime = (timeMs + offset) >>> 0;

      let noise;
      if (strip.hasFlag('Circular')) {
        const { angle, radius } = getPosOnCircle(ledCount, ledIdx);

        // Create 2 octave Perlin noise by averaging multiple samples.
        noise =
          (Math.trunc(
            perlinNoisePolar(
              Math.trunc(wrappedTime / 8),
              sideDifferentiator,
              angle,
              radius,
            ) / 4,
          ) *
            3 +
            Math.trunc(
              perlinNoisePolar(
                (Math.trunc(wrappedTime / 2) + 1234567) >>> 0,
                0,
                angle,
                radius,
              ) / 4,
            ) *
              1) &
          0xff;
      } else {
        // Create 2 octave Perlin noise by averaging multiple samples.
        noise =
          (Math.trunc(
            perlinNoise(ledIdx * 20, Math.trunc(wrappedTime / 8)) / 4,
          ) *
            3 +
            Math.trunc(
              perlinNoise(
                ledIdx * 10 + 1234,
                (Math.trunc(wrappedTime / 2) + 1234567) >>> 0,
              ) / 4,
            ) *
              1) &
          0xff;
      }

      const color = paletteGetGradient(
        FIRE_PALETTE,
        (noise << 8) & 0xffff,
        false,
      );
      return hsv2rgbRainbow(color);
    },
  };
}
