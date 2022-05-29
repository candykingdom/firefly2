// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min
#include "../../../lib/arduino/FastLedManager.hpp"
#include "../../../lib/arduino/RadioHeadRadio.hpp"
#include "../../../lib/device/DeviceDescription.hpp"
#include "../../../lib/network/NetworkManager.hpp"
#include "../../../lib/radio/RadioStateMachine.hpp"
#include "../../../lib/storage/Storage.hpp"

const int kLedPin = 0;

const DeviceDescription *const fallback_device = new DeviceDescription(10, {});
const DeviceDescription *device;

RadioHeadRadio *radio;
NetworkManager *nm;
FastLedManager *led_manager;
RadioStateMachine *state_machine;

void FeedWatchdog() { WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY; }

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  state_machine = new RadioStateMachine(nm);

  bool valid_description = GetDeviceDescription(device);
  if (!valid_description) {
    // If the number of LEDs looks roughly correct, try to use the bit-banged
    // description, otherwise, use the fallback.
    if (device->led_count < 1 || device->led_count > 200) {
      device = fallback_device;
    }
  }

  led_manager = new FastLedManager(device, state_machine);
  // NOTE: can check if we watchdog rebooted by checking REG_PM_RCAUSE
  // See https://github.com/gjt211/SAMD21-Reset-Cause

  led_manager->PlayStartupAnimation(!valid_description);

  // Set up the watchdog timer: this will reset the processor if it hasn't
  // 'fed' the watchdog in ~100ms.
  // Inspired by
  // https://github.com/adafruit/Adafruit_SleepyDog/blob/master/utility/WatchdogSAMD.cpp
  // Disable watchdog and wait for it to take effect
  WDT->CTRL.reg = 0;
  while (WDT->STATUS.bit.SYNCBUSY) {
  }

  // Configure the watchdog clock
  // Note: feeding the watchdog is slightly expensive - it seems to take O(1
  // tick of the oscillator). So, don't use a prescaler. I tried using a /32
  // prescaler, and the animations were horribly slow.
  // Generic clock generator 2, no divisor
  GCLK->GENDIV.reg = GCLK_GENDIV_ID(2);
  // Enable clock generator 2 using low-power 32KHz oscillator.
  GCLK->GENCTRL.reg =
      GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K;
  while (GCLK->STATUS.bit.SYNCBUSY) {
  }
  // WDT clock = clock gen 2
  GCLK->CLKCTRL.reg =
      GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2;

  // Now, configure
  WDT->INTENCLR.bit.EW = 1;  // Disable early warning interrupt
  // Documentation for this field here:
  // http://asf.atmel.com/docs/latest/samd20/html/group__asfdoc__sam0__wdt__group.html
  WDT->CONFIG.bit.PER =
      9;  // WDT clock runs at ~32kHz (effective). A value of 9 means 4096
          // cycles, which means a period of ~128ms.
  WDT->CTRL.bit.WEN = 0;  // Disable window mode
  while (WDT->STATUS.bit.SYNCBUSY) {
  }

  FeedWatchdog();
  // Now, enable the watchdog
  WDT->CTRL.bit.ENABLE = 1;
  while (WDT->STATUS.bit.SYNCBUSY) {
  }
}

unsigned long print_alive_at = 0;
uint8_t watchdog_counter = 0;

void loop() {
  state_machine->Tick();
  led_manager->RunEffect();

  if (millis() > print_alive_at) {
    Serial.println(state_machine->GetNetworkMillis());
    print_alive_at = millis() + 1000;
  }

  // Feeding the watchdog takes some time. Each run through this loop should
  // take less than 10ms, so it should be safe to feed the watchdog every 5
  // cycles = ~50ms.
  if (watchdog_counter > 5) {
    FeedWatchdog();
    watchdog_counter = 0;
  }
  watchdog_counter++;
}
