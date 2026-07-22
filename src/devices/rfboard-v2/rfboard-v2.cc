#include <Arduino.h>
#include <FastLED.h>
#include <FlashStorage_STM32.h>

#include <DeviceDescription.hpp>
#include <Devices.hpp>
#include <StripDescription.hpp>
#include <vector>

#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

// The onboard addressable LED, which then feeds the external LED connector
constexpr int kPinLedInternal = PA4; 
constexpr int kPinRedLed = PB1;
constexpr int kPinSw1 = PB6;
constexpr int kPinSw2 = PB7;

constexpr int kPin5vDivided = PA0;
constexpr int kPinCc1 = PA1;
constexpr int kPinCc2 = PB2;
constexpr int kPinVibrationSensor = PA8;

RadioHeadRadio *radio = new RadioHeadRadio();
NetworkManager network_manager(radio);
RadioStateMachine state_machine(&network_manager);
FastLedManager *led_manager;

void setup() {
  pinMode(kPinLedInternal, OUTPUT);
  pinMode(kPinRedLed, OUTPUT);
  pinMode(kPinSw1, INPUT_PULLUP);
  pinMode(kPinSw2, INPUT_PULLUP);

  pinMode(kPin5vDivided, INPUT_ANALOG);
  pinMode(kPinCc1, INPUT_ANALOG);
  pinMode(kPinCc2, INPUT_ANALOG);
  pinMode(kPinVibrationSensor, INPUT);

  // TODO: support reading the device description from flash
  led_manager = new FastLedManager(Devices::current, &state_machine);

  if (!radio->Begin()) {
    led_manager->FatalErrorAnimation();
  }

  // NOTE: can check if we watchdog rebooted by checking REG_PM_RCAUSE
  // See https://github.com/gjt211/SAMD21-Reset-Cause
  led_manager->PlayStartupAnimation();

  // TODO: configure and enable the watchdog
}

void loop() {
  digitalWrite(kPinRedLed, (millis() / 100) % 2);
  state_machine.Tick();
  led_manager->RunEffect();

  // Test code - remove after bringup
  if (false) {
    digitalWrite(kPinRedLed, digitalRead(kPinSw1));
  }

  if (false) {
    digitalWrite(kPinRedLed, digitalRead(kPinSw2));
  }

  if (false) {
    digitalWrite(kPinRedLed, digitalRead(kPinVibrationSensor));
  }
}
