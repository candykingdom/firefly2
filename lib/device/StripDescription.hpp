#ifndef __STRIP_DESCRIPTION_HPP__
#define __STRIP_DESCRIPTION_HPP__

#include <Types.hpp>
#include <vector>

enum StripFlag {
  Tiny = 1 << 0,
  Bright = 1 << 1,
  Circular = 1 << 2,
  Mirrored = 1 << 3,
  Reversed = 1 << 4,
  Controller = 1 << 5,
};

class StripDescription {
 public:
  const uint16_t led_count;

  explicit StripDescription(uint16_t led_count, std::vector<StripFlag> flags);

  bool FlagEnabled(StripFlag flag) const;

 private:
  const uint8_t flags;
};
#endif  // __STRIP_DESCRIPTION_HPP__
