#include "DeviceDescription.hpp"

// DeviceDescription::DeviceDescription(DeviceType type, uint8_t physicalLeds, uint8_t virtualLeds):type(type),physicalLeds(physicalLeds),virtualLeds(virtualLeds) {}

LinearDescription::LinearDescription(uint8_t physicalLeds, DeviceType deviceType)
  : DeviceDescription(type, physicalLeds, physicalLeds) {}

const uint8_t LinearDescription::PhysicalToVirtual(uint8_t pIndex) {
  return pIndex;
}
