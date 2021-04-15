#ifndef __PERLIN_HPP__
#define __PERLIN_HPP__

#include "../types/Types.hpp"

// Each tile contains 2^resolution number of integers.
static const uint8_t resolution = 8;

static inline uint16_t tileHash(uint32_t x, uint32_t y) { return x + y * 7919; }

// Generates 2D perlin noise in the range [0, 256).
inline uint8_t perlinNoise(uint32_t x, uint32_t y) {
  // The lowest r "resolution" bits of the coordinates are used as relative
  // coordinate space within a tile. The remaining bits are the tile index.

  // Tile index.
  uint32_t ix = x >> resolution;
  uint32_t iy = y >> resolution;

  // Relative coordinate within tile space [0, 128).
  fract16 fx = (x - (ix << resolution)) << (8 - resolution);
  fract16 fy = (y - (iy << resolution)) << (8 - resolution);

  // Calculate vectors at each corner of the tile.
  random16_set_seed(tileHash(ix, iy));
  int8_t aax = random8();
  int8_t aay = random8();

  random16_set_seed(tileHash(ix, iy + 1));
  int8_t abx = random8();
  int8_t aby = random8();

  random16_set_seed(tileHash(ix + 1, iy));
  int8_t bax = random8();
  int8_t bay = random8();

  random16_set_seed(tileHash(ix + 1, iy + 1));
  int8_t bbx = random8();
  int8_t bby = random8();

  // Calculate dot product of point relative to each corner.
  int16_t aa_dot = (fx * (int32_t)aax + fy * (int32_t)aay) >> 8;
  int16_t ab_dot = (fx * (int32_t)abx + (-256 + fy) * (int32_t)aby) >> 8;
  int16_t ba_dot = ((-256 + fx) * (int32_t)bax + fy * (int32_t)bay) >> 8;
  int16_t bb_dot =
      ((-256 + fx) * (int32_t)bbx + (-256 + fy) * (int32_t)bby) >> 8;

  // Interpolate between all the corners.
  int16_t val = lerp15by8(lerp15by8(aa_dot, ba_dot, ease8InOutApprox(fx)),
                          lerp15by8(ab_dot, bb_dot, ease8InOutApprox(fx)),
                          ease8InOutApprox(fy));

  // Scale and normalize noise.
  val = val << 2;
  val += 128;
  if (val >= 256) {
    val = 255;
  } else if (val < 0) {
    val = 0;
  }

  return val;
}

#endif
