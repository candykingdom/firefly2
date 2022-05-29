// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min

#include "../../../lib/device/DeviceDescription.hpp"
#include "../../../lib/storage/Storage.hpp"
const int kLedPin = 0;

// Common descriptions
const DeviceDescription *const bike = new DeviceDescription(30, {Bright});
const DeviceDescription *const scarf = new DeviceDescription(46, {});
const DeviceDescription *const lantern = new DeviceDescription(5, {Tiny});
const DeviceDescription *const puck =
    new DeviceDescription(12, {Tiny, Circular});
const DeviceDescription *const two_side_puck =
    new DeviceDescription(24, {Tiny, Circular, Mirrored});

const DeviceDescription *const device = two_side_puck;

bool successful_write;

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);

  Serial.print("Writing DeviceDescription version ");
  Serial.print(device->description_version);
  Serial.println("...");
  successful_write = PutDeviceDescription(device);

  if (successful_write) {
    Serial.println("Write successful!");
  } else {
    Serial.println("Write failed!");
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
