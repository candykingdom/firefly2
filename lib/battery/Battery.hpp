#pragma once

#include "Types.hpp"

// References for battery level at startup. We can only really measure the
// battery state-of-charge when the battery is unloaded, and has been unloaded
// for at least a second or so. At startup, we check the approximate charge
// level. We display this charge level on the onboard LED, and also put the
// device into low power mode if the battery is effectively empty.
static constexpr float kBatteryEmpty = 3.7;
static constexpr float kBatteryFull = 4.2;

// voltage = battery * (62 + 180) / 180 * 3.3 / 1024
constexpr float kBatteryDividerHigh = 62;
constexpr float kBatteryDividerLow = 180;
constexpr uint16_t BatteryVoltageToRawReading(float voltage) {
  const float divided_voltage =
      voltage /
      ((kBatteryDividerHigh + kBatteryDividerLow) / kBatteryDividerLow);
  return divided_voltage / 3.3 * 1024.0;
}
