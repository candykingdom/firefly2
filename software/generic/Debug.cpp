#include "Debug.hpp"
#include <cstdarg>
#include <cstdio>

// This is a hack to ignore the unused parameter warning for a single instance.
template <typename... Args>
inline void unused(Args &&...) {}

void debug_printf(const char *fmt, ...) {
  unused(fmt);
#if DEBUG
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
#endif
}
