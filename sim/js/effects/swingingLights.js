// Port of lib/effect/SwingingLights.{hpp,cpp}.

import { getPalette, paletteGetColor } from '../palette.js';
import { sin16, qadd8, hsv2rgbRainbow, MAX_UINT16 } from '../fastled.js';
import { mirrorIndex } from '../perlin.js';

const PERIOD_MS = 5000; // kPeriod: 5 second period.
const SPREAD = Math.trunc(MAX_UINT16 * 0.2);
const HALF_MAX_UINT16 = Math.trunc(MAX_UINT16 / 2);

// Add a CHSV value to a CRGB in place (addInPlace in C++).
function addInPlace(value, result) {
  const color = hsv2rgbRainbow(value);
  result.r = qadd8(result.r, color.r);
  result.g = qadd8(result.g, color.g);
  result.b = qadd8(result.b, color.b);
}

export function makeSwingingLightsEffect() {
  return {
    name: 'Swinging Lights',
    getRGB(ledIndex, timeMs, strip, show) {
      // This effect looks bad on small devices. Instead of creating another
      // effect we can just make the LEDs flash when a light pulse hits the
      // end of a "long" strip which looks pretty cool.
      let numLeds = strip.ledCount;
      if (strip.hasFlag('Tiny') && !strip.hasFlag('Circular')) {
        ledIndex = 0;
        numLeds = 50;
      } else if (strip.hasFlag('Mirrored')) {
        [ledIndex, numLeds] = mirrorIndex(ledIndex, numLeds);
      }

      // Map [0, period) to [0, MAX_UINT16)
      const angle =
        Math.trunc(((timeMs % PERIOD_MS) * MAX_UINT16) / PERIOD_MS) & 0xffff;

      // Map [0, num_leds) to [0, MAX_UINT16)
      const ledPos = Math.trunc((ledIndex * MAX_UINT16) / numLeds) & 0xffff;

      const palette = getPalette(show.paletteIndex);

      const color = { r: 0, g: 0, b: 0 };

      for (let i = 0; i < palette.colors.length; ++i) {
        const lightOffset =
          Math.trunc((i * MAX_UINT16) / palette.colors.length) & 0xffff;
        const sinArg = ((lightOffset + angle) << 16) >> 16;
        const lightPos = (sin16(sinArg) + HALF_MAX_UINT16) & 0xffff;

        const dist = Math.abs(lightPos - ledPos) - SPREAD;

        if (dist < 0) {
          const modifier = { ...paletteGetColor(palette, i) };
          modifier.v = Math.trunc((-dist * modifier.v) / SPREAD) & 0xff;
          addInPlace(modifier, color);
        }
      }

      return color;
    },
  };
}
