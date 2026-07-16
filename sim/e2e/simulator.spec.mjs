import { expect, test } from '@playwright/test';
import { Buffer } from 'node:buffer';

const DEFAULT_TIMEOUT_MS = 15_000;

async function waitForSimulator(page) {
  await page.waitForFunction(
    () => typeof window.sim?.getSnapshot === 'function',
    null,
    { timeout: DEFAULT_TIMEOUT_MS },
  );
  await expect(page.locator('#sim-init')).toBeHidden();
  await expect(page.locator('#sim-app')).toBeVisible();
  await expect(page.locator('#status')).toBeVisible();
}

async function waitForPaint(page) {
  await page.evaluate(() => new Promise((resolve) => {
    requestAnimationFrame(() => requestAnimationFrame(resolve));
  }));
}

async function canvasDigest(page) {
  return page.locator('#stage').evaluate(async (canvas) => {
    const context = canvas.getContext('2d');
    const pixels = context.getImageData(
      0,
      0,
      canvas.width,
      canvas.height,
    ).data;
    const digest = await globalThis.crypto.subtle.digest('SHA-256', pixels);
    return [...new Uint8Array(digest)]
      .map((byte) => byte.toString(16).padStart(2, '0'))
      .join('');
  });
}

function collectUnexpectedBrowserErrors(page) {
  const errors = [];
  page.on('pageerror', (error) => errors.push(`pageerror: ${error.message}`));
  page.on('response', (response) => {
    if (response.status() >= 400) {
      errors.push(`response ${response.status()}: ${response.url()}`);
    }
  });
  page.on('console', (message) => {
    if (message.type() === 'error') {
      errors.push(`console: ${message.text()}`);
    }
  });
  return errors;
}

test('boots the served production Wasm page from a deep link',
  async ({ page }) => {
    const errors = collectUnexpectedBrowserErrors(page);
    const response = await page.goto(
      '/?device=scarf,puck&effect=Fire&palette=Fire&t=12345&paused=1',
    );
    expect(response?.ok()).toBe(true);
    await waitForSimulator(page);

    await expect.poll(() => page.evaluate(() => window.sim.getState()))
      .toMatchObject({
        devices: ['scarf', 'puck'],
        effectName: 'Fire',
        paletteName: 'Fire',
        time: 12345,
        paused: true,
      });
    await expect(page.locator('#device-list input')).toHaveCount(22);
    await expect(page.locator('#effect-list button')).toHaveCount(19);
    await expect(page.locator('#palette-list button')).toHaveCount(22);
    await expect(page.locator('#paused-badge')).toBeVisible();
    await expect(page.locator('#stage')).toBeVisible();
    expect(await page.locator('#stage').evaluate((canvas) => ({
      width: canvas.width,
      height: canvas.height,
    }))).toEqual(expect.objectContaining({
      width: expect.any(Number),
      height: expect.any(Number),
    }));

    await page.reload();
    await waitForSimulator(page);
    expect(await page.evaluate(() => window.sim.getState())).toMatchObject({
      devices: ['scarf', 'puck'],
      effectName: 'Fire',
      paletteName: 'Fire',
      time: 12345,
      paused: true,
    });
    expect(errors).toEqual([]);
  });

test('UI controls drive window.sim and production-rendered canvas pixels',
  async ({ page }) => {
    const errors = collectUnexpectedBrowserErrors(page);
    await page.goto('/?device=scarf&paused=1');
    await waitForSimulator(page);

    const puck = page.locator('[data-device-name="puck"]');
    const scarf = page.locator('[data-device-name="scarf"]');
    await puck.check();
    await scarf.uncheck();
    await page.locator('[data-effect-name="Fire"]').click();
    const firePaletteIndex = await page.evaluate(() => window.sim.listPalettes()
      .find((palette) => palette.name === 'Fire').index);
    await page.locator(`[data-palette-index="${firePaletteIndex}"]`).click();
    await page.locator('#time-exact').fill('4242');
    await page.locator('#time-set').click();

    await expect.poll(() => page.evaluate(() => window.sim.getState()))
      .toMatchObject({
        devices: ['puck'],
        effectName: 'Fire',
        paletteName: 'Fire',
        time: 4242,
      });

    await page.locator('#control-color').fill('#ff2200');
    await page.locator('#control-delay').fill('0');
    await page.locator('#control-send').click();
    await expect.poll(() => page.evaluate(() => window.sim.getState().control))
      .toEqual({ rgb: [255, 34, 0], delaySeconds: 0 });
    await waitForPaint(page);

    const visual = await page.evaluate(() => {
      const snapshot = window.sim.getSnapshot();
      const canvas = document.getElementById('stage');
      const context = canvas.getContext('2d');
      const pixels = context.getImageData(
        0, 0, canvas.width, canvas.height,
      ).data;
      let exactControlPixels = 0;
      for (let offset = 0; offset < pixels.length; offset += 4) {
        if (pixels[offset] === 255 && pixels[offset + 1] === 34 &&
            pixels[offset + 2] === 0 && pixels[offset + 3] === 255) {
          exactControlPixels++;
        }
      }
      return {
        firstLed: snapshot.devices[0].strips[0].leds[0],
        ledCount: snapshot.devices[0].strips[0].leds.length,
        exactControlPixels,
      };
    });
    expect(visual.firstLed).toEqual([255, 34, 0]);
    expect(visual.ledCount).toBe(12);
    expect(visual.exactControlPixels).toBeGreaterThanOrEqual(visual.ledCount);

    await page.locator('#control-clear').click();
    await expect.poll(() => page.evaluate(() => window.sim.getState().control))
      .toBeNull();
    expect(errors).toEqual([]);
  });

test('public API changes are reflected by the live UI and animation loop',
  async ({ page }) => {
    const errors = collectUnexpectedBrowserErrors(page);
    await page.goto('/?device=scarf&paused=1');
    await waitForSimulator(page);

    await page.evaluate(() => {
      window.sim
        .setDevices(['rainbow_cloak'])
        .setEffect('Dark')
        .setPalette('Candy Cane')
        .setTime(9876)
        .setSpeed(2)
        .setMasterMode(true, 7)
        .pause();
    });
    await waitForPaint(page);

    await expect(page.locator('[data-effect-name="Dark"]'))
      .toHaveClass(/selected/);
    const paletteIndex = await page.evaluate(() =>
      window.sim.getState().paletteIndex % window.sim.listPalettes().length);
    await expect(page.locator(`[data-palette-index="${paletteIndex}"]`))
      .toHaveClass(/selected/);
    await expect(page.locator('#master-toggle')).toBeChecked();
    await expect(page.locator('#paused-badge')).toBeVisible();
    await expect(page.locator('#time-readout')).toHaveText('9.876 s');
    await expect(page.locator('[data-device-name="rainbow_cloak"]'))
      .toBeChecked();
    await expect(page.locator('[data-device-name="scarf"]'))
      .not.toBeChecked();
    await expect(page.locator('#status')).toContainText('rainbow_cloak');
    await expect(page.locator('#status')).toContainText('master');

    const before = await page.evaluate(() =>
      window.sim.getState().effectIndex);
    await page.evaluate(() => window.sim.step(60_000));
    await expect.poll(() => page.evaluate(() =>
      window.sim.getState().effectIndex))
      .not.toBe(before);
    expect(errors).toEqual([]);
  });

test('deterministic linear, circular, and multi-strip visuals survive reload',
  async ({ page }, testInfo) => {
    const errors = collectUnexpectedBrowserErrors(page);
    const url = '/?device=scarf,puck,rainbow_cloak&effect=Rainbow' +
      '&palette=Rainbow&t=24680&paused=1';
    await page.goto(url);
    await waitForSimulator(page);
    await waitForPaint(page);

    const firstDigest = await canvasDigest(page);
    const snapshot = await page.evaluate(() => window.sim.getSnapshot());
    expect(snapshot.devices.map((device) => device.name))
      .toEqual(['scarf', 'puck', 'rainbow_cloak']);
    expect(snapshot.devices[0].strips).toHaveLength(1);
    expect(snapshot.devices[1].strips[0].flags).toContain('Circular');
    expect(snapshot.devices[2].strips).toHaveLength(3);

    await testInfo.attach('simulator-linear-circular-multistrip.png', {
      body: await page.screenshot({ fullPage: true }),
      contentType: 'image/png',
    });
    await testInfo.attach('simulator-snapshot.json', {
      body: Buffer.from(`${JSON.stringify(snapshot, null, 2)}\n`),
      contentType: 'application/json',
    });

    await page.reload();
    await waitForSimulator(page);
    await waitForPaint(page);
    const secondDigest = await canvasDigest(page);
    const secondSnapshot = await page.evaluate(() => window.sim.getSnapshot());
    expect(secondDigest).toBe(firstDigest);
    expect(secondSnapshot).toEqual(snapshot);
    expect(errors).toEqual([]);
  });

test('narrow viewport keeps both the stage and controls usable',
  async ({ page }, testInfo) => {
    const errors = collectUnexpectedBrowserErrors(page);
    await page.setViewportSize({ width: 760, height: 1000 });
    await page.goto('/?device=puck&effect=Rainbow&t=5000&paused=1');
    await waitForSimulator(page);
    await waitForPaint(page);

    const layout = await page.evaluate(() => {
      const application = document.getElementById('sim-app');
      const stage = document.getElementById('stage-wrap')
        .getBoundingClientRect();
      const controls = document.getElementById('controls')
        .getBoundingClientRect();
      return {
        direction: globalThis.getComputedStyle(application).flexDirection,
        stage: {
          width: stage.width,
          height: stage.height,
          bottom: stage.bottom,
        },
        controls: {
          width: controls.width,
          height: controls.height,
          top: controls.top,
        },
      };
    });
    expect(layout.direction).toBe('column');
    expect(layout.stage.width).toBeGreaterThan(700);
    expect(layout.stage.height).toBeGreaterThan(150);
    expect(layout.controls.width).toBeGreaterThan(700);
    expect(layout.controls.height).toBeGreaterThan(150);
    expect(layout.controls.top).toBeGreaterThanOrEqual(layout.stage.bottom - 1);
    await testInfo.attach('simulator-narrow-stage.png', {
      body: await page.screenshot(),
      contentType: 'image/png',
    });
    await page.locator('#control-send').scrollIntoViewIfNeeded();
    await expect(page.locator('#control-send')).toBeVisible();
    await testInfo.attach('simulator-narrow-controls.png', {
      body: await page.screenshot(),
      contentType: 'image/png',
    });
    expect(errors).toEqual([]);
  });

test('served in-browser parity harness completes with no failures',
  async ({ page }) => {
    const errors = collectUnexpectedBrowserErrors(page);
    await page.goto('/test.html');
    await page.waitForFunction(
      () => window.__testResults?.done === true,
      null,
      { timeout: 30_000 },
    );
    const results = await page.evaluate(() => window.__testResults);
    expect(results.fail).toBe(0);
    expect(results.pass).toBeGreaterThanOrEqual(60);
    await expect(page.locator('#summary')).toHaveClass('pass');
    await expect(page.locator('.case.fail')).toHaveCount(0);
    expect(errors).toEqual([]);
  });

test('missing Wasm artifact produces a visible fatal state without fallback',
  async ({ page }, testInfo) => {
    await page.route('**/generated/firefly-renderer.wasm', (route) =>
      route.fulfill({
        status: 404,
        contentType: 'text/plain',
        body: 'artifact intentionally unavailable in E2E test',
      }));
    await page.goto('/');

    await expect(page.locator('#sim-init')).toHaveAttribute(
      'data-state',
      'fatal',
      { timeout: DEFAULT_TIMEOUT_MS },
    );
    await expect(page.locator('#sim-init-title'))
      .toHaveText('Simulator could not start');
    await expect(page.locator('#sim-init-detail'))
      .toContainText('shared C++ renderer failed to initialize');
    await expect(page.locator('#sim-app')).toBeHidden();
    expect(await page.evaluate(() => typeof window.sim)).toBe('undefined');
    await testInfo.attach('simulator-missing-artifact.png', {
      body: await page.screenshot({ fullPage: true }),
      contentType: 'image/png',
    });
  });
