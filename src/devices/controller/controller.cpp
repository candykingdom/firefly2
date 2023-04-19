#include <Arduino.h>
#include <FastLED.h>
#include <arduino-timer.h>
#include <debounce-filter.h>
#include <median-filter.h>

#include <array>

#include "FakeLedManager.hpp"
#include "analog-button.h"
#include "arduino/RadioHeadRadio.hpp"
#include "buttons.h"
#include "generic/NetworkManager.hpp"
#include "generic/RadioStateMachine.hpp"
#include "leds.h"

// Which mode of control the device is in
enum class ControllerMode {
  // Choose an effect and a color palette.
  Effect,

  // Left/right buttons immediately make everything light up a single color.
  DirectColor,

  // Left/right buttons immediately change the palette for the current
  // effect.
  DirectPalette,
};

ControllerMode mode = ControllerMode::Effect;
ControllerMode prev_mode = mode;

// Pin definitions
constexpr std::array<uint8_t, 3> kLeftButtons = {PB2, PB14, PB15};
constexpr std::array<uint8_t, 3> kRightButtons = {PA8, PC6, PC7};
constexpr std::array<uint8_t, 3> kBottomButtons = {PA11, PA12, PA15};

constexpr int kModeSwitch = PA6;
constexpr int kVbattDiv = PA7;

// Note: for some reason, `PA4` has a resolved pin number of 196, which confuses
// FastLED.
constexpr int kNeopixelPin = PA_4;

const StripDescription kRowStrip =
    StripDescription(/*led_count=*/12, {Bright, Controller});
const DeviceDescription kRowDescription(2000, {kRowStrip});
const StripDescription kPaletteStrip =
    StripDescription(/*led_count=*/5, {Bright, Controller});

constexpr uint8_t kSetEffectDelay = 60;

// Constants for the mode switch, which uses an evenly-divided voltage divider.
// See
// http://www.ignorantofthings.com/2018/07/the-perfect-multi-button-input-resistor.html?m=1
static constexpr uint16_t kMode1Threshold = 346 / 2;
static constexpr uint16_t kMode2Threshold = (346 + 678) / 2;
static constexpr uint16_t kMode3Threshold = (678 + 1023) / 2;

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

std::array<DebounceFilter, 3> left_buttons = {
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kLeftButtons[0]>()),
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kLeftButtons[1]>()),
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kLeftButtons[2]>()),
};

std::array<DebounceFilter, 3> right_buttons = {
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kRightButtons[0]>()),
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kRightButtons[1]>()),
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kRightButtons[2]>()),
};

std::array<DebounceFilter, 3> bottom_buttons = {
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kBottomButtons[0]>()),
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kBottomButtons[1]>()),
    DebounceFilter(filter_functions::ForInvertedDigitalRead<kBottomButtons[2]>()),
};

MedianFilter<uint16_t, uint16_t, 5> mode_switch(
    filter_functions::ForAnalogRead<kModeSwitch>());

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

void RunEffectMode() {
  const uint8_t num_unique_effects = led_manager.GetNumUniqueEffects();
  const uint8_t num_palettes = Effect::palettes().size();
  if (left_buttons[0].Rose()) {
    effect_index = (effect_index - 1 + num_unique_effects) % num_unique_effects;
  } else if (right_buttons[0].Rose()) {
    effect_index = (effect_index + 1) % num_unique_effects;
  } else if (left_buttons[1].Rose()) {
    palette_index = (palette_index - 1 + num_palettes) % num_palettes;
  } else if (right_buttons[1].Rose()) {
    palette_index = (palette_index + 1) % num_palettes;
  } else if (bottom_buttons[0].Rose()) {
    state_machine.SetEffect(&set_effect_packet);
    broadcast_led_timer.Reset();
  }

  if (left_buttons[0].Rose()) {
    SetLeftButtonLeds(kButtonPressedBrightness, kButtonActiveBrightness, 0);
  } else if (left_buttons[1].Rose()) {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonPressedBrightness, 0);
  } else {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness, 0);
  }

  if (right_buttons[0].Rose()) {
    SetRightButtonLeds(kButtonPressedBrightness, kButtonActiveBrightness, 0);
  } else if (left_buttons[1].Rose()) {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonPressedBrightness, 0);
  } else {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness, 0);
  }

  if (bottom_buttons[0].Rose()) {
    SetBottomButtonLeds(kButtonPressedBrightness, 0, 0);
  } else {
    SetBottomButtonLeds(kButtonActiveBrightness, 0, 0);
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
  if (left_buttons[0].Rose()) {
    pressed = true;
    color_index = 0;
  } else if (left_buttons[1].Rose()) {
    pressed = true;
    color_index = 2;
  } else if (left_buttons[2].Rose()) {
    pressed = true;
    color_index = 4;
  } else if (right_buttons[0].Rose()) {
    pressed = true;
    color_index = 1;
  } else if (right_buttons[1].Rose()) {
    pressed = true;
    color_index = 3;
  } else if (right_buttons[2].Rose()) {
    pressed = true;
    color_index = 5;
  }

  if (left_buttons[0].Rose()) {
    SetLeftButtonLeds(kButtonPressedBrightness, kButtonActiveBrightness,
                      kButtonActiveBrightness);
  } else if (left_buttons[1].Rose()) {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonPressedBrightness,
                      kButtonActiveBrightness);
  } else if (left_buttons[2].Rose()) {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                      kButtonPressedBrightness);
  } else {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                      kButtonActiveBrightness);
  }

  if (right_buttons[0].Rose()) {
    SetRightButtonLeds(kButtonPressedBrightness, kButtonActiveBrightness,
                       kButtonActiveBrightness);
  } else if (left_buttons[1].Rose()) {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonPressedBrightness,
                       kButtonActiveBrightness);
  } else if (left_buttons[2].Rose()) {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                       kButtonPressedBrightness);
  } else {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                       kButtonActiveBrightness);
  }

  SetBottomButtonLeds(0, 0, 0);

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
  if (left_buttons[0].Rose()) {
    pressed = true;
    palette_index = 0;
  } else if (left_buttons[1].Rose()) {
    pressed = true;
    palette_index = 2;
  } else if (left_buttons[2].Rose()) {
    pressed = true;
    palette_index = 4;
  } else if (right_buttons[0].Rose()) {
    pressed = true;
    palette_index = 1;
  } else if (right_buttons[1].Rose()) {
    pressed = true;
    palette_index = 3;
  } else if (right_buttons[2].Rose()) {
    pressed = true;
    palette_index = 5;
  }

  if (left_buttons[0].Rose()) {
    SetLeftButtonLeds(kButtonPressedBrightness, kButtonActiveBrightness,
                      kButtonActiveBrightness);
  } else if (left_buttons[1].Rose()) {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonPressedBrightness,
                      kButtonActiveBrightness);
  } else if (left_buttons[2].Rose()) {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                      kButtonPressedBrightness);
  } else {
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                      kButtonActiveBrightness);
  }

  if (right_buttons[0].Rose()) {
    SetRightButtonLeds(kButtonPressedBrightness, kButtonActiveBrightness,
                       kButtonActiveBrightness);
  } else if (right_buttons[1].Rose()) {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonPressedBrightness,
                       kButtonActiveBrightness);
  } else if (right_buttons[2].Rose()) {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                       kButtonPressedBrightness);
  } else {
    SetRightButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness,
                       kButtonActiveBrightness);
  }

  SetBottomButtonLeds(0, 0, 0);

  if (pressed) {
    set_effect_packet.writeSetEffect(state_machine.GetEffectIndex(),
                                     kSetEffectDelay, palettes[palette_index]);
    state_machine.SetEffect(&set_effect_packet);
  }
}

void setup() {
  // check if nBOOT_SEL bit is set
  if (FLASH->OPTR & FLASH_OPTR_nBOOT_SEL) {
    // unlock flash/option
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
    FLASH->OPTKEYR = 0x08192A3B;
    FLASH->OPTKEYR = 0x4C5D6E7F;

    while (FLASH->SR & FLASH_SR_BSY1)
      ;

    // clear nBOOT_SEL bit
    FLASH->OPTR &= ~FLASH_OPTR_nBOOT_SEL;

    // write
    FLASH->CR |= FLASH_CR_OPTSTRT;
    while (FLASH->SR & FLASH_SR_BSY1)
      ;
  }

  FastLED.addLeds<NEOPIXEL, kNeopixelPin>(leds.data(), kLedCount)
      .setCorrection(TypicalLEDStrip);
  FastLED.clear(/*writeData=*/true);

  SPI.setMISO(PB4);
  SPI.setMOSI(PB5);
  SPI.setSCLK(PB3);

  if (!radio.Begin()) {
    while (true) {
      leds[0] = CRGB((millis() / 200) % 2 == 0 ? 255 : 0, 0, 0);
      FastLED.show();
    }
  }

  pinMode(kLeftButtons[0], INPUT_PULLUP);
  pinMode(kLeftButtons[1], INPUT_PULLUP);
  pinMode(kLeftButtons[2], INPUT_PULLUP);
  pinMode(kRightButtons[0], INPUT_PULLUP);
  pinMode(kRightButtons[1], INPUT_PULLUP);
  pinMode(kRightButtons[2], INPUT_PULLUP);
  pinMode(kBottomButtons[0], INPUT_PULLUP);
  pinMode(kBottomButtons[1], INPUT_PULLUP);
  pinMode(kBottomButtons[2], INPUT_PULLUP);
}

void loop() {
  state_machine.Tick();

  left_buttons[0].Run();
  left_buttons[1].Run();
  left_buttons[2].Run();
  right_buttons[0].Run();
  right_buttons[1].Run();
  right_buttons[2].Run();
  bottom_buttons[0].Run();
  bottom_buttons[1].Run();
  bottom_buttons[2].Run();

  mode_switch.Run();
  if (mode_switch.GetFilteredValue() < kMode1Threshold) {
    mode = ControllerMode::DirectColor;
  } else if (mode_switch.GetFilteredValue() < kMode2Threshold) {
    mode = ControllerMode::DirectPalette;
  } else {
    mode = ControllerMode::Effect;
  }
  if (prev_mode != mode) {
    FastLED.clearData();
  }
  prev_mode = mode;

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
