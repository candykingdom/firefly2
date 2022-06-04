#include "ColorPalette.hpp"

CHSV ColorPalette::GetColor(uint8_t index) const {
  return colors[index % colors.size()];
}

CHSV ColorPalette::GetGradient(fract16 position, bool wrap) const {
  if (colors.size() == 0) {
    return {0, 0, 0};
  } else if (colors.size() == 1) {
    return colors[0];
  }

  uint8_t size;
  if (wrap) {
    size = colors.size();
  } else {
    size = colors.size() - 1;
  }

  const uint16_t color_range = MAX_UINT16 / size;
  uint8_t index = position / color_range;
  fract16 t =
      (position - (uint32_t)index * color_range) * MAX_UINT16 / color_range;

  CHSV start = GetColor(index);
  if (position % color_range == 0) {
    return start;
  }
  CHSV finish = GetColor(index + 1);
  CHSV result(0, 0, 0);

  if (abs(start.h - finish.h) < MAX_UINT8 >> 1) {
    result.h = lerp16by16(start.h, finish.h, t);
  } else {
    uint16_t hue;
    if (start.h < finish.h) {
      hue = lerp16by16(((uint16_t)start.h) + MAX_UINT8, finish.h, t);
    } else {
      hue = lerp16by16(start.h, ((uint16_t)finish.h) + MAX_UINT8, t);
    }
    if (hue > MAX_UINT8) {
      hue -= MAX_UINT8;
    }
    result.h = hue;
  }

  result.s = lerp16by16(start.s, finish.s, t);
  result.v = lerp16by16(start.v, finish.v, t);

  return result;
}
