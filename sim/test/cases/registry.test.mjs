// FR-004: effect/palette wire-index mappings match firmware registration order.

import { test, assert, assertEqual, assertDeepEqual } from '../harness.js';
import { createRegistry } from '../../js/effects/registry.js';
import { SimEngine } from '../../js/engine.js';

// Transcribed from LedManager.cpp:12-35 construction order (see registry.js
// header comment for the weight->duplicate-entry expansion rule).
const EXPECTED_NAMES = [
  'Color Cycle', 'Color Cycle',
  'Contrast Bumps', 'Contrast Bumps',
  'Fire',
  'Firefly', 'Firefly',
  'Lightning',
  'Pride',
  'Rainbow Bumps', 'Rainbow Bumps', 'Rainbow Bumps', 'Rainbow Bumps',
  'Rainbow', 'Rainbow', 'Rainbow', 'Rainbow',
  'Rorschach', 'Rorschach',
  'Spark', 'Spark', 'Spark', 'Spark',
  'Swinging Lights', 'Swinging Lights', 'Swinging Lights', 'Swinging Lights',
  'Swinging Lights (Police)',
  'Stop Light',
  'Simple Blink 60ms',
  'Simple Blink 30ms',
  'Simple Blink 12ms',
  'Simple Blink 300ms',
  'Display Color Palette',
  'Dark',
];

const EXPECTED_PALETTE_NAMES = [
  'Red', 'Orange', 'Yellow', 'Green', 'Aqua', 'Blue', 'Purple', 'Pink',
  'Rainbow', 'Warm', 'Cool', 'Yellow-Green', '80s Miami', 'Vaporwave',
  'Cool Popo', 'Candy Cane', 'Winter Mint', 'Fire', 'Pastel Rainbow',
  'Jazz Cup', 'Yellow & Double Purp', 'Double Rainbow',
];

test('wireTable has exactly 35 entries', () => {
  const registry = createRegistry();
  assertEqual(registry.wireTable.length, 35);
});

test('randomPoolSize is exactly 27', () => {
  const registry = createRegistry();
  assertEqual(registry.randomPoolSize, 27);
});

test('every wire index has the expected name (LedManager.cpp order)', () => {
  const registry = createRegistry();
  const engine = new SimEngine({ paused: true });
  const names = engine.listEffects().map((e) => e.name);
  assertDeepEqual(names, EXPECTED_NAMES);
  // Cross-check against the registry directly too.
  assertDeepEqual(
    registry.wireTable.map((e) => e.name), EXPECTED_NAMES);
});

test(
  'index 33 is Display Color Palette and 34 is Dark (must be last two)',
  () => {
    const registry = createRegistry();
    assertEqual(registry.wireTable.length - 2, 33);
    assertEqual(registry.wireTable[33].name, 'Display Color Palette');
    assertEqual(registry.wireTable[34].name, 'Dark');
  });

test('wireTable length stays under 256 (wire-byte invariant)', () => {
  const registry = createRegistry();
  assert(registry.wireTable.length < 256);
});

test('name-to-index round trip resolves to the FIRST occurrence index', () => {
  const registry = createRegistry();
  const seen = new Set();
  for (const entry of registry.wireTable) {
    if (seen.has(entry.name)) continue;
    seen.add(entry.name);
    assertEqual(registry.getIndexByName(entry.name), entry.index,
      `name "${entry.name}" should resolve back to its first index`);
  }
});

test('listPalettes() has exactly 22 palettes in the expected order', () => {
  const engine = new SimEngine({ paused: true });
  const palettes = engine.listPalettes();
  assertEqual(palettes.length, 22);
  assertDeepEqual(palettes.map((p) => p.name), EXPECTED_PALETTE_NAMES);
});

test('duplicate wire indices for the same effect render identically', () => {
  // Indices 13-16 are all "Rainbow" (weight 4) -> same effect object.
  const engineA = new SimEngine(
    { devices: ['scarf'], effect: 13, palette: 'Rainbow', time: 4242,
      paused: true });
  const engineB = new SimEngine(
    { devices: ['scarf'], effect: 14, palette: 'Rainbow', time: 4242,
      paused: true });
  assertDeepEqual(engineA.getSnapshot().devices, engineB.getSnapshot().devices);
  assertEqual(engineA.getState().effectName, engineB.getState().effectName);
});
