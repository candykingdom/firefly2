// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min

#include <DeviceDescription.hpp>
#include <DeviceTable.hpp>
#include <Storage.hpp>
#include <Types.hpp>

const uint16_t DEVICE_INDEX = 1;

const int kLedPin = 0;

bool successful_write = true;

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);

  // Turn off the on-board LED
  CRGB leds[] = {CRGB::Black};
  FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, 1).setCorrection(TypicalLEDStrip);
  FastLED.showColor(CRGB(0, 0, 0));

  delay(2000);

  if (DEVICE_INDEX >= device_table.size() || DEVICE_INDEX == 0) {
    Serial.println("Device index out of range!");
    Serial.print(DEVICE_INDEX);
    Serial.print(" must be in the range [1,");
    Serial.print(device_table.size());
    Serial.println(")");
    successful_write = false;
  } else {
    const DeviceDescription* device = device_table[DEVICE_INDEX];
    Serial.print("Writing index ");
    Serial.print(DEVICE_INDEX);
    Serial.print(": ");
    Serial.println(device->name);
    PutDeviceIndex(DEVICE_INDEX);
    successful_write = GetDeviceIndex() == DEVICE_INDEX;
    if (!successful_write) {
      Serial.println("Could not write to storage!");
    } else {
      Serial.println("Write successful!");
    }
  }
}

void loop() {
  if (successful_write) {
    for (uint8_t i = 0; i < 2; ++i) {
      digitalWrite(kLedPin, HIGH);
      delay(200);
      digitalWrite(kLedPin, LOW);
      delay(200);
    }

    while (true) {
    }
  }

  digitalWrite(kLedPin, HIGH);
  delay(200);
  digitalWrite(kLedPin, LOW);
  delay(200);
}
