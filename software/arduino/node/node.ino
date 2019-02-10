#include <FastLED.h>
#include <NetworkManager.hpp>
#include <RadioHeadRadio.hpp>
#include <RadioStateMachine.hpp>

const int kLedPin = 0;
#define kNumLeds 4
#define WS2812_PIN 6

CRGB leds[kNumLeds];

RadioHeadRadio* radio;
NetworkManager* nm;
RadioStateMachine* stateMachine;

void setup() {
  Serial.begin(115200);
  // Delay makes it easier to reset the board in case of failure
  delay(2000);

  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, kNumLeds);
  pinMode(kLedPin, OUTPUT);

  // Yellow LED on boot indicates a problem initializing the radio
  FastLED.showColor(CRGB(16, 16, 0));
  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  stateMachine = new RadioStateMachine(nm);

  FastLED.showColor(CRGB(0, 0, 0));
}

const int kBufLen = 10;
char buf[kBufLen];

unsigned long printAliveAt = 0;

void loop() {
  stateMachine->Tick();

  if (stateMachine->GetNetworkMillis() % 1000 < 300) {
    if (stateMachine->GetEffectIndex() == 0) {
      FastLED.showColor(CRGB(32, 0, 0));
    } else {
      FastLED.showColor(CRGB(16, 0, 16));
    }
  } else {
    if (stateMachine->GetCurrentState() == RadioState::Master) {
      FastLED.showColor(CRGB(0, 8, 0));
    } else {
      FastLED.showColor(CRGB(0, 0, 8));
    }
  }

  if (millis() > printAliveAt) {
    Serial.println(stateMachine->GetNetworkMillis());
    printAliveAt = millis() + 1000;
  }
}
