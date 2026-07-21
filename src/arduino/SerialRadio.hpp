#ifndef __SERIAL_RADIO_HPP__
#define __SERIAL_RADIO_HPP__

#include <Arduino.h>

#include <Radio.hpp>
#include <SerialCodec.hpp>

/**
 * A generic, platform-independent Radio that bridges packets over an Arduino
 * serial link (any object implementing the `Stream` interface). This lets a
 * node board (SAMD, 915 MHz RFM69) and an ESP32-C3 (2.4 GHz ESP-NOW) exchange
 * packets over a physical UART, so each acts as a gateway for the other.
 *
 * The radio is transport-only: the owning device is responsible for calling
 * `.begin()` on the underlying serial port with the correct baud/pins before
 * `Begin()` is called. This keeps the class portable across platforms whose
 * pin muxing differs.
 *
 * Wire framing is COBS (Consistent Overhead Byte Stuffing) with a 0x00 frame
 * delimiter, wrapping the standard `RadioPacket` wire encoding plus a CRC-16
 * trailer, which rejects noise that COBS would otherwise decode. COBS makes the
 * stream self-synchronizing: a receiver that starts mid-frame, or drops bytes,
 * recovers at the next delimiter without any startup delay.
 */
class SerialRadio : public Radio {
 public:
  explicit SerialRadio(Stream& serial);

  bool Begin() override;

  bool readPacket(RadioPacket& packet) override;
  void sendPacket(RadioPacket& packet) override;
  void sleep() override;

  // The serial link is a point-to-point bridge: the single peer already has
  // anything that arrived on it. See Radio::RebroadcastsToSource.
  bool RebroadcastsToSource() const override { return false; }

  int16_t LastRssi();

 private:
  // Max wire encoding of a RadioPacket: 3-byte header + 58-byte payload.
  static constexpr size_t kMaxWireLength =
      PACKET_HEADER_LENGTH + PACKET_DATA_LENGTH;
  // CRC-16 trailer, covering the wire encoding.
  static constexpr size_t kCrcLength = 2;
  static constexpr size_t kMaxBodyLength = kMaxWireLength + kCrcLength;
  // The COBS encoding of a full body, excluding the 0x00 delimiter.
  static constexpr size_t kMaxFrameLength =
      serial_codec::MaxEncodedLength(kMaxBodyLength);

  Stream& serial_;

  // Accumulates a single inbound COBS frame (without the delimiter) across
  // readPacket calls, so reads never block waiting for a full frame.
  uint8_t rx_buffer_[kMaxFrameLength];
  size_t rx_len_ = 0;
};

#endif  // __SERIAL_RADIO_HPP__
