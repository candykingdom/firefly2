#ifndef __DEVICE_DESCRIPTION_HPP__
#define __DEVICE_DESCRIPTION_HPP__

#include <Types.hpp>

enum DeviceFlag {
  Tiny = 1 << 0,
  Bright = 1 << 1,
  Circular = 1 << 2,
};

class DeviceDescription {
 public:
  const uint8_t led_count;

  DeviceDescription(uint8_t led_count, uint8_t flags);

  bool FlagEnabled(DeviceFlag flag) const;

 private:
  const uint8_t flags;
};
#endif  // __DEVICE_DESCRIPTION_HPP__
