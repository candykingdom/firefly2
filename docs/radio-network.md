# Radio & Networking

How Firefly2 devices talk to each other. The stack, bottom to top:

| Layer | Class | Location | Role |
|---|---|---|---|
| Hardware | `RadioHeadRadio` | `src/arduino/RadioHeadRadio.{hpp,cpp}` | RFM69 driver via RadioHead |
| Abstraction | `Radio` | `lib/radio/Radio.hpp` | `readPacket` / `sendPacket` / `sleep` interface |
| Mesh | `NetworkManager` | `src/generic/NetworkManager.{hpp,cpp}` | Rebroadcast + duplicate suppression |
| Protocol | `RadioStateMachine` | `src/generic/RadioStateMachine.{hpp,cpp}` | Master/slave election, time sync, effect scheduling |
| Wire format | `RadioPacket` | `lib/radio/Radio.hpp:27` | Packet struct + wire codec (`Serialize`/`Deserialize`) + payload helpers |

Test doubles: `FakeRadio` (`lib/fake-radio/`) and `FakeNetwork` (`test/FakeNetwork.{hpp,cpp}`) simulate a multi-node network, including configurable packet loss.

## RadioPacket wire format

Struct (`lib/radio/Radio.hpp:27-71`):

- `uint16_t packet_id`
- `PacketType type` — sent as 1 byte
- `uint8_t dataLength`
- `std::array<uint8_t, 58> data` — `PACKET_DATA_LENGTH = 58` (`Radio.hpp:25`)

On-air framing (`RadioPacket::Serialize`/`Deserialize`, `lib/radio/Radio.cpp`; `RadioHeadRadio` delegates to them): `[packet_id high][packet_id low][type][data...]` — a 3-byte header (`PACKET_HEADER_LENGTH = 3`; `RadioHeadRadio.hpp` keeps the equivalent `kFrontPacketPadding`). The RFM69 FIFO is 64 bytes; RadioHead uses 3, leaving 61 (`kMaxPacketSize`), minus the 3-byte header = 58 bytes of payload. That is where `PACKET_DATA_LENGTH` comes from. The codec is host-tested for byte-exact round-trips (`test/RadioPacketTest.cpp`); `Deserialize` sets `dataLength` from the wire length — a receive-side bug where it was never set used to truncate every mesh rebroadcast to header-only, breaking multi-hop sync (fixed in specs/002-fix-audit-findings, D1). Header-only frames (3 bytes, e.g. CLAIM_MASTER) are still dropped on receive by `RadioHeadRadio` — a pre-existing quirk, deliberately unchanged.

### Packet types (`Radio.hpp:7-23`)

| Type | Value | dataLength | Payload layout |
|---|---|---|---|
| `HEARTBEAT` | 0 | 4 | big-endian `uint32` network time (`writeHeartbeat` / `readTimeFromHeartbeat`, `Radio.cpp:7-26`) |
| `CLAIM_MASTER` | 1 | 0 | none — type alone is the message |
| `SET_EFFECT` | 2 | 3 | `data[0]`=effect index, `data[1]`=delay (seconds; lock time), `data[2]`=palette index (`Radio.cpp:28-59`) |
| `SET_CONTROL` | 3 | 4 | `data[0]`=delay, `data[1..3]`=R,G,B (`Radio.cpp:61-85`) |

`operator==` (`Radio.hpp:73-83`) compares id, type, length, and `dataLength` bytes of data — and deliberately returns `false` for any packet with `dataLength > 58`, so an oversized/invalid packet is never equal to anything, including itself.

## NetworkManager — mesh rebroadcast

Flood-style mesh with best-effort loop suppression:

- **Recent-ID cache**: circular buffer of the last **5** packet ids (`kRecentIdsCacheSize = 5`, `NetworkManager.hpp:25`).
- **`receive()`** (`NetworkManager.cpp:10-26`): if the id is in the cache, drop it (no rebroadcast, not surfaced to caller). Otherwise immediately rebroadcast the packet **with the same id**, cache the id, and surface it. Preserving the id is what lets the mesh converge and stop flooding.
- **`send()`** (`NetworkManager.cpp:28-34`): assigns a fresh random id in `[2, 0xFFFF)` and caches it so the node ignores its own echo.

Reserved ids: **0** is the empty-cache sentinel; **1** is never generated so tests can inject it as an "always wins election" id.

Gotcha: the cache is only 5 deep — with enough interleaved traffic an old id can fall out and be rebroadcast again. Dedup is best-effort, not exact.

## RadioStateMachine

Two states: `Slave` (initial) and `Master` (`RadioStateMachine.hpp:34-37`).

### Timing constants (`RadioStateMachine.hpp:75-91`)

| Constant | Value | Meaning |
|---|---|---|
| `kSlaveNoPacketTimeout` | 5000 ms | base slave→master promotion timeout |
| `kSlaveNoPacketRandom` | 2000 ms | jitter added to the above → effective [5000, 6999] ms |
| `kMasterHeartbeatInterval` | 1000 ms | heartbeat cadence |
| `kChangeEffectInterval` | 60000 ms | auto effect-change cadence |
| `kBroadcastEffectInterval` | 2000 ms | current-effect rebroadcast cadence |

The jitter de-synchronizes slaves so they don't all promote at once when a master disappears. A timer value of 0 means disabled; `TimerExpired` returns the first expired timer in enum order, so Heartbeat > ChangeEffect > BroadcastEffect priority.

### Master election

- A slave that hears no heartbeat/claim within its jittered timeout promotes itself to master.
- When a **master receives a HEARTBEAT** (two masters coexist), it runs `PerformMasterElection` (`RadioStateMachine.cpp:154-165`): generate `our_id = random(1, 0xFFFF)`; if greater than the received `packet_id`, keep mastership and send `CLAIM_MASTER`, else step down to Slave.
- A master hearing `CLAIM_MASTER` immediately becomes Slave; a slave hearing it just resets its timeout.

### Network time sync

`millis_offset_` (`int32_t`) is defined by `local millis() + offset = network time`. A slave receiving a heartbeat sets `offset = heartbeat_time - (int32_t)millis()` (`RadioStateMachine.cpp:120-122`); `GetNetworkMillis()` adds it back. The master broadcasts its own `GetNetworkMillis()`, so its clock propagates. Effects animate off network time, which is what keeps devices visually synchronized. Note: the signed/unsigned mix is flagged `DANGER` in the source and is fragile near uint32 wraparound.

### Effect scheduling

- While master, `TimerChangeEffect` fires every 60 s: pick a random effect and palette, broadcast `SET_EFFECT` (delay 0), re-arm. `TimerBroadcastEffect` rebroadcasts the current effect every 2 s so late joiners converge.
- `SetEffect()` (public, used by the controller) sends the packet and holds it for `delay` seconds if nonzero, else the normal 60 s. A master *receiving* SET_EFFECT/SET_CONTROL likewise respects the embedded delay as a lock.
- `effect_change_seen_at_` preserves the change cadence across master handover: a newly elected master schedules its first change for the *remaining* time in the 60 s window rather than restarting it (`beginMaster`, `RadioStateMachine.cpp:228-240`).
- Default effect at boot: index 1, palette 0, no delay (`RadioStateMachine.cpp:16`).
- `num_effects_`/`num_palettes_` default to 1 and are pushed in by the LedManager via `SetNumEffects`/`SetNumPalettes`.

### Tick mechanics

`RadioTick()` processes **one event per call** — a received packet takes priority over an expired timer (`RadioStateMachine.cpp:76-81`). `Tick()` calls `RadioTick` once on the first call and twice thereafter (`RadioStateMachine.cpp:23-36`); the double call compensates for LED writes taking several ms, and the first-call-once behavior works around a suspected compiler/hardware hang on the Trellis (see comments there before touching this).

## RadioHeadRadio — RFM69 config

`Begin()` (`RadioHeadRadio.cpp:7-16`):

- TX power **13 dBm**, `setTxPower(13, false)` — configured as a non-high-power module.
- Frequency **915.0 MHz** (US ISM band).
- No sync words, encryption key, or custom modem config anywhere — RadioHead `RH_RF69` defaults apply.
- SS/DIO pins come from `RADIO_SS`/`RADIO_DIO` build flags in `platformio.ini` (per-board).

`readPacket` drops frames ≤ 3 bytes and copies the payload byte-by-byte because the RadioHead buffer is volatile (comment at `RadioHeadRadio.cpp:31-34` — don't "optimize" it to memcpy).

## Invariants verified by tests

- **Exactly one master converges** under: clean startup, master disappearance, node dropout, and 1% packet loss (`test/RadioStateIntegrationTest.cpp`, using `FakeNetwork::setPacketLoss`).
- **Invalid packets never crash**: `test/InvalidPacketTest.cpp` fuzzes ids, unknown types, and `dataLength` up to 64 (larger than the 58-byte array). Unknown types fall through `switch` statements with no `default` and are silently ignored. Any new packet handling must preserve this.
- **Effect index converges network-wide** within 3 change intervals (`RadioStateIntegrationTest.cpp:180-198`).
- `FakeRadio` zero-fills `data` beyond `dataLength` to catch code reading past the declared length.
