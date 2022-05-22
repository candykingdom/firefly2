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
  const uint8_t physicalLeds;
  const uint8_t virtualLeds;

  virtual const uint8_t PhysicalToVirtual(uint8_t pIndex) = 0;

 protected:
  DeviceDescription(DeviceType type, uint8_t physicalLeds, uint8_t virtualLeds)
    : type(type), physicalLeds(physicalLeds), virtualLeds(virtualLeds) {}
};

class LinearDescription: public DeviceDescription {
 public:
  LinearDescription(uint8_t physicalLeds, DeviceType deviceType);

  const uint8_t PhysicalToVirtual(uint8_t pIndex);
};
#endif
