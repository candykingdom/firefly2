# Data Model: Web Simulator

**Feature**: 001-web-simulator | **Date**: 2026-07-10

All numeric color/index/time types mirror firmware widths: channels and indices are uint8, positions `fract16` are uint16, network time is uint32 (wraps).

## CHSV / CRGB

| Field | Type | Notes |
|-------|------|-------|
| h, s, v / r, g, b | uint8 | CHSV→CRGB only via ported `hsv2rgb_rainbow` (matches FakeFastLED) |

Represented as `{h,s,v}` / `{r,g,b}` plain objects (or `[r,g,b]` triples in snapshots). No alpha, no floats in engine math.

## Palette

| Field | Type | Notes |
|-------|------|-------|
| name | string | display only, not on the wire |
| colors | CHSV[] (1–6) | order matters |

Methods (ported from `lib/color/ColorPalette.cpp`): `getColor(index)` → `colors[index % length]`; `getGradient(fract16 pos, wrap=true)` → HSV lerp via `lerp16by16` with shortest-hue wrapping. Registry: array of 22, **index = wire byte** (`Effect.cpp:19-65` order).

## Effect

| Field | Type | Notes |
|-------|------|-------|
| name | string | unique, human-readable (e.g. "Rainbow", "Simple Blink 30ms") |
| getRGB(ledIndex, timeMs, strip, show) | fn → CRGB | pure; `show` carries paletteIndex + control RGB (packet equivalent) |
| seedOffset | uint16? | only Fire/Firefly/Rorschach; defaults to host-build-matching value (research R3) |

## EffectRegistry

| Field | Type | Notes |
|-------|------|-------|
| entries | {effect, weight}[] | declaration order = `LedManager.cpp:12-35` |
| wireTable | Effect[35] | weighted duplicates expanded; indices 0–26 pool, 27–34 non-random |
| randomPoolSize | 27 | master picks `random(0, 27)` |

Invariants (asserted in tests): `wireTable[33].name === "Display Color Palette"`, `wireTable[34].name === "Dark"`, `wireTable.length < 256`, lookup uses `index % wireTable.length` (Arduino tolerance path).

## StripDescription

| Field | Type | Notes |
|-------|------|-------|
| ledCount | uint8 | 0 allowed (renders nothing) |
| flags | Set<Flag> | Flag ∈ {Tiny, Bright, Circular, Mirrored, Reversed, Controller, Dim, Off} |

Central semantics (engine, from `LedManager.cpp:76-107`): Reversed→index inversion; Off→black without calling effect; Dim→per-channel integer `/8` after effect. Others pass through to `getRGB`.

## DeviceDescription

| Field | Type | Notes |
|-------|------|-------|
| name | string | catalog key (scarf, puck, lantern, bike, ufo, rainbow_cloak, …) |
| strips | StripDescription[] | order matters (global LED index = concatenation) |
| milliampsSupported | uint32 | carried for display only; power limiting not simulated |

Catalog transcribed from `lib/device/Devices.hpp`.

## ShowState (SET_EFFECT / SET_CONTROL equivalent)

| Field | Type | Notes |
|-------|------|-------|
| effectIndex | uint8 | any byte tolerated (mod lookup) |
| paletteIndex | uint8 | any byte tolerated (`% 22` inside palette access) |
| delaySeconds | uint8 | holds effect against master change (master mode) |
| control | {rgb: CRGB, delaySeconds: uint8} \| null | active SET_CONTROL override; while set, ControlEffect renders `rgb` on all devices; expires after delay |

Transitions: `setEffect` replaces effect/palette/delay **and clears any control override** (a SET_EFFECT packet replaces the control packet, as in `RadioStateMachine::SetEffect`) — likewise `setPalette`/`setDelay`/master-mode changes; `setControl` sets override (takes precedence, mirroring `GetCurrentEffect()`'s SET_CONTROL branch) and re-arms the master change timer with its delay; override expiry (forward elapsed time only — backward scrubs never expire it) or `clearControl` reverts to current effect; in master mode, effect-change timer (60 000 ms, reset by manual set and extended by delay) draws from weighted pool via seeded RNG.

## SimClock

| Field | Type | Notes |
|-------|------|-------|
| networkMillis | uint32 | wraps at 2^32 |
| mode | running \| paused | pinning = pause + setTime |
| speed | float > 0 | multiplier on wall-clock delta (running mode only) |

Wall time only ever enters as *deltas*; `getRGB` sees only `networkMillis`.

## Snapshot (readback surface, FR-012)

```json
{
  "time": 123456,
  "effectIndex": 7, "effectName": "Rainbow",
  "paletteIndex": 8, "paletteName": "Rainbow",
  "control": null,
  "masterMode": false,
  "devices": [
    { "name": "scarf",
      "strips": [ { "flags": [], "leds": [[255,0,0], [254,2,0]] } ] }
  ]
}
```

Deterministic: equal (devices, effectIndex, paletteIndex, seeds, control, time) ⇒ deep-equal snapshot.
