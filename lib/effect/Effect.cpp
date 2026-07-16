#include "Effect.hpp"

#include <cassert>
#include <cstring>

Effect::Effect() {}

uint8_t Effect::GetThresholdSin(int16_t x, uint8_t threshold) const {
  int16_t val = sin16(x);
  int16_t shifted_val = val / 128;
  if (shifted_val < threshold) {
    return 0;
  } else {
    return (shifted_val - threshold) * 256 / (256 - threshold);
  }
}

CRGB Effect::FlattenedGradientRGB(const CHSV &color) const {
  CRGB rgb;
  hsv2rgb_rainbow(color, rgb);
  const uint16_t sum = rgb.r + rgb.g + rgb.b;

  // Reference drive: what an un-boosted hue (red) outputs at this
  // saturation and value.
  CRGB reference;
  hsv2rgb_rainbow(CHSV(HUE_RED, color.s, color.v), reference);
  const uint16_t reference_sum = reference.r + reference.g + reference.b;

  if (sum > reference_sum) {
    rgb.r = (uint32_t)rgb.r * reference_sum / sum;
    rgb.g = (uint32_t)rgb.g * reference_sum / sum;
    rgb.b = (uint32_t)rgb.b * reference_sum / sum;
  }
  return rgb;
}

// If this is a simple static variable, then it might not be initialized before
// it's used, static variable initialization order is undefined. This method
// ensures that it is initialized when it's used.
const std::vector<ColorPalette> &Effect::palettes() {
  static const std::vector<ColorPalette> palettes = {
      // Solid color
      {{HUE_RED, 255, 255}},
      {{HUE_ORANGE, 255, 255}},
      {{HUE_YELLOW, 255, 255}},
      {{HUE_GREEN, 255, 255}},
      {{HUE_AQUA, 255, 255}},
      {{HUE_BLUE, 255, 255}},
      {{HUE_PURPLE, 255, 255}},
      {{HUE_PINK, 255, 255}},

      // Rainbow
      {{HUE_RED, 255, 255}, {HUE_GREEN, 255, 255}, {HUE_BLUE, 255, 255}},
      // Warm
      {{HUE_RED, 255, 255}, {HUE_PURPLE, 255, 255}},
      // Cool
      {{HUE_GREEN, 255, 255}, {HUE_BLUE, 255, 255}},
      // Yellow-green
      {{HUE_YELLOW, 255, 255}, {HUE_AQUA, 255, 255}},
      // 80s Miami
      {{HUE_PURPLE, 255, 255}, {HUE_ORANGE, 255, 255}},
      // Vaporwave
      // https://i.redd.it/aepphltiqy911.png
      {{33, 241, 249}, {247, 188, 255}, {201, 225, 160}, {153, 251, 150}},
      // Cool, Formerly Popo but antifa got to them.
      {{HUE_AQUA, 0, 255}, {HUE_BLUE, 255, 255}},
      // Candy-cane
      {{HUE_RED, 255, 255}, {HUE_RED, 0, 255}},
      // Winter-mint candy-cane
      {{HUE_AQUA, 255, 255}, {HUE_AQUA, 0, 255}},
      // Fire
      {{HUE_RED, 255, 255}, {HUE_ORANGE, 255, 255}, {HUE_YELLOW, 255, 255}},
      // Pastel rainbow
      {{HUE_RED, 127, 192}, {HUE_GREEN, 127, 192}, {HUE_BLUE, 127, 192}},
      // Jazz cup - teal, purple, white
      {{132, 255, 255}, {192, 255, 255}, {0, 0, 200}},
      // Yellow and double purp
      {{HUE_PURPLE, 255, 255}, {HUE_YELLOW, 255, 255}, {HUE_PURPLE, 255, 255}},
      // Double rainbow (makes effects that depend on the number of colors, like
      // swinging lights, do cool things)
      {{HUE_RED, 255, 255},
       {HUE_GREEN, 255, 255},
       {HUE_BLUE, 255, 255},
       {HUE_RED, 255, 255},
       {HUE_GREEN, 255, 255},
       {HUE_BLUE, 255, 255}},
  };
#ifndef ARDUINO
  assert(palettes.size() == palette_names().size());
#endif
  return palettes;
}

const std::vector<const char *> &Effect::palette_names() {
  static const std::vector<const char *> names = {
      "Red",
      "Orange",
      "Yellow",
      "Green",
      "Aqua",
      "Blue",
      "Purple",
      "Pink",
      "Rainbow",
      "Warm",
      "Cool",
      "Yellow-Green",
      "80s Miami",
      "Vaporwave",
      "Cool Popo",
      "Candy Cane",
      "Winter Mint",
      "Fire",
      "Pastel Rainbow",
      "Jazz Cup",
      "Yellow & Double Purp",
      "Double Rainbow",
  };
#ifndef ARDUINO
  static const bool names_are_valid = [] {
    for (size_t i = 0; i < names.size(); ++i) {
      assert(names[i] != nullptr && names[i][0] != '\0');
      for (size_t j = i + 1; j < names.size(); ++j) {
        assert(std::strcmp(names[i], names[j]) != 0);
      }
    }
    return true;
  }();
  (void)names_are_valid;
#endif
  return names;
}
