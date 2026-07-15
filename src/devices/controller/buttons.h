#pragma once

#include <Types.hpp>

#include "leds.h"

constexpr uint8_t kLeftButtonLed = 41;
constexpr uint8_t kRightButtonLed = 39;
constexpr uint8_t kBottomButtonLed = 40;

constexpr uint8_t kButtonActiveBrightness = 64;
constexpr uint8_t kButtonPressedBrightness = 255;

// button_index: 1-3
void SetLeftButtonLed(uint8_t button_index, uint8_t brightness);
void SetLeftButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3);

// button_index: 1-3
void SetRightButtonLed(uint8_t button_index, uint8_t brightness);
void SetRightButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3);

// button_index: 1-3
void SetBottomButtonLed(uint8_t button_index, uint8_t brightness);
void SetBottomButtonLeds(uint8_t button1, uint8_t button2, uint8_t button3);