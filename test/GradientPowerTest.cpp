#include <DisplayColorPaletteEffect.hpp>
#include <Radio.hpp>
#include <RainbowEffect.hpp>
#include <StripDescription.hpp>
#include <algorithm>
#include <cstdint>

#include "gtest/gtest.h"

namespace {

uint16_t Drive(const CRGB &rgb) { return rgb.r + rgb.g + rgb.b; }

uint16_t EndpointMax(uint8_t value) {
  uint16_t endpoint_max = 0;
  for (uint8_t hue : {HUE_RED, HUE_GREEN, HUE_BLUE}) {
    CRGB rgb;
    hsv2rgb_rainbow(CHSV(hue, 255, value), rgb);
    endpoint_max = std::max(endpoint_max, Drive(rgb));
  }
  return endpoint_max;
}

void ExpectUniformDrive(const char *effect_name, const Effect &effect,
                        const StripDescription &strip, RadioPacket *packet,
                        uint32_t time_ms, uint8_t value) {
  uint8_t worst_led = 0;
  CRGB worst_rgb = CRGB::Black;
  uint16_t worst_drive = 0;

  for (uint8_t led = 0; led < strip.led_count; ++led) {
    const CRGB rgb = effect.GetRGB(led, time_ms, strip, packet);
    const uint16_t drive = Drive(rgb);
    if (drive > worst_drive) {
      worst_led = led;
      worst_rgb = rgb;
      worst_drive = drive;
    }
  }

  const uint16_t allowed = EndpointMax(value) * 105 / 100;
  EXPECT_LE(worst_drive, allowed)
      << effect_name << " at time " << time_ms << ", LED "
      << static_cast<int>(worst_led) << " RGB ("
      << static_cast<int>(worst_rgb.r) << "," << static_cast<int>(worst_rgb.g)
      << "," << static_cast<int>(worst_rgb.b) << ") drive " << worst_drive
      << " exceeds allowed " << allowed;
}

}  // namespace

TEST(GradientPowerTest, RainbowGradientDriveStaysWithinEndpoints) {
  RadioPacket packet;
  packet.writeSetEffect(0, 0, 8);
  RainbowEffect effect;
  StripDescription normal_strip(60, {});
  StripDescription bright_strip(60, {Bright});

  for (uint32_t time_ms : {0U, 1000U, 5000U}) {
    ExpectUniformDrive("Rainbow", effect, normal_strip, &packet, time_ms, 128);
    ExpectUniformDrive("Rainbow", effect, bright_strip, &packet, time_ms, 255);
  }
}

TEST(GradientPowerTest, DisplayPaletteDriveStaysWithinEndpoints) {
  RadioPacket packet;
  packet.writeSetEffect(0, 0, 8);
  DisplayColorPaletteEffect effect;
  StripDescription strip(100, {});

  ExpectUniformDrive("Display Color Palette", effect, strip, &packet, 0, 127);
}
