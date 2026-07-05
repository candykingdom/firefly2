#include <Arduino.h>

#include "Devices.hpp"

#include "../../arduino/FastLedManager.hpp"
#include "../../lib/fake-radio/FakeRadio.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

#define FASTLED_ESP32_I2S_NUM_DMA_BUFFERS 4

const int onBoardLed = 8;
long time_unit_ms = 100;
// morse code "hehe haha"
long delays[] = {1,1,1,1,-2,1,-2,1,1,1,1,-2,1,-6,1,1,1,1,-2,1,3,-2,1,1,1,1,-2,1,3,-6};
unsigned long delay_index = 0;

unsigned long last_effect = 0;

FakeRadio *radio = new FakeRadio();
NetworkManager nm(radio);
RadioStateMachine state_machine(&nm);
FastLedManager *led_manager;

void setup() {
    Serial.begin(115200);
    led_manager = new FastLedManager(Devices::current, &state_machine);
    led_manager->PlayStartupAnimation();
}

void loop() {
    if (millis() - last_effect > 10000) {
        RadioPacket packet = RadioPacket();
        packet.writeSetEffect(random(led_manager->GetNumEffects()), 60, random(Effect::palettes().size()));
        state_machine.SetEffect(&packet);
        last_effect = millis();
    }
    led_manager->RunEffect();
}