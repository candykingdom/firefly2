#include "Debug.hpp"
#include <cstdarg>
#include <cstdio>

#if DEBUG
#ifdef ARDUINO
#include "Arduino.h"
#endif
#endif

// This is a hack to ignore the unused parameter warning for a single instance.
template <typename... Args> inline void unused(Args &&...) {}

void debug_printf(const char *fmt, ...) {
  unused(fmt);
#if DEBUG
#ifdef ARDUINO
#pragma message "DEBUG is enabled for Arduino"
  // If we are on Arduino print over serial.
  static char buf[256]; // Statically allocate 256 bytes for printf'ing.
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 128, fmt, args);
  va_end(args);
  Serial.print(buf);
#else
#pragma message "DEBUG is enabled for generic systems"
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
#endif
#else
#pragma message "DEBUG is DISABLED"
#endif
}
