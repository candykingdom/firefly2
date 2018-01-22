#include "Types.hpp"

uint16_t random(uint16_t max) { return rand() % max; }

uint16_t random(uint16_t min, uint16_t max) { return min + (rand() % max); }

// Oh noes, a global variable!
uint16_t currentTime = 0;
uint16_t millis() { return currentTime; }

void setMillis(uint16_t time) { currentTime = time; }
