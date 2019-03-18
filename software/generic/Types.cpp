#include "Types.hpp"

#ifndef ARDUINO
uint16_t random(uint16_t max) { return rand() % max; }

uint16_t random(uint16_t min, uint16_t max) { return min + (rand() % max); }

// Oh noes, a global variable!
uint32_t currentTime = 0;
uint32_t millis() { return currentTime; }

void setMillis(uint32_t time) { currentTime = time; }

void advanceMillis(uint32_t time) { currentTime += time; }

uint16_t XY(uint8_t x, uint8_t y) { return 0; }
#endif
