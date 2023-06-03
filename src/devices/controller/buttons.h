#pragma once

#include "Types.hpp"
#include <button-filter.h>

#include "leds.h"
#include "config.h"

constexpr std::array<uint8_t, 3> kLeftButtons = {PB2, PB14, PB15};
constexpr std::array<uint8_t, 3> kRightButtons = {PA8, PC6, PC7};
constexpr std::array<uint8_t, 3> kBottomButtons = {PA11, PA12, PA15};

constexpr uint8_t kLeftButtonLed = 41;
constexpr uint8_t kRightButtonLed = 39;
constexpr uint8_t kBottomButtonLed = 40;

constexpr uint8_t kButtonActiveBrightness = 64;
constexpr uint8_t kButtonBlinkBrightness = 128;
constexpr uint8_t kButtonPressedBrightness = 255;

constexpr uint16_t kButtonHeldMillis = 800;

// button_index: 1-3
void SetLeftButtonLed(uint8_t button_index, uint8_t brightness);
void SetLeftButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3);

// button_index: 1-3
void SetRightButtonLed(uint8_t button_index, uint8_t brightness);
void SetRightButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3);

// button_index: 1-3
void SetBottomButtonLed(uint8_t button_index, uint8_t brightness);
void SetBottomButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3);

extern std::array<ButtonFilter, 3> left_buttons;
extern std::array<ButtonFilter, 3> right_buttons;
extern std::array<ButtonFilter, 3> bottom_buttons;
