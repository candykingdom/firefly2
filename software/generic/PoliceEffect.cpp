#include "PoliceEffect.hpp"

PoliceEffect::PoliceEffect(DeviceDescription *const device) : Effect(device) {}

CRGB PoliceEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                          RadioPacket *setEffectPacket) {
  const bool red_cycle = (timeMs / redSpeed & 0b100000) == 0;
  const bool blue_cycle = (timeMs / blueSpeed & 0b100000) == 0;
  const bool red_flash = (timeMs / redSpeed & 0b100) == 0;
  const bool blue_flash = (timeMs / blueSpeed & 0b100) == 0;

  if (device->virtualLeds < lightWidth * 2) {
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
    if (ledIndex > device->virtualLeds - lightWidth / 2) {
      if (blue_cycle) {
        return blue;
      } else {
        return off;
      }
    } else if (ledIndex > device->virtualLeds - lightWidth) {
      if (blue_cycle) {
        return off;
      } else {
        return blue;
      }
    }
  }

  return off;
}
