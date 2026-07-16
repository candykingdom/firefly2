import { test, assert, assertEqual, assertThrows } from '../harness.js';
import { SimEngine } from '../../js/engine.js';
import renderer, { DEFAULT_SEEDS } from '../../js/renderer.js';

function assertValidSnapshot(snapshot) {
  for (const device of snapshot.devices) {
    for (const strip of device.strips) {
      for (const led of strip.leds) {
        for (const channel of led) {
          assert(Number.isInteger(channel));
          assert(channel >= 0 && channel <= 255);
        }
      }
    }
  }
}

test('every effect byte 0-255 renders without throwing', () => {
  for (let byte = 0; byte <= 255; byte++) {
    const engine = new SimEngine({ devices: ['puck'], effect: byte,
      palette: 8, time: 12345, paused: true });
    assertValidSnapshot(engine.getSnapshot());
  }
});

test('effect byte wraps at the wire table length', () => {
  const render = (effect) => new SimEngine({ devices: ['puck'], effect,
    palette: 8, time: 12345, paused: true }).getSnapshot().devices;
  for (const index of [0, 13, 34]) {
    assert(JSON.stringify(render(35 + index)) ===
      JSON.stringify(render(index)));
  }
});

test('every palette byte 0-255 renders without throwing', () => {
  for (let byte = 0; byte <= 255; byte++) {
    const engine = new SimEngine({ devices: ['puck'], effect: 13,
      palette: byte, time: 12345, paused: true });
    assertValidSnapshot(engine.getSnapshot());
  }
});

test('palette byte wraps at the palette table length', () => {
  const render = (palette) => new SimEngine({ devices: ['puck'], effect: 13,
    palette, time: 12345, paused: true }).getSnapshot().devices;
  assert(JSON.stringify(render(22)) === JSON.stringify(render(0)));
});

test('a 0-LED custom strip renders an empty byte array', () => {
  const bytes = renderer.renderStrip(
    DEFAULT_SEEDS,
    { ledCount: 0, flags: [] },
    { effectIndex: 4, paletteIndex: 17, controlRgb: null },
    100,
  );
  assertEqual(bytes.length, 0);
});

test('time boundaries render valid snapshots', () => {
  for (const time of [0, 1, 2147483648, 4294967295]) {
    for (const effect of [0, 4, 13, 23, 33]) {
      const engine = new SimEngine({ devices: ['puck'], effect, palette: 8,
        time, paused: true });
      assertValidSnapshot(engine.getSnapshot());
    }
  }
});

test('unknown catalog names throw helpful errors', () => {
  assertThrows(() => new SimEngine({ devices: ['not_a_real_device'] }));
  assertThrows(() => new SimEngine({ effect: 'Not A Real Effect' }));
  assertThrows(() => new SimEngine({ palette: 'Not A Real Palette' }));
});

test('invalid clock inputs throw', () => {
  const engine = new SimEngine({ paused: true });
  assertThrows(() => engine.setTime(NaN));
  assertThrows(() => engine.setSpeed(0));
});
