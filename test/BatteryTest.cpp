#include <Battery.hpp>

#include "gtest/gtest.h"

namespace {

// Inverse of BatteryVoltageToRawReading's math (the firmware has no inverse;
// this reconstructs the battery voltage a raw ADC reading represents).
float RawReadingToBatteryVoltage(uint16_t raw) {
  return static_cast<float>(raw) * 3.3 / 1024.0 *
         ((kBatteryDividerHigh + kBatteryDividerLow) / kBatteryDividerLow);
}

// One ADC count expressed in battery volts - the quantization step.
const float kVoltsPerCount = RawReadingToBatteryVoltage(1);

TEST(Battery, voltageToRawIsMonotonicOverCalibrationRange) {
  uint16_t previous = BatteryVoltageToRawReading(kBatteryEmpty - 0.5);
  for (float voltage = kBatteryEmpty; voltage <= kBatteryFull + 0.5;
       voltage += 0.1) {
    const uint16_t raw = BatteryVoltageToRawReading(voltage);
    EXPECT_GT(raw, previous) << "at " << voltage << " V";
    previous = raw;
  }
}

TEST(Battery, voltageRoundTripsWithinOneQuantizationStep) {
  const float voltages[] = {kBatteryEmpty, (kBatteryEmpty + kBatteryFull) / 2,
                            kBatteryFull};
  for (const float voltage : voltages) {
    const uint16_t raw = BatteryVoltageToRawReading(voltage);
    EXPECT_LE(raw, 1023) << "raw reading must fit the 10-bit ADC";
    const float back = RawReadingToBatteryVoltage(raw);
    EXPECT_NEAR(back, voltage, kVoltsPerCount) << "at " << voltage << " V";
  }
}

TEST(Battery, isUsableAtCompileTime) {
  static_assert(BatteryVoltageToRawReading(kBatteryFull) >
                    BatteryVoltageToRawReading(kBatteryEmpty),
                "BatteryVoltageToRawReading must stay constexpr");
}

}  // namespace
