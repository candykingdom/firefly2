import { test, assert } from '../harness.js';
import { makeStrip } from '../../js/devices.js';
import {
  makeDisplayColorPaletteEffect,
} from '../../js/effects/displayColorPalette.js';
import { makeRainbowEffect } from '../../js/effects/rainbow.js';
import { hsv2rgbRainbow } from '../../js/fastled.js';

function drive(rgb) {
  return rgb.r + rgb.g + rgb.b;
}

function endpointMax(value) {
  return Math.max(
    drive(hsv2rgbRainbow({ h: 0, s: 255, v: value })),
    drive(hsv2rgbRainbow({ h: 96, s: 255, v: value })),
    drive(hsv2rgbRainbow({ h: 160, s: 255, v: value })),
  );
}

function assertUniformDrive(effect, strip, show, timeMs, value) {
  let worst = { led: 0, rgb: { r: 0, g: 0, b: 0 }, drive: 0 };
  for (let led = 0; led < strip.ledCount; ++led) {
    const rgb = effect.getRGB(led, timeMs, strip, show);
    const ledDrive = drive(rgb);
    if (ledDrive > worst.drive) {
      worst = { led, rgb, drive: ledDrive };
    }
  }

  const allowed = Math.trunc((endpointMax(value) * 105) / 100);
  assert(
    worst.drive <= allowed,
    `${effect.name} at time ${timeMs}, LED ${worst.led} RGB ` +
      `(${worst.rgb.r},${worst.rgb.g},${worst.rgb.b}) drive ` +
      `${worst.drive} exceeds allowed ${allowed}`,
  );
}

test('Rainbow gradient drive stays within palette endpoints', () => {
  const effect = makeRainbowEffect();
  const show = { paletteIndex: 8 };
  const normalStrip = makeStrip(60, []);
  const brightStrip = makeStrip(60, ['Bright']);

  for (const timeMs of [0, 1000, 5000]) {
    assertUniformDrive(effect, normalStrip, show, timeMs, 128);
    assertUniformDrive(effect, brightStrip, show, timeMs, 255);
  }
});

test('Display Color Palette drive stays within palette endpoints', () => {
  const effect = makeDisplayColorPaletteEffect();
  const show = { paletteIndex: 8 };
  const strip = makeStrip(100, []);

  assertUniformDrive(effect, strip, show, 0, 127);
});
