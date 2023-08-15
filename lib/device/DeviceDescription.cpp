#include "DeviceDescription.hpp"

#include <functional>
#include <numeric>
#include <vector>

DeviceDescription::DeviceDescription(uint32_t milliamps_supported,
                                     const std::vector<StripDescription> strips)
    : milliamps_supported(milliamps_supported), strips(strips) {}

uint16_t DeviceDescription::GetLedCount() const {
  uint16_t led_count = 0;
  for (const StripDescription& strip : strips) {
    led_count += strip.led_count;
  }
  return led_count;
}
