// Canvas renderer + control panel. Strictly a client of the SimEngine API —
// everything the UI does goes through the same methods window.sim exposes.

const LED_SPACING = 14;
const LED_RADIUS = 4.5;
const STRIP_GAP = 26;
const DEVICE_GAP = 44;
const MARGIN = 56;
const LABEL_COLOR = '#8b93a3';
const RING_MIN_RADIUS = 26;

export function startUi(sim) {
  const canvas = document.getElementById('stage');
  const ctx = canvas.getContext('2d');
  const stageWrap = document.getElementById('stage-wrap');
  const pausedBadge = document.getElementById('paused-badge');
  const statusBar = document.getElementById('status');

  buildDeviceList(sim);
  buildEffectList(sim);
  buildPaletteList(sim);
  wireClock(sim);
  wireMaster(sim);
  wireControl(sim);

  let lastWall = performance.now();
  function frame(now) {
    sim.advanceWall(now - lastWall);
    lastWall = now;
    const snapshot = sim.getSnapshot();
    draw(snapshot);
    reflectState(sim);
    requestAnimationFrame(frame);
  }
  // Paint synchronously so the page never shows an empty stage (and headless
  // screenshots don't race the first rAF), then hand off to the loop.
  frame(lastWall);

  document.addEventListener('visibilitychange', () => {
    // Skip the hidden gap so the show continues from the correct clock
    // position without a burst of catch-up frames.
    if (!document.hidden) {
      const now = performance.now();
      sim.advanceWall(now - lastWall);
      lastWall = now;
    }
  });

  // --- layout & drawing ---

  function stripExtent(strip) {
    if (strip.flags.includes('Circular')) {
      const r = ringRadius(strip);
      return { w: r * 2 + LED_RADIUS * 4, h: r * 2 + LED_RADIUS * 4 };
    }
    return {
      w: Math.max(1, strip.leds.length) * LED_SPACING,
      h: LED_RADIUS * 4,
    };
  }

  function ringRadius(strip) {
    return Math.max(RING_MIN_RADIUS,
      (strip.leds.length * LED_SPACING * 0.8) / (2 * Math.PI));
  }

  function draw(snapshot) {
    // Measure
    let width = 0;
    let height = MARGIN;
    for (const device of snapshot.devices) {
      let deviceH = 20;  // label
      let deviceW = 0;
      for (const strip of device.strips) {
        const { w, h } = stripExtent(strip);
        deviceH += h + STRIP_GAP;
        deviceW = Math.max(deviceW, w);
      }
      width = Math.max(width, deviceW);
      height += deviceH + DEVICE_GAP;
    }
    width += MARGIN * 2;

    const wrapW = stageWrap.clientWidth;
    const cssW = Math.max(width, wrapW);
    const cssH = Math.max(height, stageWrap.clientHeight);
    const dpr = window.devicePixelRatio || 1;
    if (canvas.width !== Math.round(cssW * dpr) ||
        canvas.height !== Math.round(cssH * dpr)) {
      canvas.width = Math.round(cssW * dpr);
      canvas.height = Math.round(cssH * dpr);
      canvas.style.width = `${cssW}px`;
      canvas.style.height = `${cssH}px`;
    }
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, cssW, cssH);

    let y = MARGIN;
    for (const device of snapshot.devices) {
      ctx.fillStyle = LABEL_COLOR;
      ctx.font = '600 12px system-ui, sans-serif';
      ctx.textBaseline = 'alphabetic';
      ctx.fillText(device.name, MARGIN, y);
      y += 20;
      for (const strip of device.strips) {
        const extent = stripExtent(strip);
        if (strip.flags.includes('Circular')) {
          drawRing(strip, MARGIN + extent.w / 2, y + extent.h / 2);
        } else {
          drawLine(strip, MARGIN, y + extent.h / 2);
        }
        drawFlagBadges(strip, MARGIN + extent.w + 12, y + extent.h / 2);
        y += extent.h + STRIP_GAP;
      }
      y += DEVICE_GAP;
    }
  }

  function drawLed(x, y, [r, g, b]) {
    const luma = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    ctx.beginPath();
    if (luma > 4) {
      ctx.shadowColor = `rgb(${r},${g},${b})`;
      ctx.shadowBlur = 6 + (luma / 255) * 14;
      ctx.fillStyle = `rgb(${r},${g},${b})`;
    } else {
      // An unlit LED is still visible as a dark bead.
      ctx.shadowBlur = 0;
      ctx.fillStyle = '#1d222d';
    }
    ctx.arc(x, y, LED_RADIUS, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;
  }

  function drawLine(strip, x0, cy) {
    strip.leds.forEach((led, i) => {
      drawLed(x0 + i * LED_SPACING + LED_SPACING / 2, cy, led);
    });
  }

  function drawRing(strip, cx, cy) {
    const radius = ringRadius(strip);
    const n = strip.leds.length;
    strip.leds.forEach((led, i) => {
      const angle = (i / n) * Math.PI * 2 - Math.PI / 2;
      drawLed(
        cx + radius * Math.cos(angle),
        cy + radius * Math.sin(angle),
        led,
      );
    });
  }

  function drawFlagBadges(strip, x, cy) {
    const flags = strip.flags.join(' · ');
    ctx.fillStyle = LABEL_COLOR;
    ctx.font = '11px system-ui, sans-serif';
    ctx.textBaseline = 'middle';
    const label = flags ? `${strip.leds.length} · ${flags}`
      : `${strip.leds.length}`;
    ctx.fillText(label, x, cy);
  }

  // --- controls ---

  function buildDeviceList(sim) {
    const host = document.getElementById('device-list');
    const selected = new Set(sim.getState().devices);
    for (const device of sim.listDevices()) {
      const label = document.createElement('label');
      label.className = 'opt';
      const box = document.createElement('input');
      box.type = 'checkbox';
      box.checked = selected.has(device.name);
      box.addEventListener('change', () => {
        if (box.checked) selected.add(device.name);
        else selected.delete(device.name);
        if (selected.size === 0) {  // never leave the stage empty
          selected.add(device.name);
          box.checked = true;
          return;
        }
        sim.setDevices([...selected]);
      });
      const name = document.createElement('span');
      name.textContent = device.name;
      const meta = document.createElement('span');
      meta.className = 'meta';
      meta.textContent =
          device.strips.map((s) => s.ledCount).join('+') + ' leds';
      label.append(box, name, meta);
      host.appendChild(label);
    }
  }

  function buildEffectList(sim) {
    const host = document.getElementById('effect-list');
    const seen = new Set();
    for (const { index, name, weight } of sim.listEffects()) {
      if (seen.has(name)) continue;  // duplicates = weights, list each once
      seen.add(name);
      const btn = document.createElement('button');
      btn.className = 'opt';
      btn.dataset.effectName = name;
      const idx = document.createElement('span');
      idx.className = 'idx';
      idx.textContent = index;
      const label = document.createElement('span');
      label.textContent = name;
      const meta = document.createElement('span');
      meta.className = 'meta';
      meta.textContent = weight > 0 ? `w${weight}` : '';
      btn.append(idx, label, meta);
      btn.addEventListener('click', () => sim.setEffect(name));
      host.appendChild(btn);
    }
  }

  function buildPaletteList(sim) {
    const host = document.getElementById('palette-list');
    for (const { index, name, colors } of sim.listPalettes()) {
      const btn = document.createElement('button');
      btn.className = 'swatch';
      btn.dataset.paletteIndex = index;
      const bar = document.createElement('span');
      bar.className = 'bar';
      const stops = colors.map((c, i) => {
        const { r, g, b } = hsvPreview(c);
        const pct = colors.length === 1 ? '' :
          ` ${(i / (colors.length - 1)) * 100}%`;
        return `rgb(${r},${g},${b})${pct}`;
      });
      bar.style.background = colors.length === 1 ?
        stops[0] : `linear-gradient(90deg, ${stops.join(', ')})`;
      const label = document.createElement('span');
      label.className = 'name';
      label.textContent = name;
      btn.append(bar, label);
      btn.addEventListener('click', () => sim.setPalette(index));
      host.appendChild(btn);
    }
  }

  function wireClock(sim) {
    const playPause = document.getElementById('play-pause');
    const speed = document.getElementById('speed');
    const scrub = document.getElementById('scrub');
    const exact = document.getElementById('time-exact');
    const setBtn = document.getElementById('time-set');

    playPause.addEventListener('click', () => {
      if (sim.getState().paused) sim.play();
      else sim.pause();
    });
    speed.addEventListener('change', () => sim.setSpeed(Number(speed.value)));
    scrub.addEventListener('input', () => sim.setTime(Number(scrub.value)));
    setBtn.addEventListener('click', () => sim.setTime(Number(exact.value)));
    exact.addEventListener('keydown', (event) => {
      if (event.key === 'Enter') sim.setTime(Number(exact.value));
    });
  }

  function wireMaster(sim) {
    const toggle = document.getElementById('master-toggle');
    toggle.addEventListener('change', () => sim.setMasterMode(toggle.checked));
  }

  function wireControl(sim) {
    const color = document.getElementById('control-color');
    const delay = document.getElementById('control-delay');
    document.getElementById('control-send').addEventListener('click', () => {
      const hex = color.value;
      const rgb = [1, 3, 5].map((i) => parseInt(hex.slice(i, i + 2), 16));
      sim.setControl(rgb, Number(delay.value));
    });
    document.getElementById('control-clear')
      .addEventListener('click', () => sim.clearControl());
  }

  // Reflect engine state back into the controls every frame, so master-mode
  // changes and programmatic (console) driving stay visible.
  function reflectState(sim) {
    const state = sim.getState();
    pausedBadge.hidden = !state.paused;
    document.getElementById('play-pause').textContent =
        state.paused ? 'Play' : 'Pause';

    for (const btn of document.querySelectorAll('#effect-list .opt')) {
      btn.classList.toggle('selected',
        btn.dataset.effectName === state.effectName);
    }
    for (const btn of document.querySelectorAll('#palette-list .swatch')) {
      btn.classList.toggle('selected',
        Number(btn.dataset.paletteIndex) ===
                               state.paletteIndex % 22);
    }

    document.getElementById('time-readout').textContent =
        `${(state.time / 1000).toFixed(3)} s`;
    const scrub = document.getElementById('scrub');
    if (document.activeElement !== scrub) {
      scrub.value = Math.min(state.time, Number(scrub.max));
    }

    const masterStatus = document.getElementById('master-status');
    if (state.masterMode) {
      const remaining = sim._master.nextChangeInMs(state.time);
      masterStatus.textContent =
          `Next change in ${(remaining / 1000).toFixed(1)} s`;
    } else {
      masterStatus.textContent =
          'Changes effect every 60 s from the weighted pool.';
    }
    document.getElementById('master-toggle').checked = state.masterMode;

    const controlNote = state.control ?
      ` · control [${state.control.rgb}] ${state.control.delaySeconds}s` : '';
    statusBar.textContent =
        `t=${state.time} ms · ` +
        `effect ${state.effectIndex} (${state.effectName})` +
        ` · palette ${state.paletteIndex} (${state.paletteName})` +
        `${state.masterMode ? ' · master' : ''}${controlNote}` +
        ` · ${state.speed}×${state.paused ? ' · paused' : ''}`;
  }
}

// Approximate CHSV→sRGB preview for palette swatches (UI only — the engine
// uses the byte-exact hsv2rgbRainbow; this just needs to look right).
function hsvPreview({ h, s, v }) {
  const hue = (h / 255) * 360;
  const sat = s / 255;
  const val = v / 255;
  const c = val * sat;
  const x = c * (1 - Math.abs(((hue / 60) % 2) - 1));
  const m = val - c;
  const [r1, g1, b1] =
      hue < 60 ? [c, x, 0] : hue < 120 ? [x, c, 0] : hue < 180 ? [0, c, x] :
        hue < 240 ? [0, x, c] : hue < 300 ? [x, 0, c] : [c, 0, x];
  return {
    r: Math.round((r1 + m) * 255),
    g: Math.round((g1 + m) * 255),
    b: Math.round((b1 + m) * 255),
  };
}
