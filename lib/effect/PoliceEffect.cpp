#include "PoliceEffect.hpp"

#include <Math.hpp>

PoliceEffect::PoliceEffect() : Effect() {}

CRGB PoliceEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                          const StripDescription *strip,
                          RadioPacket *setEffectPacket) {
  const bool red_cycle = (time_ms / red_speed & 0b100000) == 0;
  const bool blue_cycle = (time_ms / blue_speed & 0b100000) == 0;
  const bool red_flash = (time_ms / red_speed & 0b100) == 0;
  const bool blue_flash = (time_ms / blue_speed & 0b100) == 0;

  if (strip->FlagEnabled(Tiny) && strip->FlagEnabled(Mirrored)) {
    if (led_index >= strip->led_count / 2) {
      if (red_cycle && red_flash) {
        return red;
      } else {
        return off;
      }
    } else {
      if (!red_cycle && blue_flash) {
        return blue;
      } else {
        return off;
      }
    }
  }

  uint8_t led_count = strip->led_count;
  if (strip->FlagEnabled(Mirrored)) {
    MirrorIndex(&led_index, &led_count);
  }

  if (strip->FlagEnabled(Tiny)) {
    if (red_flash) {
      if (red_cycle) {
        return red;
      } else {
        return blue;
      }
    } else {
      return off;
    }
  }

  if (red_flash) {
    if (led_index < light_width / 2) {
      if (red_cycle) {
        return red;
      } else {
        return off;
      }
    } else if (led_index < light_width) {
      if (red_cycle) {
        return off;
      } else {
        return red;
      }
    }
  }

  if (blue_flash) {
    if (led_index > led_count - light_width / 2) {
      if (blue_cycle) {
        return blue;
      } else {
        return off;
      }
    } else if (led_index > led_count - light_width) {
      if (blue_cycle) {
        return off;
      } else {
        return blue;
      }
    }
  }

  return off;
}
