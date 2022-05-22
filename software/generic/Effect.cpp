#include "Effect.hpp"

Effect::Effect(DeviceDescription *const device): device(device) {}

uint8_t Effect::GetThresholdSin(int16_t x, uint8_t threshold) {
  int16_t val = sin16(x);
  int16_t shiftedVal = val / 128;
  if (shiftedVal < threshold) {
    return 0;
  } else {
    return (shiftedVal - threshold) * 256 / (256 - threshold);
  }
}

const std::vector<ColorPalette> Effect::palettes = {
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
    // Popo
    {{HUE_RED, 255, 255}, {HUE_BLUE, 255, 255}},
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
