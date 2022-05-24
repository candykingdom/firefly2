#include "DeviceDescription.hpp"

DeviceDescription::DeviceDescription(DeviceType type, uint8_t physical_leds,
                                     uint8_t virtual_leds)
    : type(type), physical_leds(physical_leds), virtual_leds(virtual_leds) {}

LinearDescription::LinearDescription(uint8_t physical_leds,
                                     DeviceType device_type)
    : DeviceDescription(type, physical_leds, physical_leds) {}

uint8_t LinearDescription::PhysicalToVirtual(uint8_t p_index) const {
  return p_index;
}
