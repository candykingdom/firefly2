#ifndef __DEVICE_DESCRIPTION_HPP__
#define __DEVICE_DESCRIPTION_HPP__

#include <Types.hpp>
#include <vector>

#include "StripDescription.hpp"

enum class DeviceMode {
  CURRENT_FROM_HEADER,
  READ_FROM_FLASH,
  WRITE_TO_FLASH,
};

class DeviceDescription {
 public:
  /**
   * @brief How many milliamps this device can support at 5v.
   *
   * This is used as a safety mechanism to prevent the LEDs from pulling too
   * much power.
   */
  const uint32_t milliamps_supported;

  const std::vector<StripDescription> strips;

  explicit DeviceDescription(const uint32_t milliamps_supported,
                             const std::vector<StripDescription> strips);

  uint16_t GetLedCount() const;

  static constexpr uint32_t kCheckValue = 0x12345678;

  uint32_t check_value = kCheckValue;

  static constexpr uint32_t kMaxSize = 128;
};
#endif  // __DEVICE_DESCRIPTION_HPP__
