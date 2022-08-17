#include "../../arduino/FastLedControllerManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"
#include "Arduino.h"

const int kLedPin = 0;

RadioHeadRadio* radio;
NetworkManager* nm;
FastLedControllerManager* led_manager;
RadioStateMachine* state_machine;

void setup() {
  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  state_machine = new RadioStateMachine(nm);
  led_manager = new FastLedControllerManager(state_machine);
}

unsigned long print_alive_at = 0;

void loop() {
  state_machine->Tick();
  led_manager->RunEffect();

  delay(10);
}