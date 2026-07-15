# Contract: Radio Wire Format & Packet Codec

**Date**: 2026-07-10 | **Plan**: [../plan.md](../plan.md) | **Consumers**: `RadioHeadRadio` (hardware), host tests

## On-air byte layout (FROZEN — pre-existing, must not change)

RH_RF69 payload bytes, after the RadioHead-level framing:

```
offset  size  field
0       2     packet_id, big-endian (buf[0] = high byte)
1̶       —     (part of packet_id)
2       1     type (PacketType as raw byte)
3       0–58  payload (dataLength bytes)
```

Total wire length = `3 + dataLength`, range [3, 61]. The RFM69 FIFO bounds received frames well below the 64-byte read buffer, so `dataLength` ≤ 58 always fits; the codec still enforces it defensively.

## Codec API (NEW in D1 — `RadioPacket` members, `lib/radio/`)

```cpp
// Writes this packet to buf (which must hold at least 3 + dataLength bytes,
// i.e. 61 to be safe for any valid packet). Returns bytes written (3 + dataLength).
// Precondition: dataLength <= PACKET_DATA_LENGTH (58). Implementations clamp
// to 58 rather than read past the payload array if handed a corrupt packet.
uint8_t Serialize(uint8_t* buf) const;

// Parses len wire bytes into this packet. Sets packet_id, type,
// dataLength = len - 3, and copies the payload.
// Returns false (leaving the packet unspecified) if len < 3 or
// len - 3 > PACKET_DATA_LENGTH. Never reads past buf[len-1], never
// writes past data[57], never crashes on any byte content.
bool Deserialize(const uint8_t* buf, uint8_t len);
```

### Contract guarantees

1. **Round-trip (decode∘encode)**: for any wire frame `w` with `3 ≤ len(w) ≤ 61`, `Deserialize(w)` succeeds and `Serialize` then reproduces `w` byte-for-byte. *(This is the D1 fix: rebroadcast = re-serialize must be lossless.)*
2. **Round-trip (encode∘decode)**: for any valid in-memory packet `p` (`dataLength ≤ 58`), `Deserialize(Serialize(p))` yields a packet equal to `p` (`operator==`: id, type, dataLength, first dataLength payload bytes).
3. **Totality**: `Deserialize` is safe on arbitrary byte content (unknown `type` values are stored as-is, per the tolerance invariant) and on any `len` 0–255 (returns false outside [3, 61]).
4. **No format authority elsewhere**: `RadioHeadRadio::sendPacket`/`readPacket` MUST delegate framing to this codec (they keep radio-mode management and the pre-existing `received_length > 3` drop check — CLAIM_MASTER's header-only frames remain dropped on hardware receive, an out-of-scope pre-existing behavior).

### Field-level payload contracts (pre-existing, unchanged)

`writeHeartbeat`/`readTimeFromHeartbeat`: `uint32_t` network time, big-endian in `data[0..3]`; **must round-trip exactly for all 2^32 values with no UB** (D2 adds the all-values guarantee — previously UB for times ≥ 0x80000000).

`writeSetEffect`/`read*FromSetEffect`, `writeControl`/`read*FromSetControl`: unchanged.

## Conformance tests (test/RadioPacketTest.cpp)

| Case | Covers |
|------|--------|
| Encode→decode→compare for payload lengths 0, 1, 4, 58 across all four types | Guarantee 2, FR-002 |
| Decode a hand-built wire frame → re-encode → byte-compare | Guarantee 1 (rebroadcast losslessness), FR-001 |
| Decode with len 0, 1, 2 → false; len 62+ (oversized payload) → false; garbage type byte → true, tolerated | Guarantee 3, invariants |
| Heartbeat time round-trip at 0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF (UBSan-clean) | D2, FR-003 |
| Relay scenario: decode(master frame) → re-encode → decode as third node → same network time | Spec US1 scenario 3 |
