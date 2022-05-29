#include "PoliceEffect.hpp"

#include "Math.hpp"

PoliceEffect::PoliceEffect(const DeviceDescription *device) : Effect(device) {}

CRGB PoliceEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                          RadioPacket *setEffectPacket) {
  const bool red_cycle = (timeMs / redSpeed & 0b100000) == 0;
  const bool blue_cycle = (timeMs / blueSpeed & 0b100000) == 0;
  const bool red_flash = (timeMs / redSpeed & 0b100) == 0;
  const bool blue_flash = (timeMs / blueSpeed & 0b100) == 0;

  if (device->FlagEnabled(Tiny) && device->FlagEnabled(Mirrored)) {
    if (ledIndex > device->led_count / 2) {
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

  uint8_t led_count = device->led_count;
  if (device->FlagEnabled(Mirrored)) {
    MirrorIndex(&ledIndex, &led_count);
  }

  if (device->FlagEnabled(Tiny)) {
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
    if (ledIndex < lightWidth / 2) {
      if (red_cycle) {
        return red;
      } else {
        return off;
      }
    } else if (ledIndex < lightWidth) {
      if (red_cycle) {
        return off;
      } else {
        return red;
      }
    }
  }

  if (blue_flash) {
    if (ledIndex > led_count - lightWidth / 2) {
      if (blue_cycle) {
        return blue;
      } else {
        return off;
      }
    } else if (ledIndex > led_count - lightWidth) {
      if (blue_cycle) {
        return off;
      } else {
        return blue;
      }
    }
  }

  return off;
}
