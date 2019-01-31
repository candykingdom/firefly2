#include "Radio.hpp"
#include <cstdio>
#include "Debug.hpp"

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
