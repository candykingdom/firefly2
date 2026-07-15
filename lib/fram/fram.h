#pragma once

#include <cstddef>
#include <cstdint>

namespace fram {

constexpr uint8_t kBaseAddress = 0x50;
constexpr uint16_t kPageSize = 256;
constexpr uint8_t kPageCount = 8;
constexpr uint16_t kCapacity = kPageSize * kPageCount;

constexpr uint16_t LinearAddress(uint8_t page, uint8_t word) {
  return static_cast<uint16_t>(page) * kPageSize + word;
}

constexpr bool RangeIsValid(uint8_t page, uint8_t word, uint16_t size) {
  return page < kPageCount && LinearAddress(page, word) + size <= kCapacity;
}

constexpr uint8_t DeviceAddress(uint16_t address) {
  return kBaseAddress | ((address / kPageSize) & (kPageCount - 1));
}

constexpr uint8_t WordAddress(uint16_t address) {
  return static_cast<uint8_t>(address % kPageSize);
}

constexpr uint8_t ChunkSize(uint16_t address, uint16_t remaining,
                            uint8_t max_chunk) {
  const uint16_t page_remaining = kPageSize - WordAddress(address);
  const uint16_t bounded =
      remaining < page_remaining ? remaining : page_remaining;
  return static_cast<uint8_t>(bounded < max_chunk ? bounded : max_chunk);
}

bool Write(uint8_t page, uint8_t word, const uint8_t *data, uint16_t size);
bool Read(uint8_t page, uint8_t word, uint8_t *data, uint16_t size);

}  // namespace fram
