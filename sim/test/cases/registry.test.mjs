import {
  test,
  assert,
  assertEqual,
  assertDeepEqual,
} from '../harness.js';
import { SimEngine } from '../../js/engine.js';
import renderer from '../../js/renderer.js';

const EXPECTED_NAMES = [
  'Color Cycle', 'Color Cycle', 'Contrast Bumps', 'Contrast Bumps', 'Fire',
  'Firefly', 'Firefly', 'Lightning', 'Pride', 'Rainbow Bumps',
  'Rainbow Bumps', 'Rainbow Bumps', 'Rainbow Bumps', 'Rainbow', 'Rainbow',
  'Rainbow', 'Rainbow', 'Rorschach', 'Rorschach', 'Spark', 'Spark', 'Spark',
  'Spark', 'Swinging Lights', 'Swinging Lights', 'Swinging Lights',
  'Swinging Lights', 'Swinging Lights (Police)', 'Stop Light',
  'Simple Blink 60ms', 'Simple Blink 30ms', 'Simple Blink 12ms',
  'Simple Blink 300ms', 'Display Color Palette', 'Dark',
];

const EXPECTED_PALETTE_NAMES = [
  'Red', 'Orange', 'Yellow', 'Green', 'Aqua', 'Blue', 'Purple', 'Pink',
  'Rainbow', 'Warm', 'Cool', 'Yellow-Green', '80s Miami', 'Vaporwave',
  'Cool Popo', 'Candy Cane', 'Winter Mint', 'Fire', 'Pastel Rainbow',
  'Jazz Cup', 'Yellow & Double Purp', 'Double Rainbow',
];

test('adapter exposes the exact 35-entry wire table', () => {
  assertEqual(renderer.metadata.effects.length, 35);
  assertDeepEqual(
    renderer.metadata.effects.map((effect) => effect.name),
    EXPECTED_NAMES,
  );
  assert(renderer.metadata.effects.length < 256);
});

test('adapter exposes the exact weighted random pool size', () => {
  assertEqual(renderer.metadata.randomEffectCount, 27);
});

test('Display Color Palette and Dark remain the final two effects', () => {
  const effects = renderer.metadata.effects;
  assertEqual(effects[33].name, 'Display Color Palette');
  assertEqual(effects[34].name, 'Dark');
});

test('effect names resolve to their first wire index', () => {
  const engine = new SimEngine({ paused: true });
  const seen = new Set();
  for (const entry of renderer.metadata.effects) {
    if (seen.has(entry.name)) continue;
    seen.add(entry.name);
    engine.setEffect(entry.name);
    assertEqual(engine.getState().effectIndex, entry.index);
  }
});

test('adapter exposes all 22 palettes in production order', () => {
  assertDeepEqual(
    renderer.metadata.palettes.map((palette) => palette.name),
    EXPECTED_PALETTE_NAMES,
  );
});

test('duplicate wire entries render identically', () => {
  const first = new SimEngine({ devices: ['scarf'], effect: 13,
    palette: 'Rainbow', time: 4242, paused: true });
  const duplicate = new SimEngine({ devices: ['scarf'], effect: 14,
    palette: 'Rainbow', time: 4242, paused: true });
  assertDeepEqual(
    first.getSnapshot().devices,
    duplicate.getSnapshot().devices,
  );
  assertEqual(first.getState().effectName,
    duplicate.getState().effectName);
});
