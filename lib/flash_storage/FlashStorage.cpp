#include <FlashStorage_SAMD.h>

#include "../storage/Storage.hpp"

const int DD_LOCATION = 0x0;

bool GetDeviceDescription(const DeviceDescription* const device) {
  EEPROM.get(DD_LOCATION, device);

  return device->description_version == DEVICE_DESCRIPTION_VERSION;
}

bool PutDeviceDescription(const DeviceDescription* device) {
  EEPROM.put(DD_LOCATION, device);

  return true;
}