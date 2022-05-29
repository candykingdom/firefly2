#ifndef __DEVICE_DESCRIPTION_HPP__
#define __DEVICE_DESCRIPTION_HPP__

#include <list>

#include "../types/Types.hpp"

/**
 * @brief The version number of the current DeviceDescription schema.
 *
 * This is intended to help with deserializing and need only be updated when
 * breaking changes to the DeviceDescription object are made.
 */
const uint16_t DEVICE_DESCRIPTION_VERSION = 1;

enum DeviceFlag {
  Tiny = 1 << 0,
  Bright = 1 << 1,
  Circular = 1 << 2,
  Mirrored = 1 << 3,
};

class DeviceDescription {
 public:
  const uint16_t description_version;

  const uint8_t led_count;

  DeviceDescription(uint8_t led_count, std::list<DeviceFlag> flags);

  bool FlagEnabled(DeviceFlag flag) const;

 private:
  const uint8_t flags;
};
#endif  // __DEVICE_DESCRIPTION_HPP__
