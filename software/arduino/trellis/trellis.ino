/**
   Main code for an rfboard with an Adafruit NeoTrellis attached. Used for
   controlling the effects on the network.
*/

#include <Adafruit_NeoTrellis.h>

// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min
#include <DisplayColorPaletteEffect.hpp>
#include <FastLedManager.hpp>
#include <NetworkManager.hpp>
#include <RadioHeadRadio.hpp>
#include <RadioStateMachine.hpp>

const int kLedPin = 0;
const int kNumLeds = 1;
const uint8_t kNumKeys = 16;
const uint8_t kLeftKey = 12;
const uint8_t kRightKey = 15;

Adafruit_NeoTrellis trellis;

RadioHeadRadio* radio;
NetworkManager* nm;
FastLedManager* ledManager;
RadioStateMachine* stateMachine;
DisplayColorPaletteEffect* colorPaletteEffect;

void setup() {
  Serial.begin(115200);
  // Delay makes it easier to reset the board in case of failure
  delay(500);

  pinMode(kLedPin, OUTPUT);

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  stateMachine = new RadioStateMachine(nm);

  ledManager = new FastLedManager(kNumLeds, stateMachine);
  ledManager->SetGlobalColor(CRGB(CRGB::Black));

  initTrellis();

  ledManager->SetGlobalColor(CRGB(CRGB::Black));
  colorPaletteEffect = new DisplayColorPaletteEffect(1);
}

unsigned long printAliveAt = 0;
RadioPacket fakeSetEffect;
uint8_t palettePage = 0;
const uint8_t kPalettePageSize = 12;

void loop() {
  const uint8_t kNumPalettes = ledManager->GetCurrentEffect()->palettes.size();
  const uint8_t kNumPalettePages =
      (kNumPalettes / kPalettePageSize) + (kNumPalettes % kPalettePageSize > 0)
          ? 1
          : 0;

  stateMachine->Tick();
  ledManager->RunEffect();

  // Poll the trellis and fire callbacks
  trellis.read();

  for (uint8_t i = 0; i < kPalettePageSize; i++) {
    const uint8_t paletteIndex = keyIndexToPaletteIndex(i);
    if (paletteIndex >= kNumPalettes) {
      trellis.pixels.setPixelColor(i, 0);
    } else {
      // Use a fake SetEffect packet to change the palette index
      fakeSetEffect.writeSetEffect(0, 0, paletteIndex);
      CRGB rgb = colorPaletteEffect->GetRGB(0, stateMachine->GetNetworkMillis(),
                                            &fakeSetEffect);
      uint32_t trellisColor = rgbToTrellisColor(rgb, 128);
      trellis.pixels.setPixelColor(i, trellisColor);
    }
  }
  trellis.pixels.show();

  if (millis() > printAliveAt) {
    Serial.println(stateMachine->GetNetworkMillis());
    printAliveAt = millis() + 1000;
  }
}

// TODO: bounds checking?
uint8_t keyIndexToPaletteIndex(uint16_t keyIndex) {
  const uint8_t kNumPalettes = ledManager->GetCurrentEffect()->palettes.size();
  return palettePage * kPalettePageSize + keyIndex;
}

/** Converts a key number to FastLED hue, 0-255. */
uint8_t keyIndexToHue(uint16_t keyIndex) {
  return (uint8_t)(keyIndex * 255 / kNumKeys);
}

/** Converts a hue to a Trellis color bytestring, 0x00RRGGBB. */
uint32_t hueToTrellisColor(uint8_t hue, uint8_t value) {
  CRGB rgb = CHSV(hue, 255, value);
  rgbToTrellisColor(rgb, 255);
}

uint32_t rgbToTrellisColor(CRGB rgb, uint8_t value) {
  return ((rgb.r * value / 255) << 16) | ((rgb.g * value / 255) << 8) |
         (rgb.b * value / 255);
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

  trellis.pixels.setPixelColor(kLeftKey, 8, 0, 0);
  trellis.pixels.setPixelColor(kRightKey, 0, 8, 0);
  trellis.pixels.show();
}

RadioPacket setEffect;
TrellisCallback trellisHandler(keyEvent evt) {
  const uint8_t kNumPalettes = ledManager->GetCurrentEffect()->palettes.size();
  const uint8_t kNumPalettePages =
      (kNumPalettes / kPalettePageSize) +
      ((kNumPalettes % kPalettePageSize > 0) ? 1 : 0);

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    if (evt.bit.NUM == kLeftKey) {
      trellis.pixels.setPixelColor(kLeftKey, 128, 0, 0);
      // Get a positive modulus
      palettePage = ((palettePage - 1) % kNumPalettePages + kNumPalettePages) %
                    kNumPalettePages;
    } else if (evt.bit.NUM == kRightKey) {
      trellis.pixels.setPixelColor(kRightKey, 0, 128, 0);
      palettePage = (palettePage + 1) % kNumPalettePages;
    } else {
      trellis.pixels.setPixelColor(evt.bit.NUM, 128, 128, 128);

      const uint8_t paletteIndex = keyIndexToPaletteIndex(evt.bit.NUM);
      if (paletteIndex < kNumPalettes) {
        setEffect.writeSetEffect(1, 10, keyIndexToPaletteIndex(evt.bit.NUM));
        stateMachine->SetEffect(&setEffect);
      }
    }
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    if (evt.bit.NUM == kLeftKey) {
      trellis.pixels.setPixelColor(kLeftKey, 8, 0, 0);
    } else if (evt.bit.NUM == kRightKey) {
      trellis.pixels.setPixelColor(kRightKey, 0, 8, 0);
    } else {
      // trellis.pixels.setPixelColor(evt.bit.NUM, 0);
    }
  }

  trellis.pixels.show();

  return 0;
}
