#include <SparkFunDMX.h>

#include <RadioHeadRadio.hpp>
#include <NetworkManager.hpp>

const uint8_t DMX_CHANNELS = 192;

const uint8_t RESERVATION_SECONDS = 5;
const uint8_t UPDATE_TIMEOUT_SECONDS = 1;

RadioHeadRadio* radio;

SparkFunDMX dmx;

void setup() {
  Serial.begin(115200);

  Serial.println("Initializing...");

  Serial.print("Number of channels in universe: ");
  Serial.println(DMX_CHANNELS);
  dmx.initRead(DMX_CHANNELS);

  pinMode(LED_BUILTIN, OUTPUT);

  radio = new RadioHeadRadio();
  radio->sleep();

  Serial.println("Starting...");
}

unsigned long printAliveAt = 0;

RadioPacket packet;
uint16_t packetId;

uint32_t last_sent = 0;
uint8_t last_r;
uint8_t last_g;
uint8_t last_b;

void loop() {
  dmx.update();
  // TODO (will): Figure out why the channels don't start at 0
  uint8_t r = dmx.read(3);
  uint8_t g = dmx.read(4);
  uint8_t b = dmx.read(5);

  uint32_t time = millis();

  if (r != last_r || g != last_g || b != last_b ||
      time > last_sent + UPDATE_TIMEOUT_SECONDS * 1000) {
    digitalWrite(LED_BUILTIN, HIGH);

    last_r = r;
    last_g = g;
    last_b = b;
    last_sent = time;

    packet.writeControl(RESERVATION_SECONDS, CRGB(r, g, b));
    packet.packetId = packetId++;
    radio->sendPacket(packet);

    Serial.printf("Sent r: %u\tg: %u\tb: %u\n", r, g, b);

    digitalWrite(LED_BUILTIN, LOW);
  }

  delay(50);
}