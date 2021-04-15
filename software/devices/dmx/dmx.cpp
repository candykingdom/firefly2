#include "../../arduino/RadioHeadRadio.hpp"

const int kLedPin = 13;
const int kButton0 = 0;

RadioHeadRadio* radio;
RadioPacket packet;

void setup() {
  Serial.begin(115200);

  pinMode(kLedPin, OUTPUT);
  pinMode(kButton0, INPUT_PULLUP);

  radio = new RadioHeadRadio();
  radio->sleep();
}

const int kBufLen = 10;
char buf[kBufLen];

unsigned long printAliveAt = 0;

bool debounce = false;

void loop() {
  if (!digitalRead(kButton0)) {
    digitalWrite(kLedPin, HIGH);
  } else {
    digitalWrite(kLedPin, LOW);
  }

  if (!digitalRead(kButton0) && !debounce) {
    debounce = true;
    Serial.println("Sending...");
    packet.writeSetEffect(2, 5, (millis() >> 3) % 256);
    radio->sendPacket(packet);

    // Note: need to sleep just a little after sending a packet before sleeping
    delay(5);
    radio->sleep();
    delay(100);
  } else if (digitalRead(kButton0)) {
    debounce = false;
  }

  if (millis() > printAliveAt) {
    Serial.println(millis());
    printAliveAt = millis() + 1000;
  }
}
