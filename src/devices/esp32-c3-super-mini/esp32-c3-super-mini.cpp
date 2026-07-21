#include <Arduino.h>

#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/SerialRadio.hpp"
#include "../../generic/FireflyNetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"
#include "../../lib/fake-radio/FakeRadio.hpp"
#include "Devices.hpp"
#include "EspNowRadio.hpp"

#define FASTLED_ESP32_I2S_NUM_DMA_BUFFERS 4

// Serial bridge to a node board over Serial1. Opt-in via -DSERIAL_BRIDGE=1;
// only enable it when a node board is wired to the UART pins (see the hazard
// note in SerialRadio.cpp).
#ifndef SERIAL_BRIDGE
#define SERIAL_BRIDGE 0
#endif

EspNowRadio* esp_radio = new EspNowRadio();

#if SERIAL_BRIDGE
// GPIO0 drives the LEDs, so use GPIO1 (RX) / GPIO2 (TX) for the UART.
static constexpr int kSerialRxPin = 1;
static constexpr int kSerialTxPin = 2;
static constexpr uint32_t kSerialBaud = 115200;

SerialRadio* serial_radio = new SerialRadio(Serial1);
#endif

FireflyNetworkManager nm(esp_radio);
RadioStateMachine state_machine(&nm);
FastLedManager* led_manager;

void setup() {
  Serial.begin(115200);
  led_manager = new FastLedManager(Devices::current, &state_machine);
  if (!esp_radio->Begin()) {
    led_manager->FatalErrorAnimation();
  }

#if SERIAL_BRIDGE
  Serial1.begin(kSerialBaud, SERIAL_8N1, kSerialRxPin, kSerialTxPin);
  if (!serial_radio->Begin()) {
    led_manager->FatalErrorAnimation();
  }
  nm.addRadio(serial_radio);
#endif

  led_manager->PlayStartupAnimation();
}

void loop() {
  state_machine.Tick();
  led_manager->RunEffect();
}
