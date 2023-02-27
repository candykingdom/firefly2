#include <FastLED.h>

#include "Arduino.h"
#include "FakeLedManager.hpp"
#include "analog-button.h"
#include "arduino/RadioHeadRadio.hpp"
#include "generic/NetworkManager.hpp"
#include "generic/RadioStateMachine.hpp"

// Pin definitions
constexpr int kSwitchLeft = PA0;
constexpr int kSwitchRight = PA1;
constexpr int kSwitchBottom = PA2;

constexpr int kVbattDiv = PA3;

constexpr int kNeopixelPin = PB15;

constexpr uint16_t kLedCount = 42;
CRGB leds[kLedCount];

// LED indices
constexpr uint8_t kStatusLeft = 39;
constexpr uint8_t kStatusMiddle = 38;
constexpr uint8_t kStatusRight = 37;

const StripDescription kRowStrip = StripDescription(/*led_count=*/12, {Bright});
const DeviceDescription kRowDescription(2000, {kRowStrip});

RadioHeadRadio radio;
NetworkManager nm(&radio);
RadioStateMachine state_machine(&nm);
FakeLedManager led_manager(kRowDescription, &state_machine);

DisplayColorPaletteEffect palette_effect;

uint8_t effect_index = 0;
uint8_t palette_index = 0;
RadioPacket set_effect_packet;

AnalogButton left_buttons(kSwitchLeft);
AnalogButton right_buttons(kSwitchRight);
AnalogButton bottom_buttons(kSwitchBottom);

void setup() {
  FastLED.addLeds<NEOPIXEL, kNeopixelPin>(leds, kLedCount)
      .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(32);

  if (!radio.Begin()) {
    while (true) {
      leds[0] = CRGB((millis() / 200) % 2 == 0 ? 255 : 0, 0, 0);
      FastLED.show();
    }
  }
  FastLED.clear(/*writeData=*/true);
}

void loop() {
  state_machine.Tick();

  left_buttons.Tick();
  right_buttons.Tick();
  bottom_buttons.Tick();

  const uint8_t num_unique_effects = led_manager.GetNumUniqueEffects();
  if (left_buttons.Button1Pressed()) {
    effect_index = (effect_index - 1 + num_unique_effects) % num_unique_effects;
  } else if (right_buttons.Button1Pressed()) {
    effect_index = (effect_index + 1) % num_unique_effects;
  }

  const uint8_t num_palettes = Effect::palettes().size();
  if (left_buttons.Button2Pressed()) {
    palette_index = (palette_index - 1 + num_palettes) % num_palettes;
  } else if (right_buttons.Button2Pressed()) {
    palette_index = (palette_index + 1) % num_palettes;
  }

  Effect* current_effect = led_manager.GetEffect(
      led_manager.UniqueEffectNumberToIndex(effect_index));
  set_effect_packet.writeSetEffect(effect_index, 0, palette_index);

  for (uint8_t i = 0; i < kRowStrip.led_count; i++) {
    leds[i] = current_effect->GetRGB(i, state_machine.GetNetworkMillis(),
                                     kRowStrip, &set_effect_packet);
  }

  for (uint8_t i = 0; i < kRowStrip.led_count; i++) {
    leds[i + kRowStrip.led_count] = palette_effect.GetRGB(
        i, state_machine.GetNetworkMillis(), kRowStrip, &set_effect_packet);
  }
  leds[kStatusLeft] = state_machine.GetCurrentState() == RadioState::Slave
                          ? CRGB(255, 0, 0)
                          : CRGB(0, 255, 0);
  FastLED.show();

  delay(5);
}
