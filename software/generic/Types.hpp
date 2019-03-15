#ifndef __TYPES_H__
#define __TYPES_H__

// This header defines types depending on the platform.

// Arduino provides the types of the form 'int8_t'. In vanilla C++, we need to
// include these manually.
#ifndef ARDUINO
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "FakeFastLED/pixeltypes.h"

// Provide the arduino random function
uint16_t random(uint16_t max);

uint16_t random(uint16_t min, uint16_t max);

// Provide the millis function, and allowing setting it for tests.
uint32_t millis();
void setMillis(uint32_t time);
void advanceMillis(uint32_t time);

#else
// Arduino
#include "Arduino.h"
#include "string.h"
#endif

#endif
