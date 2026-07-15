// Port of lib/device/StripDescription.hpp, lib/device/DeviceDescription.hpp,
// and lib/device/Devices.hpp.

// Flag names in enum declaration order (StripDescription.hpp). Numeric
// values (bit position) are not exposed; only Set-of-name membership matters
// downstream.
export const FLAGS = [
  'Tiny',
  'Bright',
  'Circular',
  'Mirrored',
  'Reversed',
  'Controller',
  'Dim',
  'Off',
];

export function makeStrip(ledCount, flags = []) {
  const flagSet = new Set(flags);
  return {
    ledCount,
    flags: flagSet,
    hasFlag(name) {
      return flagSet.has(name);
    },
  };
}

const RF_BOARD_MA_SUPPORTED = 2400 - 50;

function simpleRfBoard(name, ledCount, flags = []) {
  return {
    name,
    milliamps: RF_BOARD_MA_SUPPORTED,
    strips: [makeStrip(ledCount, flags)],
  };
}

function multiStripRfBoard(name, stripSpecs) {
  return {
    name,
    milliamps: RF_BOARD_MA_SUPPORTED,
    strips: stripSpecs.map(([ledCount, flags]) => makeStrip(ledCount, flags)),
  };
}

// Transcribed EXACTLY from lib/device/Devices.hpp, in order.
export const DEVICES = {
  bike: simpleRfBoard('bike', 30, ['Bright']),
  ben_s_bike: simpleRfBoard('ben_s_bike', 28, ['Bright']),
  will_bike: simpleRfBoard('will_bike', 63, ['Bright']),
  scarf: simpleRfBoard('scarf', 46, []),
  lantern: simpleRfBoard('lantern', 5, ['Tiny']),
  puck: simpleRfBoard('puck', 12, ['Tiny', 'Circular']),
  two_side_puck: simpleRfBoard('two_side_puck', 24, [
    'Tiny',
    'Circular',
    'Mirrored',
  ]),
  rainbow_cloak: multiStripRfBoard('rainbow_cloak', [
    [11, ['Tiny', 'Circular']],
    [94, []],
    [11, ['Tiny', 'Circular', 'Reversed']],
  ]),
  backpack_tail: simpleRfBoard('backpack_tail', 11, []),
  dan_jacket: simpleRfBoard('dan_jacket', 60, []),
  will_jacket: simpleRfBoard('will_jacket', 56, []),
  will_bike_front: simpleRfBoard('will_bike_front', 27, ['Circular']),
  will_top_hat: simpleRfBoard('will_top_hat', 50, ['Circular']),
  bike_front: simpleRfBoard('bike_front', 18, ['Circular']),
  hex_light: simpleRfBoard('hex_light', 12, ['Circular', 'Tiny']),
  half_matrix_panel: multiStripRfBoard('half_matrix_panel', [
    [16, []],
    [16, ['Reversed']],
    [16, []],
    [16, ['Reversed']],
    [16, []],
    [16, ['Reversed']],
    [16, []],
    [16, ['Reversed']],
  ]),
  backpack_rope: multiStripRfBoard('backpack_rope', [
    [96, ['Dim']],
    [96, ['Dim', 'Reversed']],
  ]),
  ufo: multiStripRfBoard('ufo', [
    [12, ['Circular']], // Top Circle
    [16, ['Circular']], // Bottom Circle
    [12, ['Bright', 'Circular']], // Bottom Lights
    [52, ['Dim', 'Circular']], // Rim
  ]),
  brooke_bike: multiStripRfBoard('brooke_bike', [
    [15, ['Circular']],
    [19, []],
    [16, ['Circular']],
    [33, ['Bright']],
  ]),
  ross_backpack: multiStripRfBoard('ross_backpack', [
    [13, ['Reversed']],
    [12, []],
    [13, ['Reversed']],
    [12, []],
  ]),
  whatever: simpleRfBoard('whatever', 18, ['Circular']),
  will_backpack: multiStripRfBoard('will_backpack', [
    [96, []],
    [96, ['Reversed']],
  ]),
};

export function getDevice(name) {
  const device = DEVICES[name];
  if (!device) {
    throw new Error(
      `Unknown device "${name}". Valid names: ` +
        `${Object.keys(DEVICES).join(', ')}`,
    );
  }
  return device;
}

export function totalLeds(device) {
  return device.strips.reduce((sum, strip) => sum + strip.ledCount, 0);
}
