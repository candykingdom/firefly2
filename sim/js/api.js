// Browser bootstrap: initialize the committed C++ renderer, build the engine
// from URL params, expose it as window.sim, and only then start the UI.

async function loadSimulatorModules() {
  const [{ SimEngine }, { attachMaster }, { startUi }] = await Promise.all([
    import('./engine.js'),
    import('./master.js'),
    import('./ui.js'),
  ]);
  return { SimEngine, attachMaster, startUi };
}

export function parseUrlOptions(search, deviceNames, warn = console.warn) {
  const params = new URLSearchParams(search);
  const knownDevices = new Set(deviceNames);
  const options = {
    devices: ['scarf'],
    effect: 'Rainbow',
    palette: 'Rainbow',
    time: 0,
    paused: params.get('paused') === '1',
  };
  if (params.has('device')) {
    const names = params.get('device').split(',').filter(Boolean);
    const known = names.filter((name) => knownDevices.has(name));
    const unknown = names.filter((name) => !knownDevices.has(name));
    if (unknown.length) {
      warn(`ignoring unknown device(s): ${unknown.join(', ')}`);
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
    const time = Number(params.get('t'));
    if (Number.isFinite(time)) options.time = time;
  }
  return options;
}

function buildEngineFromUrl(SimEngine, search, warn) {
  const catalog = new SimEngine({ devices: [], paused: true });
  const deviceNames = catalog.listDevices().map((device) => device.name);
  const options = parseUrlOptions(search, deviceNames, warn);
  try {
    return new SimEngine(options);
  } catch (error) {
    // Unknown effect/palette names and similar URL input degrade to defaults;
    // renderer initialization failures are handled by initializeSimulator.
    warn(`invalid URL state (${error.message}); using defaults`);
    return new SimEngine({ paused: options.paused });
  }
}

function showInitializationState(documentObject, state, detail = '') {
  const initialization = documentObject.getElementById('sim-init');
  const title = documentObject.getElementById('sim-init-title');
  const detailElement = documentObject.getElementById('sim-init-detail');
  const application = documentObject.getElementById('sim-app');
  const status = documentObject.getElementById('status');
  if (!initialization || !title || !detailElement || !application || !status) {
    throw new Error('simulator page is missing initialization-state elements');
  }

  initialization.dataset.state = state;
  if (state === 'ready') {
    initialization.hidden = true;
    application.hidden = false;
    status.hidden = false;
    return;
  }

  initialization.hidden = false;
  application.hidden = true;
  status.hidden = true;
  if (state === 'loading') {
    title.textContent = 'Loading the production C++ renderer…';
    detailElement.textContent =
      'Initializing the checked-in WebAssembly artifact.';
  } else {
    title.textContent = 'Simulator could not start';
    detailElement.textContent =
      `The shared C++ renderer failed to initialize: ${detail}`;
  }
}

export async function initializeSimulator({
  windowObject = globalThis.window,
  documentObject = globalThis.document,
  consoleObject = globalThis.console,
  search = windowObject?.location?.search ?? '',
  loadModules = loadSimulatorModules,
} = {}) {
  showInitializationState(documentObject, 'loading');
  try {
    const { SimEngine, attachMaster, startUi } = await loadModules();
    const warn = (message) => consoleObject.warn(message);
    const sim = attachMaster(buildEngineFromUrl(SimEngine, search, warn));
    windowObject.sim = sim;
    startUi(sim);
    showInitializationState(documentObject, 'ready');
    return sim;
  } catch (error) {
    delete windowObject.sim;
    showInitializationState(documentObject, 'fatal', error.message);
    consoleObject.error(error);
    return null;
  }
}

// Top-level await means any module waiting for the simulator page's api.js
// import observes a fully initialized, synchronous window.sim surface.
if (typeof document !== 'undefined' &&
    document.documentElement.hasAttribute('data-firefly-simulator')) {
  await initializeSimulator();
}
