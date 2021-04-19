#undef max
#undef min
#include "../../arduino/FastLedManager.hpp"
#include <Radio.hpp>
#include <NetworkManager.hpp>
#include <RadioStateMachine.hpp>

const int kNumLeds = 60;

RadioHeadRadio* radio;
FastLedManager* ledManager;

RadioPacket packetToSend;
void setup() {
  Serial.begin(115200);
  // Delay makes it easier to reset the board in case of failure
  delay(1000);

  ledManager = new FastLedManager(kNumLeds);
  // Yellow LED on boot indicates a problem initializing the radio
  ledManager->SetGlobalColor(CHSV(HUE_YELLOW, 255, 128));
  radio = new RadioHeadRadio();
  ledManager->SetGlobalColor(CRGB(CRGB::Black));

  packetToSend.writeSetEffect(0, 10, 4);
}

RadioPacket packet;
uint32_t receivedPacketAt = 0;
void loop() {
  bool receivedPacket = false;
  uint32_t origMillis = millis();
  while ((millis() - origMillis) < 5) {
    if (radio->readPacket(packet)) {
      receivedPacket = true;
      receivedPacketAt = millis();
      break;
    }
  }

  if (receivedPacket) {
    // RSSI range is -120 to 0 (0 is best), so this maps from aqua (high
    // strength) to red (low strength)
    uint8_t hue = 120 + (radio->LastRssi());
    ledManager->SetGlobalColor(CHSV(hue, 255, 255));

    // Wait for the other radio to transition to receiving
    delay(1);
    radio->packetToSend(packetToSend);
  } else {
    radio->packetToSend(packetToSend);
    if (millis() - receivedPacketAt > 500) {
      ledManager->SetGlobalColor(CRGB(CRGB::Black));
    }
  }
}
