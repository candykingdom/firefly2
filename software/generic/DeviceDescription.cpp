#include "DeviceDescription.hpp"

DeviceDescription::DeviceDescription(uint8_t led_count, uint8_t flags)
    : led_count(led_count), flags(flags) {}

bool DeviceDescription::FlagEnabled(DeviceFlag flag) const {
  return (flags & (int)flag) == (int)flag;
}
