// Browser bootstrap: build the engine from URL params, expose it as
// window.sim (the documented programmatic API — see
// specs/001-web-simulator/contracts/sim-api.md), and start the UI.

import { SimEngine } from './engine.js';
import { DEVICES } from './devices.js';
import { attachMaster } from './master.js';
import { startUi } from './ui.js';

// URL params come from anywhere (hand-edited links, automation) — invalid
// values must degrade to defaults with a console warning, never a blank page
// (FR-014's tolerance applies to this entry point too).
function fromUrl() {
  const params = new URLSearchParams(location.search);
  const options = {
    devices: ['scarf'],
    effect: 'Rainbow',
    palette: 'Rainbow',
    time: 0,
    paused: params.get('paused') === '1',
  };
  if (params.has('device')) {
    const names = params.get('device').split(',').filter(Boolean);
    const known = names.filter((name) => name in DEVICES);
    const unknown = names.filter((name) => !(name in DEVICES));
    if (unknown.length) {
      console.warn(`ignoring unknown device(s): ${unknown.join(', ')}`);
    }
    if (known.length) options.devices = known;
  }
  for (const param of ['effect', 'palette']) {
    if (params.has(param)) {
      const raw = params.get(param);
      // Numeric params are wire bytes: clamp to 0-255 like the radio would.
      options[param] = /^\d+$/.test(raw) ? Number(raw) & 0xff : raw;
    }
  }
  if (params.has('t')) {
    const t = Number(params.get('t'));
    if (Number.isFinite(t)) options.time = t;
  }
  return options;
}

function buildEngine() {
  const options = fromUrl();
  try {
    return new SimEngine(options);
  } catch (err) {
    // Unknown effect/palette names and similar — fall back to defaults so the
    // page always comes up.
    console.warn(`invalid URL state (${err.message}); using defaults`);
    return new SimEngine({ paused: options.paused });
  }
}

const sim = attachMaster(buildEngine());
window.sim = sim;
startUi(sim);
