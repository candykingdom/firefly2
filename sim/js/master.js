// Master-mode autoplay: reproduces RadioStateMachine's master cadence
// (src/generic/RadioStateMachine.cpp:201-208): every kChangeEffectInterval
// (60 s) pick random(0, num_effects) from the weighted pool and
// random(0, num_palettes), delay 0. A manual SET_EFFECT with delay > 0 holds
// the effect for that many seconds (slave logic, RadioStateMachine.cpp:51-53).
// Uses its own seeded RNG (mulberry32) so autoplay runs are reproducible;
// the firmware uses libc rand() here, which is not part of the fidelity
// contract (only the pool and cadence are).

const CHANGE_EFFECT_INTERVAL_MS = 60000;
const UINT32 = 0x100000000;

function mulberry32(seed) {
  let a = seed >>> 0;
  return function next() {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / UINT32;
  };
}

export function attachMaster(engine) {
  const master = {
    enabled: false,
    _rng: mulberry32(1),
    _nextChangeAt: 0,

    setEnabled(on, seed, nowMs) {
      this.enabled = on;
      if (on) {
        this._rng = mulberry32(seed);
        this._scheduleFrom(nowMs);
      }
    },

    onManualSet(nowMs) {
      if (this.enabled) this._scheduleFrom(nowMs);
    },

    // A received SET_CONTROL re-arms the change timer with the control delay
    // (or the normal interval for delay 0), mirroring RadioStateMachine's
    // SET_CONTROL timer handling.
    onControlSet(nowMs, delaySeconds) {
      if (!this.enabled) return;
      const holdMs = delaySeconds * 1000;
      this._nextChangeAt =
          (nowMs + (holdMs > 0 ? holdMs : CHANGE_EFFECT_INTERVAL_MS)) % UINT32;
    },

    // setTime is a teleport, not elapsed time — re-arm relative to the new
    // clock so jumps larger than 2^31 ms can't strand the modular-window
    // check in tick().
    onTimeJump(nowMs) {
      if (this.enabled) this._scheduleFrom(nowMs);
    },

    tick(nowMs) {
      if (!this.enabled) return;
      const remaining = (this._nextChangeAt - nowMs + UINT32) % UINT32;
      // Fire when we reach/pass the scheduled time. The modular distance
      // reads > UINT32/2 once nowMs passes _nextChangeAt (or lands exactly
      // on it, giving 0).
      if (remaining === 0 || remaining > UINT32 / 2) {
        const effectIndex =
            Math.floor(this._rng() * engine.registry.randomPoolSize);
        const paletteIndex =
            Math.floor(this._rng() * engine.listPalettes().length);
        // Mirror writeSetEffect(effect, delay=0, palette) without re-entering
        // onManualSet via engine.setEffect. The new SET_EFFECT replaces any
        // SET_CONTROL override, as on real hardware.
        engine._effectIndex = effectIndex;
        engine._paletteIndex = paletteIndex;
        engine._delaySeconds = 0;
        engine._control = null;
        this._scheduleFrom(nowMs);
      }
    },

    nextChangeInMs(nowMs) {
      if (!this.enabled) return null;
      return (this._nextChangeAt - nowMs + UINT32) % UINT32;
    },

    _scheduleFrom(nowMs) {
      // RadioStateMachine::SetEffect: a nonzero delay REPLACES the change
      // interval (SetTimer(TimerChangeEffect, delay * 1000)); zero delay
      // uses kChangeEffectInterval.
      const holdMs = engine._delaySeconds * 1000;
      const interval = holdMs > 0 ? holdMs : CHANGE_EFFECT_INTERVAL_MS;
      this._nextChangeAt = (nowMs + interval) % UINT32;
    },
  };
  engine._master = master;
  return engine;
}
