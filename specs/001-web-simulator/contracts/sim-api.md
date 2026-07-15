# Contract: Simulator Programmatic API (`window.sim`)

**Feature**: 001-web-simulator

The page exposes exactly one global, `window.sim`, which is also the engine object Node tests construct directly (`new SimEngine(...)` from `sim/js/engine.js`). Everything the UI can do goes through this surface; nothing UI-only mutates engine state. All setters return the engine (chainable) and take effect on the next rendered frame (or immediately for `getSnapshot()` after `setTime`).

## State

| Method | Signature | Semantics |
|--------|-----------|-----------|
| `setDevices` | `(names: string[]) => sim` | Replace displayed devices; unknown name → throws with the valid list |
| `setEffect` | `(indexOrName: number \| string) => sim` | Number = wire byte 0–255 (tolerated via `% 35`); string = unique effect name; resets master change timer; ends any control override (SET_EFFECT replaces the control packet) |
| `setPalette` | `(indexOrName: number \| string) => sim` | Wire byte tolerated; string = palette name |
| `setDelay` | `(seconds: uint8) => sim` | SET_EFFECT delay field; holds against master changes |
| `setControl` | `(rgb: [r,g,b], delaySeconds: uint8) => sim` | SET_CONTROL: all devices render solid rgb until the delay expires, `clearControl` is called, or a new SET_EFFECT arrives (`setEffect`/`setPalette`/`setDelay`/a master-mode change) — matching the firmware, where any SET_EFFECT packet replaces the control packet. Delay 0 = no timed expiry. With master mode on, the change timer is re-armed to the control delay |
| `clearControl` | `() => sim` | Cancel override |
| `setMasterMode` | `(on: boolean, seed?: number) => sim` | Autoplay with firmware cadence (60 s change, weighted pool 0–26, random palette 0–21); seed makes the sequence reproducible |
| `setEffectSeed` | `(effectName: string, offset: number) => sim` | Override the constructor seed offset of Fire/Firefly/Rorschach; default matches the reference-vector host build |

## Clock

| Method | Signature | Semantics |
|--------|-----------|-----------|
| `pause` / `play` | `() => sim` | Freeze / resume network clock |
| `setTime` | `(ms: uint32) => sim` | Pin exact network time (implies nothing about mode; pairs with `pause` for pinned rendering) |
| `step` | `(ms: number) => sim` | Advance exactly `ms` while paused, render one frame |
| `setSpeed` | `(mult: number) => sim` | Playback speed multiplier (> 0) |

## Readback

| Method | Signature | Semantics |
|--------|-----------|-----------|
| `getState` | `() => {time, effectIndex, effectName, paletteIndex, paletteName, delaySeconds, control, masterMode, speed, paused, devices: string[]}` | Control-plane state |
| `getSnapshot` | `() => Snapshot` | Full per-LED colors (see data-model.md Snapshot) computed **synchronously** for the current state/time — this is the verification surface |
| `listEffects` | `() => {index, name, weight}[]` | The 35-entry wire table |
| `listPalettes` | `() => {index, name, colors}[]` | The 22 palettes |
| `listDevices` | `() => {name, strips}[]` | Catalog |

## Guarantees

1. **Determinism**: identical (devices, effectIndex, paletteIndex, seeds, control, time) ⇒ `getSnapshot()` deep-equals across calls, frames, and page loads (FR-013).
2. **Tolerance**: any uint8 for effect/palette indices, any device set incl. 0-LED strips, never throws from `getSnapshot`/render (FR-014). Only programmer errors (unknown name string, non-numeric time) throw, synchronously, with descriptive messages.
3. **Purity**: `getSnapshot()` has no side effects and does not advance the clock.
4. **UI parity**: every UI control maps 1:1 onto these methods; URL query params (`?device=…&effect=…&palette=…&t=…&paused=1`) initialize the same state on load.
5. **Headless parity**: `SimEngine` in Node produces byte-identical snapshots to `window.sim` in a browser for the same inputs (asserted by running the same case modules in both).
