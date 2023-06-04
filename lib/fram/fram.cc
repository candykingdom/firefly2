#include "fram.h"
#include "Wire.h"

namespace fram {
constexpr uint8_t kAddress = 0xA0 >> 1;

uint8_t Write(uint8_t page, uint8_t word, const uint8_t* data, uint8_t size) {
  page &= 0b111;
  Wire.beginTransmission(kAddress | page);
  Wire.write(word);
  Wire.write(data, size);
  return Wire.endTransmission();
}

uint8_t Read(uint8_t page, uint8_t word, uint8_t *data, uint8_t size) {
  // First, set the internal address register with a 0-byte write
  page &= 0b111;
  Wire.beginTransmission(kAddress | page);
  Wire.write(word);
  Wire.endTransmission(false);

  uint8_t code = Wire.requestFrom(kAddress, size);
  Wire.readBytes(data, size);
  return code;
}

}