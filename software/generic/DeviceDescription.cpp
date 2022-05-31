#include "DeviceDescription.hpp"

#include <functional>
#include <numeric>

DeviceDescription::DeviceDescription(
    uint32_t milliamps_supported,
    const std::list<const StripDescription*> strips)
    : milliamps_supported(milliamps_supported), strips(strips) {}
