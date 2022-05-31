#ifndef __DEVICE_DESCRIPTION_HPP__
#define __DEVICE_DESCRIPTION_HPP__

#include <Types.hpp>
#include <list>

#include "StripDescription.hpp"

class DeviceDescription {
 public:
  /**
   * @brief How many milliamps this device can support at 5v.
   *
   * This is used as a safety mechanism to prevent the LEDs from pulling too
   * much power.
   */
  const uint32_t milliamps_supported;

  const std::list<const StripDescription*> strips;

  DeviceDescription(const uint32_t milliamps_supported,
                    const std::list<const StripDescription*> strips);
};
#endif  // __DEVICE_DESCRIPTION_HPP__
