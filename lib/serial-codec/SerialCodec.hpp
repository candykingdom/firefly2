#ifndef __SERIAL_CODEC_HPP__
#define __SERIAL_CODEC_HPP__

#include <stddef.h>
#include <stdint.h>

/**
 * Framing primitives for the serial bridge (see src/arduino/SerialRadio.hpp).
 *
 * These are pure byte math with no Arduino dependency, so they live here and
 * are covered by SerialRadioCodecTest on the host. A defect in any of them
 * corrupts mesh traffic silently rather than failing loudly, which is exactly
 * why they are tested away from the hardware.
 */
namespace serial_codec {

// COBS adds one overhead byte per 254 bytes of input, plus one leading code
// byte. Frames are delimited by a separate 0x00, which is not counted here.
constexpr size_t MaxEncodedLength(size_t decoded_length) {
  return decoded_length + (decoded_length / 254) + 1;
}

/**
 * Standard COBS (Consistent Overhead Byte Stuffing) encoder. Encodes `len`
 * bytes from `in` into `out`, which must hold at least
 * MaxEncodedLength(len) bytes. Returns the encoded length.
 *
 * The output is guaranteed to contain no 0x00 bytes, so a 0x00 can be appended
 * as an unambiguous frame delimiter. That makes the stream self-synchronizing:
 * a receiver that starts mid-frame, or drops bytes, recovers at the next
 * delimiter.
 */
size_t CobsEncode(const uint8_t *in, size_t len, uint8_t *out);

/**
 * Standard COBS decoder. Decodes `len` encoded bytes from `in` into `out`,
 * which must hold at least `len` bytes (decoding never expands: the output is
 * at most len - 1 bytes). Returns the decoded length, or 0 if the frame is
 * malformed.
 */
size_t CobsDecode(const uint8_t *in, size_t len, uint8_t *out);

/**
 * CRC-16/CCITT-FALSE. Its residue is 0, so running it over a decoded body
 * that includes its own big-endian trailer yields 0 exactly when the frame is
 * intact. The serial bridge relies on this to reject line noise that COBS
 * would otherwise decode into a plausible-looking packet.
 */
uint16_t Crc16(const uint8_t *in, size_t len);

}  // namespace serial_codec

#endif  // __SERIAL_CODEC_HPP__
