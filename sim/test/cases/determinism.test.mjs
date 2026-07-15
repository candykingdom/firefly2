// FR-013: rendering is a deterministic function of (device, effect, palette,
// time); the clock's pause/step/speed/wrap semantics are also pinned here.

import { test, assert, assertEqual, assertDeepEqual } from '../harness.js';
import { SimEngine } from '../../js/engine.js';

const PARAMS = { devices: ['scarf'], effect: 'Rainbow', palette: 'Rainbow',
  time: 9000, paused: true };

test('getSnapshot() is stable across two consecutive calls', () => {
  const engine = new SimEngine(PARAMS);
  assertDeepEqual(engine.getSnapshot(), engine.getSnapshot());
});

test(
  'getSnapshot() is identical for a fresh instance with the same inputs',
  () => {
    const a = new SimEngine(PARAMS);
    const b = new SimEngine(PARAMS);
    assertDeepEqual(a.getSnapshot(), b.getSnapshot());
  });

test('getSnapshot() is identical after mutating and restoring state', () => {
  const engine = new SimEngine(PARAMS);
  const before = engine.getSnapshot();
  engine.setEffect('Fire').setPalette('Fire').setTime(123456);
  engine.setEffect(PARAMS.effect).setPalette(PARAMS.palette)
    .setTime(PARAMS.time);
  assertDeepEqual(engine.getSnapshot(), before);
});

test('snapshot differs after step(50) for a time-varying effect', () => {
  const engine = new SimEngine(PARAMS);
  const before = engine.getSnapshot();
  engine.step(50);
  const after = engine.getSnapshot();
  assert(JSON.stringify(before.devices) !== JSON.stringify(after.devices),
    'expected Rainbow to change after advancing time by 50ms');
});

test('getSnapshot() does not advance time', () => {
  const engine = new SimEngine(PARAMS);
  engine.getSnapshot();
  const t1 = engine.getState().time;
  engine.getSnapshot();
  const t2 = engine.getState().time;
  assertEqual(t1, t2);
});

test('paused engine ignores advanceWall', () => {
  const engine = new SimEngine(PARAMS);
  engine.advanceWall(1000);
  assertEqual(engine.getState().time, PARAMS.time);
});

test('step works while paused', () => {
  const engine = new SimEngine(PARAMS);
  engine.step(50);
  assertEqual(engine.getState().time, PARAMS.time + 50);
});

test('setSpeed(2) + advanceWall(500) advances exactly 1000ms', () => {
  const engine = new SimEngine({ ...PARAMS, paused: false, time: 0 });
  engine.setSpeed(2);
  engine.advanceWall(500);
  assertEqual(engine.getState().time, 1000);
});

test('fractional wall deltas accumulate exactly (4x0.25ms == 1ms)', () => {
  const engine = new SimEngine({ ...PARAMS, paused: false, time: 0 });
  engine.setSpeed(1);
  engine.advanceWall(0.25);
  engine.advanceWall(0.25);
  engine.advanceWall(0.25);
  engine.advanceWall(0.25);
  assertEqual(engine.getState().time, 1);
});

test('network time wraps at uint32', () => {
  const engine = new SimEngine({ ...PARAMS, time: 4294967295 });
  engine.step(1);
  assertEqual(engine.getState().time, 0);
});

test(
  'seeded effects: fresh engines share identical seeds and Fire output',
  () => {
    const a = new SimEngine({ devices: ['puck'], effect: 4, palette: 'Fire',
      time: 777, paused: true });
    const b = new SimEngine({ devices: ['puck'], effect: 4, palette: 'Fire',
      time: 777, paused: true });
    assertDeepEqual(a.registry.seeds, b.registry.seeds);
    assertDeepEqual(a.getSnapshot().devices, b.getSnapshot().devices);
  });
