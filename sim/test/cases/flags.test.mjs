// FR-007: centrally-applied strip flags (Reversed, Dim, Off) and flag
// pass-through to effects (Tiny/Circular).

import { test, assert, assertEqual, assertDeepEqual } from '../harness.js';
import { SimEngine } from '../../js/engine.js';
import { DEVICES, makeStrip } from '../../js/devices.js';

test(
  'Reversed strip renders as the exact reverse of an equivalent strip',
  () => {
  // will_backpack: strip0 is 96 plain LEDs, strip1 is 96 LEDs + Reversed.
  // Rainbow's getRGB depends only on (ledIndex, timeMs, strip flags other
  // than Reversed), and neither strip sets any other flag, so this
  // comparison is exact.
    const engine = new SimEngine({
      devices: ['will_backpack'], effect: 'Rainbow',
      palette: 'Rainbow', time: 54321,
      paused: true });
    const [strip0, strip1] = engine.getSnapshot().devices[0].strips;
    assertEqual(strip0.leds.length, strip1.leds.length);
    assert(strip0.leds.length > 0);
    assertDeepEqual(strip1.leds, [...strip0.leds].reverse());
  });

test('Off strip always renders black, even under a control override', () => {
  DEVICES.__test_off = {
    name: '__test_off',
    milliamps: 0,
    strips: [makeStrip(5, ['Off'])],
  };
  try {
    const engine = new SimEngine({ devices: ['__test_off'], effect: 'Rainbow',
      palette: 'Rainbow', time: 1000,
      paused: true });
    let leds = engine.getSnapshot().devices[0].strips[0].leds;
    assert(leds.every(([r, g, b]) => r === 0 && g === 0 && b === 0));

    engine.setControl([200, 100, 50], 10);
    leds = engine.getSnapshot().devices[0].strips[0].leds;
    assert(leds.every(([r, g, b]) => r === 0 && g === 0 && b === 0),
      'Off must black out even while a control override is active');
  } finally {
    delete DEVICES.__test_off;
  }
});

test('Dim strip divides rendered color by 8', () => {
  DEVICES.__test_dim = {
    name: '__test_dim',
    milliamps: 0,
    strips: [makeStrip(3, ['Dim'])],
  };
  try {
    const engine = new SimEngine({ devices: ['__test_dim'], effect: 'Dark',
      palette: 0, time: 0, paused: true });
    // Dark: trivially 0 / 8 == 0.
    let leds = engine.getSnapshot().devices[0].strips[0].leds;
    assert(leds.every(([r, g, b]) => r === 0 && g === 0 && b === 0));

    engine.setControl([200, 100, 50]);
    leds = engine.getSnapshot().devices[0].strips[0].leds;
    for (const [r, g, b] of leds) {
      assertEqual(r, 25); // (200/8)|0
      assertEqual(g, 12); // (100/8)|0
      assertEqual(b, 6);  // (50/8)|0
    }
  } finally {
    delete DEVICES.__test_dim;
  }
});

test(
  'multi-strip device snapshot preserves per-strip order and lengths',
  () => {
  // rainbow_cloak: [11, Tiny+Circular], [94], [11, Tiny+Circular+Reversed].
    const engine = new SimEngine({
      devices: ['rainbow_cloak'], effect: 'Rainbow',
      palette: 'Rainbow', time: 100,
      paused: true });
    const strips = engine.getSnapshot().devices[0].strips;
    assertEqual(strips.length, 3);
    assertEqual(strips[0].leds.length, 11);
    assertEqual(strips[1].leds.length, 94);
    assertEqual(strips[2].leds.length, 11);
  });

test(
  'Tiny flag makes Rainbow uniform across the strip; non-Tiny varies',
  () => {
    const time = 33333;
    const puck = new SimEngine({ devices: ['puck'], effect: 'Rainbow',
      palette: 'Rainbow', time, paused: true });
    const scarf = new SimEngine({ devices: ['scarf'], effect: 'Rainbow',
      palette: 'Rainbow', time, paused: true });

    const puckLeds = puck.getSnapshot().devices[0].strips[0].leds;
    const scarfLeds = scarf.getSnapshot().devices[0].strips[0].leds;

    const allEqual = (leds) =>
      leds.every((led) => JSON.stringify(led) === JSON.stringify(leds[0]));

    assert(
      allEqual(puckLeds), 'Tiny (puck) strip should render one flat color');
    assert(!allEqual(scarfLeds),
      'non-Tiny (scarf) strip should vary color by LED index');
  });
