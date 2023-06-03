#include "palette-mode.h"

#include <array>

#include "Types.hpp"
#include "buttons.h"
#include "generic/RadioStateMachine.hpp"
#include "leds.h"
#include "DisplayColorPaletteEffect.hpp"
#include "DeviceDescription.hpp"


extern DisplayColorPaletteEffect palette_effect;
extern RadioPacket set_effect_packet;
extern RadioStateMachine state_machine;
extern uint8_t kSetEffectDelay;

// Palette indices for palette mode.
std::array<uint8_t, 6> palettes = {
    8, 9, 10, 11, 12, 13,
};

const StripDescription kPaletteStrip =
    StripDescription(/*led_count=*/5, {Bright, Controller});

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