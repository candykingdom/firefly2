#include "Radio.hpp"

#include <Debug.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>

uint8_t RadioPacket::Serialize(uint8_t* buf) const {
  const uint8_t length = this->dataLength <= PACKET_DATA_LENGTH
                             ? this->dataLength
                             : PACKET_DATA_LENGTH;
  buf[0] = this->packet_id >> 8;
  buf[1] = this->packet_id & 0xFF;
  buf[2] = this->type;
  memcpy(buf + PACKET_HEADER_LENGTH, this->data.data(), length);
  return PACKET_HEADER_LENGTH + length;
}

bool RadioPacket::Deserialize(const uint8_t* buf, uint8_t len) {
  if (len < PACKET_HEADER_LENGTH ||
      len - PACKET_HEADER_LENGTH > PACKET_DATA_LENGTH) {
    return false;
  }
  this->packet_id = (buf[0] << 8) | buf[1];
  this->type = (PacketType)buf[2];
  this->dataLength = len - PACKET_HEADER_LENGTH;
  memcpy(this->data.data(), buf + PACKET_HEADER_LENGTH, this->dataLength);
  return true;
}

void RadioPacket::writeHeartbeat(uint32_t time) {
  this->type = HEARTBEAT;
  this->dataLength = 4;
  this->data[0] = time >> 24;
  this->data[1] = time >> 16;
  this->data[2] = time >> 8;
  this->data[3] = time;
}

uint32_t RadioPacket::readTimeFromHeartbeat() const {
#ifndef ARDUINO
  assert(this->type == HEARTBEAT);
  assert(this->dataLength == 4);
#endif
  // Cast before shifting: uint8_t promotes to (signed) int, and shifting a
  // top-bit-set byte into the sign bit is undefined behavior in pre-C++14
  // language modes (e.g. the SAMD node target's gnu++11); C++14 defines it.
  uint32_t time = (uint32_t)this->data[3];
  time |= (uint32_t)this->data[2] << 8;
  time |= (uint32_t)this->data[1] << 16;
  time |= (uint32_t)this->data[0] << 24;
  return time;
}

void RadioPacket::writeSetEffect(uint8_t effect_index, uint8_t delay,
                                 uint8_t hue) {
  this->type = SET_EFFECT;
  this->dataLength = 3;
  this->data[0] = effect_index;
  this->data[1] = delay;
  this->data[2] = hue;
}

uint8_t RadioPacket::readEffectIndexFromSetEffect() const {
#ifndef ARDUINO
  assert(this->type == SET_EFFECT);
  assert(this->dataLength == 3);
#endif
  return this->data[0];
}

uint8_t RadioPacket::readDelayFromSetEffect() const {
#ifndef ARDUINO
  assert(this->type == SET_EFFECT);
  assert(this->dataLength == 3);
#endif
  return this->data[1];
}

uint8_t RadioPacket::readPaletteIndexFromSetEffect() const {
#ifndef ARDUINO
  assert(this->type == SET_EFFECT);
  assert(this->dataLength == 3);
#endif
  return this->data[2];
}

void RadioPacket::writeControl(uint8_t delay, CRGB rgb) {
  this->type = SET_CONTROL;
  this->dataLength = 4;
  this->data[0] = delay;
  this->data[1] = rgb.r;
  this->data[2] = rgb.g;
  this->data[3] = rgb.b;
}

uint8_t RadioPacket::readDelayFromSetControl() const {
#ifndef ARDUINO
  assert(this->type == SET_CONTROL);
  assert(this->dataLength == 4);
#endif
  return this->data[0];
}

CRGB RadioPacket::readRgbFromSetControl() const {
#ifndef ARDUINO
  assert(this->type == SET_CONTROL);
  assert(this->dataLength == 4);
#endif
  return CRGB(this->data[1], this->data[2], this->data[3]);
}
