// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min
#include <FastLedManager.hpp>
#include <NetworkManager.hpp>
#include <RadioHeadRadio.hpp>
#include <RadioStateMachine.hpp>

const int kLedPin = 0;
const int kNumLeds = 30;  // Bike
// const int kNumLeds = 46;  // Scarf
// const int kNumLeds = 5; // Lantern

const DeviceType deviceType = DeviceType::Bike;

RadioHeadRadio* radio;
NetworkManager* nm;
FastLedManager* ledManager;
RadioStateMachine* stateMachine;

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  stateMachine = new RadioStateMachine(nm);

  ledManager = new FastLedManager(kNumLeds, deviceType, stateMachine);
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
