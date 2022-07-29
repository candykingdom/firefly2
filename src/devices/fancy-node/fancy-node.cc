#include <Arduino.h>
#include <FastLED.h>
#include <DeviceDescription.hpp>
#include <StripDescription.hpp>
#include <vector>

#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

const int kLedPin = PB0;
const int kNeopixelPin = PA12;

static const DeviceDescription *SimpleRfBoardDescription(
    uint8_t led_count, std::vector<StripFlag> flags) {
  return new DeviceDescription(2000,
                               {
                                   new StripDescription(led_count, flags),
                               });
}

const DeviceDescription *const test_device = SimpleRfBoardDescription(1, {Bright});

RadioHeadRadio *radio;
NetworkManager *nm;
FastLedManager *led_manager;
RadioStateMachine *state_machine;

void setup() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, HIGH);


  SPI.setMISO(PA6);
  SPI.setMOSI(PA7);
  SPI.setSCLK(PA5);
  SPI.setSSEL(PA4);
  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  state_machine = new RadioStateMachine(nm);

  led_manager = new FastLedManager(test_device, state_machine);
  led_manager->PlayStartupAnimation();
}

void loop() {
  state_machine->Tick();
  led_manager->RunEffect();
}