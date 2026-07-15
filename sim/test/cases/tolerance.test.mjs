// FR-014: out-of-range wire indices and degenerate strip configurations must
// never throw or corrupt state; only programmer errors (unknown names,
// non-numeric time/speed) throw.

import { test, assert, assertEqual, assertThrows } from '../harness.js';
import { SimEngine } from '../../js/engine.js';
import { DEVICES, makeStrip } from '../../js/devices.js';

function assertValidSnapshot(snapshot) {
  for (const device of snapshot.devices) {
    for (const strip of device.strips) {
      for (const [r, g, b] of strip.leds) {
        for (const channel of [r, g, b]) {
          assert(Number.isInteger(channel),
            `channel ${channel} is not an integer`);
          assert(channel >= 0 && channel <= 255,
            `channel ${channel} out of 0-255 range`);
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

test('effect byte wraps at the wire table length (35)', () => {
  const make = (effect) => new SimEngine({ devices: ['puck'], effect,
    palette: 8, time: 12345,
    paused: true }).getSnapshot().devices;
  for (const k of [0, 13, 34]) {
    assert(JSON.stringify(make(35 + k)) === JSON.stringify(make(k)),
      `effect ${35 + k} should render identically to effect ${k}`);
  }
});

test(
  'every palette byte 0-255 renders without throwing (effect Rainbow)',
  () => {
    for (let byte = 0; byte <= 255; byte++) {
      const engine = new SimEngine({ devices: ['puck'], effect: 13,
        palette: byte, time: 12345, paused: true });
      assertValidSnapshot(engine.getSnapshot());
    }
  });

test('palette byte wraps at the palette table length (22)', () => {
  const make = (palette) => new SimEngine({ devices: ['puck'], effect: 13,
    palette, time: 12345,
    paused: true }).getSnapshot().devices;
  assert(JSON.stringify(make(22)) === JSON.stringify(make(0)),
    'palette 22 should render identically to palette 0');
});

test('a 0-LED strip renders an empty leds array without throwing', () => {
  DEVICES.__test_zero = {
    name: '__test_zero',
    milliamps: 0,
    strips: [makeStrip(0)],
  };
  try {
    const engine = new SimEngine({ devices: ['__test_zero'], effect: 'Fire',
      palette: 'Fire', time: 100, paused: true });
    const snapshot = engine.getSnapshot();
    assertEqual(snapshot.devices[0].strips[0].leds.length, 0);
  } finally {
    delete DEVICES.__test_zero;
  }
});

test('time boundaries render valid snapshots for a range of effects', () => {
  const times = [0, 1, 2147483648, 4294967295];
  const effects = [0, 4, 13, 23, 33];
  for (const time of times) {
    for (const effect of effects) {
      const engine = new SimEngine({ devices: ['puck'], effect, palette: 8,
        time, paused: true });
      assertValidSnapshot(engine.getSnapshot());
    }
  }
});

test('unknown device name throws with a helpful message', () => {
  assertThrows(() => new SimEngine({ devices: ['not_a_real_device'] }));
});

test('unknown effect name throws with a helpful message', () => {
  assertThrows(() => new SimEngine({ effect: 'Not A Real Effect' }));
});

test('unknown palette name throws with a helpful message', () => {
  assertThrows(() => new SimEngine({ palette: 'Not A Real Palette' }));
});

test('setTime(NaN) throws', () => {
  const engine = new SimEngine({ paused: true });
  assertThrows(() => engine.setTime(NaN));
});

test('setSpeed(0) throws', () => {
  const engine = new SimEngine({ paused: true });
  assertThrows(() => engine.setSpeed(0));
});
