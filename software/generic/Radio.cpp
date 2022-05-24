#include "Radio.hpp"

#include <Debug.hpp>
#include <cstdio>

void RadioPacket::writeHeartbeat(uint32_t time) {
  this->type = HEARTBEAT;
  this->dataLength = 4;
  this->data[0] = time >> 24;
  this->data[1] = time >> 16;
  this->data[2] = time >> 8;
  this->data[3] = time;
}

uint32_t RadioPacket::readTimeFromHeartbeat() {
  // TODO: add a check for the type and length if debug is enabled?
  uint32_t time = this->data[3];
  time |= this->data[2] << 8;
  time |= this->data[1] << 16;
  time |= this->data[0] << 24;
  return time;
}

void RadioPacket::writeSetEffect(uint8_t effectIndex, uint8_t delay,
                                 uint8_t hue) {
  this->type = SET_EFFECT;
  this->dataLength = 3;
  this->data[0] = effectIndex;
  this->data[1] = delay;
  this->data[2] = hue;
}

uint8_t RadioPacket::readEffectIndexFromSetEffect() { return this->data[0]; }

uint8_t RadioPacket::readDelayFromSetEffect() { return this->data[1]; }

uint8_t RadioPacket::readPaletteIndexFromSetEffect() { return this->data[2]; }

void RadioPacket::writeControl(uint8_t delay, CRGB rgb) {
  this->type = SET_CONTROL;
  this->dataLength = 4;
  this->data[0] = delay;
  this->data[1] = rgb.r;
  this->data[2] = rgb.g;
  this->data[3] = rgb.b;
}

uint8_t RadioPacket::readDelayFromSetControl() { return this->data[0]; }

CRGB RadioPacket::readRgbFromSetControl() {
  return CRGB(this->data[1], this->data[2], this->data[3]);
}
