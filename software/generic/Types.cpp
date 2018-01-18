#include "Types.hpp"

uint16_t random(uint16_t max) { return rand() % max; }

uint16_t random(uint16_t min, uint16_t max) { return min + (rand() % max); }
