#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "../src/devices/controller/config-storage.h"
#include "../src/devices/controller/config-utils.h"
#include "gtest/gtest.h"

namespace {

struct IoCall {
  uint8_t page;
  uint8_t word;
  std::vector<uint8_t> data;
};

std::array<uint8_t, fram::kCapacity> storage;
std::vector<IoCall> reads;
std::vector<IoCall> writes;
int fail_read = -1;
int fail_write = -1;

void ResetStorage() {
  storage.fill(0xFF);
  reads.clear();
  writes.clear();
  fail_read = -1;
  fail_write = -1;
}

}  // namespace

namespace fram {

bool Write(uint8_t page, uint8_t word, const uint8_t *data, uint16_t size) {
  writes.push_back({page, word, std::vector<uint8_t>(data, data + size)});
  if (static_cast<int>(writes.size()) - 1 == fail_write ||
      !RangeIsValid(page, word, size)) {
    return false;
  }
  std::copy(data, data + size, storage.begin() + LinearAddress(page, word));
  return true;
}

bool Read(uint8_t page, uint8_t word, uint8_t *data, uint16_t size) {
  reads.push_back({page, word, {}});
  if (static_cast<int>(reads.size()) - 1 == fail_read ||
      !RangeIsValid(page, word, size)) {
    return false;
  }
  const auto begin = storage.begin() + LinearAddress(page, word);
  std::copy(begin, begin + size, data);
  return true;
}

}  // namespace fram

class ControllerStorageTest : public testing::Test {
 protected:
  void SetUp() override { ResetStorage(); }
};

TEST_F(ControllerStorageTest, StoreInvalidatesThenWritesDataThenPublishes) {
  constexpr std::array<uint8_t, 4> marker = {0xDE, 0xAD, 0xBE, 0xEF};
  constexpr std::array<uint8_t, 3> data = {1, 2, 3};

  ASSERT_TRUE(
      controller_config::StoreRecord<data.size()>(4, 0, marker, data.data()));
  ASSERT_EQ(writes.size(), 3);
  EXPECT_EQ(writes[0].page, 4);
  EXPECT_EQ(writes[0].word, 0);
  EXPECT_EQ(writes[0].data, (std::vector<uint8_t>{0, 0, 0, 0}));
  EXPECT_EQ(writes[1].word, marker.size());
  EXPECT_EQ(writes[1].data, (std::vector<uint8_t>{1, 2, 3}));
  EXPECT_EQ(writes[2].data, (std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF}));
}

TEST_F(ControllerStorageTest, FailedPayloadWriteLeavesRecordInvalid) {
  constexpr std::array<uint8_t, 4> marker = {0xDE, 0xAD, 0xBE, 0xEF};
  constexpr std::array<uint8_t, 3> data = {1, 2, 3};
  std::copy(marker.begin(), marker.end(), storage.begin());
  fail_write = 1;

  EXPECT_FALSE(
      controller_config::StoreRecord<data.size()>(0, 0, marker, data.data()));
  EXPECT_EQ(writes.size(), 2);
  EXPECT_EQ(std::vector<uint8_t>(storage.begin(), storage.begin() + 4),
            (std::vector<uint8_t>{0, 0, 0, 0}));
}

TEST_F(ControllerStorageTest, LoadChecksMarkerAndReadCompletionBeforeMutation) {
  constexpr std::array<uint8_t, 4> marker = {0xBA, 0xAD, 0xF0, 0x0D};
  constexpr std::array<uint8_t, 3> saved = {4, 5, 6};
  ASSERT_TRUE(
      controller_config::StoreRecord<saved.size()>(4, 0, marker, saved.data()));

  std::array<uint8_t, 3> loaded = {9, 9, 9};
  ASSERT_TRUE(controller_config::LoadRecord<loaded.size()>(4, 0, marker,
                                                           loaded.data()));
  EXPECT_EQ(loaded, saved);
  ASSERT_EQ(reads.size(), 2);
  EXPECT_EQ(reads[0].page, 4);
  EXPECT_EQ(reads[1].page, 4);

  loaded = {9, 9, 9};
  reads.clear();
  fail_read = 1;
  EXPECT_FALSE(controller_config::LoadRecord<loaded.size()>(4, 0, marker,
                                                            loaded.data()));
  EXPECT_EQ(loaded, (std::array<uint8_t, 3>{9, 9, 9}));
}

TEST_F(ControllerStorageTest, BadMarkerDoesNotReadOrMutatePayload) {
  constexpr std::array<uint8_t, 4> marker = {0xBA, 0xAD, 0xF0, 0x0D};
  std::array<uint8_t, 3> loaded = {9, 9, 9};

  EXPECT_FALSE(controller_config::LoadRecord<loaded.size()>(4, 0, marker,
                                                            loaded.data()));
  EXPECT_EQ(reads.size(), 1);
  EXPECT_EQ(loaded, (std::array<uint8_t, 3>{9, 9, 9}));
}

TEST(FramAddressTest, UsesPageBitsAndNeverChunksAcrossPageBoundary) {
  EXPECT_EQ(fram::DeviceAddress(fram::LinearAddress(0, 0)), 0x50);
  EXPECT_EQ(fram::DeviceAddress(fram::LinearAddress(4, 0)), 0x54);
  EXPECT_EQ(fram::DeviceAddress(fram::LinearAddress(7, 255)), 0x57);
  EXPECT_EQ(fram::ChunkSize(fram::LinearAddress(0, 0), 54, 31), 31);
  EXPECT_EQ(fram::ChunkSize(fram::LinearAddress(0, 250), 20, 31), 6);
  const uint16_t next_page = fram::LinearAddress(0, 250) + 6;
  EXPECT_EQ(fram::DeviceAddress(next_page), 0x51);
  EXPECT_EQ(fram::WordAddress(next_page), 0);
  EXPECT_FALSE(fram::RangeIsValid(7, 250, 7));
  EXPECT_FALSE(fram::RangeIsValid(8, 0, 0));
}

TEST(ControllerStorageValidationTest, RejectsOutOfRangePaletteIndices) {
  std::array<std::array<uint8_t, 2>, 2> palettes = {{{0, 1}, {2, 1}}};
  EXPECT_TRUE(PaletteIndicesAreValid(palettes, 3));
  palettes[1][1] = 3;
  EXPECT_FALSE(PaletteIndicesAreValid(palettes, 3));
  EXPECT_FALSE(PaletteIndicesAreValid(palettes, 0));
}
