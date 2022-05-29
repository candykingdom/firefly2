#include "DeviceDescription.hpp"

#include <functional>
#include <numeric>

DeviceDescription::DeviceDescription(uint8_t led_count,
                                     std::list<DeviceFlag> flag_list)
    : description_version(DEVICE_DESCRIPTION_VERSION),
      led_count(led_count),
      flags(std::accumulate(flag_list.begin(), flag_list.end(), 0,
                            std::bit_or<int>())) {}

bool DeviceDescription::FlagEnabled(DeviceFlag flag) const {
  return (flags & (int)flag) == (int)flag;
}
