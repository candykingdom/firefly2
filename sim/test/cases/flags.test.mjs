import {
  test,
  assert,
  assertEqual,
  assertDeepEqual,
} from '../harness.js';
import { SimEngine } from '../../js/engine.js';
import renderer, { DEFAULT_SEEDS } from '../../js/renderer.js';

function renderStrip(flags, controlRgb = null) {
  const bytes = renderer.renderStrip(
    DEFAULT_SEEDS,
    { ledCount: 5, flags },
    { effectIndex: 13, paletteIndex: 8, controlRgb },
    1000,
  );
  return [...bytes];
}

test(
  'Reversed strip renders as the exact reverse of an equivalent strip',
  () => {
    const engine = new SimEngine({
      devices: ['will_backpack'], effect: 'Rainbow',
      palette: 'Rainbow', time: 54321, paused: true,
    });
    const [plain, reversed] = engine.getSnapshot().devices[0].strips;
    assertEqual(plain.leds.length, reversed.leds.length);
    assert(plain.leds.length > 0);
    assertDeepEqual(reversed.leds, [...plain.leds].reverse());
  },
);

test('Off strip always renders black, including under control', () => {
  assertDeepEqual(renderStrip(['Off']), new Array(15).fill(0));
  assertDeepEqual(
    renderStrip(['Off'], [200, 100, 50]),
    new Array(15).fill(0),
  );
});

test('Dim strip divides a control color by 8', () => {
  assertDeepEqual(
    renderStrip(['Dim'], [200, 100, 50]),
    [25, 12, 6, 25, 12, 6, 25, 12, 6, 25, 12, 6, 25, 12, 6],
  );
});

test('multi-strip snapshot preserves per-strip order and lengths', () => {
  const engine = new SimEngine({
    devices: ['rainbow_cloak'], effect: 'Rainbow',
    palette: 'Rainbow', time: 100, paused: true,
  });
  const strips = engine.getSnapshot().devices[0].strips;
  assertDeepEqual(strips.map((strip) => strip.leds.length), [11, 94, 11]);
});

test('Tiny makes Rainbow uniform while a normal strip varies', () => {
  const puck = new SimEngine({ devices: ['puck'], effect: 'Rainbow',
    palette: 'Rainbow', time: 33333, paused: true });
  const scarf = new SimEngine({ devices: ['scarf'], effect: 'Rainbow',
    palette: 'Rainbow', time: 33333, paused: true });
  const puckLeds = puck.getSnapshot().devices[0].strips[0].leds;
  const scarfLeds = scarf.getSnapshot().devices[0].strips[0].leds;
  const allEqual = (leds) => leds.every((led) =>
    JSON.stringify(led) === JSON.stringify(leds[0]));
  assert(allEqual(puckLeds));
  assert(!allEqual(scarfLeds));
});
