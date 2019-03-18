/**
   Main code for an rfboard with an Adafruit NeoTrellis attached. Used for
   controlling the effects on the network.
*/

#include <Adafruit_NeoTrellis.h>

// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min
#include <FastLedManager.hpp>
#include <NetworkManager.hpp>
#include <RadioHeadRadio.hpp>
#include <RadioStateMachine.hpp>

const int kLedPin = 0;
const int kNumLeds = 1;
const uint8_t kNumKeys = 16;

Adafruit_NeoTrellis trellis;

RadioHeadRadio* radio;
NetworkManager* nm;
FastLedManager* ledManager;
RadioStateMachine* stateMachine;

void setup() {
  Serial.begin(115200);
  // Delay makes it easier to reset the board in case of failure
  delay(2000);

  pinMode(kLedPin, OUTPUT);
  ledManager = new FastLedManager(kNumLeds);
  // Yellow LED on boot indicates a problem initializing the radio
  ledManager->SetGlobalColor(CHSV(HUE_YELLOW, 255, 128));

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  stateMachine = new RadioStateMachine(nm, ledManager);

  initTrellis();

  ledManager->SetGlobalColor(CRGB(CRGB::Black));
}

void loop() {
  stateMachine->Tick();

  // Poll the trellis and fire callbacks
  trellis.read();

  const uint8_t kNumPalettes = ledManager->GetCurrentEffect()->palettes.size();
  for (uint8_t i = 0; i < kNumPalettes && i < kNumKeys; i++) {
    // Use a fake SetEffect packet to change the palette index
    RadioPacket fakeSetEffect;
    fakeSetEffect.writeSetEffect(0, 0, /* paletteIndex= */ i);
    CRGB rgb = ledManager->GetCurrentEffect()->GetRGB(
        i, stateMachine->GetNetworkMillis(), &fakeSetEffect);
    uint32_t trellisColor = rgbToTrellisColor(rgb);
    trellis.pixels.setPixelColor(i, trellisColor);
  }
  trellis.pixels.show();
}

/** Converts a key number to FastLED hue, 0-255. */
uint8_t keyIndexToHue(uint16_t keyIndex) {
  return (uint8_t)(keyIndex * 255 / kNumKeys);
}

/** Converts a hue to a Trellis color bytestring, 0x00RRGGBB. */
uint32_t hueToTrellisColor(uint8_t hue, uint8_t value) {
  CRGB rgb = CHSV(hue, 255, value);
  rgbToTrellisColor(rgb);
}

uint32_t rgbToTrellisColor(CRGB rgb) {
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

  // Reset all keys' color
  for (int i = 0; i < kNumKeys; i++) {
    trellis.pixels.setPixelColor(i, 0);
  }
  trellis.pixels.show();
}

RadioPacket setEffect;
TrellisCallback trellisHandler(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    trellis.pixels.setPixelColor(
        evt.bit.NUM,
        hueToTrellisColor(keyIndexToHue(evt.bit.NUM), 128));  // on rising
    // TODO: bounds check the key number against the number of palettes
    setEffect.writeSetEffect(1, 10, evt.bit.NUM);
    stateMachine->SetEffect(&setEffect);
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    // or is the pad released?
    trellis.pixels.setPixelColor(
        evt.bit.NUM, hueToTrellisColor(keyIndexToHue(evt.bit.NUM), 32));
  }

  trellis.pixels.show();

  return 0;
}
