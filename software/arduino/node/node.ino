// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min
#include <FastLedManager.hpp>
#include <NetworkManager.hpp>
#include <RadioHeadRadio.hpp>
#include <RadioStateMachine.hpp>

const int kLedPin = 0;
const int kNumLeds = 60;
// const int kNumLeds = 6; // Lantern
// const int kNumLeds = 47; // Scarf

RadioHeadRadio* radio;
NetworkManager* nm;
FastLedManager* ledManager;
RadioStateMachine* stateMachine;

void setup() {
  Serial.begin(115200);
  // Delay makes it easier to reset the board in case of failure
  delay(2000);

  pinMode(kLedPin, OUTPUT);

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  stateMachine = new RadioStateMachine(nm);

  ledManager = new FastLedManager(kNumLeds, stateMachine);
  ledManager->SetGlobalColor(CHSV(HUE_YELLOW, 255, 128));
  delay(10);
  ledManager->SetGlobalColor(CRGB(CRGB::Black));
}

unsigned long printAliveAt = 0;

void loop() {
  stateMachine->Tick();
  ledManager->RunEffect();

  if (millis() > printAliveAt) {
    Serial.println(stateMachine->GetNetworkMillis());
    printAliveAt = millis() + 1000;
  }
}
