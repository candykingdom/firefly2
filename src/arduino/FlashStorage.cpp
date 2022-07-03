#include <FlashStorage_SAMD.h>

#include <Storage.hpp>
#include <Types.hpp>

const uint16_t EEPROM_START = 0x2000;
const uint16_t DEVICE_INDEX = EEPROM_START;

uint16_t GetDeviceIndex() {
  uint16_t index;
  EEPROM.get(DEVICE_INDEX, index);
  return index;
}

void PutDeviceIndex(uint16_t index) {
  EEPROM.put(DEVICE_INDEX, index);
  EEPROM.commit();
}