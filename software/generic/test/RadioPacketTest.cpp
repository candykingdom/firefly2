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

TEST(RadioPacket, serializesSetEffect0) {
  RadioPacket packet;

  packet.writeSetEffect(0, 1);
  EXPECT_EQ(0, packet.readEffectIndexFromSetEffect());
  EXPECT_EQ(1, packet.readDelayFromSetEffect());

  packet.writeSetEffect(1, 0);
  EXPECT_EQ(1, packet.readEffectIndexFromSetEffect());
  EXPECT_EQ(0, packet.readDelayFromSetEffect());

  packet.writeSetEffect(250, 199);
  EXPECT_EQ(250, packet.readEffectIndexFromSetEffect());
  EXPECT_EQ(199, packet.readDelayFromSetEffect());
}
