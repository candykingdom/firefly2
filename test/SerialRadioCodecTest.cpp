#include <Radio.hpp>
#include <SerialCodec.hpp>
#include <vector>

#include "gtest/gtest.h"

// Tests for the serial bridge's framing primitives (lib/serial-codec). These
// are the byte-level guarantees SerialRadio depends on: a defect here corrupts
// mesh traffic silently instead of failing loudly, and it can't be caught on
// the host anywhere else.

using serial_codec::CobsDecode;
using serial_codec::CobsEncode;
using serial_codec::Crc16;
using serial_codec::MaxEncodedLength;

namespace {

// The largest body SerialRadio ever frames: a full wire packet plus its CRC.
constexpr size_t kMaxWireLength = PACKET_HEADER_LENGTH + PACKET_DATA_LENGTH;
constexpr size_t kCrcLength = 2;
constexpr size_t kMaxBodyLength = kMaxWireLength + kCrcLength;

// Deterministic pseudo-random generator, so a failure is always reproducible.
class Rng {
 public:
  explicit Rng(uint32_t seed) : state_(seed) {}
  uint32_t Next() {
    state_ = state_ * 1664525u + 1013904223u;
    return state_ >> 16;
  }
  uint8_t NextByte() { return static_cast<uint8_t>(Next() & 0xFF); }

 private:
  uint32_t state_;
};

// Round-trips `in` through the encoder and decoder, asserting the encoding is
// delimiter-safe and the decode reproduces the input exactly.
void ExpectRoundTrip(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> encoded(MaxEncodedLength(in.size()));
  const size_t encoded_len = CobsEncode(in.data(), in.size(), encoded.data());
  ASSERT_LE(encoded_len, encoded.size());

  // The whole point of COBS: no 0x00 in the body, so 0x00 delimits frames.
  for (size_t i = 0; i < encoded_len; i++) {
    ASSERT_NE(encoded[i], 0) << "zero byte at " << i << " of encoding";
  }

  std::vector<uint8_t> decoded(encoded_len + 1);
  const size_t decoded_len =
      CobsDecode(encoded.data(), encoded_len, decoded.data());
  ASSERT_EQ(decoded_len, in.size());
  EXPECT_EQ(
      std::vector<uint8_t>(decoded.begin(), decoded.begin() + decoded_len), in);
}

}  // namespace

TEST(SerialRadioCodec, cobsRoundTripsEveryLengthUpToAFullFrame) {
  Rng rng(1);
  for (size_t len = 1; len <= kMaxBodyLength; len++) {
    std::vector<uint8_t> in(len);
    for (size_t i = 0; i < len; i++) {
      in[i] = rng.NextByte();
    }
    ASSERT_NO_FATAL_FAILURE(ExpectRoundTrip(in)) << "length " << len;
  }
}

// Zeroes are what COBS actually encodes around, so bodies dense in 0x00 (a
// short packet whose payload is zero-filled, for instance) exercise the
// code-block logic hardest.
TEST(SerialRadioCodec, cobsRoundTripsZeroHeavyBodies) {
  for (size_t len = 1; len <= kMaxBodyLength; len++) {
    ASSERT_NO_FATAL_FAILURE(ExpectRoundTrip(std::vector<uint8_t>(len, 0x00)))
        << "all-zero length " << len;
  }

  Rng rng(2);
  for (int trial = 0; trial < 2000; trial++) {
    const size_t len = 1 + rng.Next() % kMaxBodyLength;
    std::vector<uint8_t> in(len);
    for (size_t i = 0; i < len; i++) {
      // Mostly zeroes, with occasional real data.
      in[i] = (rng.Next() % 4) ? 0x00 : rng.NextByte();
    }
    ASSERT_NO_FATAL_FAILURE(ExpectRoundTrip(in)) << "trial " << trial;
  }
}

TEST(SerialRadioCodec, cobsRejectsFramesWithAnEmbeddedZero) {
  // A 0x00 inside a frame can't come from the encoder, so it means the framing
  // has gone wrong; the decoder must not accept it as data.
  const uint8_t frame[] = {0x03, 0x11, 0x00, 0x22};
  uint8_t out[sizeof(frame)];
  EXPECT_EQ(CobsDecode(frame, sizeof(frame), out), 0u);
}

TEST(SerialRadioCodec, cobsRejectsFramesWhoseCodeOverrunsTheFrame) {
  // A code byte pointing past the end of the frame indicates a truncated or
  // corrupt frame.
  const uint8_t frame[] = {0x7F, 0x11, 0x22};
  uint8_t out[sizeof(frame)];
  EXPECT_EQ(CobsDecode(frame, sizeof(frame), out), 0u);
}

// The decoder runs on whatever a floating RX pin produces. It must never
// report more bytes than the caller's buffer can hold, whatever the input.
TEST(SerialRadioCodec, cobsDecodeNeverExpandsBeyondItsInput) {
  Rng rng(3);
  for (int trial = 0; trial < 20000; trial++) {
    const size_t len = 1 + rng.Next() % MaxEncodedLength(kMaxBodyLength);
    std::vector<uint8_t> junk(len);
    for (size_t i = 0; i < len; i++) {
      junk[i] = rng.NextByte();
    }
    // Deliberately sized to the documented bound so ASan catches an overrun.
    std::vector<uint8_t> out(len);
    const size_t decoded_len = CobsDecode(junk.data(), len, out.data());
    ASSERT_LE(decoded_len, len) << "trial " << trial << " input length " << len;
  }
}

// SerialRadio checks `Crc16(body, body_len) == 0` over the body *including*
// its trailer, which only works because CCITT-FALSE has a residue of zero.
TEST(SerialRadioCodec, crc16ResidueOverBodyPlusTrailerIsZero) {
  Rng rng(4);
  for (size_t len = 1; len <= kMaxWireLength; len++) {
    std::vector<uint8_t> body(len + kCrcLength);
    for (size_t i = 0; i < len; i++) {
      body[i] = rng.NextByte();
    }
    const uint16_t crc = Crc16(body.data(), len);
    body[len] = crc >> 8;
    body[len + 1] = crc & 0xFF;
    EXPECT_EQ(Crc16(body.data(), body.size()), 0) << "length " << len;
  }
}

// Pins the algorithm itself, so a well-intentioned "optimization" that changes
// the polynomial or init value can't silently break compatibility with an
// already-flashed peer on the other end of the UART.
TEST(SerialRadioCodec, crc16MatchesKnownCcittFalseVector) {
  const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  EXPECT_EQ(Crc16(check, sizeof(check)), 0x29B1);
}

TEST(SerialRadioCodec, crc16DetectsSingleBitFlips) {
  Rng rng(5);
  std::vector<uint8_t> body(kMaxWireLength);
  for (size_t i = 0; i < body.size(); i++) {
    body[i] = rng.NextByte();
  }
  const uint16_t expected = Crc16(body.data(), body.size());

  for (size_t byte = 0; byte < body.size(); byte++) {
    for (uint8_t bit = 0; bit < 8; bit++) {
      body[byte] ^= (1 << bit);
      EXPECT_NE(Crc16(body.data(), body.size()), expected)
          << "flip of byte " << byte << " bit " << (int)bit
          << " went unnoticed";
      body[byte] ^= (1 << bit);
    }
  }
}

// End-to-end proof of the property SerialRadio actually relies on: a real
// packet survives the full send path (Serialize -> CRC -> COBS) and comes back
// out of the receive path (COBS -> CRC check -> Deserialize) unchanged.
TEST(SerialRadioCodec, framedPacketSurvivesTheFullRoundTrip) {
  RadioPacket original;
  original.packet_id = 0xBEEF;
  original.writeSetEffect(7, 42, 3);

  uint8_t body[kMaxBodyLength];
  const uint8_t wire_len = original.Serialize(body);
  const uint16_t crc = Crc16(body, wire_len);
  body[wire_len] = crc >> 8;
  body[wire_len + 1] = crc & 0xFF;

  uint8_t frame[MaxEncodedLength(kMaxBodyLength)];
  const size_t frame_len = CobsEncode(body, wire_len + kCrcLength, frame);

  // Receive side.
  uint8_t decoded[MaxEncodedLength(kMaxBodyLength)];
  const size_t decoded_len = CobsDecode(frame, frame_len, decoded);
  ASSERT_EQ(decoded_len, wire_len + kCrcLength);
  ASSERT_EQ(Crc16(decoded, decoded_len), 0);

  RadioPacket received;
  ASSERT_TRUE(received.Deserialize(decoded, decoded_len - kCrcLength));
  EXPECT_EQ(received, original);
}

// A corrupted frame must be rejected by the CRC rather than surfacing as a
// plausible packet -- this is what keeps line noise out of the mesh.
TEST(SerialRadioCodec, corruptedFrameFailsTheCrcCheck) {
  RadioPacket original;
  original.packet_id = 0x1234;
  original.writeHeartbeat(0xDEADBEEF);

  uint8_t body[kMaxBodyLength];
  const uint8_t wire_len = original.Serialize(body);
  const uint16_t crc = Crc16(body, wire_len);
  body[wire_len] = crc >> 8;
  body[wire_len + 1] = crc & 0xFF;
  const size_t body_len = wire_len + kCrcLength;

  // Flip each bit of the body in turn; every corruption must be caught.
  for (size_t byte = 0; byte < body_len; byte++) {
    for (uint8_t bit = 0; bit < 8; bit++) {
      body[byte] ^= (1 << bit);

      uint8_t frame[MaxEncodedLength(kMaxBodyLength)];
      const size_t frame_len = CobsEncode(body, body_len, frame);
      uint8_t decoded[MaxEncodedLength(kMaxBodyLength)];
      const size_t decoded_len = CobsDecode(frame, frame_len, decoded);

      // The corruption is caught either by COBS framing or by the CRC; it must
      // never decode cleanly into a packet.
      EXPECT_TRUE(decoded_len == 0 || Crc16(decoded, decoded_len) != 0)
          << "corruption of byte " << byte << " bit " << (int)bit
          << " passed both checks";

      body[byte] ^= (1 << bit);
    }
  }
}
