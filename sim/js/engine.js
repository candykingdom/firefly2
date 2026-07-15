// SimEngine: the DOM-free simulator core. Ports LedManager::RunEffect's
// central strip handling (lib/led_manager/LedManager.cpp:76-107) and
// LedManager::GetCurrentEffect's SET_CONTROL precedence, on top of a
// pausable/scrubbable network clock. This object IS the programmatic API
// (contracts/sim-api.md); ui.js and the tests are both clients of it.

import { rgbDiv } from './fastled.js';
import { getPalette, PALETTES } from './palette.js';
import { DEVICES, getDevice } from './devices.js';
import { createRegistry } from './effects/registry.js';

const UINT32 = 0x100000000;

export class SimEngine {
  constructor({ devices = ['scarf'], effect = 'Rainbow', palette = 'Rainbow',
    time = 0, paused = false, seeds = {} } = {}) {
    this.registry = createRegistry(seeds);
    this._devices = [];
    this._effectIndex = 0;
    this._paletteIndex = 0;
    this._delaySeconds = 0;
    this._delaySetAt = 0;
    this._control = null;  // {rgb, delaySeconds, setAtMs}
    this._time = 0;        // uint32 network millis
    this._timeFraction = 0;
    this._paused = paused;
    this._speed = 1;
    this._master = null;   // attached by master.js
    this.setDevices(devices).setEffect(effect).setPalette(palette)
      .setTime(time);
  }

  // --- state setters ---

  setDevices(names) {
    this._devices = names.map(getDevice);
    return this;
  }

  setEffect(indexOrName) {
    this._effectIndex = this._resolveIndex(
      indexOrName, (name) => this.registry.getIndexByName(name));
    this._delaySeconds = 0;
    this._delaySetAt = this._time;
    // A SET_EFFECT packet replaces the state machine's current packet
    // wholesale (RadioStateMachine::SetEffect), ending any SET_CONTROL
    // override — mirrored here and in setPalette/setDelay, which also ride
    // SET_EFFECT packets on the wire.
    this._control = null;
    if (this._master) this._master.onManualSet(this._time);
    return this;
  }

  setPalette(indexOrName) {
    this._paletteIndex = this._resolveIndex(indexOrName, (name) => {
      const index = PALETTES.findIndex((p) => p.name === name);
      if (index < 0) {
        throw new Error(`unknown palette "${name}"; valid: ${
          PALETTES.map((p) => p.name).join(', ')}`);
      }
      return index;
    });
    this._control = null;
    return this;
  }

  setDelay(seconds) {
    this._delaySeconds = seconds & 0xff;
    this._delaySetAt = this._time;
    this._control = null;
    if (this._master) this._master.onManualSet(this._time);
    return this;
  }

  setControl(rgb, delaySeconds = 0) {
    const [r, g, b] = rgb;
    this._control = {
      rgb: { r: r & 0xff, g: g & 0xff, b: b & 0xff },
      delaySeconds: delaySeconds & 0xff,
      setAtMs: this._time,
    };
    // A received SET_CONTROL also re-arms the master's change timer with the
    // control delay (RadioStateMachine's SET_CONTROL timer handling).
    if (this._master) {
      this._master.onControlSet(this._time, this._control.delaySeconds);
    }
    return this;
  }

  clearControl() {
    this._control = null;
    return this;
  }

  setEffectSeed(effectName, offset) {
    const seeds = { ...this.registry.seeds };
    if (!(effectName in seeds)) {
      throw new Error(`no seed for "${effectName}"; seeded effects: ${
        Object.keys(seeds).join(', ')}`);
    }
    seeds[effectName] = offset;
    this.registry = createRegistry({
      fireOffset: seeds.Fire,
      rorschachOffset: seeds.Rorschach,
      fireflyOffset: seeds.Firefly,
    });
    return this;
  }

  setMasterMode(on, seed = 1) {
    if (this._master) this._master.setEnabled(on, seed, this._time);
    else if (on) throw new Error('no master attached; use attachMaster()');
    return this;
  }

  // --- clock ---

  pause() {
    this._paused = true;
    return this;
  }

  play() {
    this._paused = false;
    return this;
  }

  setTime(ms) {
    if (typeof ms !== 'number' || !Number.isFinite(ms)) {
      throw new Error(`setTime needs a finite number, got ${ms}`);
    }
    this._time = ms >>> 0;
    this._timeFraction = 0;
    // Pinning the clock is a teleport, not elapsed time: re-arm the master's
    // change timer from the new time so arbitrary jumps (including > 2^31 ms)
    // behave predictably instead of hitting modular-window ambiguity.
    if (this._master) this._master.onTimeJump(this._time);
    this._tickTimers();
    return this;
  }

  step(ms) {
    this._advanceBy(ms);
    return this;
  }

  setSpeed(mult) {
    if (!(mult > 0)) throw new Error(`speed must be > 0, got ${mult}`);
    this._speed = mult;
    return this;
  }

  // Called by the UI's animation loop with the wall-clock delta; honors
  // pause and speed. Tests use setTime/step instead.
  advanceWall(wallDeltaMs) {
    if (this._paused) return this;
    this._advanceBy(wallDeltaMs * this._speed);
    return this;
  }

  _advanceBy(deltaMs) {
    const total = this._timeFraction + deltaMs;
    const whole = Math.floor(total);
    this._timeFraction = total - whole;
    this._time = (this._time + whole) % UINT32;
    this._tickTimers();
  }

  _tickTimers() {
    if (this._control && this._control.delaySeconds > 0) {
      const elapsed =
          (this._time - this._control.setAtMs + UINT32) % UINT32;
      // Expire only on genuine forward elapsed time. A backward scrub makes
      // the modular distance read as nearly UINT32 (the top half of the
      // range) — that is "before the control was set", not "expired".
      if (elapsed >= this._control.delaySeconds * 1000 &&
          elapsed < UINT32 / 2) {
        this._control = null;
      }
    }
    if (this._master) this._master.tick(this._time);
  }

  // --- readback ---

  getState() {
    const effective = this.registry.getByIndex(this._effectIndex);
    return {
      time: this._time,
      effectIndex: this._effectIndex,
      effectName: effective.name,
      paletteIndex: this._paletteIndex,
      paletteName: getPalette(this._paletteIndex).name,
      delaySeconds: this._delaySeconds,
      control: this._control ? {
        rgb: [this._control.rgb.r, this._control.rgb.g, this._control.rgb.b],
        delaySeconds: this._control.delaySeconds,
      } : null,
      masterMode: this._master ? this._master.enabled : false,
      speed: this._speed,
      paused: this._paused,
      devices: this._devices.map((d) => d.name),
    };
  }

  getSnapshot() {
    const state = this.getState();
    return {
      time: state.time,
      effectIndex: state.effectIndex,
      effectName: state.effectName,
      paletteIndex: state.paletteIndex,
      paletteName: state.paletteName,
      control: state.control,
      masterMode: state.masterMode,
      devices: this._devices.map((device) => this._renderDevice(device)),
    };
  }

  listEffects() {
    return this.registry.wireTable.map(({ index, name, weight }) =>
      ({ index, name, weight }));
  }

  listPalettes() {
    return PALETTES.map((p, index) =>
      ({ index, name: p.name, colors: p.colors.map((c) => ({ ...c })) }));
  }

  listDevices() {
    return Object.values(DEVICES).map((d) => ({
      name: d.name,
      milliamps: d.milliamps,
      strips: d.strips.map((s) => ({
        ledCount: s.ledCount,
        flags: [...s.flags],
      })),
    }));
  }

  // --- rendering (LedManager::RunEffect port) ---

  _currentEffect() {
    if (this._control) return this.registry.controlEffect;
    return this.registry.getByIndex(this._effectIndex).effect;
  }

  _renderDevice(device) {
    const effect = this._currentEffect();
    const show = {
      paletteIndex: this._paletteIndex,
      controlRgb: this._control ? { ...this._control.rgb } : {
        r: 0,
        g: 0,
        b: 0,
      },
    };
    const strips = device.strips.map((strip) => {
      const leds = [];
      for (let i = 0; i < strip.ledCount; i++) {
        const virtualIndex =
            strip.hasFlag('Reversed') ? strip.ledCount - i - 1 : i;
        let rgb;
        if (strip.hasFlag('Off')) {
          rgb = { r: 0, g: 0, b: 0 };
        } else {
          rgb = effect.getRGB(virtualIndex, this._time, strip, show);
          if (strip.hasFlag('Dim')) rgb = rgbDiv(rgb, 8);
        }
        leds.push([rgb.r, rgb.g, rgb.b]);
      }
      return { flags: [...strip.flags], leds };
    });
    return { name: device.name, strips };
  }

  _resolveIndex(indexOrName, byName) {
    if (typeof indexOrName === 'number') {
      if (!Number.isInteger(indexOrName) || indexOrName < 0 ||
          indexOrName > 255) {
        throw new Error(`index must be a byte 0-255, got ${indexOrName}`);
      }
      return indexOrName;
    }
    return byName(indexOrName);
  }
}
