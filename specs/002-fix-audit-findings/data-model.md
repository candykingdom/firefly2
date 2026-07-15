# Data Model: Fix Confirmed Audit Findings

**Date**: 2026-07-10 | **Plan**: [plan.md](plan.md)

This feature changes no persistent data and no wire format. The entities below are the in-memory/on-air structures the fixes touch, with the invariants the fixes must preserve or restore.

## RadioPacket (lib/radio/Radio.hpp) — touched by D1, D2

The single protocol message. In-memory fields:

| Field | Type | Meaning | Rules |
|-------|------|---------|-------|
| `packet_id` | `uint16_t` | Mesh dedup / master-election id | 0 = sentinel, 1 = reserved for tests ("always wins election"); live sends draw from [2, 0xFFFF) |
| `type` | `PacketType` (byte on wire) | HEARTBEAT / CLAIM_MASTER / SET_EFFECT / SET_CONTROL | Unknown values must be tolerated, never crash |
| `dataLength` | `uint8_t` | Valid bytes in `data` | **Must be ≤ 58 (`PACKET_DATA_LENGTH`).** D1's defect: never populated on hardware receive (stayed 0) |
| `data` | `uint8_t[58]` | Type-specific payload | Layouts below |

Payload layouts (unchanged; defined by existing write/read helpers):

- **HEARTBEAT** (`dataLength = 4`): network time, `uint32_t`, **big-endian** (`data[0]` = bits 31–24). D2 fix: decoding must be UB-free for all 2^32 values.
- **CLAIM_MASTER** (`dataLength = 0`): header-only.
- **SET_EFFECT** (`dataLength = 3`): `data[0]` effect index, `data[1]` delay seconds, `data[2]` palette index.
- **SET_CONTROL** (`dataLength = 4`): `data[0]` delay seconds, `data[1..3]` R, G, B.

**New invariant (D1)**: for any packet accepted from the radio, `dataLength` equals the wire length minus 3, so `Serialize(Deserialize(bytes)) == bytes`. See [contracts/wire-format.md](contracts/wire-format.md).

**State transitions**: none (value object). Lifecycle: composed → serialized → transmitted → deserialized → possibly re-serialized (rebroadcast). D1 restores losslessness of the deserialize step.

## ColorPalette (lib/color/) — touched by D4 (binding only)

Owns `std::vector<CHSV> colors`. The 20 registry palettes live in a function-local static `const std::vector<ColorPalette>&` (`Effect::palettes()`), valid for program lifetime.

**Rule established by D4**: per-LED rendering code binds palettes by `const ColorPalette&`; it never copies one. (Effects with their own member palette — Fire, Pride — are unaffected.)

## Frame render inputs (lib/led_manager/) — touched by D5

Conceptual tuple resolved by `RunEffect`: (current effect, set-effect packet, network timestamp).

**Rule established by D5**: resolved **once per frame**, immutable for the frame; every LED in the frame is computed from the same tuple. Strip descriptors are iterated by reference. Per-strip flag semantics (Reversed remaps the index; Dim divides by 8; Off forces black; all handled centrally) are unchanged.

## FastLedManager (src/arduino/) — touched by D6

Gains `const uint16_t led_count_`, fixed at construction (device strips are const). Semantic: total LEDs across strips; the `leds` buffer is `led_count_ + 1` (index 0 is the on-board pass-through LED; strip LED *i* lives at `leds[i+1]`; the single-LED special case mirrors index 0 to the on-board LED). No behavior change — only the recomputation is removed.

## Controller button/LED state (src/devices/controller/) — touched by D7

Three button banks (left/right/bottom × 3, debounced). UI rule the fix restores: **a button's pressed-state LED reflects that button's own bank** — right-button chains test `right_buttons[i]`, mirroring the (already correct) left-button chains.
