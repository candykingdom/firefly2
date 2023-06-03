#pragma once

#include "Types.hpp"

// For DirectColor and DirectPalette, customize which colors or palettes are
// shown.
enum class SubMode {
  // Normal operation
  Normal,

  // Config step: choose which slot to change
  ChooseSlot,

  // Config step: select the item to go in the chosen slot
  ChooseItem,
};
extern SubMode sub_mode;
extern SubMode prev_sub_mode;
extern uint8_t color_carousel;
extern uint8_t palette_carousel;
extern uint8_t config_carousel;
