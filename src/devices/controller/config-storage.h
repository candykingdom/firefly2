#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "fram.h"

namespace controller_config {

template <size_t DataSize>
bool LoadRecord(uint8_t page, uint8_t word,
                const std::array<uint8_t, 4> &valid_marker, uint8_t *data) {
  std::array<uint8_t, 4> marker{};
  if (!fram::Read(page, word, marker.data(), marker.size()) ||
      marker != valid_marker) {
    return false;
  }

  std::array<uint8_t, DataSize> loaded{};
  if (!fram::Read(page, word + marker.size(), loaded.data(), loaded.size())) {
    return false;
  }
  std::copy(loaded.begin(), loaded.end(), data);
  return true;
}

template <size_t DataSize>
bool StoreRecord(uint8_t page, uint8_t word,
                 const std::array<uint8_t, 4> &valid_marker,
                 const uint8_t *data) {
  // Invalidate first and publish the valid marker last. A reset or I2C failure
  // during the payload write therefore leaves a record that will not load.
  const std::array<uint8_t, 4> invalid_marker{};
  return fram::Write(page, word, invalid_marker.data(),
                     invalid_marker.size()) &&
         fram::Write(page, word + valid_marker.size(), data, DataSize) &&
         fram::Write(page, word, valid_marker.data(), valid_marker.size());
}

}  // namespace controller_config
