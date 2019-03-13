/**
 * Main code for an rfboard with an Adafruit NeoTrellis attached. Used for
 * controlling the effects on the network.
 */

#include <Adafruit_NeoTrellis.h>
#include <FastLED.h>
#include <NetworkManager.hpp>
#include <RadioHeadRadio.hpp>
#include <RadioStateMachine.hpp>

Adafruit_NeoTrellis trellis;

const int kLedPin = 0;
const int kNumLeds = 1;
const uint8_t kNumKeys = 16;

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

  initTrellis();

  FastLED.showColor(CRGB(0, 0, 0));
}

void loop() {
  stateMachine->Tick();

  // Normal state machine code: this is the same as regular nodes
  if (stateMachine->GetNetworkMillis() % 1000 < 300) {
    if (stateMachine->GetEffectIndex() < 2) {
      FastLED.showColor(
          CHSV(stateMachine->GetSetEffect()->readHueFromSetEffect(), 255, 32));
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

  // Poll the trellis and fire callbacks
  trellis.read();
}

/** Converts a key number to FastLED hue, 0-255. */
uint8_t keyIndexToHue(uint16_t keyIndex) {
  return (uint8_t)(keyIndex * 255 / kNumKeys);
}

/** Converts a hue to a Trellis color bytestring, 0x00RRGGBB. */
uint32_t hueToTrellisColor(uint8_t hue, uint8_t value) {
  CRGB rgb = CHSV(hue, 255, value);
  return (rgb.r << 16) | (rgb.g << 8) | rgb.b;
}

void initTrellis() {
  if (!trellis.begin()) {
    Serial.println("Could not start trellis");
    delay(5000);
    return;
  }

  // Activate all keys and set callbacks
  for (int i = 0; i < kNumKeys; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, trellisHandler);
  }

  // Start dim
  for (int i = 0; i < kNumKeys; i++) {
    trellis.pixels.setPixelColor(i, hueToTrellisColor(keyIndexToHue(i), 32));
  }
  trellis.pixels.show();
}

RadioPacket setEffect;
TrellisCallback trellisHandler(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    trellis.pixels.setPixelColor(
        evt.bit.NUM,
        hueToTrellisColor(keyIndexToHue(evt.bit.NUM), 128));  // on rising
    setEffect.writeSetEffect(1, 10, keyIndexToHue(evt.bit.NUM));
    stateMachine->SetEffect(&setEffect);
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    // or is the pad released?
    trellis.pixels.setPixelColor(
        evt.bit.NUM, hueToTrellisColor(keyIndexToHue(evt.bit.NUM), 32));
  }

  trellis.pixels.show();

  return 0;
}
