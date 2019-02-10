#include <RadioHeadRadio.hpp>

const int kLedPin = 0;
const int kButton0 = 8;

RadioHeadRadio* radio;

void setup() {
  Serial.begin(115200);

  pinMode(kLedPin, OUTPUT);
  pinMode(kButton0, INPUT_PULLUP);

  radio = new RadioHeadRadio();
}

const int kBufLen = 10;
char buf[kBufLen];

unsigned long printAliveAt = 0;

RadioPacket packet;

void loop() {
  // Note: buttons are inverted
  if (!digitalRead(kButton0)) {
    Serial.println("Sending...");
    packet.writeSetEffect(2, 5);
    radio->sendPacket(packet);
    delay(100);
  }

  if (millis() > printAliveAt) {
    Serial.println(millis());
    printAliveAt = millis() + 1000;
  }
}
