#!/usr/bin/env node

import { createHash } from 'node:crypto';
import {
  appendFile,
  copyFile,
  mkdir,
  mkdtemp,
  readFile,
  readdir,
  rename,
  rm,
  stat,
  writeFile,
} from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const DEFAULT_ARTIFACT_DIRECTORY = path.join(ROOT, 'sim/generated');
const EMSCRIPTEN_VERSION = '6.0.3';
const FAKE_FAST_LED_COMMIT = 'f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca';
const ARTIFACT_FILES = ['firefly-renderer.js', 'firefly-renderer.wasm'];
const GENERATED_FILES = [...ARTIFACT_FILES, 'manifest.json'];
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
const SOURCE_EXTENSIONS = new Set(['.c', '.cc', '.cpp', '.h', '.hpp', '.cmake', '.txt']);
const FINGERPRINT_PREAMBLE = [
  'firefly-simulator-artifact-v1',
  `emscripten=${EMSCRIPTEN_VERSION}`,
  `fake-fast-led=${FAKE_FAST_LED_COMMIT}`,
].join('\n');

function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: ROOT,
    encoding: options.capture ? 'utf8' : undefined,
    stdio: options.capture ? 'pipe' : 'inherit',
  });
  if (result.error) {
    throw new Error(`could not run ${command}: ${result.error.message}`);
  }
  if (result.status !== 0) {
    const detail = options.capture
      ? `\n${result.stdout ?? ''}${result.stderr ?? ''}`.trimEnd()
      : '';
    throw new Error(`${command} exited with status ${result.status}${detail}`);
  }
  return `${result.stdout ?? ''}${result.stderr ?? ''}`;
}

function runEmscriptenTool(name, args, options = {}) {
  if (process.platform !== 'win32') {
    return run(name, args, options);
  }
  if (!process.env.EMSDK) {
    throw new Error('EMSDK is not set; activate Emscripten 6.0.3 first');
  }
  const script = path.join(process.env.EMSDK, 'upstream', 'emscripten', `${name}.py`);
  return run(process.env.EMSDK_PYTHON ?? 'python', [script, ...args], options);
}

function verifyToolchain() {
  const versionOutput = runEmscriptenTool('emcc', ['--version'], { capture: true });
  const firstLine = versionOutput.split(/\r?\n/, 1)[0];
  const match = firstLine.match(/\) ([0-9]+\.[0-9]+\.[0-9]+)(?: |$)/);
  if (!match || match[1] !== EMSCRIPTEN_VERSION) {
    throw new Error(
      `Emscripten ${EMSCRIPTEN_VERSION} is required; found ${firstLine || 'unknown version'}`,
    );
  }
  run('cmake', ['--version'], { capture: true });
}

async function walkSourceDirectory(relativeDirectory) {
  const entries = await readdir(path.join(ROOT, relativeDirectory), { withFileTypes: true });
  const files = [];
  for (const entry of entries) {
    const relativePath = path.posix.join(relativeDirectory, entry.name);
    if (entry.isDirectory()) {
      if (!entry.name.startsWith('build') && !entry.name.startsWith('.build')) {
        files.push(...await walkSourceDirectory(relativePath));
      }
    } else if (entry.isFile() && SOURCE_EXTENSIONS.has(path.extname(entry.name))) {
      files.push(relativePath);
    }
  }
  return files;
}

async function calculateSourceFingerprint() {
  const files = [...SOURCE_FILES];
  for (const directory of SOURCE_DIRECTORIES) {
    files.push(...await walkSourceDirectory(directory));
  }
  files.sort();

  const hash = createHash('sha256');
  hash.update(`${FINGERPRINT_PREAMBLE}\n`);
  for (const relativePath of files) {
    const contents = (await readFile(path.join(ROOT, relativePath), 'utf8'))
      .replaceAll('\r\n', '\n');
    hash.update(`${relativePath}\0${Buffer.byteLength(contents)}\0`);
    hash.update(contents);
    hash.update('\0');
  }
  return hash.digest('hex');
}

async function generateManifest(outputDirectory, sourceFingerprint) {
  const files = {};
  for (const filename of ARTIFACT_FILES) {
    const artifactPath = path.join(outputDirectory, filename);
    const bytes = await readFile(artifactPath);
    files[filename] = {
      sha256: createHash('sha256').update(bytes).digest('hex'),
      bytes: (await stat(artifactPath)).size,
    };
  }

  const manifest = {
    schemaVersion: 1,
    rendererAbi: 1,
    emscriptenVersion: EMSCRIPTEN_VERSION,
    fakeFastLedCommit: FAKE_FAST_LED_COMMIT,
    sourceFingerprint,
    files,
  };
  await writeFile(path.join(outputDirectory, 'manifest.json'),
    `${JSON.stringify(manifest, null, 2)}\n`);
}

async function cleanBuild(temporaryRoot) {
  verifyToolchain();
  const fingerprintBeforeBuild = await calculateSourceFingerprint();
  const buildDirectory = path.join(temporaryRoot, 'build');
  const outputDirectory = path.join(temporaryRoot, 'generated');
  await mkdir(outputDirectory, { recursive: true });

  runEmscriptenTool('emcmake', [
    'cmake',
    '-S', path.join(ROOT, 'sim/wasm'),
    '-B', buildDirectory,
    '-DCMAKE_BUILD_TYPE=Release',
    `-DFIREFLY_SIM_OUTPUT_DIR=${outputDirectory}`,
  ], { capture: true });
  run('cmake', ['--build', buildDirectory, '--config', 'Release', '--parallel'],
    { capture: true });

  for (const filename of ARTIFACT_FILES) {
    await stat(path.join(outputDirectory, filename));
  }
  const fingerprintAfterBuild = await calculateSourceFingerprint();
  if (fingerprintAfterBuild !== fingerprintBeforeBuild) {
    throw new Error('renderer sources changed during the Wasm build; retry from a stable tree');
  }
  await generateManifest(outputDirectory, fingerprintBeforeBuild);
  return outputDirectory;
}

async function withCleanBuild(callback) {
  const temporaryRoot = await mkdtemp(path.join(os.tmpdir(), 'firefly-simulator-wasm-'));
  try {
    const outputDirectory = await cleanBuild(temporaryRoot);
    return await callback(outputDirectory);
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
}

async function replaceArtifacts() {
  await withCleanBuild(async outputDirectory => {
    await mkdir(DEFAULT_ARTIFACT_DIRECTORY, { recursive: true });
    const staged = [];
    try {
      for (const filename of GENERATED_FILES) {
        const stagedPath = path.join(DEFAULT_ARTIFACT_DIRECTORY,
          `.${filename}.${process.pid}.tmp`);
        await copyFile(path.join(outputDirectory, filename), stagedPath);
        staged.push([stagedPath, path.join(DEFAULT_ARTIFACT_DIRECTORY, filename)]);
      }
      for (const [stagedPath, destinationPath] of staged) {
        await rename(stagedPath, destinationPath);
      }
    } finally {
      await Promise.all(staged.map(([stagedPath]) => rm(stagedPath, { force: true })));
    }
  });
  console.log(`updated ${GENERATED_FILES.map(filename => `sim/generated/${filename}`).join(', ')}`);
}

async function compareArtifacts(outputDirectory, artifactDirectory) {
  const mismatches = [];
  for (const filename of GENERATED_FILES) {
    try {
      const [expected, actual] = await Promise.all([
        readFile(path.join(outputDirectory, filename)),
        readFile(path.join(artifactDirectory, filename)),
      ]);
      if (!expected.equals(actual)) {
        mismatches.push(filename);
      }
    } catch (error) {
      if (error.code === 'ENOENT') {
        mismatches.push(filename);
      } else {
        throw error;
      }
    }
  }
  return mismatches;
}

async function checkArtifacts() {
  const artifactDirectory = process.env.FIREFLY_SIM_ARTIFACT_DIR
    ? path.resolve(process.env.FIREFLY_SIM_ARTIFACT_DIR)
    : DEFAULT_ARTIFACT_DIRECTORY;
  const stale = await withCleanBuild(outputDirectory =>
    compareArtifacts(outputDirectory, artifactDirectory));

  if (stale.length > 0) {
    console.error(`stale simulator Wasm artifact: ${stale.join(', ')}`);
    process.exitCode = 1;
    return;
  }
  console.log('simulator Wasm artifact is current');
}

async function testArtifactFreshness() {
  await withCleanBuild(async outputDirectory => {
    const currentMismatches = await compareArtifacts(outputDirectory,
      DEFAULT_ARTIFACT_DIRECTORY);
    if (currentMismatches.length > 0) {
      throw new Error(`baseline artifact is stale: ${currentMismatches.join(', ')}`);
    }

    const staleDirectory = await mkdtemp(path.join(os.tmpdir(),
      'firefly-simulator-stale-'));
    try {
      for (const filename of GENERATED_FILES) {
        await copyFile(path.join(DEFAULT_ARTIFACT_DIRECTORY, filename),
          path.join(staleDirectory, filename));
      }
      await appendFile(path.join(staleDirectory, 'firefly-renderer.wasm'),
        Buffer.from([0]));
      const staleMismatches = await compareArtifacts(outputDirectory, staleDirectory);
      if (staleMismatches.length !== 1 ||
          staleMismatches[0] !== 'firefly-renderer.wasm') {
        throw new Error(`stale artifact was not identified: ${staleMismatches.join(', ')}`);
      }
    } finally {
      await rm(staleDirectory, { recursive: true, force: true });
    }
  });
  console.log('simulator Wasm freshness integration test passed');
}

async function main() {
  const command = process.argv[2];
  if (command === 'build') {
    await replaceArtifacts();
  } else if (command === 'check') {
    await checkArtifacts();
  } else if (command === 'test') {
    await testArtifactFreshness();
  } else {
    throw new Error('usage: node scripts/simulator-wasm.mjs <build|check|test>');
  }
}

main().catch(error => {
  console.error(`simulator Wasm ${process.argv[2] ?? 'command'} failed: ${error.message}`);
  process.exitCode = 1;
});
