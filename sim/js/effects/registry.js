// Wire-index-faithful effect registry. Mirrors LedManager's constructor
// (lib/led_manager/LedManager.cpp:12-38) exactly: same construction order,
// same weights (duplicate wire entries), same non-random tail. Invariants:
// DisplayColorPalette and Dark are the last two indices; total < 256.

import { random16, resetRandom16 } from '../fastled.js';
import { makeColorCycleEffect } from './colorCycle.js';
import { makeContrastBumpsEffect } from './contrastBumps.js';
import { makeControlEffect } from './control.js';
import { makeDarkEffect } from './dark.js';
import { makeDisplayColorPaletteEffect } from './displayColorPalette.js';
import { makeFireEffect } from './fire.js';
import { makeFireflyEffect } from './firefly.js';
import { makeLightningEffect } from './lightning.js';
import { makePrideEffect } from './pride.js';
import { makeRainbowBumpsEffect } from './rainbowBumps.js';
import { makeRainbowEffect } from './rainbow.js';
import { makeRorschachEffect } from './rorschach.js';
import { makeSimpleBlinkEffect } from './simpleBlink.js';
import { makeSparkEffect } from './spark.js';
import { makeStopLightEffect } from './stopLight.js';
import { makeSwingingLightsEffect } from './swingingLights.js';

// Host-build default for FireflyEffect's constructor offset: on the host,
// offset_ = random(0, kBlinkPeriod / 2) uses unseeded libc rand() (srand(1)),
// which JS cannot portably reproduce. The authoritative value lives in
// sim/test/vectors/reference.json meta.effectSeeds.Firefly; this constant must
// match it (vectors.test.mjs cross-checks).
export const DEFAULT_FIREFLY_OFFSET = 423;

export function createRegistry({ fireflyOffset = DEFAULT_FIREFLY_OFFSET,
  fireOffset = null,
  rorschachOffset = null } = {}) {
  // Fire and Rorschach draw offset = random16() at construction. On the host
  // (no #ifdef ARDUINO analogRead reseed) those are the 1st and 2nd draws of
  // the FastLED LCG from its initial seed 1337, in LedManager construction
  // order. Replay that sequence unless explicit offsets are given.
  resetRandom16();
  const firstDraw = random16();
  const secondDraw = random16();
  const fire = makeFireEffect(fireOffset === null ? firstDraw : fireOffset);
  const rorschach = makeRorschachEffect(
    rorschachOffset === null ? secondDraw : rorschachOffset);
  const firefly = makeFireflyEffect(fireflyOffset);

  const declarations = [
    [makeColorCycleEffect(), 2, 'Color Cycle'],
    [makeContrastBumpsEffect(), 2, 'Contrast Bumps'],
    [fire, 1, 'Fire'],
    [firefly, 2, 'Firefly'],
    [makeLightningEffect(), 1, 'Lightning'],
    [makePrideEffect(), 1, 'Pride'],
    [makeRainbowBumpsEffect(), 4, 'Rainbow Bumps'],
    [makeRainbowEffect(), 4, 'Rainbow'],
    [rorschach, 2, 'Rorschach'],
    [makeSparkEffect(), 4, 'Spark'],
    [makeSwingingLightsEffect(), 4, 'Swinging Lights'],
    // Non-random effects (weight 0)
    [makeSwingingLightsEffect(), 0, 'Swinging Lights (Police)'],
    [makeStopLightEffect(), 0, 'Stop Light'],
    [makeSimpleBlinkEffect(60), 0, 'Simple Blink 60ms'],
    [makeSimpleBlinkEffect(30), 0, 'Simple Blink 30ms'],
    [makeSimpleBlinkEffect(12), 0, 'Simple Blink 12ms'],
    [makeSimpleBlinkEffect(300), 0, 'Simple Blink 300ms'],
    // These two must be last
    [makeDisplayColorPaletteEffect(), 0, 'Display Color Palette'],
    [makeDarkEffect(), 0, 'Dark'],
  ];

  const weighted = [];
  const nonRandom = [];
  for (const [effect, weight, name] of declarations) {
    if (weight > 0) {
      for (let i = 0; i < weight; i++) weighted.push({ effect, name, weight });
    } else {
      nonRandom.push({ effect, name, weight });
    }
  }
  const wireTable = weighted.concat(nonRandom).map((entry, index) => ({
    index,
    name: entry.name,
    weight: entry.weight,
    effect: entry.effect,
  }));
  if (wireTable.length >= 256) {
    throw new Error(`effect table too large: ${wireTable.length}`);
  }

  const nameToIndex = new Map();
  for (const entry of wireTable) {
    if (!nameToIndex.has(entry.name)) nameToIndex.set(entry.name, entry.index);
  }

  return {
    wireTable,
    randomPoolSize: weighted.length,
    controlEffect: makeControlEffect(),
    seeds: {
      Fire: fireOffset === null ? firstDraw : fireOffset,
      Rorschach: rorschachOffset === null ? secondDraw : rorschachOffset,
      Firefly: fireflyOffset,
    },
    getByIndex(index) {
      return this.wireTable[(index & 0xff) % this.wireTable.length];
    },
    getIndexByName(name) {
      if (!nameToIndex.has(name)) {
        throw new Error(`unknown effect "${name}"; valid: ${
          [...nameToIndex.keys()].join(', ')}`);
      }
      return nameToIndex.get(name);
    },
  };
}
