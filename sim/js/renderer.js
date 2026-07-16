const initializationStartedAt = globalThis.performance.now();
const EXPECTED_ABI = 1;

export const DEFAULT_SEEDS = Object.freeze({
  Fire: 6198,
  Firefly: 423,
  Rorschach: 24359,
});

export const FLAG_NAMES = Object.freeze([
  'Tiny',
  'Bright',
  'Circular',
  'Mirrored',
  'Reversed',
  'Controller',
  'Dim',
  'Off',
]);

let wasm;
try {
  const generated = await import('../generated/firefly-renderer.js');
  wasm = await generated.default();
} catch (cause) {
  throw new Error(
    'Firefly shared renderer failed to initialize. The committed WebAssembly ' +
      'artifact may be missing or incompatible.',
    { cause },
  );
}

const abiVersion = wasm._ff_sim_abi_version();
if (abiVersion !== EXPECTED_ABI) {
  throw new Error(
    `Firefly renderer ABI mismatch: expected ${EXPECTED_ABI}, got ${
      abiVersion}`,
  );
}

function decodeString(pointer, label) {
  if (pointer === 0) {
    throw new Error(`Firefly renderer returned no string for ${label}`);
  }
  return wasm.UTF8ToString(pointer);
}

function expandFlags(bits) {
  return FLAG_NAMES.filter((_, bit) => (bits & (1 << bit)) !== 0);
}

function loadEffects() {
  return [...Array(wasm._ff_sim_effect_count())].map((_, index) => ({
    index,
    name: decodeString(wasm._ff_sim_effect_name(index), `effect ${index}`),
    weight: wasm._ff_sim_effect_weight(index),
  }));
}

function loadPalettes() {
  return [...Array(wasm._ff_sim_palette_count())].map((_, index) => {
    const colors = [...Array(wasm._ff_sim_palette_color_count(index))]
      .map((__, colorIndex) => {
        const packed = wasm._ff_sim_palette_color_hsv(index, colorIndex);
        return {
          h: packed & 0xff,
          s: (packed >>> 8) & 0xff,
          v: (packed >>> 16) & 0xff,
        };
      });
    return {
      index,
      name: decodeString(
        wasm._ff_sim_palette_name(index), `palette ${index}`),
      colors,
    };
  });
}

function loadDevices() {
  return [...Array(wasm._ff_sim_device_count())].map((_, index) => {
    const strips = [...Array(wasm._ff_sim_device_strip_count(index))]
      .map((__, stripIndex) => ({
        ledCount: wasm._ff_sim_strip_led_count(index, stripIndex),
        flags: expandFlags(wasm._ff_sim_strip_flags(index, stripIndex)),
      }));
    return {
      index,
      name: decodeString(wasm._ff_sim_device_name(index), `device ${index}`),
      milliamps: wasm._ff_sim_device_milliamps(index),
      strips,
    };
  });
}

const metadata = Object.freeze({
  effects: Object.freeze(loadEffects()),
  palettes: Object.freeze(loadPalettes()),
  devices: Object.freeze(loadDevices()),
  randomEffectCount: wasm._ff_sim_random_effect_count(),
});

const devicesByName = new Map(
  metadata.devices.map((device) => [device.name, device]),
);
const rendererHandles = new Map();

function normalizeSeeds(seeds = DEFAULT_SEEDS) {
  return {
    Fire: Number(seeds.Fire ?? DEFAULT_SEEDS.Fire) & 0xffff,
    Firefly: Number(seeds.Firefly ?? DEFAULT_SEEDS.Firefly) >>> 0,
    Rorschach:
      Number(seeds.Rorschach ?? DEFAULT_SEEDS.Rorschach) & 0xffff,
  };
}

function getRenderer(seeds = DEFAULT_SEEDS) {
  const normalized = normalizeSeeds(seeds);
  const key = `${normalized.Fire}:${normalized.Firefly}:` +
    `${normalized.Rorschach}`;
  if (!rendererHandles.has(key)) {
    const handle = wasm._ff_sim_renderer_create(
      normalized.Fire,
      normalized.Firefly,
      normalized.Rorschach,
    );
    if (handle === 0) {
      throw new Error(`Firefly renderer could not create seed tuple ${key}`);
    }
    rendererHandles.set(key, handle);
  }
  return rendererHandles.get(key);
}

function packControl(controlRgb) {
  if (controlRgb === null || controlRgb === undefined) {
    return { active: 0, rgb: 0 };
  }
  const values = Array.isArray(controlRgb) ? controlRgb : [
    controlRgb.r,
    controlRgb.g,
    controlRgb.b,
  ];
  return {
    active: 1,
    rgb: (values[0] & 0xff) |
      ((values[1] & 0xff) << 8) |
      ((values[2] & 0xff) << 16),
  };
}

function packFlags(flags = []) {
  const enabled = flags instanceof Set ? flags : new Set(flags);
  return FLAG_NAMES.reduce((bits, name, bit) =>
    enabled.has(name) ? bits | (1 << bit) : bits, 0);
}

function copyOutput(handle, status, operation) {
  if (status !== 0) {
    throw new Error(`${operation} failed with renderer status ${status}`);
  }
  const size = wasm._ff_sim_render_size(handle);
  if (size === 0) return new Uint8Array();
  const pointer = wasm._ff_sim_render_data(handle);
  if (pointer === 0) {
    throw new Error(`${operation} returned ${size} bytes without data`);
  }
  return wasm.HEAPU8.slice(pointer, pointer + size);
}

function renderDevice(seeds, deviceName, show, timeMs) {
  const device = devicesByName.get(deviceName);
  if (!device) {
    throw new Error(
      `Unknown device "${deviceName}". Valid names: ` +
      `${metadata.devices.map((item) => item.name).join(', ')}`,
    );
  }
  const handle = getRenderer(seeds);
  const control = packControl(show.controlRgb);
  const status = wasm._ff_sim_render_device(
    handle,
    device.index,
    show.effectIndex >>> 0,
    show.paletteIndex >>> 0,
    timeMs >>> 0,
    control.active,
    control.rgb,
  );
  return copyOutput(handle, status, `render device "${deviceName}"`);
}

function renderStrip(seeds, strip, show, timeMs) {
  const handle = getRenderer(seeds);
  const control = packControl(show.controlRgb);
  const status = wasm._ff_sim_render_strip(
    handle,
    strip.ledCount >>> 0,
    packFlags(strip.flags),
    show.effectIndex >>> 0,
    show.paletteIndex >>> 0,
    timeMs >>> 0,
    control.active,
    control.rgb,
  );
  return copyOutput(handle, status, 'render custom strip');
}

const renderer = Object.freeze({
  abiVersion,
  initializationMs: globalThis.performance.now() - initializationStartedAt,
  metadata,
  getRenderer,
  renderDevice,
  renderStrip,
});

export { metadata };
export default renderer;
