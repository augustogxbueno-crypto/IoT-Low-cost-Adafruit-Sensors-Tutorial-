// =============================================================================
// Sensor Tutorial Series — generic Web Serial client
// -----------------------------------------------------------------------------
// This file has ZERO sensor-specific logic. Every control (modes, gain, LED,
// integration time, extra params, channels) is built at runtime from the
// "info" event the board sends on connect. Swapping firmware to a different
// sensor should require zero changes here — that's the whole point of the
// SensorBase abstraction on the firmware side.
// =============================================================================

(function () {
  'use strict';

  // ── State ──
  let port = null;
  let reader = null;
  let writer = null;
  let sensorInfo = null;

  let currentMode = null;
  let currentSubmode = 'log';
  let selectedChannels = new Set();
  let referenceValid = false;
  let isMeasuring = false;

  let results = [];
  let resultIdCounter = 1;

  // ── DOM references ──
  const btnConnect      = document.getElementById('btn-connect');
  const app             = document.getElementById('app');
  const sensorNameEl    = document.getElementById('sensor-name');
  const sensorTypeEl    = document.getElementById('sensor-type');

  const modeTabsEl       = document.getElementById('mode-tabs');
  const panelLed         = document.getElementById('panel-led');
  const panelSubmode     = document.getElementById('panel-submode');
  const submodeTabs      = document.querySelectorAll('.submode-tab');
  const panelRef         = document.getElementById('panel-ref');
  const btnRef           = document.getElementById('btn-ref');
  const refStatus        = document.getElementById('ref-status');

  const panelGain        = document.getElementById('panel-gain');
  const panelTime        = document.getElementById('panel-time');
  const panelExtras      = document.getElementById('panel-extras');
  const timeHint         = document.getElementById('time-hint');
  const tintValue        = document.getElementById('tint-value');
  const btnApplyConfig   = document.getElementById('btn-apply-config');

  const channelGrid      = document.getElementById('channel-grid');

  const btnMeasure       = document.getElementById('btn-measure');
  const progressArea     = document.getElementById('progress-area');
  const progressText     = document.getElementById('progress-text');
  const progressBar      = document.getElementById('progress-bar');
  const progressLive     = document.getElementById('progress-live');

  const resultsHeadRow   = document.getElementById('results-head-row');
  const resultsBody      = document.getElementById('results-body');
  const btnClearAll      = document.getElementById('btn-clear-all');
  const btnDownloadCsv   = document.getElementById('btn-download-csv');
  const btnDownloadXlsx  = document.getElementById('btn-download-xlsx');

  // Live values the user is editing, sent to the board on "Apply Configuration"
  let pendingGainIdx = 0;
  let pendingGainValue = 0;
  let pendingTimeParams = {};   // { key: value } for TIME_FORMULA sensors
  let pendingTimePreset = 0;    // for TIME_PRESETS sensors
  let pendingLedMA = 0;
  let pendingLedIdx = 0;

  // =============================================================================
  // WEB SERIAL
  // =============================================================================
  if (!('serial' in navigator)) {
    document.body.innerHTML =
      '<div style="padding:40px;text-align:center;color:#fff;font-family:sans-serif">' +
      '<h2>Web Serial not supported</h2>' +
      '<p style="color:#999;margin-top:10px">Open this page in Google Chrome or Microsoft Edge (desktop).</p>' +
      '</div>';
    return;
  }

  btnConnect.addEventListener('click', async function () {
    try {
      port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 });
      writer = port.writable.getWriter();
      readLoop();
      btnConnect.textContent = 'Connected';
      btnConnect.disabled = true;
      requestInfoUntilReady();
    } catch (e) {
      showToast('Failed to connect: ' + e.message, 'error');
    }
  });

  // Opening the serial port resets the board (DTR toggle), and on Windows the
  // driver can take longer than any single fixed delay to actually finish
  // that reset before the sketch's setup() runs. A one-shot "wait 1.5s then
  // send get_info" can fire while the board is still resetting/bootloading,
  // so the command is silently lost and the page hangs on "waiting for
  // connection" forever. Instead, keep asking every 500ms until sensorInfo
  // actually arrives (onInfo sets it), for up to ~10s.
  function requestInfoUntilReady() {
    let attempts = 0;
    const maxAttempts = 20; // ~10s at 500ms apart
    const intervalId = setInterval(() => {
      if (sensorInfo) { clearInterval(intervalId); return; }
      attempts++;
      if (attempts > maxAttempts) {
        clearInterval(intervalId);
        showToast('Board did not respond. Try reconnecting.', 'error');
        return;
      }
      sendCommand({ cmd: 'get_info' });
    }, 500);
  }

  async function sendCommand(obj) {
    if (!writer) return;
    const line = JSON.stringify(obj) + '\n';
    console.log('[SERIAL TX]', line.trim()); // TEMP DEBUG
    try {
      await writer.write(new TextEncoder().encode(line));
    } catch (e) {
      console.log('[SERIAL TX] write FAILED:', e.message); // TEMP DEBUG
    }
  }

  async function readLoop() {
    const textDecoder = new TextDecoderStream();
    port.readable.pipeTo(textDecoder.writable);
    reader = textDecoder.readable.getReader();
    let buffer = '';
    try {
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        buffer += value;
        let idx;
        while ((idx = buffer.indexOf('\n')) >= 0) {
          const line = buffer.slice(0, idx).trim();
          buffer = buffer.slice(idx + 1);
          console.log('[SERIAL RX]', JSON.stringify(line)); // TEMP DEBUG
          if (line.length > 0) handleIncoming(line);
        }
      }
    } catch (e) {
      showToast('Serial connection lost', 'error');
    }
  }

  function handleIncoming(line) {
    let msg;
    try { msg = JSON.parse(line); } catch (e) { console.log('[SERIAL RX] JSON parse FAILED:', e.message); return; } // TEMP DEBUG

    switch (msg.evt) {
      case 'info':      onInfo(msg); break;
      case 'ack':        onAck(msg); break;
      case 'progress':   onProgress(msg); break;
      case 'ref_saved':  onRefSaved(msg); break;
      case 'result':     onResult(msg); break;
      case 'error':      showToast(msg.msg || 'Device error', 'error'); break;
    }
  }

  function onAck(msg) {
    if (msg.cmd === 'set_time' && msg.tint_ms !== undefined) {
      tintValue.textContent = msg.tint_ms.toFixed(2) + ' ms';
    }
  }

  // =============================================================================
  // "info" -> build the entire UI
  // =============================================================================
  function onInfo(msg) {
    sensorInfo = msg;

    sensorNameEl.textContent = msg.sensor;
    sensorTypeEl.textContent = (msg.sensorType === 'photodiode')
      ? 'Photodiode-Based Irradiance Sensor'
      : 'Color Sensor';

    buildChannelGrid();
    buildModeTabs();
    buildLedPanel();
    buildGainPanel();
    buildTimePanel();
    buildExtraParams();

    tintValue.textContent = (msg.time && msg.time.currentMs !== undefined)
      ? msg.time.currentMs.toFixed(2) + ' ms'
      : '—';

    currentMode = msg.modes[0];
    setActiveModeTab(currentMode);
    updateModeUI();

    app.classList.remove('hidden');
    showToast('Sensor connected: ' + msg.sensor, 'success');
  }

  // ── Channels ──
  function buildChannelGrid() {
    channelGrid.innerHTML = '';
    resultsHeadRow.querySelectorAll('[data-ch-header]').forEach(el => el.remove());
    selectedChannels.clear();

    sensorInfo.channels.forEach(ch => {
      const chip = document.createElement('div');
      chip.className = 'channel-chip';
      chip.dataset.id = ch.id;
      chip.innerHTML =
        '<span class="channel-swatch" style="background:' + ch.color + '"></span>' +
        '<span>' + ch.name + '</span>';
      chip.addEventListener('click', () => toggleChannel(ch.id, chip));
      channelGrid.appendChild(chip);

      const th = document.createElement('th');
      th.textContent = ch.name;
      th.dataset.chHeader = ch.id;
      resultsHeadRow.insertBefore(th, resultsHeadRow.lastElementChild);
    });
  }

  function toggleChannel(id, chipEl) {
    if (selectedChannels.has(id)) { selectedChannels.delete(id); chipEl.classList.remove('selected'); }
    else { selectedChannels.add(id); chipEl.classList.add('selected'); }
  }

  // ── Mode tabs ──
  function buildModeTabs() {
    modeTabsEl.innerHTML = '';
    const labels = { reflectance: 'Reflectance', absorbance: 'Absorbance', fluorescence: 'Fluorescence' };
    sensorInfo.modes.forEach(mode => {
      const tab = document.createElement('button');
      tab.className = 'mode-tab';
      tab.dataset.mode = mode;
      tab.textContent = labels[mode] || mode;
      tab.addEventListener('click', () => {
        currentMode = mode;
        referenceValid = false;
        refStatus.classList.add('hidden');
        setActiveModeTab(mode);
        updateModeUI();
        sendCommand({ cmd: 'set_mode', mode });
      });
      modeTabsEl.appendChild(tab);
    });
  }

  function setActiveModeTab(mode) {
    modeTabsEl.querySelectorAll('.mode-tab').forEach(t => t.classList.toggle('active', t.dataset.mode === mode));
  }

  function updateModeUI() {
    // LED panel only shown in Reflectance mode
    panelLed.classList.toggle('hidden', currentMode !== 'reflectance');

    // Submode (log/raw) available for Reflectance and Absorbance
    const hasSubmode = (currentMode === 'reflectance' || currentMode === 'absorbance');
    panelSubmode.classList.toggle('hidden', !hasSubmode);

    const showRef = hasSubmode && currentSubmode === 'log';
    panelRef.classList.toggle('hidden', !showRef);
  }

  submodeTabs.forEach(tab => {
    tab.addEventListener('click', () => {
      submodeTabs.forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      currentSubmode = tab.dataset.sub;
      updateModeUI();
      sendCommand({ cmd: 'set_submode', sub: currentSubmode });
    });
  });

  btnRef.addEventListener('click', async () => {
    if (selectedChannels.size === 0) {
      showToast('Select at least one channel before measuring the reference', 'error');
      return;
    }
    setMeasuringUI(true, 'Measuring reference...');
    await sendCommand({ cmd: 'measure_ref' });
  });

  // ── LED panel (shape depends on info.led.internal.type) ──
  function buildLedPanel() {
    panelLed.innerHTML = '';
    const led = sensorInfo.led.internal;

    if (led.type === 'none') {
      return; // stays hidden; Reflectance won't even be in modes[]
    }

    if (led.type === 'binary') {
      panelLed.innerHTML = '<label>Internal LED</label>';
      const btn = document.createElement('button');
      btn.className = 'btn btn-outline';
      btn.textContent = 'LED: On';
      let on = true;
      btn.addEventListener('click', () => {
        on = !on;
        btn.textContent = 'LED: ' + (on ? 'On' : 'Off');
        sendCommand({ cmd: 'set_led', on });
      });
      panelLed.appendChild(btn);

    } else if (led.type === 'discrete') {
      panelLed.innerHTML = '<label for="led-select">Internal LED Current</label>';
      const select = document.createElement('select');
      select.id = 'led-select';
      led.options.forEach((label, i) => {
        const opt = document.createElement('option');
        opt.value = i; opt.textContent = label;
        select.appendChild(opt);
      });
      select.addEventListener('change', () => {
        pendingLedIdx = parseInt(select.value, 10);
        sendCommand({ cmd: 'set_led', idx: pendingLedIdx });
      });
      panelLed.appendChild(select);

    } else if (led.type === 'continuous') {
      panelLed.innerHTML =
        '<label for="led-range">Internal LED Current (mA)</label>' +
        '<div class="slider-row">' +
          '<input type="range" id="led-range" min="' + led.minMA + '" max="' + led.maxMA + '" value="' + led.minMA + '">' +
          '<span id="led-range-value" class="value-badge">' + led.minMA + ' mA</span>' +
        '</div>';
      const range = document.getElementById('led-range');
      const valueBadge = document.getElementById('led-range-value');
      pendingLedMA = led.minMA;
      range.addEventListener('input', () => { valueBadge.textContent = range.value + ' mA'; });
      range.addEventListener('change', () => {
        pendingLedMA = parseInt(range.value, 10);
        sendCommand({ cmd: 'set_led', current: pendingLedMA });
      });
    }
  }

  // ── Gain panel (discrete dropdown or continuous slider) ──
  function buildGainPanel() {
    panelGain.innerHTML = '';
    const gain = sensorInfo.gain;

    if (gain.type === 'discrete') {
      panelGain.innerHTML = '<label for="gain-select">Gain</label>';
      const select = document.createElement('select');
      select.id = 'gain-select';
      gain.options.forEach((label, i) => {
        const opt = document.createElement('option');
        opt.value = i; opt.textContent = label;
        select.appendChild(opt);
      });
      select.addEventListener('change', () => { pendingGainIdx = parseInt(select.value, 10); });
      panelGain.appendChild(select);
      pendingGainIdx = 0;

    } else { // continuous
      panelGain.innerHTML =
        '<label for="gain-range">Gain (sensitivity)</label>' +
        '<div class="slider-row">' +
          '<input type="range" id="gain-range" min="' + gain.min + '" max="' + gain.max + '" value="' + gain.min + '">' +
          '<span id="gain-range-value" class="value-badge">' + gain.min + '</span>' +
        '</div>';
      const range = document.getElementById('gain-range');
      const valueBadge = document.getElementById('gain-range-value');
      pendingGainValue = gain.min;
      range.addEventListener('input', () => {
        valueBadge.textContent = range.value;
        pendingGainValue = parseFloat(range.value);
      });
    }
  }

  // ── Integration time panel (formula params or presets dropdown) ──
  function buildTimePanel() {
    panelTime.innerHTML = '';
    pendingTimeParams = {};
    const time = sensorInfo.time;

    timeHint.textContent = time.formula ? ('Formula: ' + time.formula) : '';

    if (time.type === 'formula') {
      time.params.forEach(p => {
        pendingTimeParams[p.key] = p.min;
        const row = document.createElement('div');
        row.className = 'form-row';
        row.innerHTML =
          '<div class="form-group">' +
            '<label>' + p.label + ' (' + p.min + '\u2013' + p.max + ')</label>' +
            '<input type="number" min="' + p.min + '" max="' + p.max + '" value="' + p.min + '" data-key="' + p.key + '">' +
          '</div>';
        row.querySelector('input').addEventListener('input', (e) => {
          pendingTimeParams[p.key] = parseInt(e.target.value, 10) || 0;
        });
        panelTime.appendChild(row);
      });
    } else { // presets
      panelTime.innerHTML = '<label for="time-select">Integration Time</label>';
      const select = document.createElement('select');
      select.id = 'time-select';
      time.presets.forEach((label, i) => {
        const opt = document.createElement('option');
        opt.value = i; opt.textContent = label;
        select.appendChild(opt);
      });
      select.addEventListener('change', () => { pendingTimePreset = parseInt(select.value, 10); });
      panelTime.appendChild(select);
      pendingTimePreset = 0;
    }
  }

  // ── Extra sensor-specific parameters ──
  function buildExtraParams() {
    panelExtras.innerHTML = '';
    const extras = sensorInfo.extraParams || [];
    extras.forEach((ep, idx) => {
      const row = document.createElement('div');
      row.className = 'form-group';
      if (ep.type === 'select') {
        row.innerHTML = '<label>' + ep.label + '</label>';
        const select = document.createElement('select');
        ep.options.forEach((label, i) => {
          const opt = document.createElement('option');
          opt.value = i; opt.textContent = label;
          select.appendChild(opt);
        });
        select.addEventListener('change', () => {
          sendCommand({ cmd: 'set_extra', idx, value: parseInt(select.value, 10) });
        });
        row.appendChild(select);
      } else {
        row.innerHTML =
          '<label>' + ep.label + ' (' + ep.min + '\u2013' + ep.max + ')</label>' +
          '<input type="number" min="' + ep.min + '" max="' + ep.max + '" value="' + ep.min + '">';
        row.querySelector('input').addEventListener('change', (e) => {
          sendCommand({ cmd: 'set_extra', idx, value: parseInt(e.target.value, 10) || 0 });
        });
      }
      panelExtras.appendChild(row);
    });
  }

  // ── Apply Configuration ──
  btnApplyConfig.addEventListener('click', async () => {
    if (selectedChannels.size === 0) {
      showToast('Select at least one channel', 'error');
      return;
    }

    if (sensorInfo.gain.type === 'discrete') {
      await sendCommand({ cmd: 'set_gain', idx: pendingGainIdx });
    } else {
      await sendCommand({ cmd: 'set_gain', value: pendingGainValue });
    }

    if (sensorInfo.time.type === 'formula') {
      await sendCommand({ cmd: 'set_time', params: pendingTimeParams });
    } else {
      await sendCommand({ cmd: 'set_time', preset: pendingTimePreset });
    }

    await sendCommand({ cmd: 'set_channels', channels: Array.from(selectedChannels) });
    showToast('Configuration applied', 'success');
  });

  // =============================================================================
  // MEASUREMENT
  // =============================================================================
  btnMeasure.addEventListener('click', async () => {
    if (selectedChannels.size === 0) {
      showToast('Select at least one channel', 'error');
      return;
    }
    const needsRef = (currentMode !== 'fluorescence') && currentSubmode === 'log';
    if (needsRef && !referenceValid) {
      showToast('Measure the reference first (\u2212log\u2081\u2080 I/I\u2080 mode)', 'error');
      return;
    }
    setMeasuringUI(true, 'Measuring sample...');
    await sendCommand({ cmd: 'measure' });
  });

  function setMeasuringUI(active, label) {
    isMeasuring = active;
    btnMeasure.disabled = active;
    btnRef.disabled = active;
    btnApplyConfig.disabled = active;
    progressArea.classList.toggle('hidden', !active);
    if (active) {
      progressText.textContent = label;
      progressBar.style.width = '0%';
      progressLive.innerHTML = '';
    }
  }

  function onProgress(msg) {
    const pct = Math.min((msg.t / 10.0) * 100, 100);
    progressBar.style.width = pct + '%';
    progressLive.innerHTML = '';
    Object.keys(msg.data || {}).forEach(chId => {
      const ch = sensorInfo.channels.find(c => String(c.id) === chId);
      const span = document.createElement('span');
      span.textContent = (ch ? ch.name : chId) + ': ' + msg.data[chId].toFixed(3);
      progressLive.appendChild(span);
    });
  }

  function onRefSaved() {
    referenceValid = true;
    refStatus.classList.remove('hidden');
    setMeasuringUI(false);
    showToast('Reference saved successfully', 'success');
  }

  function onResult(msg) {
    setMeasuringUI(false);
    results.push({
      id: resultIdCounter++,
      mode: msg.mode, sub: msg.sub,
      gain: currentGainLabel(),
      tint_ms: msg.tint_ms,
      data: msg.data
    });
    renderResults();
    showToast('Measurement complete', 'success');
  }

  function currentGainLabel() {
    if (!sensorInfo) return '';
    if (sensorInfo.gain.type === 'discrete') return sensorInfo.gain.options[pendingGainIdx] || '';
    return String(pendingGainValue);
  }

  // =============================================================================
  // RESULTS TABLE / EXPORT
  // =============================================================================
  function modeLabel(mode, sub) {
    const names = { reflectance: 'Reflectance', absorbance: 'Absorbance', fluorescence: 'Fluorescence' };
    let label = names[mode] || mode;
    if (sub === 'raw') label += ' (Raw)';
    return label;
  }

  function renderResults() {
    resultsBody.innerHTML = '';
    if (results.length === 0) {
      resultsBody.innerHTML = '<tr><td colspan="99" class="empty-row">No measurements yet</td></tr>';
      return;
    }
    results.forEach(r => {
      const tr = document.createElement('tr');
      let html =
        '<td>' + r.id + '</td>' +
        '<td>' + modeLabel(r.mode, r.sub) + '</td>' +
        '<td>' + (r.gain || '—') + '</td>' +
        '<td>' + (r.tint_ms !== undefined ? r.tint_ms.toFixed(2) : '—') + '</td>';
      sensorInfo.channels.forEach(ch => {
        const v = r.data[String(ch.id)];
        html += '<td>' + (v !== undefined ? v.toFixed(3) : '—') + '</td>';
      });
      html += '<td><button class="btn-del-row" data-id="' + r.id + '">Delete</button></td>';
      tr.innerHTML = html;
      resultsBody.appendChild(tr);
    });
    resultsBody.querySelectorAll('.btn-del-row').forEach(btn => {
      btn.addEventListener('click', () => {
        results = results.filter(r => r.id !== parseInt(btn.dataset.id, 10));
        renderResults();
      });
    });
  }

  btnClearAll.addEventListener('click', () => {
    if (results.length === 0) return;
    results = [];
    renderResults();
  });

  function buildResultRows() {
    const chCols = sensorInfo.channels.map(c => c.name);
    const header = ['id', 'mode', 'gain', 'tint_ms', ...chCols];
    const rows = [header];
    results.forEach(r => {
      rows.push([
        r.id, modeLabel(r.mode, r.sub), r.gain || '',
        r.tint_ms !== undefined ? Number(r.tint_ms.toFixed(2)) : '',
        ...sensorInfo.channels.map(ch => {
          const v = r.data[String(ch.id)];
          return v !== undefined ? Number(v.toFixed(4)) : '';
        })
      ]);
    });
    return rows;
  }

  btnDownloadCsv.addEventListener('click', () => {
    if (results.length === 0) { showToast('No measurements to export', 'error'); return; }
    const lines = buildResultRows().map(row => row.join(','));
    const blob = new Blob([lines.join('\n')], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url; a.download = 'measurements_' + sensorInfo.sensor + '_' + Date.now() + '.csv';
    a.click();
    URL.revokeObjectURL(url);
  });

  btnDownloadXlsx.addEventListener('click', () => {
    if (results.length === 0) { showToast('No measurements to export', 'error'); return; }
    if (typeof XLSX === 'undefined') { showToast('Excel library failed to load', 'error'); return; }
    const worksheet = XLSX.utils.aoa_to_sheet(buildResultRows());
    const workbook = XLSX.utils.book_new();
    XLSX.utils.book_append_sheet(workbook, worksheet, 'Measurements');
    XLSX.writeFile(workbook, 'measurements_' + sensorInfo.sensor + '_' + Date.now() + '.xlsx');
  });

  // =============================================================================
  // TOASTS
  // =============================================================================
  function showToast(message, type) {
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = 'toast ' + (type || '');
    toast.textContent = message;
    container.appendChild(toast);
    void toast.offsetWidth;
    toast.classList.add('visible');
    setTimeout(() => {
      toast.classList.remove('visible');
      setTimeout(() => toast.remove(), 300);
    }, 2800);
  }

})();
