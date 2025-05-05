#include <Arduino.h>
#include <FastLED.h>
#include <STM32RTC.h>
#include <Wire.h>
#include <arduino-timer.h>
#include <button-filter.h>
#include <median-filter.h>

#include <array>

#include "Battery.hpp"
#include "FakeLedManager.hpp"
#include "analog-button.h"
#include "arduino/RadioHeadRadio.hpp"
#include "buttons.h"
#include "color-mode.h"
#include "config.h"
#include "generic/NetworkManager.hpp"
#include "generic/RadioStateMachine.hpp"
#include "leds.h"
#include "palette-mode.h"

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
constexpr int kModeSwitch = PA6;
constexpr int kBatteryPin = PA7;

// Note: for some reason, `PA4` has a resolved pin number of 196, which confuses
// FastLED.
constexpr int kNeopixelPin = PA_4;

const StripDescription kRowStrip =
    StripDescription(/*led_count=*/12, {Bright, Controller});
const DeviceDescription kRowDescription(2000, {kRowStrip});

extern constexpr uint8_t kSetEffectDelay = 60;

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

uint8_t effect_index = 0;
uint8_t palette_index = 0;
RadioPacket set_effect_packet;
RadioPacket control_packet;
DisplayColorPaletteEffect palette_effect;

MedianFilter<uint16_t, uint16_t, 5> mode_switch(
    filter_functions::ForAnalogRead<kModeSwitch>());

// On controller v1.1, the filter cap on the battery voltage divider has a time
// constant of ~0.5s, so when the device is cold-booted, it takes a couple of
// seconds before the read battery level stabilizes. This timer ignores low
// battery detection until that happens.
// TODO(adam): replace the caps with 0.1uF and remove this timer
CountDownTimer startup_battery_timer{3000};

// Cutoff under load. The max power consumption of the controller is about
// 500mA, at which load the battery voltage will be moderately lower than the
// open-cell voltage.
// TODO(adam): tune this
static constexpr float kBatteryLowCutoff = 3.4;

// Once we detect low battery, the battery must hit this voltage before turning
// back on. This is about 50% charged (open voltage).
static constexpr float kBatteryResume = 3.85;

// Heavy filtering
static constexpr uint8_t kBatteryFilterAlpha = 1;
static constexpr uint8_t kBatteryMedianFilterSize = 5;
MedianFilter<uint16_t, uint16_t, kBatteryMedianFilterSize>
    battery_median_filter{filter_functions::ForAnalogRead<kBatteryPin>()};
ExponentialMovingAverageFilter<uint16_t> battery_average_filter{
    []() { return battery_median_filter.GetFilteredValue(); },
    kBatteryFilterAlpha};
bool battery_filters_initialized = false;

uint16_t battery_at_boot = 0;
bool battery_low = false;

void LowBatteryMode() {
  FastLED.clearData();
  leds[kStatusLeft] = CRGB(8, 0, 0);
  FastLED.show();
  radio.sleep();
}

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
  Effect *current_effect = led_manager.GetEffect(real_effect_index);
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

// See
// https://community.st.com/s/question/0D53W00000BLufdSAD/how-do-i-jump-from-application-code-to-bootloader-without-toggling-the-boot0-pin-on-stm32h745
// and
// https://community.st.com/s/article/STM32H7-bootloader-jump-from-application
void JumpToBootloader() {
  void (*SysMemBootJump)(void);

  /* Set the address of the entry point to bootloader */
  volatile uint32_t BootAddr = 0x1FFF0000;

  /* Disable all interrupts */
  __disable_irq();

  /* Disable Systick timer */
  SysTick->CTRL = 0;

  /* Set the clock to the default state */
  HAL_RCC_DeInit();

  /* Clear Interrupt Enable Register & Interrupt Pending Register */
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICPR[0] = 0xFFFFFFFF;

  /* Re-enable all interrupts */
  __enable_irq();

  /* Set up the jump to booloader address + 4 */
  SysMemBootJump = (void (*)(void))(*((uint32_t *)((BootAddr + 4))));

  /* Set the main stack pointer to the bootloader stack */
  __set_MSP(*(uint32_t *)BootAddr);

  /* Call the function to jump to bootloader location */
  SysMemBootJump();

  /* Jump is done successfully */
  while (1) {
    /* Code should never reach this loop */
  }
}

// Arbitrary (valid) date
static constexpr uint32_t kEpochTimer = 1451606400;
static constexpr uint32_t kEpochReset = 1441606400;

void setup() {
  /*
   * When the reset pin is pulled high the following operations take place:
   * 0. The system is started normally and the timer is set to `kEpochTimer`
   * 1. Within the first second (before the timer ticks) the reset pin is pulled
   *    high
   * 2. The CPU is reset but not the timer
   * 3. `setup()` is re-entered and this time the timer is set to `kEpochTimer`
   * 4. The timer is set to `kEpochReset`
   * 5. The CPU is software reset to ensure all registers are zeroed out
   * 6. `setup()` is re-entered and this time the timer is set to `kEpochReset`
   * 7. The timer is set to `kEpochTimer` (in case the reset pin is pulled high
   *    in the next second and we need to do the full restart again)
   * 8. The bootloader is entered and never returns
   */
  STM32RTC &rtc = STM32RTC::getInstance();
  rtc.begin(/*resetTime=*/false);
  // The CH340X triggers a reset on initial power-up. If the reset delay is less
  // than 100ms, don't enter the bootloader, so that we run the application code
  // when switched on.
  uint32_t sub_seconds = rtc.getSubSeconds();
  if (rtc.getEpoch() == kEpochTimer && sub_seconds > 100) {
    rtc.setEpoch(kEpochReset);
    // This call never returns
    NVIC_SystemReset();
  } else if (rtc.getEpoch() == kEpochReset) {
    rtc.setEpoch(kEpochTimer);
    // This call never returns
    JumpToBootloader();
  }
  rtc.setEpoch(kEpochTimer);
  rtc.setSubSeconds(0);

  Serial2.begin(115200);
  // Note: must have a short delay here before printing. The bootloader
  // double-taps the reset line - we can't start writing to serial after the
  // first reset, or the bootloader won't be able to use the serial port.
  delay(200);
  Serial2.println("Booting...");

  // check if nBOOT_SEL bit is set
  if (!(FLASH->OPTR & FLASH_OPTR_nBOOT_SEL)) {
    // unlock flash/option
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
    FLASH->OPTKEYR = 0x08192A3B;
    FLASH->OPTKEYR = 0x4C5D6E7F;

    while (FLASH->SR & FLASH_SR_BSY1);

    // Set nBOOT_SEL bit
    FLASH->OPTR |= FLASH_OPTR_nBOOT_SEL;

    // write
    FLASH->CR |= FLASH_CR_OPTSTRT;
    while (FLASH->SR & FLASH_SR_BSY1);
  }

  startup_battery_timer.Reset();

  pinMode(kBatteryPin, INPUT);

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

  for (uint8_t i = 0; i < 3; i++) {
    pinMode(kLeftButtons[i], INPUT_PULLUP);
    pinMode(kRightButtons[i], INPUT_PULLUP);
    pinMode(kBottomButtons[i], INPUT_PULLUP);
  }

  // TODO: change this to one of the function buttons, and extract the button
  // number into a constant.
  if (!digitalRead(kLeftButtons[0])) {
    leds[kStatusLeft] = CHSV(0, 0, 32);
    FastLED.show();
    // Give time for the battery input filter capacitor to charge.
    // TODO: remove this once Adam replace filter caps on controller boards.
    delay(2000);
    battery_at_boot = analogRead(kBatteryPin);

    uint16_t battery_val =
        battery_at_boot - BatteryVoltageToRawReading(kBatteryEmpty);
    constexpr uint16_t battery_range =
        BatteryVoltageToRawReading(kBatteryFull) -
        BatteryVoltageToRawReading(kBatteryEmpty);
    static_assert(battery_range != 0);
    if (battery_val > battery_range) {
      battery_val = 0;
    }

    // Red is hue 0
    uint8_t hue = (battery_val * HUE_GREEN) / battery_range;
    leds[kStatusLeft] = CHSV(hue, 255, 255);
    FastLED.show();
    delay(5000);
  }
}

void loop() {
  if (startup_battery_timer.Expired()) {
    if (!battery_filters_initialized) {
      for (uint8_t i = 0; i < kBatteryMedianFilterSize; i++) {
        battery_median_filter.Run();
      }
      battery_average_filter.Initialize(analogRead(kBatteryPin));
      battery_average_filter.Run();

      battery_median_filter.SetMinRunInterval(100);
      battery_average_filter.SetMinRunInterval(100);
      battery_filters_initialized = true;
    }
    battery_median_filter.Run();
    battery_average_filter.Run();

    if (!battery_low && battery_average_filter.GetFilteredValue() <
                            BatteryVoltageToRawReading(kBatteryLowCutoff)) {
      battery_low = true;
      LowBatteryMode();
    }

    if (battery_low) {
      if (battery_average_filter.GetFilteredValue() >
          BatteryVoltageToRawReading(kBatteryResume)) {
        battery_low = false;
      } else {
        delay(100);
        return;
      }
    }
  }

  state_machine.Tick();

  for (uint8_t i = 0; i < 3; i++) {
    left_buttons[i].Run();
    right_buttons[i].Run();
    bottom_buttons[i].Run();
  }

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
    sub_mode = SubMode::Normal;
  }
  prev_mode = mode;
  prev_sub_mode = sub_mode;

  switch (mode) {
    case ControllerMode::Effect:
      RunEffectMode();
      break;

    case ControllerMode::DirectColor:
      if (sub_mode == SubMode::Normal) {
        RunColorMode();
      } else {
        RunColorConfig();
      }
      break;

    case ControllerMode::DirectPalette:
      if (sub_mode == SubMode::Normal) {
        RunPaletteMode();
      } else {
        RunPaletteConfig();
      }
      break;
  }

  FastLED.show();
  delay(5);
}
