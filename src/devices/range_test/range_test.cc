#include <Arduino.h>


#undef max
#undef min
#include <DeviceDescription.hpp>
#include <StripDescription.hpp>
#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

constexpr int kLedPin = 0;
static const DeviceDescription device{200, {new StripDescription(1, {})}};

RadioHeadRadio* radio;
NetworkManager *nm;
RadioStateMachine *state_machine;
FastLedManager* led_manager;

RadioPacket packetToSend;
void setup() {
  pinMode(kLedPin, OUTPUT);
  Serial.begin(115200);
  // Delay makes it easier to reset the board in case of failure
  delay(1000);

  radio = new RadioHeadRadio();
  nm = new NetworkManager(radio);
  state_machine = new RadioStateMachine(nm);
  led_manager = new FastLedManager(&device, state_machine);
  // Yellow LED on boot indicates a problem initializing the radio
  led_manager->SetGlobalColor(CHSV(HUE_YELLOW, 255, 128));
  led_manager->SetGlobalColor(CRGB(CRGB::Black));

  packetToSend.writeSetEffect(0, 10, 4);
}

RadioPacket packet;
uint32_t receivedPacketAt = 0;
void loop() {
  digitalWrite(kLedPin, (millis() / 200) % 2);
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
    led_manager->SetGlobalColor(CHSV(hue, 255, 255));

    // Wait for the other radio to transition to receiving
    delay(1);
    radio->sendPacket(packetToSend);
  } else {
    radio->sendPacket(packetToSend);
    if (millis() - receivedPacketAt > 500) {
      led_manager->SetGlobalColor(CRGB(CRGB::Black));
    }
  }
}
