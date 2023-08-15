#include "StripDescription.hpp"

#include <functional>
#include <numeric>

StripDescription::StripDescription(uint16_t led_count,
                                   std::vector<StripFlag> flag_list)
    : led_count(led_count),
      flags(std::accumulate(flag_list.begin(), flag_list.end(), 0,
                            std::bit_or<int>())) {}

bool StripDescription::FlagEnabled(StripFlag flag) const {
  return (flags & (int)flag) == (int)flag;
}
