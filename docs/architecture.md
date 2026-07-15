# Architecture Overview

Firefly2 is a wireless LED-effect network for Burning Man art installations, bikes, and wearables. Many battery-powered devices each drive WS2812 strips; they self-organize over 915 MHz RFM69 radios into a mesh with exactly one master, share a network clock, and render the same effect in sync.

## The one-page mental model

```
        915 MHz RFM69 (flood mesh, 61-byte frames)
  ┌────────┐   ┌────────┐   ┌────────┐   ┌────────────┐
  │  node  │   │ fancy- │   │  ...   │   │ controller │  ← human picks
  │ (SAMD) │   │  node  │   │        │   │ (buttons)  │    effect/color
  └────────┘   └────────┘   └────────┘   └────────────┘
      one device is Master: heartbeats (network time),
      random effect change every 60 s, rebroadcast every 2 s
```

Per device, the layering is:

```
device main()  (src/devices/<target>/)
  ├── RadioStateMachine (src/generic/)   master/slave, time sync, effect scheduling
  │     └── NetworkManager (src/generic/) flood rebroadcast + 5-deep dedup cache
  │           └── Radio → RadioHeadRadio (src/arduino/)  RFM69 @ 915 MHz, 13 dBm
  └── LedManager → FastLedManager (src/arduino/)  effects, power cap, WS2812 out
        ├── Effect subclasses (lib/effect/)  GetRGB(index, network_time, strip, packet)
        ├── ColorPalette (lib/color/)        22 palettes, shortest-arc hue blending
        └── DeviceDescription (lib/device/)  strips, flags, milliamp budget
```

The main loop on every device is just `state_machine.Tick(); led_manager->RunEffect();` plus device-specific concerns (watchdog, battery, buttons).

## Why devices stay in sync

1. The master broadcasts a HEARTBEAT every second containing **network time**; slaves compute a local `millis_offset_` from it.
2. Effects are pure functions of `(led_index, network_time, strip, packet)` — same time + same SET_EFFECT packet ⇒ same colors everywhere, with no frame streaming.
3. The master rebroadcasts the current effect every 2 s so joiners and lossy links converge.

## Key design decisions

- **Master election is emergent**: everyone boots as slave; a jittered 5–7 s silence timeout promotes someone; dueling masters resolve by random-id comparison, loser demotes. No configuration, no fixed roles.
- **Flood mesh, not routing**: every node rebroadcasts every unseen packet once (dedup by a 5-entry recent-id cache). Simple and robust at the cost of airtime.
- **Weighted randomness by duplication**: an effect's probability is the number of copies of its pointer in the registry vector — the wire index space *is* the weight table.
- **Host-testable core**: everything except `src/arduino/` and `src/devices/` compiles on the host against `FakeRadio`/`FakeLedManager`/`FakeFastLED`; `FakeNetwork` simulates 5 nodes with packet loss, so mesh behavior is unit-tested deterministically with sanitizers on.
- **Compile-time device selection**: `Devices::current` in `lib/device/Devices.hpp` picks the strip layout; optionally burned to flash (`DeviceMode`) so one binary can serve differently-shaped hardware.

## Directory map

| Path | Contents |
|---|---|
| `lib/` | Platform-independent core: effects, palettes, LED manager, device descriptions, math, radio interface, fakes |
| `src/generic/` | NetworkManager + RadioStateMachine (also platform-independent; historical split from `lib/`) |
| `src/arduino/` | Hardware backends: `FastLedManager`, `RadioHeadRadio` |
| `src/devices/<target>/` | One folder per firmware target; selected by PlatformIO `build_src_filter` |
| `test/` | GoogleTest suites + `FakeNetwork` harness |
| `hardware/` | KiCad PCB projects |
| `boards/`, `bootloader/`, `tools/` | PlatformIO board defs, SAMD bootloader binaries + install tooling, UF2 generation |
| `lib/simulator/` | SDL desktop visualizer of the real effect code |
