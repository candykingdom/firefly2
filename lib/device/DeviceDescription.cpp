#include "DeviceDescription.hpp"

#include <functional>
#include <numeric>
#include <vector>

DeviceDescription::DeviceDescription(
    uint32_t milliamps_supported,
    const std::vector<const StripDescription*> strips)
    : milliamps_supported(milliamps_supported), strips(strips) {}
