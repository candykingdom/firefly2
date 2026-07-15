#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

constexpr uint8_t ButtonRowToSlot(uint8_t row, bool right_side) {
  return row * 2 + (right_side ? 1 : 0);
}

constexpr uint8_t PreviousPaletteIndex(uint8_t current, uint8_t palette_count) {
  return palette_count == 0 ? current
                            : (current + palette_count - 1) % palette_count;
}

template <size_t Rows, size_t Columns>
bool PaletteIndicesAreValid(
    const std::array<std::array<uint8_t, Columns>, Rows> &palettes,
    uint8_t palette_count) {
  if (palette_count == 0) {
    return false;
  }
  for (const auto &row : palettes) {
    for (uint8_t palette : row) {
      if (palette >= palette_count) {
        return false;
      }
    }
  }
  return true;
}
