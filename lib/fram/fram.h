#pragma once

#include <array>

#include "Types.hpp"

namespace fram {

uint8_t Write(uint8_t page, uint8_t word, const uint8_t* data, uint8_t size);
uint8_t Read(uint8_t page, uint8_t word, uint8_t *data, uint8_t size);

};
