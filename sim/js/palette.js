// Port of lib/color/ColorPalette.{hpp,cpp}, lib/effect/Effect.cpp (lines 5-67),
// and lib/effect/Effect.hpp (palette table + GetThresholdSin).

import { sin16, lerp16by16, MAX_UINT8, MAX_UINT16 } from './fastled.js';

// HSHue constants from FakeFastLED pixeltypes.h (HSVHue enum).
const HUE_RED = 0;
const HUE_ORANGE = 32;
const HUE_YELLOW = 64;
const HUE_GREEN = 96;
const HUE_AQUA = 128;
const HUE_BLUE = 160;
const HUE_PURPLE = 192;
const HUE_PINK = 224;

function chsv(h, s, v) {
  return { h, s, v };
}

// Transcribed EXACTLY from Effect::palettes() in lib/effect/Effect.cpp, in
// order.
export const PALETTES = [
  // Solid color
  { name: 'Red', colors: [chsv(HUE_RED, 255, 255)] },
  { name: 'Orange', colors: [chsv(HUE_ORANGE, 255, 255)] },
  { name: 'Yellow', colors: [chsv(HUE_YELLOW, 255, 255)] },
  { name: 'Green', colors: [chsv(HUE_GREEN, 255, 255)] },
  { name: 'Aqua', colors: [chsv(HUE_AQUA, 255, 255)] },
  { name: 'Blue', colors: [chsv(HUE_BLUE, 255, 255)] },
  { name: 'Purple', colors: [chsv(HUE_PURPLE, 255, 255)] },
  { name: 'Pink', colors: [chsv(HUE_PINK, 255, 255)] },

  // Rainbow
  {
    name: 'Rainbow',
    colors: [
      chsv(HUE_RED, 255, 255),
      chsv(HUE_GREEN, 255, 255),
      chsv(HUE_BLUE, 255, 255),
    ],
  },
  // Warm
  {
    name: 'Warm',
    colors: [chsv(HUE_RED, 255, 255), chsv(HUE_PURPLE, 255, 255)],
  },
  // Cool
  {
    name: 'Cool',
    colors: [chsv(HUE_GREEN, 255, 255), chsv(HUE_BLUE, 255, 255)],
  },
  // Yellow-green
  {
    name: 'Yellow-Green',
    colors: [chsv(HUE_YELLOW, 255, 255), chsv(HUE_AQUA, 255, 255)],
  },
  // 80s Miami
  {
    name: '80s Miami',
    colors: [chsv(HUE_PURPLE, 255, 255), chsv(HUE_ORANGE, 255, 255)],
  },
  // Vaporwave
  // https://i.redd.it/aepphltiqy911.png
  {
    name: 'Vaporwave',
    colors: [
      chsv(33, 241, 249),
      chsv(247, 188, 255),
      chsv(201, 225, 160),
      chsv(153, 251, 150),
    ],
  },
  // Cool, Formerly Popo but antifa got to them.
  {
    name: 'Cool Popo',
    colors: [chsv(HUE_AQUA, 0, 255), chsv(HUE_BLUE, 255, 255)],
  },
  // Candy-cane
  {
    name: 'Candy Cane',
    colors: [chsv(HUE_RED, 255, 255), chsv(HUE_RED, 0, 255)],
  },
  // Winter-mint candy-cane
  {
    name: 'Winter Mint',
    colors: [chsv(HUE_AQUA, 255, 255), chsv(HUE_AQUA, 0, 255)],
  },
  // Fire
  {
    name: 'Fire',
    colors: [
      chsv(HUE_RED, 255, 255),
      chsv(HUE_ORANGE, 255, 255),
      chsv(HUE_YELLOW, 255, 255),
    ],
  },
  // Pastel rainbow
  {
    name: 'Pastel Rainbow',
    colors: [
      chsv(HUE_RED, 127, 192),
      chsv(HUE_GREEN, 127, 192),
      chsv(HUE_BLUE, 127, 192),
    ],
  },
  // Jazz cup - teal, purple, white
  {
    name: 'Jazz Cup',
    colors: [chsv(132, 255, 255), chsv(192, 255, 255), chsv(0, 0, 200)],
  },
  // Yellow and double purp
  {
    name: 'Yellow & Double Purp',
    colors: [
      chsv(HUE_PURPLE, 255, 255),
      chsv(HUE_YELLOW, 255, 255),
      chsv(HUE_PURPLE, 255, 255),
    ],
  },
  // Double rainbow (makes effects that depend on the number of colors, like
  // swinging lights, do cool things)
  {
    name: 'Double Rainbow',
    colors: [
      chsv(HUE_RED, 255, 255),
      chsv(HUE_GREEN, 255, 255),
      chsv(HUE_BLUE, 255, 255),
      chsv(HUE_RED, 255, 255),
      chsv(HUE_GREEN, 255, 255),
      chsv(HUE_BLUE, 255, 255),
    ],
  },
];

// Port of ColorPalette wire-byte tolerance: palette indices come off the
// radio as a single byte, so out-of-range values wrap.
export function getPalette(index) {
  return PALETTES[index % PALETTES.length];
}

// Port of ColorPalette::GetColor. C++ returns CHSV by value, so callers are
// free to mutate the result — always hand out a fresh copy, never a
// reference into the shared PALETTES table.
export function paletteGetColor(palette, index) {
  const colors = palette.colors;
  const color = colors[index % colors.length];
  return { h: color.h, s: color.s, v: color.v };
}

// Port of ColorPalette::GetGradient (lib/color/ColorPalette.cpp).
export function paletteGetGradient(palette, fract16Position, wrap = true) {
  const colors = palette.colors;
  if (colors.length === 0) {
    return { h: 0, s: 0, v: 0 };
  } else if (colors.length === 1) {
    // By-value semantics: see paletteGetColor.
    return { h: colors[0].h, s: colors[0].s, v: colors[0].v };
  }

  const position = fract16Position & 0xffff;

  let size;
  if (wrap) {
    size = colors.length;
  } else {
    size = colors.length - 1;
  }

  const colorRange = (MAX_UINT16 / size) | 0;
  const index = (position / colorRange) | 0;
  const t = (((position - index * colorRange) * MAX_UINT16) / colorRange) | 0;

  const start = paletteGetColor(palette, index);
  if (position % colorRange === 0) {
    return start;
  }
  const finish = paletteGetColor(palette, index + 1);
  const result = { h: 0, s: 0, v: 0 };

  if (Math.abs(start.h - finish.h) < MAX_UINT8 >> 1) {
    result.h = lerp16by16(start.h, finish.h, t) & 0xff;
  } else {
    let hue;
    if (start.h < finish.h) {
      hue = lerp16by16(start.h + MAX_UINT8, finish.h, t);
    } else {
      hue = lerp16by16(start.h, finish.h + MAX_UINT8, t);
    }
    if (hue > MAX_UINT8) {
      hue -= MAX_UINT8;
    }
    result.h = hue & 0xff;
  }

  result.s = lerp16by16(start.s, finish.s, t) & 0xff;
  result.v = lerp16by16(start.v, finish.v, t) & 0xff;

  return result;
}

// Port of Effect::GetThresholdSin (lib/effect/Effect.cpp:5-13).
export function getThresholdSin(x, threshold) {
  const wrappedX = (x << 16) >> 16;
  const val = sin16(wrappedX);
  const shiftedVal = Math.trunc(val / 128);
  let result;
  if (shiftedVal < threshold) {
    result = 0;
  } else {
    result = Math.trunc(((shiftedVal - threshold) * 256) / (256 - threshold));
  }
  return result & 0xff;
}
