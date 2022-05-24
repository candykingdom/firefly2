#include "DeviceDescription.hpp"

DeviceDescription::DeviceDescription(uint8_t led_count, uint8_t flags)
    : led_count(led_count), flags(flags) {}

bool DeviceDescription::FlagEnabled(DeviceFlag flag) const {
  return (flags & (int)flag) == (int)flag;
}

const String DeviceDescription::ToString() const {
  String string_builder;

  string_builder.concat("Firefly device with ");
  string_builder.concat(led_count);
  string_builder.concat(" LEDs");

  if (flags != 0) {
    string_builder.concat("\nFlags:\n");

    for (uint8_t i = 1; i < 8; ++i) {
      uint8_t flag = 1 << i;

      if (FlagEnabled((DeviceFlag)flag)) {
        switch (flag) {
          case (1 << 0):
            string_builder.concat("Tiny\n");
            break;
          case (1 << 1):
            string_builder.concat("Bright\n");
            break;
          case (1 << 2):
            string_builder.concat("Circular\n");
            break;
          default:
            string_builder.concat("Unrecognized flag: ");
            string_builder.concat(flag);
            string_builder.concat("\n");
        }
      }
    }
  }

  return string_builder;
}