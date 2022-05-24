#include <tuple>

#include "../NetworkManager.hpp"
#include "../Radio.hpp"
#include "../RadioStateMachine.hpp"
#include "FakeNetwork.hpp"
#include "FakeRadio.hpp"
#include "RadioIntegrationTest.hpp"
#include "gtest/gtest.h"

class InvalidPacketTest
    : public RadioIntegrationTest,
      public ::testing::WithParamInterface<
          std::tuple<uint16_t, uint8_t, uint8_t, std::vector<uint8_t>>> {
 protected:
  FakeNetwork network;
};

const std::vector<uint16_t> kPacketIds = {0,     1,     2,      0xFF,   0x100,
                                          0x101, 0x1FF, 0xFF00, 0xFF01, 0xFFFF};
const std::vector<uint8_t> kPacketTypes = {0, 1, 2, 3, 4, 5, 0xFE, 0xFF};
const std::vector<uint8_t> kPacketLengths = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 56, 57, 58, 59, 60, 61, 62, 63, 64};
const std::vector<std::vector<uint8_t>> kData = {{0},
                                                 {1},
                                                 {2},
                                                 {255},
                                                 {0, 1},
                                                 {1, 1},
                                                 {255, 255},
                                                 {1, 1, 1, 1, 1},
                                                 {255, 255, 255, 255, 255}};

TEST_P(InvalidPacketTest, Runs) {
  runTicks(100);
  RadioPacket packet = {
    packetId : std::get<0>(GetParam()),
    type : (PacketType)std::get<1>(GetParam()),
    dataLength : std::get<2>(GetParam()),
    data : {5, 6}
  };
  network.TransmitPacket(packet);
  runTicks(100);
}

INSTANTIATE_TEST_CASE_P(InvalidPacketTests, InvalidPacketTest,
                        ::testing::Combine(::testing::ValuesIn(kPacketIds),
                                           ::testing::ValuesIn(kPacketTypes),
                                           ::testing::ValuesIn(kPacketLengths),
                                           ::testing::ValuesIn(kData)));