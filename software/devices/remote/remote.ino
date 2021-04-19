#include <Radio.hpp>

const int kLedPin = 0;
const int kButton0 = 8;

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

void loop() {
  // Note: buttons are inverted
  if (!digitalRead(kButton0)) {
    Serial.println("Sending...");
    packet.writeSetEffect(2, 5);
    radio->sendPacket(packet);

    // Note: need to sleep just a little after sending a packet before sleeping
    delay(5);
    radio->sleep();
    delay(100);
  }

  if (millis() > printAliveAt) {
    Serial.println(millis());
    printAliveAt = millis() + 1000;
  }
}
