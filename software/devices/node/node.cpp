// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min
#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../generic/DeviceDescription.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

const int kLedPin = 0;

const DeviceDescription *const bike = new DeviceDescription(30, {Bright});
const DeviceDescription *const scarf = new DeviceDescription(46, {});
const DeviceDescription *const lantern = new DeviceDescription(5, {Tiny});
const DeviceDescription *const puck =
    new DeviceDescription(12, {Tiny, Circular});
const DeviceDescription *const two_side_puck =
    new DeviceDescription(24, {Tiny, Circular, Mirrored});

const DeviceDescription *const device = two_side_puck;

RadioHeadRadio *radio;
NetworkManager *nm;
FastLedManager *ledManager;
RadioStateMachine *stateMachine;

void FeedWatchdog() { WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY; }

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  stateMachine = new RadioStateMachine(nm);

  ledManager = new FastLedManager(device, stateMachine);
  // NOTE: can check if we watchdog rebooted by checking REG_PM_RCAUSE
  // See https://github.com/gjt211/SAMD21-Reset-Cause

  ledManager->PlayStartupAnimation();

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

unsigned long printAliveAt = 0;
uint8_t watchdog_counter = 0;

void loop() {
  stateMachine->Tick();
  ledManager->RunEffect();

  if (millis() > printAliveAt) {
    Serial.println(stateMachine->GetNetworkMillis());
    printAliveAt = millis() + 1000;
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
