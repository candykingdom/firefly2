import {
  test,
  assert,
  assertEqual,
  assertDeepEqual,
} from '../harness.js';
import renderer, { DEFAULT_SEEDS } from '../../js/renderer.js';

const vectorsUrl = new URL('../vectors/reference.json', import.meta.url);
let vectors;
if (typeof process !== 'undefined' && process.versions?.node) {
  const { readFile } = await import('node:fs/promises');
  vectors = JSON.parse(await readFile(vectorsUrl, 'utf8'));
} else {
  vectors = await (await fetch(vectorsUrl)).json();
}

function showFor(vectorCase) {
  return vectorCase.control ? {
    effectIndex: 0,
    paletteIndex: 0,
    controlRgb: vectorCase.control.rgb,
  } : {
    effectIndex: vectorCase.effectIndex,
    paletteIndex: vectorCase.paletteIndex,
    controlRgb: null,
  };
}

test('shared renderer initializes ABI v1 and authoritative metadata', () => {
  assertEqual(renderer.abiVersion, 1);
  assertEqual(renderer.metadata.effects.length, 35);
  assertEqual(renderer.metadata.randomEffectCount, 27);
  assertEqual(renderer.metadata.palettes.length, 22);
  assertEqual(renderer.metadata.devices.length, 22);
  assertEqual(renderer.metadata.effects[33].name, 'Display Color Palette');
  assertEqual(renderer.metadata.effects[34].name, 'Dark');
  assert(renderer.metadata.devices.some((device) => device.name === 'ufo'));
});

test('seed tuples reuse native handles without sharing different seeds', () => {
  const first = renderer.getRenderer(DEFAULT_SEEDS);
  const same = renderer.getRenderer({ ...DEFAULT_SEEDS });
  const different = renderer.getRenderer({ ...DEFAULT_SEEDS, Fire: 7 });
  assertEqual(first, same);
  assert(first !== different);
});

test('render results are copied out of mutable Wasm memory', () => {
  const show = { effectIndex: 13, paletteIndex: 8, controlRgb: null };
  const first = renderer.renderDevice(
    DEFAULT_SEEDS, 'scarf', show, 1000);
  const saved = new Uint8Array(first);
  renderer.renderDevice(DEFAULT_SEEDS, 'ufo', show, 2000);
  assertDeepEqual([...first], [...saved]);
});

test('initialization and normal frame rendering fit the performance budget',
  () => {
    const show = { effectIndex: 13, paletteIndex: 8, controlRgb: null };
    const devices = renderer.metadata.devices.map((device) => device.name);
    for (const device of devices) {
      renderer.renderDevice(DEFAULT_SEEDS, device, show, 1000);
    }

    const representativeIterations = 200;
    const representativeStart = globalThis.performance.now();
    for (let iteration = 0; iteration < representativeIterations;
      iteration++) {
      renderer.renderDevice(DEFAULT_SEEDS, 'scarf', show, iteration * 16);
    }
    const representativeMs =
      (globalThis.performance.now() - representativeStart) /
      representativeIterations;

    const allDeviceIterations = 20;
    const allDeviceStart = globalThis.performance.now();
    for (let iteration = 0; iteration < allDeviceIterations; iteration++) {
      for (const device of devices) {
        renderer.renderDevice(
          DEFAULT_SEEDS, device, show, iteration * 16);
      }
    }
    const allDevicesMs = (globalThis.performance.now() - allDeviceStart) /
      allDeviceIterations;

    globalThis.console.info(
      `renderer performance: init=${renderer.initializationMs.toFixed(2)}ms, ` +
      `scarf=${representativeMs.toFixed(3)}ms/frame, ` +
      `all-devices=${allDevicesMs.toFixed(3)}ms/frame`,
    );
    assert(renderer.initializationMs < 1000,
      `renderer initialization took ${renderer.initializationMs}ms`);
    assert(representativeMs < 16,
      `representative frame took ${representativeMs}ms`);
    assert(allDevicesMs < 16,
      `all-device frame took ${allDevicesMs}ms`);
  });

test('every reference case renders byte-identically through Wasm', () => {
  const failures = [];
  for (const [caseIndex, vectorCase] of vectors.cases.entries()) {
    const actual = renderer.renderDevice(
      DEFAULT_SEEDS,
      vectorCase.device,
      showFor(vectorCase),
      vectorCase.timeMs,
    );
    const expected = vectorCase.leds.flat();
    if (actual.length !== expected.length) {
      failures.push(
        `case ${caseIndex}: expected ${expected.length} bytes, ` +
        `got ${actual.length}`,
      );
      continue;
    }
    const firstBad = expected.findIndex((value, index) =>
      value !== actual[index]);
    if (firstBad >= 0) {
      failures.push(
        `case ${caseIndex} (${vectorCase.device}, t=${
          vectorCase.timeMs}) byte ${firstBad}: expected ` +
        `${expected[firstBad]}, got ${actual[firstBad]}`,
      );
    }
  }
  assert(
    failures.length === 0,
    `${failures.length}/${vectors.cases.length} cases mismatched:\n` +
      failures.slice(0, 20).join('\n'),
  );
});

test('browser source has no handwritten production renderer modules',
  async () => {
    if (typeof process === 'undefined' || !process.versions?.node) return;
    const { access, readFile, readdir } = await import('node:fs/promises');
    const obsolete = [
      '../../js/effects',
      '../../js/fastled.js',
      '../../js/perlin.js',
      '../../js/palette.js',
      '../../js/devices.js',
    ];
    for (const relative of obsolete) {
      let exists = true;
      try {
        await access(new URL(relative, import.meta.url));
      } catch {
        exists = false;
      }
      assert(!exists, `${relative} must be removed`);
    }

    const jsDirectory = new URL('../../js/', import.meta.url);
    const files = await readdir(jsDirectory);
    const rendererCopy =
      /Port of lib\/|Transcribed EXACTLY|hsv2rgbRainbow|paletteGetGradient/;
    const rendererImport =
      /from ['"].*(fastled|perlin|palette|devices|effects)/;
    for (const file of files.filter((name) => name.endsWith('.js'))) {
      const source = await readFile(new URL(file, jsDirectory), 'utf8');
      assert(!rendererCopy.test(source), `${file} copies renderer code`);
      assert(!rendererImport.test(source), `${file} imports renderer copies`);
    }
  });
