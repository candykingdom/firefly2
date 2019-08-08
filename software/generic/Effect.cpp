#include "Effect.hpp"

Effect::Effect(uint8_t numLeds) : numLeds(numLeds) {}

uint8_t Effect::GetThresholdSin(int16_t x, uint8_t threshold) {
  int16_t val = sin16(x);
  int16_t shiftedVal = val / 128;
  if (shiftedVal < threshold) {
    return 0;
  } else {
    return (shiftedVal - threshold) * 256 / (256 - threshold);
  }
}
