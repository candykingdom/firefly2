#!/usr/bin/env node

import { createReadStream } from 'node:fs';
import { stat } from 'node:fs/promises';
import { createServer } from 'node:http';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HOST = '127.0.0.1';
const PORT = 8643;
const ROOT = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  '../sim',
);
const MIME_TYPES = new Map([
  ['.css', 'text/css; charset=utf-8'],
  ['.html', 'text/html; charset=utf-8'],
  ['.js', 'text/javascript; charset=utf-8'],
  ['.json', 'application/json; charset=utf-8'],
  ['.mjs', 'text/javascript; charset=utf-8'],
  ['.wasm', 'application/wasm'],
]);

function sendText(response, statusCode, message) {
  response.writeHead(statusCode, {
    'Content-Type': 'text/plain; charset=utf-8',
    'Cache-Control': 'no-store',
  });
  response.end(`${message}\n`);
}

async function serveFile(request, response) {
  const url = new URL(request.url, `http://${request.headers.host ?? HOST}`);
  if (url.pathname === '/__health') {
    sendText(response, 200, 'ok');
    return;
  }
  if (url.pathname === '/__test-results') {
    response.writeHead(204, { 'Cache-Control': 'no-store' });
    response.end();
    return;
  }

  let pathname;
  try {
    pathname = decodeURIComponent(url.pathname);
  } catch {
    sendText(response, 400, 'invalid URL encoding');
    return;
  }
  if (pathname.endsWith('/')) pathname += 'index.html';
  const filename = path.resolve(ROOT, `.${pathname}`);
  if (filename !== ROOT && !filename.startsWith(`${ROOT}${path.sep}`)) {
    sendText(response, 403, 'outside simulator root');
    return;
  }

  try {
    const metadata = await stat(filename);
    if (!metadata.isFile()) throw new Error('not a file');
    response.writeHead(200, {
      'Content-Type': MIME_TYPES.get(path.extname(filename)) ??
        'application/octet-stream',
      'Content-Length': metadata.size,
      'Cache-Control': 'no-store',
    });
    createReadStream(filename).pipe(response);
  } catch {
    sendText(response, 404, 'not found');
  }
}

const server = createServer((request, response) => {
  serveFile(request, response).catch((error) => {
    console.error(error);
    if (!response.headersSent) sendText(response, 500, 'server error');
    else response.destroy(error);
  });
});

server.listen(PORT, HOST, () => {
  console.log(`Firefly simulator E2E server: http://${HOST}:${PORT}/`);
});

function shutdown() {
  server.close(() => process.exit(0));
}

process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
