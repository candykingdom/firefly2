#pragma once

#include <cstdint>

constexpr uint8_t ButtonRowToSlot(uint8_t row, bool right_side) {
  return row * 2 + (right_side ? 1 : 0);
}

constexpr uint8_t PreviousPaletteIndex(uint8_t current, uint8_t palette_count) {
  return palette_count == 0 ? current
                            : (current + palette_count - 1) % palette_count;
}
