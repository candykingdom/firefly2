// FR-015 (US4 scenario 2): SET_CONTROL solid-color override semantics.

import { test, assert, assertEqual, assertDeepEqual } from '../harness.js';
import { SimEngine } from '../../js/engine.js';

function allLedsEqual(snapshot, rgb) {
  for (const device of snapshot.devices) {
    for (const strip of device.strips) {
      for (const led of strip.leds) {
        assertDeepEqual(led, rgb);
      }
    }
  }
}

for (const effect of ['Dark', 'Rainbow']) {
  test(`setControl overrides the "${effect}" effect on every LED`, () => {
    const engine = new SimEngine({ devices: ['scarf'], effect,
      palette: 'Rainbow', time: 1000,
      paused: true });
    engine.setControl([255, 0, 0], 10);
    allLedsEqual(engine.getSnapshot(), [255, 0, 0]);
  });
}

test('setControl on a Dim strip divides the control color by 8', () => {
  // backpack_rope: both strips are Dim.
  const engine = new SimEngine({ devices: ['backpack_rope'], effect: 'Dark',
    palette: 0, time: 1000, paused: true });
  engine.setControl([255, 0, 0], 10);
  allLedsEqual(engine.getSnapshot(), [31, 0, 0]); // (255/8)|0
});

test('control overrides any effect index, including Dark (34)', () => {
  const engine = new SimEngine({ devices: ['scarf'], effect: 34, palette: 0,
    time: 1000, paused: true });
  engine.setControl([10, 20, 30], 10);
  allLedsEqual(engine.getSnapshot(), [10, 20, 30]);
});

test('control expires exactly at setAt + delaySeconds*1000', () => {
  const engine = new SimEngine({ devices: ['scarf'], effect: 'Dark',
    palette: 0, time: 1000, paused: true });
  engine.setControl([255, 0, 0], 10);

  engine.setTime(10999);
  assert(engine.getState().control !== null, 'control should still be active');
  allLedsEqual(engine.getSnapshot(), [255, 0, 0]);

  engine.step(1); // now at 11000
  assertEqual(engine.getState().control, null);
  allLedsEqual(engine.getSnapshot(), [0, 0, 0]); // back to Dark
});

test('delay 0 holds the control until clearControl() is called', () => {
  const engine = new SimEngine({ devices: ['scarf'], effect: 'Dark',
    palette: 0, time: 1000, paused: true });
  engine.setControl([1, 2, 3], 0);
  engine.step(1000000);
  assert(engine.getState().control !== null);
  allLedsEqual(engine.getSnapshot(), [1, 2, 3]);

  engine.clearControl();
  assertEqual(engine.getState().control, null);
  allLedsEqual(engine.getSnapshot(), [0, 0, 0]);
});

test('clearControl is idempotent', () => {
  const engine = new SimEngine({ devices: ['scarf'], paused: true });
  engine.clearControl();
  engine.clearControl();
  assertEqual(engine.getState().control, null);
});

test('getState().control reports rgb and delaySeconds while active', () => {
  const engine = new SimEngine({ devices: ['scarf'], time: 500, paused: true });
  engine.setControl([9, 8, 7], 42);
  assertDeepEqual(
    engine.getState().control,
    { rgb: [9, 8, 7], delaySeconds: 42 },
  );
});

test('a new SET_EFFECT (setEffect/setPalette/setDelay) ends the override',
  () => {
    const engine = new SimEngine({ devices: ['scarf'], effect: 'Rainbow',
      palette: 8, time: 1000, paused: true });

    engine.setControl([255, 0, 0], 100);
    engine.setEffect('Dark');
    assertEqual(engine.getState().control, null,
      'setEffect must replace the control packet');
    allLedsEqual(engine.getSnapshot(), [0, 0, 0]);

    engine.setControl([255, 0, 0], 100);
    engine.setPalette(0);
    assertEqual(engine.getState().control, null,
      'setPalette rides SET_EFFECT and must replace the control packet');

    engine.setControl([255, 0, 0], 100);
    engine.setDelay(5);
    assertEqual(engine.getState().control, null,
      'setDelay rides SET_EFFECT and must replace the control packet');
  });

test('scrubbing backward does not expire an active control', () => {
  const engine = new SimEngine({ devices: ['scarf'], effect: 'Dark',
    palette: 0, time: 1000, paused: true });
  engine.setControl([255, 0, 0], 10);

  engine.setTime(500); // before the control was even set
  assert(engine.getState().control !== null,
    'backward scrub must not read as expiry');
  allLedsEqual(engine.getSnapshot(), [255, 0, 0]);

  engine.setTime(10999); // forward again, still inside the hold
  assert(engine.getState().control !== null);
  engine.setTime(11000); // exactly at expiry
  assertEqual(engine.getState().control, null);
});
