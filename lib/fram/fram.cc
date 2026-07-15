#include "fram.h"

#include "Wire.h"

namespace fram {

namespace {

constexpr uint8_t kMaxWriteChunk = BUFFER_LENGTH - 1;
constexpr uint8_t kMaxReadChunk = BUFFER_LENGTH;

static_assert(kMaxWriteChunk > 0, "Wire buffer must fit an address byte");

}  // namespace

bool Write(uint8_t page, uint8_t word, const uint8_t *data, uint16_t size) {
  if (!RangeIsValid(page, word, size)) {
    return false;
  }

  uint16_t address = LinearAddress(page, word);
  uint16_t offset = 0;
  while (offset < size) {
    const uint8_t chunk = ChunkSize(address, size - offset, kMaxWriteChunk);
    Wire.beginTransmission(DeviceAddress(address));
    if (Wire.write(WordAddress(address)) != 1 ||
        Wire.write(data + offset, chunk) != chunk) {
      Wire.endTransmission();
      return false;
    }
    if (Wire.endTransmission() != 0) {
      return false;
    }
    address += chunk;
    offset += chunk;
  }
  return true;
}

bool Read(uint8_t page, uint8_t word, uint8_t *data, uint16_t size) {
  if (!RangeIsValid(page, word, size)) {
    return false;
  }

  uint16_t address = LinearAddress(page, word);
  uint16_t offset = 0;
  while (offset < size) {
    const uint8_t chunk = ChunkSize(address, size - offset, kMaxReadChunk);

    Wire.beginTransmission(DeviceAddress(address));
    if (Wire.write(WordAddress(address)) != 1 ||
        Wire.endTransmission(false) != 0) {
      return false;
    }

    if (Wire.requestFrom(DeviceAddress(address), chunk) != chunk ||
        Wire.readBytes(data + offset, chunk) != chunk) {
      return false;
    }
    address += chunk;
    offset += chunk;
  }
  return true;
}

}  // namespace fram
