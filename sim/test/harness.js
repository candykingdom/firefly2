// Minimal environment-agnostic test harness. In Node, cases register with
// node:test (so `node --test sim/test/` works with zero dependencies); in the
// browser, test.html collects the registered cases and runs them with a DOM
// reporter. Assertions are hand-rolled so both environments share them.

const isNode =
    typeof process !== 'undefined' && !!process.versions &&
    !!process.versions.node;

// Browser-side registry, consumed by test.html.
export const browserTests = [];

let nodeTest = null;
if (isNode) {
  ({ test: nodeTest } = await import('node:test'));
}

export function test(name, fn) {
  if (nodeTest) {
    nodeTest(name, fn);
  } else {
    browserTests.push({ name, fn });
  }
}

export function assert(condition, message = 'assertion failed') {
  if (!condition) throw new Error(message);
}

export function assertEqual(actual, expected, message = '') {
  if (actual !== expected) {
    throw new Error(
      `${message ? message + ': ' : ''}expected ` +
        `${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
  }
}

export function assertDeepEqual(actual, expected, message = '') {
  const a = JSON.stringify(actual);
  const e = JSON.stringify(expected);
  if (a !== e) {
    throw new Error(`${message ? message + ': ' : ''}expected ${e}, got ${a}`);
  }
}

export function assertThrows(fn, message = 'expected an exception') {
  let threw = false;
  try {
    fn();
  } catch {
    threw = true;
  }
  if (!threw) throw new Error(message);
}
