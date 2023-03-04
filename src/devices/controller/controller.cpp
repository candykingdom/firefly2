#include <Arduino.h>
#include <FastLED.h>
#include <arduino-timer.h>

#include "FakeLedManager.hpp"
#include "analog-button.h"
#include "arduino/RadioHeadRadio.hpp"
#include "generic/NetworkManager.hpp"
#include "generic/RadioStateMachine.hpp"

// Which mode of control the device is in
enum class ControllerMode {
  // Choose an effect and a color palette.
  Effect,

  // Left/right buttons immediately make everything light up a single color.
  DirectColor,

  // Left/right buttons immediately change the palette for the current effect.
  DirectPalette,
};

ControllerMode mode = ControllerMode::DirectPalette;

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

// Button LEDs
constexpr uint8_t kLeftButtonLed = 36;
constexpr uint8_t kRightButtonLed = 40;
constexpr uint8_t kBottomButtonLed = 41;

const StripDescription kRowStrip =
    StripDescription(/*led_count=*/12, {Bright, Controller});
const DeviceDescription kRowDescription(2000, {kRowStrip});
const StripDescription kPaletteStrip =
    StripDescription(/*led_count=*/5, {Bright, Controller});

constexpr uint8_t kSetEffectDelay = 60;

// Used to show when the radio is broadcasting
CountDownTimer broadcast_led_timer{1000};

RadioHeadRadio radio;
NetworkManager nm(&radio);
RadioStateMachine state_machine(&nm);
FakeLedManager led_manager(kRowDescription, &state_machine);

DisplayColorPaletteEffect palette_effect;

uint8_t effect_index = 0;
uint8_t palette_index = 0;
RadioPacket set_effect_packet;
RadioPacket control_packet;

AnalogButton left_buttons(kSwitchLeft);
AnalogButton right_buttons(kSwitchRight);
AnalogButton bottom_buttons(kSwitchBottom);

// TODO(adam): add pagination for color and palette mode
// Colors for color mode
std::array<CHSV, 6> colors = {
    CHSV{0, 255, 255},           CHSV{256 / 6, 255, 255},
    CHSV{256 * 2 / 6, 255, 255}, CHSV{256 * 3 / 6, 255, 255},
    CHSV{256 * 4 / 6, 255, 255}, CHSV{256 * 5 / 6, 255, 255}};

// Palette indices for palette mode.
std::array<uint8_t, 6> palettes = {
    8, 9, 10, 11, 12, 13,
};

void SetMainLed(uint8_t led_index, CRGB rgb) {
  if (led_index > 11 && led_index < 24) {
    leds[12 + (23 - led_index)] = rgb;
  } else {
    leds[led_index] = rgb;
  }
}

void SetLeftButtonLed(uint8_t led_index, uint8_t brightness) {}

void RunEffectMode() {
  const uint8_t num_unique_effects = led_manager.GetNumUniqueEffects();
  const uint8_t num_palettes = Effect::palettes().size();
  if (left_buttons.Button1Pressed()) {
    effect_index = (effect_index - 1 + num_unique_effects) % num_unique_effects;
  } else if (right_buttons.Button1Pressed()) {
    effect_index = (effect_index + 1) % num_unique_effects;
  } else if (left_buttons.Button2Pressed()) {
    palette_index = (palette_index - 1 + num_palettes) % num_palettes;
  } else if (right_buttons.Button2Pressed()) {
    palette_index = (palette_index + 1) % num_palettes;
  } else if (bottom_buttons.Button1Pressed()) {
    state_machine.SetEffect(&set_effect_packet);
    broadcast_led_timer.Reset();
  }

  const uint8_t real_effect_index =
      led_manager.UniqueEffectNumberToIndex(effect_index);
  Effect* current_effect = led_manager.GetEffect(real_effect_index);
  set_effect_packet.writeSetEffect(real_effect_index, kSetEffectDelay,
                                   palette_index);

  // Update the top row with the effect output.
  for (uint8_t i = 0; i < kRowStrip.led_count; i++) {
    SetMainLed(i, current_effect->GetRGB(i, state_machine.GetNetworkMillis(),
                                         kRowStrip, &set_effect_packet));
  }

  // Update the middle row with the palette colors.
  for (uint8_t i = 0; i < kRowStrip.led_count; i++) {
    SetMainLed(i + kRowStrip.led_count,
               palette_effect.GetRGB(i, state_machine.GetNetworkMillis(),
                                     kRowStrip, &set_effect_packet));
  }

  // Update slave status LED.
  leds[kStatusLeft] = state_machine.GetCurrentState() == RadioState::Slave
                          ? CRGB(255, 0, 0)
                          : CRGB(0, 255, 0);

  // Update "sending" status LED.
  leds[kStatusRight] = CRGB(0, 0, broadcast_led_timer.Active() ? 255 : 0);
}

void RunColorMode() {
  for (uint8_t i = 0; i < 36; i++) {
    if ((i % 12) > 4 && (i % 12) <= 6) {
      SetMainLed(i, CRGB(0, 0, 0));
    } else {
      SetMainLed(i, colors[(i / 6)]);
    }
  }

  bool pressed = false;
  uint8_t color_index = 0;
  if (left_buttons.Button1Pressed()) {
    pressed = true;
    color_index = 0;
  } else if (left_buttons.Button2Pressed()) {
    pressed = true;
    color_index = 2;
  } else if (left_buttons.Button3Pressed()) {
    pressed = true;
    color_index = 4;
  } else if (right_buttons.Button1Pressed()) {
    pressed = true;
    color_index = 1;
  } else if (right_buttons.Button2Pressed()) {
    pressed = true;
    color_index = 3;
  } else if (right_buttons.Button3Pressed()) {
    pressed = true;
    color_index = 5;
  }

  if (pressed) {
    control_packet.writeControl(kSetEffectDelay, colors[color_index]);
    state_machine.SetEffect(&control_packet);
  }
}

void RunPaletteMode() {
  for (uint8_t i = 0; i < 36; i++) {
    if ((i % 12) > 4 && (i % 12) <= 6) {
      SetMainLed(i, CRGB(0, 0, 0));
    } else {
      set_effect_packet.writeSetEffect(/*effect_index=*/0, /*delay=*/1,
                                       /*palette_index=*/palettes[i / 6]);
      uint8_t led_index = (i % 6);
      SetMainLed(
          i, palette_effect.GetRGB(led_index, state_machine.GetNetworkMillis(),
                                   kPaletteStrip, &set_effect_packet));
    }
  }

  bool pressed = false;
  uint8_t palette_index = 0;
  if (left_buttons.Button1Pressed()) {
    pressed = true;
    palette_index = 0;
  } else if (left_buttons.Button2Pressed()) {
    pressed = true;
    palette_index = 2;
  } else if (left_buttons.Button3Pressed()) {
    pressed = true;
    palette_index = 4;
  } else if (right_buttons.Button1Pressed()) {
    pressed = true;
    palette_index = 1;
  } else if (right_buttons.Button2Pressed()) {
    pressed = true;
    palette_index = 3;
  } else if (right_buttons.Button3Pressed()) {
    pressed = true;
    palette_index = 5;
  }

  if (pressed) {
    set_effect_packet.writeSetEffect(state_machine.GetEffectIndex(),
                                     kSetEffectDelay, palettes[palette_index]);
    state_machine.SetEffect(&set_effect_packet);
  }
}

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

  // TODO(adam): choose the mode based off of the mode switch, once it's
  // electrically connected to the MCU (d'oh)

  switch (mode) {
    case ControllerMode::Effect:
      RunEffectMode();
      break;

    case ControllerMode::DirectColor:
      RunColorMode();
      break;

    case ControllerMode::DirectPalette:
      RunPaletteMode();
      break;
  }

  FastLED.show();
  delay(5);
}
