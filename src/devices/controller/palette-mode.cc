#include "palette-mode.h"

#include <array>

#include "DeviceDescription.hpp"
#include "DisplayColorPaletteEffect.hpp"
#include "Types.hpp"
#include "buttons.h"
#include "generic/RadioStateMachine.hpp"
#include "leds.h"

extern DisplayColorPaletteEffect palette_effect;
extern RadioPacket set_effect_packet;
extern RadioStateMachine state_machine;
extern uint8_t kSetEffectDelay;

// Palette indices for palette mode.
std::array<std::array<uint8_t, 6>, 3> palettes = {
    std::array<uint8_t, 6>{8, 9, 10, 11, 12, 13},
    std::array<uint8_t, 6>{14, 15, 16, 17, 18, 19},
    std::array<uint8_t, 6>{20, 21, 0, 1, 2, 3},
};

const StripDescription kPaletteStrip =
    StripDescription(/*led_count=*/5, {Bright, Controller});

void WritePalettesToMainLeds() {
  for (uint8_t i = 0; i < 36; i++) {
    if ((i % 12) > 4 && (i % 12) <= 6) {
      SetMainLed(i, CRGB(0, 0, 0));
    } else {
      set_effect_packet.writeSetEffect(
          /*effect_index=*/0, /*delay=*/1,
          /*palette_index=*/palettes[palette_carousel][i / 6]);
      uint8_t led_index = (i % 6);
      SetMainLed(
          i, palette_effect.GetRGB(led_index, state_machine.GetNetworkMillis(),
                                   kPaletteStrip, &set_effect_packet));
    }
  }
}

void RunPaletteMode() {
  WritePalettesToMainLeds();

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

  for (uint8_t i = 0; i < 3; i++) {
    if (bottom_buttons[i].Pressed()) {
      palette_carousel = i;
    }
  }
  SetBottomButtonLeds(0, 0, 0);
  SetBottomButtonLed(palette_carousel + 1, kButtonActiveBrightness);

  if (pressed) {
    set_effect_packet.writeSetEffect(state_machine.GetEffectIndex(),
                                     kSetEffectDelay,
                                     palettes[palette_carousel][palette_index]);
    state_machine.SetEffect(&set_effect_packet);
  }

  for (uint8_t i = 0; i < 3; i++) {
    if (bottom_buttons[i].Held()) {
      sub_mode = SubMode::ChooseSlot;
      config_carousel = i;
    }
  }
}

void RunPaletteConfig() {
  static uint8_t selected_slot = 255;
  static uint8_t current_palette = 0;

  uint8_t blink_brightness = (millis() / 500) % 2 == 0
                                 ? kButtonBlinkBrightness
                                 : kButtonActiveBrightness;

  if (sub_mode == SubMode::ChooseSlot) {
    SetLeftButtonLeds(blink_brightness, blink_brightness, blink_brightness);
    SetRightButtonLeds(blink_brightness, blink_brightness, blink_brightness);
    SetBottomButtonLeds(0, 0, 0);
    SetBottomButtonLed(config_carousel + 1, kButtonActiveBrightness);
    WritePalettesToMainLeds();

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
      left_buttons[0].SetRepeatDelay(300);
      right_buttons[0].SetRepeatDelay(300);
    }

  } else {
    if (left_buttons[0].Pressed() || left_buttons[0].Held() ||
        left_buttons[0].Repeated()) {
      current_palette = (current_palette - 1) % Effect::palettes().size();
    } else if (right_buttons[0].Pressed() || right_buttons[0].Held() ||
               right_buttons[0].Repeated()) {
      current_palette = (current_palette + 1) % Effect::palettes().size();
    }

    for (uint8_t i = 0; i < 3; i++) {
      if (bottom_buttons[i].Rose()) {
        // Pressing the blinking bottom button saves the current color. Pressing
        // the other bottom buttons cancels and returns to normal mode.
        if (i == config_carousel) {
          palettes[config_carousel][selected_slot] = current_palette;
        }
        sub_mode = SubMode::Normal;
      }
    }
    SetLeftButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness, 0);
    SetRightButtonLeds(kButtonActiveBrightness, kButtonActiveBrightness, 0);

    // Blink the carousel button
    SetBottomButtonLeds(0, 0, 0);
    SetBottomButtonLed(config_carousel + 1, blink_brightness);

    set_effect_packet.writeSetEffect(/*effect_index=*/0, kSetEffectDelay,
                                     current_palette);
    for (uint8_t i = 0; i < 12; i++) {
      SetMainLed(i, palette_effect.GetRGB(i, state_machine.GetNetworkMillis(),
                                          kPaletteStrip, &set_effect_packet));
    }
    for (uint8_t i = 12; i < 36; i++) {
      SetMainLed(i, CRGB(0, 0, 0));
    }
  }
}
