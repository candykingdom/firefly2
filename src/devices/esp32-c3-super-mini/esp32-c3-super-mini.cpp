#include <Arduino.h>

#include "../../arduino/FastLedManager.hpp"
#include "../../generic/FireflyNetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"
#include "../../lib/fake-radio/FakeRadio.hpp"
#include "Devices.hpp"
#include "EspNowRadio.hpp"
#include "SerialRadio.hpp"

#define FASTLED_ESP32_I2S_NUM_DMA_BUFFERS 4

// EspNowRadio* radio = new EspNowRadio();
SerialRadio* radio = new SerialRadio();

FireflyNetworkManager nm(radio);
RadioStateMachine state_machine(&nm);
FastLedManager* led_manager;

void setup() {
  Serial.begin(115200);
  led_manager = new FastLedManager(Devices::current, &state_machine);
  if (!radio->Begin()) {
    led_manager->FatalErrorAnimation();
  }
  // if (!serial_radio->Begin()) {
  //   led_manager->FatalErrorAnimation();
  // }
  led_manager->PlayStartupAnimation();
}

void loop() {
  state_machine.Tick();
  led_manager->RunEffect();
}