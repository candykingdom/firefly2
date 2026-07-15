// FR-015 (US4 scenarios 1 & 3): master-mode autoplay cadence, weighted pool,
// and manual-set/delay timer interactions.

import { test, assert, assertEqual } from '../harness.js';
import { SimEngine } from '../../js/engine.js';
import { attachMaster } from '../../js/master.js';

// Index 34 (Dark) is outside the weighted pool (0-26), so any post-change
// value being in-pool is proof a change actually happened.
const SENTINEL_EFFECT = 34;

function makeMasterEngine(seed) {
  const engine = new SimEngine({ devices: ['scarf'], effect: SENTINEL_EFFECT,
    palette: 0, time: 0, paused: true });
  attachMaster(engine);
  engine.setMasterMode(true, seed);
  return engine;
}

test('no change before the 60s cadence elapses', () => {
  const engine = makeMasterEngine(1);
  engine.step(59999);
  assertEqual(engine.getState().effectIndex, SENTINEL_EFFECT);
});

test('change happens at/after 60000ms', () => {
  const engine = makeMasterEngine(1);
  engine.step(59999);
  engine.step(1);
  const effectIndex = engine.getState().effectIndex;
  assert(effectIndex >= 0 && effectIndex <= 26,
    `expected a pool index after the change, got ${effectIndex}`);
});

test('every autoplay change stays within the weighted pool (0-26)', () => {
  const engine = makeMasterEngine(2);
  const effectIndices = [];
  const paletteIndices = [];
  for (let i = 0; i < 50; i++) {
    engine.step(60000);
    const state = engine.getState();
    effectIndices.push(state.effectIndex);
    paletteIndices.push(state.paletteIndex);
  }
  for (const idx of effectIndices) {
    assert(idx >= 0 && idx <= 26, `effectIndex ${idx} outside pool range`);
  }
  for (const idx of paletteIndices) {
    assert(idx >= 0 && idx <= 21, `paletteIndex ${idx} outside palette range`);
  }
  assert(new Set(effectIndices).size >= 5,
    'expected at least 5 distinct effect indices over 50 changes');
});

test('same seed reproduces the same (effect, palette) sequence', () => {
  const engineA = makeMasterEngine(7);
  const engineB = makeMasterEngine(7);
  const seqA = [];
  const seqB = [];
  for (let i = 0; i < 10; i++) {
    engineA.step(60000);
    engineB.step(60000);
    seqA.push(
      [engineA.getState().effectIndex, engineA.getState().paletteIndex]);
    seqB.push(
      [engineB.getState().effectIndex, engineB.getState().paletteIndex]);
  }
  assertEqual(JSON.stringify(seqA), JSON.stringify(seqB));
});

test('different seeds produce different sequences', () => {
  const engineA = makeMasterEngine(1);
  const engineB = makeMasterEngine(2);
  const seqA = [];
  const seqB = [];
  for (let i = 0; i < 10; i++) {
    engineA.step(60000);
    engineB.step(60000);
    seqA.push(
      [engineA.getState().effectIndex, engineA.getState().paletteIndex]);
    seqB.push(
      [engineB.getState().effectIndex, engineB.getState().paletteIndex]);
  }
  assert(JSON.stringify(seqA) !== JSON.stringify(seqB),
    'expected different seeds to diverge over 10 changes');
});

test(
  'setDelay holds the effect until delaySeconds elapse (US4 scenario 3)',
  () => {
    const engine = makeMasterEngine(3);
    engine.setEffect(33);
    engine.setDelay(10);
    assertEqual(engine.getState().effectIndex, 33);

    engine.step(9999);
    assertEqual(
      engine.getState().effectIndex, 33, 'should hold until delay elapses');

    engine.step(1);
    const effectIndex = engine.getState().effectIndex;
    assert(
      effectIndex !== 33, 'expected master to change after the hold expired');
  });

test('a manual setEffect resets the 60s change timer', () => {
  const engine = makeMasterEngine(4);
  engine.step(59000);
  assertEqual(engine.getState().effectIndex, SENTINEL_EFFECT);

  engine.setEffect(5);
  assertEqual(engine.getState().effectIndex, 5);

  engine.step(59999);
  assertEqual(engine.getState().effectIndex, 5,
    'manual set should have restarted the 60s timer');

  engine.step(1);
  assert(engine.getState().effectIndex !== 5,
    'expected a change 60s after the manual set');
});

test('a master-mode change replaces a delay-0 control override', () => {
  const engine = attachMaster(new SimEngine({ devices: ['scarf'],
    effect: 'Dark', palette: 0, time: 0, paused: true }));
  engine.setMasterMode(true, 7);
  engine.setControl([4, 5, 6], 0); // would persist forever without a master
  engine.step(59999);
  assert(engine.getState().control !== null,
    'control holds until the master change fires');
  engine.step(1);
  assertEqual(engine.getState().control, null,
    "the master's SET_EFFECT must replace the control packet");
});

test('setControl with a delay re-arms the master change timer to it', () => {
  const engine = attachMaster(new SimEngine({ devices: ['scarf'],
    effect: 'Dark', palette: 0, time: 0, paused: true }));
  engine.setMasterMode(true, 7);
  engine.step(50000); // 10s before the normal change would fire
  engine.setControl([4, 5, 6], 30);
  engine.step(29999); // normal 60s boundary passes without a change
  assertEqual(engine.getState().effectName, 'Dark',
    'control delay postpones the master change');
  assert(engine.getState().control !== null);
  engine.step(1); // control expires and the re-armed change fires
  assertEqual(engine.getState().control, null);
});
