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

enum class ChooserMode {
  Color,
  Effect,
};

const int kLedPin = 0;
const int kNumLeds = 1;
const uint8_t kNumKeys = 16;
const uint8_t kLeftKey = 12;
const uint8_t kColorKey = 13;
const uint8_t kEffectKey = 14;
const uint8_t kRightKey = 15;

// This is the SetEffect delay sent - this controls the timeout before the master resumes randomly choosing the effect.
const uint8_t kEffectDelay = 60;

Adafruit_NeoTrellis trellis;

RadioHeadRadio* radio;
NetworkManager* nm;
FastLedManager* ledManager;
RadioStateMachine* stateMachine;
DisplayColorPaletteEffect* colorPaletteEffect;

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  stateMachine = new RadioStateMachine(nm);

  ledManager = new FastLedManager(kNumLeds, stateMachine);

  initTrellis();

  ledManager->SetGlobalColor(CRGB(CRGB::Black));
  colorPaletteEffect = new DisplayColorPaletteEffect(1);
}

unsigned long printAliveAt = 0;
RadioPacket fakeSetEffect;
uint8_t palettePage = 0;
uint8_t effectPage = 0;
const uint8_t kChooserPageSize = 12;
ChooserMode mode = ChooserMode::Color;

void loop() {
  stateMachine->Tick();

  switch (mode) {
    case ChooserMode::Effect:
      runChooseEffect();
      break;

    case ChooserMode::Color:
      runChooseColor();
      break;
  }

  if (millis() > printAliveAt) {
    Serial.println(stateMachine->GetNetworkMillis());
    printAliveAt = millis() + 1000;
  }
}

void runChooseEffect() {
  const uint8_t kNumEffects = ledManager->GetNumUniqueEffects();
  const uint8_t kNumEffectPages =
      (kNumEffects / kChooserPageSize) + (kNumEffects % kChooserPageSize > 0)
          ? 1
          : 0;

  // Poll the trellis and fire callbacks
  trellis.read();

  for (uint8_t i = 0; i < kChooserPageSize; i++) {
    const uint8_t effectIndex = keyIndexToEffectIndex(i);
    if (effectIndex >= kNumEffects) {
      trellis.pixels.setPixelColor(i, 0);
    } else {
      RadioPacket* currentSetEffect = stateMachine->GetSetEffect();
      CRGB rgb =
          ledManager
              ->GetEffect(ledManager->UniqueEffectNumberToIndex(effectIndex))
              ->GetRGB(0, stateMachine->GetNetworkMillis(), currentSetEffect);
      uint32_t trellisColor = rgbToTrellisColor(rgb, 128);
      trellis.pixels.setPixelColor(i, trellisColor);
    }
  }
  trellis.pixels.show();
}

void runChooseColor() {
  const uint8_t kNumPalettes = ledManager->GetCurrentEffect()->palettes.size();
  const uint8_t kNumPalettePages =
      (kNumPalettes / kChooserPageSize) + (kNumPalettes % kChooserPageSize > 0)
          ? 1
          : 0;

  // Poll the trellis and fire callbacks
  trellis.read();

  for (uint8_t i = 0; i < kChooserPageSize; i++) {
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
}

// TODO: bounds checking?
uint8_t keyIndexToEffectIndex(uint16_t keyIndex) {
  const uint8_t kNumEffects = ledManager->GetNumUniqueEffects();
  return effectPage * kChooserPageSize + keyIndex;
}

// TODO: bounds checking?
uint8_t keyIndexToPaletteIndex(uint16_t keyIndex) {
  const uint8_t kNumPalettes = ledManager->GetCurrentEffect()->palettes.size();
  return palettePage * kChooserPageSize + keyIndex;
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
      (kNumPalettes / kChooserPageSize) +
      ((kNumPalettes % kChooserPageSize > 0) ? 1 : 0);
  const uint8_t kNumEffects = ledManager->GetNumUniqueEffects();
  const uint8_t kNumEffectPages =
      (kNumEffects / kChooserPageSize) + ((kNumEffects % kChooserPageSize > 0)
          ? 1
          : 0);

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    if (evt.bit.NUM == kColorKey) {
      trellis.pixels.setPixelColor(kColorKey, 128, 128, 128);
      // Pressing the color button in color mode turns off the lights
      if (mode == ChooserMode::Color) {
        const uint8_t darkEffectIndex = kNumEffects - 1;
        setEffect.writeSetEffect(
            ledManager->UniqueEffectNumberToIndex(darkEffectIndex), kEffectDelay, 0);
        stateMachine->SetEffect(&setEffect);
      }
      mode = ChooserMode::Color;

    } else if (evt.bit.NUM == kEffectKey) {
      trellis.pixels.setPixelColor(kEffectKey, 128, 128, 128);
      mode = ChooserMode::Effect;
    } else if (evt.bit.NUM == kLeftKey) {
      trellis.pixels.setPixelColor(kLeftKey, 128, 0, 0);

      switch (mode) {
        case ChooserMode::Color:
          // Get a positive modulus
          palettePage = positive_mod8(palettePage - 1, kNumPalettePages);
          break;

        case ChooserMode::Effect:
          effectPage = positive_mod8(effectPage - 1, kNumEffectPages);
          break;
      }
    } else if (evt.bit.NUM == kRightKey) {
      trellis.pixels.setPixelColor(kRightKey, 0, 128, 0);
      switch (mode) {
        case ChooserMode::Color:
          palettePage = (palettePage + 1) % kNumPalettePages;
          break;

        case ChooserMode::Effect:
          effectPage = (effectPage + 1) % kNumEffectPages;
          break;
      }
    } else {
      trellis.pixels.setPixelColor(evt.bit.NUM, 128, 128, 128);

      switch (mode) {
        case ChooserMode::Color: {
          const uint8_t paletteIndex = keyIndexToPaletteIndex(evt.bit.NUM);
          if (paletteIndex < kNumPalettes) {
            RadioPacket* currentEffect = stateMachine->GetSetEffect();
            uint8_t currentEffectIndex =
                currentEffect->readEffectIndexFromSetEffect();
            // If the current effect is DarkEffect (which displays only black),
            // bump it down to the DisplayColorPaletteEffect so that the chosen
            // color is actually displayed.
            if (currentEffectIndex == ledManager->GetNumEffects() +
                                          ledManager->GetNumNonRandomEffects() -
                                          1) {
              currentEffectIndex--;
            }
            setEffect.writeSetEffect(currentEffectIndex, kEffectDelay, paletteIndex);
            // Send the packet twice, to make sure the network picks it up
            stateMachine->SetEffect(&setEffect);
            delay(1);
            stateMachine->SetEffect(&setEffect);
          }
        } break;

        case ChooserMode::Effect: {
          const uint8_t uniqueEffectIndex = keyIndexToEffectIndex(evt.bit.NUM);

          if (uniqueEffectIndex < kNumEffects) {
            RadioPacket* currentEffect = stateMachine->GetSetEffect();
            setEffect.writeSetEffect(
                ledManager->UniqueEffectNumberToIndex(uniqueEffectIndex), kEffectDelay,
                currentEffect->readPaletteIndexFromSetEffect());
            // Send the packet twice, to make sure the network picks it up
            stateMachine->SetEffect(&setEffect);
            delay(1);
            stateMachine->SetEffect(&setEffect);
          }
        } break;
      }
    }
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    switch (evt.bit.NUM) {
      case kLeftKey:
        trellis.pixels.setPixelColor(kLeftKey, 8, 0, 0);
        break;

      case kRightKey:
        trellis.pixels.setPixelColor(kRightKey, 0, 8, 0);
        break;

      case kEffectKey:
      case kColorKey:
        trellis.pixels.setPixelColor(evt.bit.NUM, 0);
        break;
    }
  }

  trellis.pixels.show();

  return 0;
}

uint8_t positive_mod8(uint8_t n, uint8_t d) {
  return (n % d + d) % d;
}
