#include "gtest/gtest.h"

#define DEBUG 1
#include "../Radio.hpp"

TEST(RadioPacket, serializesHeartbeat0) {
  RadioPacket packet;

  packet.writeHeartbeat(0);
  EXPECT_EQ(0, packet.readTimeFromHeartbeat());
}

TEST(RadioPacket, serializesHeartbeat1) {
  RadioPacket packet;

  packet.writeHeartbeat(1);
  EXPECT_EQ(1, packet.readTimeFromHeartbeat());
}

TEST(RadioPacket, serializesHeartbeatBig) {
  RadioPacket packet;

  const uint32_t test = 0x81234567;
  packet.writeHeartbeat(test);
  EXPECT_EQ(test, packet.readTimeFromHeartbeat());
}
