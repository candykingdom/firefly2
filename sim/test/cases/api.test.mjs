import {
  assert,
  assertDeepEqual,
  assertEqual,
  test,
} from '../harness.js';
import { SimEngine } from '../../js/engine.js';
import { initializeSimulator, parseUrlOptions } from '../../js/api.js';

function initializationDocument() {
  const elements = new Map([
    ['sim-init', { hidden: true, dataset: {}, textContent: '' }],
    ['sim-init-title', { hidden: false, dataset: {}, textContent: '' }],
    ['sim-init-detail', { hidden: false, dataset: {}, textContent: '' }],
    ['sim-app', { hidden: false, dataset: {}, textContent: '' }],
    ['status', { hidden: false, dataset: {}, textContent: '' }],
  ]);
  return {
    elements,
    getElementById(id) {
      return elements.get(id) ?? null;
    },
  };
}

function realModules(startUi = () => {}) {
  return {
    SimEngine,
    attachMaster: (engine) => engine,
    startUi,
  };
}

test(
  'URL parsing preserves public query parameters and tolerant defaults',
  () => {
    const warnings = [];
    const options = parseUrlOptions(
      '?device=scarf,unknown,bike&effect=300&palette=21&t=4294967297&paused=1',
      ['bike', 'scarf'],
      (warning) => warnings.push(warning),
    );
    assertDeepEqual(options, {
      devices: ['scarf', 'bike'],
      effect: 44,
      palette: 21,
      time: 4294967297,
      paused: true,
    });
    assertEqual(warnings.length, 1);
    assert(warnings[0].includes('unknown'));

    assertDeepEqual(parseUrlOptions('', ['scarf'], () => {}), {
      devices: ['scarf'],
      effect: 'Rainbow',
      palette: 'Rainbow',
      time: 0,
      paused: false,
    });
  },
);

test(
  'window.sim is complete and synchronous after initialization resolves',
  async () => {
    const documentObject = initializationDocument();
    const windowObject = {
      location: {
        search: '?device=scarf&effect=Dark&palette=Rainbow&t=123&paused=1',
      },
    };
    const sim = await initializeSimulator({
      windowObject,
      documentObject,
      consoleObject: { warn() {}, error() {} },
      loadModules: async () => realModules(),
    });

    assertEqual(windowObject.sim, sim);
    assertEqual(sim.pause().setTime(456), sim);
    const state = sim.getState();
    const snapshot = sim.getSnapshot();
    assertEqual(state.time, 456);
    assertEqual(state.effectName, 'Dark');
    assert(!(snapshot instanceof Promise),
      'getSnapshot must remain synchronous');
    assertEqual(snapshot.time, 456);
    const controlled = sim.setControl([9, 18, 27], 1).getSnapshot();
    assert(!(controlled instanceof Promise),
      'setControl must return a synchronous chainable surface');
    assertDeepEqual(controlled.devices[0].strips[0].leds[0], [9, 18, 27]);
    assertEqual(documentObject.elements.get('sim-init').hidden, true);
    assertEqual(documentObject.elements.get('sim-app').hidden, false);
  },
);

test(
  'renderer initialization failure becomes a fatal state with no fallback',
  async () => {
    const documentObject = initializationDocument();
    const windowObject = { location: { search: '' } };
    const errors = [];
    let rejectLoad;
    const loadModules = () => new Promise((resolve, reject) => {
      rejectLoad = reject;
    });
    const initialization = initializeSimulator({
      windowObject,
      documentObject,
      consoleObject: { warn() {}, error: (error) => errors.push(error) },
      loadModules,
    });

    assertEqual(
      documentObject.elements.get('sim-init').dataset.state, 'loading');
    assertEqual(documentObject.elements.get('sim-app').hidden, true);
    rejectLoad(new Error('firefly-renderer.wasm is missing'));
    const result = await initialization;

    assertEqual(result, null);
    assertEqual(windowObject.sim, undefined);
    assertEqual(documentObject.elements.get('sim-init').dataset.state, 'fatal');
    assertEqual(documentObject.elements.get('sim-init').hidden, false);
    assert(documentObject.elements.get('sim-init-detail').textContent
      .includes('firefly-renderer.wasm is missing'));
    assertEqual(errors.length, 1);
  },
);
