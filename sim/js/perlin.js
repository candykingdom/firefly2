// Byte-exact JS port of lib/math/Perlin.hpp (perlinNoise, perlinNoisePolar)
// and the MirrorIndex helper from lib/math/Math.hpp / Math.cpp.
//
// perlinNoise reseeds the shared FastLED PRNG (random16_set_seed) at render
// time, so it imports and mutates the SAME seed state exported by fastled.js
// rather than keeping its own.

import {
  cos8,
  ease8InOutApprox,
  random16SetSeed,
  random8,
  sin8,
} from './fastled.js';

// Each tile contains 2^resolution number of integers.
const RESOLUTION = 8;

function toInt8(v) {
  v &= 0xff;
  return v & 0x80 ? v - 0x100 : v;
}

function toInt16(v) {
  v &= 0xffff;
  return v & 0x8000 ? v - 0x10000 : v;
}

function tileHash(x, y) {
  return (x + y * 7919) >>> 0 & 0xffff; // uint16_t return, x/y are uint32_t
}

// lib8tion.h: lerp15by8( int16_t a, int16_t b, fract8 frac )
function lerp15by8(a, b, frac) {
  frac &= 0xff;
  let result;
  if (b > a) {
    const delta = (b - a) & 0xffff;
    const scaled = Math.floor((delta * frac) / 256) & 0xffff; // scale16by8
    result = a + scaled;
  } else {
    const delta = (a - b) & 0xffff;
    const scaled = Math.floor((delta * frac) / 256) & 0xffff; // scale16by8
    result = a - scaled;
  }
  return toInt16(result);
}

// Generates 2D perlin noise in the range [0, 256).
export function perlinNoise(x, y) {
  x = x >>> 0;
  y = y >>> 0;

  // Tile index.
  const ix = (x >>> RESOLUTION) >>> 0;
  const iy = (y >>> RESOLUTION) >>> 0;

  // Relative coordinate within tile space [0, 128) (fract16, but only the
  // low 8 bits are meaningful since resolution == 8 here).
  const fx = ((x - (ix << RESOLUTION)) << (8 - RESOLUTION)) & 0xffff;
  const fy = ((y - (iy << RESOLUTION)) << (8 - RESOLUTION)) & 0xffff;

  // Calculate vectors at each corner of the tile.
  random16SetSeed(tileHash(ix, iy));
  const aax = toInt8(random8());
  const aay = toInt8(random8());

  random16SetSeed(tileHash(ix, iy + 1));
  const abx = toInt8(random8());
  const aby = toInt8(random8());

  random16SetSeed(tileHash(ix + 1, iy));
  const bax = toInt8(random8());
  const bay = toInt8(random8());

  random16SetSeed(tileHash(ix + 1, iy + 1));
  const bbx = toInt8(random8());
  const bby = toInt8(random8());

  // Calculate dot product of point relative to each corner.
  const aa_dot = toInt16((fx * aax + fy * aay) >> 8);
  const ab_dot = toInt16((fx * abx + (-256 + fy) * aby) >> 8);
  const ba_dot = toInt16(((-256 + fx) * bax + fy * bay) >> 8);
  const bb_dot = toInt16(((-256 + fx) * bbx + (-256 + fy) * bby) >> 8);

  // Interpolate between all the corners.
  let val = toInt16(
    lerp15by8(
      lerp15by8(aa_dot, ba_dot, ease8InOutApprox(fx)),
      lerp15by8(ab_dot, bb_dot, ease8InOutApprox(fx)),
      ease8InOutApprox(fy),
    ),
  );

  // Scale and normalize noise.
  val *= 4;
  val += 128;
  if (val >= 256) {
    val = 255;
  } else if (val < 0) {
    val = 0;
  }

  return val & 0xff;
}

// Generates 2D perlin noise in the range [0, 256) given an initial cartesian
// coordinate and a polar offset.
export function perlinNoisePolar(x, y, angle, magnitude) {
  angle &= 0xff;
  magnitude &= 0xff;

  const x_offset = Math.trunc(((toInt16(cos8(angle)) - 128) * magnitude) / 128);
  const y_offset = Math.trunc(((toInt16(sin8(angle)) - 128) * magnitude) / 127);

  return perlinNoise((x + x_offset) >>> 0, (y + y_offset) >>> 0);
}

// Math.hpp: MirrorIndex(uint8_t *led_index, uint8_t *led_count) — the C++
// mutates both via pointers, so this returns [newIndex, newCount].
export function mirrorIndex(ledIndex, ledCount) {
  ledIndex &= 0xff;
  ledCount &= 0xff;

  ledIndex = ledIndex % ledCount;

  const newCount = ((ledCount + 1) / 2) | 0;

  if (ledIndex >= newCount) {
    ledIndex = (ledCount - ledIndex - 1) & 0xff;
  }

  return [ledIndex, newCount];
}
