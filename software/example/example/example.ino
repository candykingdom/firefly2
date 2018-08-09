#include <NetworkManager.hpp>
#include <RFM69Radio.hpp>
#include <FastLED.h>

const int kLedPin = 0;
#define kNumLeds 1
#define WS2812_PIN 6

CRGB leds[kNumLeds];

RFM69Radio* radio;
NetworkManager* nm;

void setup() {
  radio = new RFM69Radio();
  nm = new NetworkManager(radio);

  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, kNumLeds);
  pinMode(kLedPin, OUTPUT);
}

const int kBufLen = 10;
char buf[kBufLen];

void loop() {
  leds[0] = CHSV(random(100), 100, 100);
  FastLED.show();
  delay(500);
}
