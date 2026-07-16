import { test, assert } from '../harness.js';
import renderer, { DEFAULT_SEEDS } from '../../js/renderer.js';

function drive(bytes, offset) {
  return bytes[offset] + bytes[offset + 1] + bytes[offset + 2];
}

function assertUniformDrive(effectName, effectIndex, strip, timeMs, value) {
  const bytes = renderer.renderStrip(
    DEFAULT_SEEDS,
    strip,
    { effectIndex, paletteIndex: 8, controlRgb: null },
    timeMs,
  );
  const endpointDrive = value === 255 ? 255 : value === 128 ? 65 : 64;
  const allowed = Math.trunc((endpointDrive * 105) / 100);
  let worst = { led: 0, drive: 0, rgb: [0, 0, 0] };
  for (let offset = 0; offset < bytes.length; offset += 3) {
    const ledDrive = drive(bytes, offset);
    if (ledDrive > worst.drive) {
      worst = {
        led: offset / 3,
        drive: ledDrive,
        rgb: [...bytes.slice(offset, offset + 3)],
      };
    }
  }
  assert(
    worst.drive <= allowed,
    `${effectName} at time ${timeMs}, LED ${worst.led} RGB ` +
      `(${worst.rgb.join(',')}) drive ${worst.drive} exceeds ${allowed}`,
  );
}

test('Rainbow gradient drive stays within palette endpoints', () => {
  for (const timeMs of [0, 1000, 5000]) {
    assertUniformDrive('Rainbow', 13, { ledCount: 60, flags: [] },
      timeMs, 128);
    assertUniformDrive('Rainbow', 13,
      { ledCount: 60, flags: ['Bright'] }, timeMs, 255);
  }
});

test('Display Color Palette drive stays within palette endpoints', () => {
  assertUniformDrive('Display Color Palette', 33,
    { ledCount: 100, flags: [] }, 0, 127);
});
