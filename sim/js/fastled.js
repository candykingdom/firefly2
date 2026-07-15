// Byte-exact JS port of FakeFastLED (build/_deps/fake-fast-led-src/src/) math
// used by the effect renderers. Ground truth is the fetched FakeFastLED
// sources, not upstream FastLED — see
// specs/001-web-simulator/research-fastled-notes.md
// for the #if branches that apply on host builds (this is the same branch set
// used here: SCALE8_C / EASE8_C plain-C bodies,
// FASTLED_SCALE8_FIXED undefined).

export const MAX_UINT8 = 255; // lib/types/Types.hpp
export const MAX_UINT16 = 65535; // lib/types/Types.hpp

function toInt8(v) {
  v &= 0xff;
  return v & 0x80 ? v - 0x100 : v;
}

function toInt16(v) {
  v &= 0xffff;
  return v & 0x8000 ? v - 0x10000 : v;
}

// --- lib8tion/scale8.h (FASTLED_SCALE8_FIXED == 0 bodies, SCALE8_C == 1) ---

export function scale8(i, scale) {
  i &= 0xff;
  scale &= 0xff;
  return (i * scale) >> 8;
}

export function scale16(i, scale) {
  i &= 0xffff;
  scale &= 0xffff;
  return Math.floor((i * scale) / 65536) & 0xffff;
}

// scale8_video (used inside hsv2rgb_rainbow's sat/val scaling paths).
function scale8Video(i, scale) {
  i &= 0xff;
  scale &= 0xff;
  const j = ((i * scale) >> 8) + (i && scale ? 1 : 0);
  return j & 0xff;
}

// --- lib8tion/math8.h ---

export function qadd8(i, j) {
  const t = (i & 0xff) + (j & 0xff);
  return t > 255 ? 255 : t;
}

// --- lib8tion/trig8.h (sin16_C / sin8_C / cos16 / cos8, the non-AVR
// bodies) ---

const SIN16_BASE = [0, 6393, 12539, 18204, 23170, 27245, 30273, 32137];
const SIN16_SLOPE = [49, 48, 44, 38, 31, 23, 14, 4];

export function sin16(theta) {
  theta &= 0xffff;
  let offset = (theta & 0x3fff) >> 3; // 0..2047
  if (theta & 0x4000) offset = 2047 - offset;

  const section = (offset >> 8) & 0xff; // 0..7 (offset / 256)
  const b = SIN16_BASE[section];
  const m = SIN16_SLOPE[section];

  const secoffset8 = (offset & 0xff) >> 1; // (uint8_t)(offset) / 2

  const mx = (m * secoffset8) & 0xffff;
  let y = mx + b; // int16_t y = mx + b (fits in int16 range for all sections)

  if (theta & 0x8000) y = -y;

  return toInt16(y);
}

export function cos16(theta) {
  return sin16(theta + 16384);
}

const B_M16_INTERLEAVE = [0, 49, 49, 41, 90, 27, 117, 10];

export function sin8(theta) {
  theta &= 0xff;
  let offset = theta;
  if (theta & 0x40) {
    offset = (255 - offset) & 0xff;
  }
  offset &= 0x3f; // 0..63

  let secoffset = offset & 0x0f; // 0..15
  if (theta & 0x40) secoffset = (secoffset + 1) & 0xff;

  const section = (offset >> 4) & 0xff; // 0..3
  const s2 = (section * 2) & 0xff;
  const b = B_M16_INTERLEAVE[s2];
  const m16 = B_M16_INTERLEAVE[s2 + 1];

  const mx = ((m16 * secoffset) >> 4) & 0xff;

  let y = toInt8((mx + b) & 0xff);
  if (theta & 0x80) y = toInt8((-y) & 0xff);

  return (y + 128) & 0xff;
}

export function cos8(theta) {
  return sin8((theta + 64) & 0xff);
}

// --- lib8tion.h (waveforms + easing, the EASE8_C bodies) ---

export function ease8InOutQuad(i) {
  i &= 0xff;
  let j = i;
  if (j & 0x80) j = 255 - j;
  const jj = scale8(j, j);
  let jj2 = (jj << 1) & 0xff;
  if (i & 0x80) jj2 = 255 - jj2;
  return jj2 & 0xff;
}

export function ease8InOutCubic(i) {
  i &= 0xff;
  const ii = scale8(i, i);
  const iii = scale8(ii, i);

  const r1 = (3 * ii - 2 * iii) & 0xffff; // uint16_t, wraps on underflow
  let result = r1 & 0xff;
  if (r1 & 0x100) result = 255;
  return result;
}

export function ease8InOutApprox(i) {
  i &= 0xff;
  if (i < 64) {
    i = (i / 2) | 0;
  } else if (i > 255 - 64) {
    i = 255 - i;
    i = (i / 2) | 0;
    i = 255 - i;
  } else {
    i -= 64;
    i += (i / 2) | 0;
    i += 32;
  }
  return i & 0xff;
}

export function triwave8(in_) {
  in_ &= 0xff;
  if (in_ & 0x80) in_ = 255 - in_;
  return (in_ << 1) & 0xff;
}

export function quadwave8(in_) {
  return ease8InOutQuad(triwave8(in_));
}

export function cubicwave8(in_) {
  return ease8InOutCubic(triwave8(in_));
}

export function lerp16by16(a, b, frac) {
  a &= 0xffff;
  b &= 0xffff;
  frac &= 0xffff;
  let result;
  if (b > a) {
    const delta = (b - a) & 0xffff;
    const scaled = scale16(delta, frac);
    result = (a + scaled) & 0xffff;
  } else {
    const delta = (a - b) & 0xffff;
    const scaled = scale16(delta, frac);
    result = (a - scaled) & 0xffff;
  }
  return result;
}

// --- lib8tion/random8.h ---
// Single shared mutable seed, same aliasing as the C global `rand16seed`
// (init RAND16_SEED = 1337, per lib8tion.cpp).

const RAND16_SEED = 1337;
let rand16seed = RAND16_SEED;

function advanceSeed() {
  rand16seed = (rand16seed * 2053 + 13849) & 0xffff;
  return rand16seed;
}

export function random16() {
  return advanceSeed();
}

export function random8() {
  const s = advanceSeed();
  return ((s & 0xff) + (s >>> 8)) & 0xff;
}

export function random8Lim(lim) {
  lim &= 0xff;
  const r = random8();
  return (r * lim) >> 8;
}

export function random16Lim(lim) {
  lim &= 0xffff;
  const r = random16();
  const p = lim * r; // uint32_t p, fits below 2^32
  return (p >>> 16) & 0xffff;
}

export function random16Range(min, lim) {
  min &= 0xffff;
  lim &= 0xffff;
  const delta = (lim - min) & 0xffff;
  return (random16Lim(delta) + min) & 0xffff;
}

export function random16SetSeed(seed) {
  rand16seed = seed & 0xffff;
}

export function random16GetSeed() {
  return rand16seed;
}

export function resetRandom16() {
  rand16seed = RAND16_SEED;
}

// --- colorutils.cpp: nblend(CHSV&, const CHSV&, fract8,
// TGradientDirectionCode) specialized to directionCode ==
// SHORTEST_HUES (used by PrideEffect). ---

export function blendHsvShortestHues(a, b, amountOfOverlay) {
  amountOfOverlay &= 0xff;

  if (amountOfOverlay === 0) {
    return { h: a.h & 0xff, s: a.s & 0xff, v: a.v & 0xff };
  }
  if (amountOfOverlay === 255) {
    return { h: b.h & 0xff, s: b.s & 0xff, v: b.v & 0xff };
  }

  const amountOfKeep = (255 - amountOfOverlay) & 0xff;

  let huedelta8 = (b.h - a.h) & 0xff;

  // directionCode == SHORTEST_HUES
  const backward = huedelta8 > 127;

  let hue;
  if (!backward) {
    hue = ((a.h & 0xff) + scale8(huedelta8, amountOfOverlay)) & 0xff;
  } else {
    huedelta8 = (-huedelta8) & 0xff;
    hue = ((a.h & 0xff) - scale8(huedelta8, amountOfOverlay)) & 0xff;
  }

  const sat = (scale8(a.s, amountOfKeep) + scale8(b.s, amountOfOverlay)) & 0xff;
  const val = (scale8(a.v, amountOfKeep) + scale8(b.v, amountOfOverlay)) & 0xff;

  return { h: hue, s: sat, v: val };
}

// --- hsv2rgb.cpp: hsv2rgb_rainbow (port EXACTLY) ---

const K255 = 255;
const K171 = 171;
const K170 = 170;
const K85 = 85;

export function hsv2rgbRainbow(hsv) {
  const hue = hsv.h & 0xff;
  const sat = hsv.s & 0xff;
  const val = hsv.v & 0xff;

  const offset = hue & 0x1f; // 0..31

  // offset8 = offset * 8 (non-AVR: plain <<3)
  const offset8 = (offset << 3) & 0xff;

  const third = scale8(offset8, Math.floor(256 / 3)); // max = 85

  let r, g, b;

  if (!(hue & 0x80)) {
    // 0XX
    if (!(hue & 0x40)) {
      // 00X
      if (!(hue & 0x20)) {
        // 000: R -> O
        r = (K255 - third) & 0xff;
        g = third;
        b = 0;
      } else {
        // 001: O -> Y (Y1 branch)
        r = K171;
        g = (K85 + third) & 0xff;
        b = 0;
      }
    } else {
      // 01X
      if (!(hue & 0x20)) {
        // 010: Y -> G (Y1 branch)
        const twothirds = scale8(offset8, Math.floor((256 * 2) / 3)); // max=170
        r = (K171 - twothirds) & 0xff;
        g = (K170 + third) & 0xff;
        b = 0;
      } else {
        // 011: G -> A
        r = 0;
        g = (K255 - third) & 0xff;
        b = third;
      }
    }
  } else {
    // 1XX
    if (!(hue & 0x40)) {
      // 10X
      if (!(hue & 0x20)) {
        // 100: A -> B
        const twothirds = scale8(offset8, Math.floor((256 * 2) / 3)); // max=170
        r = 0;
        g = (K171 - twothirds) & 0xff;
        b = (K85 + twothirds) & 0xff;
      } else {
        // 101: B -> P
        r = third;
        g = 0;
        b = (K255 - third) & 0xff;
      }
    } else {
      if (!(hue & 0x20)) {
        // 110: P -> K
        r = (K85 + third) & 0xff;
        g = 0;
        b = (K171 - third) & 0xff;
      } else {
        // 111: K -> R
        r = (K170 + third) & 0xff;
        g = 0;
        b = (K85 - third) & 0xff;
      }
    }
  }

  // Scale down colors if we're desaturated at all, and add the
  // brightness_floor to r, g, and b.
  if (sat !== 255) {
    if (sat === 0) {
      r = 255;
      g = 255;
      b = 255;
    } else {
      let desat = (255 - sat) & 0xff;
      desat = scale8Video(desat, desat);

      const satscale = (255 - desat) & 0xff;

      if (r) r = (scale8(r, satscale) + 1) & 0xff;
      if (g) g = (scale8(g, satscale) + 1) & 0xff;
      if (b) b = (scale8(b, satscale) + 1) & 0xff;

      const brightness_floor = desat;
      r = (r + brightness_floor) & 0xff;
      g = (g + brightness_floor) & 0xff;
      b = (b + brightness_floor) & 0xff;
    }
  }

  // Now scale everything down if we're at value < 255.
  if (val !== 255) {
    const scaledVal = scale8Video(val, val);
    if (scaledVal === 0) {
      r = 0;
      g = 0;
      b = 0;
    } else {
      if (r) r = (scale8(r, scaledVal) + 1) & 0xff;
      if (g) g = (scale8(g, scaledVal) + 1) & 0xff;
      if (b) b = (scale8(b, scaledVal) + 1) & 0xff;
    }
  }

  return { r, g, b };
}

// Port of Effect::FlattenedGradientRGB (lib/effect/Effect.cpp).
export function flattenedGradientRGB(color) {
  const rgb = hsv2rgbRainbow(color);
  const sum = rgb.r + rgb.g + rgb.b;
  const reference = hsv2rgbRainbow({ h: 0, s: color.s, v: color.v });
  const referenceSum = reference.r + reference.g + reference.b;
  if (sum > referenceSum) {
    rgb.r = Math.trunc((rgb.r * referenceSum) / sum);
    rgb.g = Math.trunc((rgb.g * referenceSum) / sum);
    rgb.b = Math.trunc((rgb.b * referenceSum) / sum);
  }
  return rgb;
}

// --- pixeltypes.h: CRGB operator/(uint8_t) — per-channel integer division ---

export function rgbDiv(rgb, d) {
  return {
    r: (rgb.r / d) | 0,
    g: (rgb.g / d) | 0,
    b: (rgb.b / d) | 0,
  };
}
