#ifndef __COLOR_PALETTE_HPP__
#define __COLOR_PALETTE_HPP__

#include <Types.hpp>
#include <initializer_list>
#include <vector>

class ColorPalette {
 public:
  ColorPalette(std::initializer_list<CHSV> c) : colors(c) {}

  // Returns the number of colors in this palette.
  uint8_t Size() const { return colors.size(); }

  // Returns one of the colors used to create this palette by its index.
  // Note that out-of-bounds indicies are simply wrapped.
  CHSV GetColor(uint8_t index) const;

  // Returns a color created by linearly interpolating the CHSV values evenly
  // across the color space. If `wrap` is set to `true` the color at the max
  // `position` will be the same as the color at the 0th position.
  CHSV GetGradient(fract16 position, bool wrap = true) const;

 private:
  const std::vector<CHSV> colors;
};

#endif
