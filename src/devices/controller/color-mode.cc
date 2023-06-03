#include "color-mode.h"

#include "Types.hpp"
#include "buttons.h"
#include "fram.h"
#include "generic/RadioStateMachine.hpp"
#include "leds.h"

extern RadioPacket control_packet;
extern RadioStateMachine state_machine;
extern uint8_t kSetEffectDelay;

constexpr uint32_t kRepeatDelay = 30;
constexpr uint8_t kColorConfigPage = 0;
constexpr uint8_t kColorConfigWord = 0;
constexpr std::array<uint8_t, 4> kColorInitialized = {0xDE, 0xAD, 0xBE, 0xEF};

// Colors for color mode
const CHSV kHsvBlack = CHSV{0, 255, 0};
std::array<std::array<CHSV, 6>, 3> colors = {
    std::array<CHSV, 6>{
        CHSV{0, 255, 255}, CHSV{256 / 6, 255, 255}, CHSV{256 * 2 / 6, 255, 255},
        CHSV{256 * 3 / 6, 255, 255}, CHSV{256 * 4 / 6, 255, 255},
        CHSV{256 * 5 / 6, 255, 255}},
    std::array<CHSV, 6>{
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
    },
    std::array<CHSV, 6>{
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
        kHsvBlack,
    },
};

void MaybeLoadColorConfig() {
  std::array<uint8_t, 4> fram_init;
  fram::Read(kColorConfigPage, kColorConfigWord, fram_init.data(),
             sizeof(fram_init));
  if (fram_init == kColorInitialized) {
    fram::Read(kColorConfigPage, kColorConfigWord + sizeof(kColorInitialized),
               reinterpret_cast<uint8_t*>(colors.data()), sizeof(colors));
  }
}

void WriteColorsToMainLeds() {
  for (uint8_t i = 0; i < 36; i++) {
    if ((i % 12) > 4 && (i % 12) <= 6) {
      SetMainLed(i, CRGB(0, 0, 0));
    } else {
      SetMainLed(i, colors[color_carousel][(i / 6)]);
    }
  }
}

void RunColorMode() {
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

  for (uint8_t i = 0; i < 3; i++) {
    if (bottom_buttons[i].Pressed()) {
      color_carousel = i;
    }
  }
  SetBottomButtonLeds(0, 0, 0);
  SetBottomButtonLed(color_carousel + 1, kButtonActiveBrightness);

  WriteColorsToMainLeds();

  if (pressed) {
    control_packet.writeControl(kSetEffectDelay,
                                colors[color_carousel][color_index]);
    state_machine.SetEffect(&control_packet);
  }

  for (uint8_t i = 0; i < 3; i++) {
    if (bottom_buttons[i].Held()) {
      sub_mode = SubMode::ChooseSlot;
      config_carousel = i;
    }
  }
}

void RunColorConfig() {
  static uint8_t selected_slot = 255;
  static uint8_t current_color = 0;
  static uint8_t current_saturation = 255;
  static uint8_t current_value = 255;
  static constexpr uint8_t kColorPressStep = 256 / 16;
  static constexpr uint8_t kColorHeldStep = 1;

  uint8_t blink_brightness = (millis() / kButtonBlinkPeriod) % 2 == 0
                                 ? kButtonBlinkBrightness
                                 : kButtonActiveBrightness;

  if (sub_mode == SubMode::ChooseSlot) {
    SetLeftButtonLeds(blink_brightness, blink_brightness, blink_brightness);
    SetRightButtonLeds(blink_brightness, blink_brightness, blink_brightness);
    SetBottomButtonLeds(0, 0, 0);
    SetBottomButtonLed(config_carousel + 1, kButtonActiveBrightness);
    WriteColorsToMainLeds();

    selected_slot = 255;
    for (uint8_t i = 0; i < 3; ++i) {
      if (left_buttons[i].Pressed()) {
        SetLeftButtonLed(i, kButtonPressedBrightness);
        selected_slot = i * 2;
        break;
      }
      if (right_buttons[i].Pressed()) {
        SetRightButtonLed(i, kButtonPressedBrightness);
        selected_slot = i * 2 + 1;
        break;
      }
    }
    if (selected_slot != 255) {
      sub_mode = SubMode::ChooseItem;
      left_buttons[0].SetRepeatDelay(kRepeatDelay);
      left_buttons[1].SetRepeatDelay(kRepeatDelay);
      left_buttons[2].SetRepeatDelay(kRepeatDelay);
      right_buttons[0].SetRepeatDelay(kRepeatDelay);
      right_buttons[1].SetRepeatDelay(kRepeatDelay);
      right_buttons[2].SetRepeatDelay(kRepeatDelay);
    }

  } else {
    // Hue
    if (left_buttons[0].Pressed()) {
      current_color = current_color - kColorPressStep;
    } else if (left_buttons[0].Held() || left_buttons[0].Repeated()) {
      current_color = current_color - kColorHeldStep;
    } else if (right_buttons[0].Pressed()) {
      current_color = current_color + kColorPressStep;
    } else if (right_buttons[0].Held() || right_buttons[0].Repeated()) {
      current_color = current_color + kColorHeldStep;
    }

    // Saturation
    if (left_buttons[1].Pressed()) {
      if (current_saturation < kColorPressStep) {
        // When wrapping around from low saturation to high saturation, force
        // the saturation to full.
        current_saturation = 255;
      } else {
        current_saturation = current_saturation - kColorPressStep;
      }
    } else if (left_buttons[1].Held() || left_buttons[1].Repeated()) {
      current_saturation = current_saturation - kColorHeldStep;
    } else if (right_buttons[1].Pressed()) {
      // When wrapping around from high saturation to low, force the saturation
      // to 0.
      if (current_saturation > (255 - kColorPressStep)) {
        current_saturation = 0;
      } else {
        current_saturation = current_saturation + kColorPressStep;
      }
    } else if (right_buttons[1].Held() || right_buttons[1].Repeated()) {
      current_saturation = current_saturation + kColorHeldStep;
    }

    // Value
    if (left_buttons[2].Pressed()) {
      if (current_value < kColorPressStep) {
        // When wrapping around from low value to high value, force
        // the value to full.
        current_value = 255;
      } else {
        current_value = current_value - kColorPressStep;
      }
    } else if (left_buttons[2].Held() || left_buttons[2].Repeated()) {
      current_value = current_value - kColorHeldStep;
    } else if (right_buttons[2].Pressed()) {
      // When wrapping around from high value to low, force the value
      // to 0.
      if (current_value > (255 - kColorPressStep)) {
        current_value = 0;
      } else {
        current_value = current_value + kColorPressStep;
      }
    } else if (right_buttons[2].Held() || right_buttons[2].Repeated()) {
      current_value = current_value + kColorHeldStep;
    }
    if (current_value < 4) {
      current_value = 0;
    }

    const CHSV current_hsv =
        CHSV(current_color, current_saturation, current_value);
    for (uint8_t i = 0; i < 3; i++) {
      if (bottom_buttons[i].Pressed()) {
        // Pressing the blinking bottom button saves the current color. Pressing
        // the other bottom buttons cancels and returns to normal mode.
        if (i == config_carousel) {
          colors[config_carousel][selected_slot] = current_hsv;
        }
        fram::Write(kColorConfigPage,
                    kColorConfigWord + sizeof(kColorInitialized),
                    reinterpret_cast<uint8_t*>(colors.data()), sizeof(colors));
        fram::Write(kColorConfigPage, kColorConfigWord,
                    reinterpret_cast<const uint8_t*>(kColorInitialized.data()),
                    sizeof(kColorInitialized));
        sub_mode = SubMode::Normal;
      }
    }
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness, 0);
    SetRightButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness, 0);

    // Blink the carousel button
    SetBottomButtonLeds(0, 0, 0);
    SetBottomButtonLed(config_carousel + 1, blink_brightness);

    for (uint8_t i = 0; i < 12; i++) {
      SetMainLed(i, current_hsv);
    }
    for (uint8_t i = 12; i < 36; i++) {
      SetMainLed(i, CRGB(0, 0, 0));
    }

    const CHSV indicator = CHSV(0, 0, 128);
    // Light one LED in the second row as an indicator for saturation level
    SetMainLed(12 + (current_saturation * 12 / 256), indicator);
    // Light one LED in the third row as an indicator for saturation level
    SetMainLed(24 + (current_value * 12 / 256), indicator);
  }
}
