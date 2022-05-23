#ifndef __DEVICE_DESCRIPTION_HPP__
#define __DEVICE_DESCRIPTION_HPP__

#include <Types.hpp>

enum class DeviceType {
  Wearable,
  Bike,
};

class DeviceDescription {
 public:
  const DeviceType type;
  const uint8_t physical_leds;
  const uint8_t virtual_leds;

  virtual uint8_t PhysicalToVirtual(uint8_t p_index) const = 0;

 protected:
  DeviceDescription(DeviceType type, uint8_t physical_leds, uint8_t virtual_leds);
};

class CircularDescription: public DeviceDescription {
 public:
  CircularDescription(uint8_t physical_leds, DeviceType device_type);

  uint8_t PhysicalToVirtual(uint8_t p_index) const;
};

class LinearDescription: public DeviceDescription {
 public:
  LinearDescription(uint8_t physical_leds, DeviceType device_type);

  uint8_t PhysicalToVirtual(uint8_t p_index) const;
};
#endif // __DEVICE_DESCRIPTION_HPP__
