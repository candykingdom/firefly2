import assert from 'node:assert/strict';
import { Buffer } from 'node:buffer';
import { createHash } from 'node:crypto';
import { readFile, readdir, stat } from 'node:fs/promises';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)), '../../..');
const GENERATED = path.join(ROOT, 'sim/generated');
const MANIFEST_PATH = path.join(GENERATED, 'manifest.json');
const EMSCRIPTEN_VERSION = '6.0.3';
const FAKE_FAST_LED_COMMIT = 'f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca';
const ARTIFACT_FILES = ['firefly-renderer.js', 'firefly-renderer.wasm'];
const SOURCE_DIRECTORIES = [
  'lib/color',
  'lib/debug',
  'lib/device',
  'lib/effect',
  'lib/fake-radio',
  'lib/led_manager',
  'lib/math',
  'lib/radio',
  'lib/types',
  'src/generic',
  'sim/wasm',
];
const SOURCE_FILES = [
  'scripts/build-simulator-wasm.sh',
  'scripts/simulator-wasm.mjs',
];
const SOURCE_EXTENSIONS = new Set([
  '.c', '.cc', '.cpp', '.h', '.hpp', '.cmake', '.txt',
]);
const FINGERPRINT_PREAMBLE = [
  'firefly-simulator-artifact-v1',
  `emscripten=${EMSCRIPTEN_VERSION}`,
  `fake-fast-led=${FAKE_FAST_LED_COMMIT}`,
].join('\n');

async function walk(relativeDirectory) {
  const entries = await readdir(path.join(ROOT, relativeDirectory), {
    withFileTypes: true,
  });
  const files = [];
  for (const entry of entries) {
    const relativePath = path.posix.join(relativeDirectory, entry.name);
    if (entry.isDirectory()) {
      if (!entry.name.startsWith('build') &&
          !entry.name.startsWith('.build')) {
        files.push(...await walk(relativePath));
      }
    } else if (entry.isFile() &&
               SOURCE_EXTENSIONS.has(path.extname(entry.name))) {
      files.push(relativePath);
    }
  }
  return files;
}

async function sourceFingerprint() {
  const files = [...SOURCE_FILES];
  for (const directory of SOURCE_DIRECTORIES) {
    files.push(...await walk(directory));
  }
  files.sort();

  const hash = createHash('sha256');
  hash.update(`${FINGERPRINT_PREAMBLE}\n`);
  for (const relativePath of files) {
    const contents = (await readFile(path.join(ROOT, relativePath), 'utf8'))
      .replaceAll('\r\n', '\n');
    const byteLength = Buffer.byteLength(contents);
    hash.update(`${relativePath}\0${byteLength}\0`);
    hash.update(contents);
    hash.update('\0');
  }
  return hash.digest('hex');
}

async function readManifest() {
  return JSON.parse(await readFile(MANIFEST_PATH, 'utf8'));
}

test('artifact manifest has the fixed schema and toolchain pins', async () => {
  const manifest = await readManifest();
  assert.deepEqual(Object.keys(manifest), [
    'schemaVersion',
    'rendererAbi',
    'emscriptenVersion',
    'fakeFastLedCommit',
    'sourceFingerprint',
    'files',
  ]);
  assert.equal(manifest.schemaVersion, 1);
  assert.equal(manifest.rendererAbi, 1);
  assert.equal(manifest.emscriptenVersion, EMSCRIPTEN_VERSION);
  assert.equal(manifest.fakeFastLedCommit, FAKE_FAST_LED_COMMIT);
  assert.match(manifest.sourceFingerprint, /^[0-9a-f]{64}$/);
  assert.deepEqual(Object.keys(manifest.files), ARTIFACT_FILES);
});

test(
  'artifact manifest hashes and sizes match the committed bytes',
  async () => {
    const manifest = await readManifest();
    for (const filename of ARTIFACT_FILES) {
      const bytes = await readFile(path.join(GENERATED, filename));
      assert.deepEqual(
        Object.keys(manifest.files[filename]), ['sha256', 'bytes']);
      assert.equal(manifest.files[filename].sha256,
        createHash('sha256').update(bytes).digest('hex'));
      const artifactStat = await stat(path.join(GENERATED, filename));
      assert.equal(manifest.files[filename].bytes, artifactStat.size);
    }
  },
);

test(
  'artifact source fingerprint matches all normalized renderer inputs',
  async () => {
    const manifest = await readManifest();
    assert.equal(manifest.sourceFingerprint, await sourceFingerprint());
  },
);

test(
  'artifact metadata is canonical and contains no machine-specific data',
  async () => {
    const manifestBytes = await readFile(MANIFEST_PATH, 'utf8');
    const manifest = JSON.parse(manifestBytes);
    assert.equal(manifestBytes, `${JSON.stringify(manifest, null, 2)}\n`);

    const generatedText = [manifestBytes, await readFile(path.join(GENERATED,
      'firefly-renderer.js'), 'utf8')].join('\n');
    const machineSpecific =
    /Users[\\/]|home[\\/]|dirty|gitDescribe|timestamp/i;
    const windowsPath = /(?:^|[^A-Za-z])[A-Za-z]:[\\/]/;
    assert.doesNotMatch(generatedText, machineSpecific);
    assert.doesNotMatch(generatedText, windowsPath);

    const wasmText = (await readFile(path.join(GENERATED,
      'firefly-renderer.wasm'))).toString('latin1');
    assert(!wasmText.includes(ROOT), 'Wasm contains the checkout path');
    assert.doesNotMatch(wasmText, machineSpecific);
    assert.doesNotMatch(wasmText, windowsPath);
  },
);
