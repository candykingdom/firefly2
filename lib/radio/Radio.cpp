#include "Radio.hpp"

#include <Debug.hpp>
#include <cassert>
#include <cstdio>

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
  uint32_t time = this->data[3];
  time |= this->data[2] << 8;
  time |= this->data[1] << 16;
  time |= this->data[0] << 24;
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
