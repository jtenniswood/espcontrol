// ── Screen Rotation State ──────────────────────────────────────────────
// @web-module-requires: state

var SCREEN_ROTATION_STARTUP_FALLBACK_MS = 1200;

function normalizeScreenRotation(value) {
  value = String(value == null ? "" : value);
  return allScreenRotationOptions().indexOf(value) !== -1 ? value : "0";
}

function activeScreenRotationOptions() {
  return sortScreenRotationOptions(uniqueOptions(state.screenRotationOptions || []));
}

function allScreenRotationOptions() {
  return uniqueOptions(
    (state.screenRotationOptions || [])
      .concat(state.screenRotationDeviceOptions || [])
  );
}

function syncScreenRotationSelect() {
  if (!els.setScreenRotation) return;
  els.setScreenRotation.innerHTML = "";
  activeScreenRotationOptions().forEach(function (opt) {
    appendScreenRotationOption(els.setScreenRotation, opt);
  });
  els.setScreenRotation.value = state.screenRotation;
}

function displayScreenRotation(value) {
  var labels = CFG.features && CFG.features.screenRotationDisplayLabels;
  value = String(value == null ? "" : value);
  if (labels && Object.prototype.hasOwnProperty.call(labels, value)) return labels[value];
  var offset = (CFG.features && parseInt(CFG.features.screenRotationDisplayOffset, 10)) || 0;
  var n = parseInt(value, 10);
  if (!isFinite(n)) return value;
  return String((n + offset + 360) % 360);
}

function screenRotationSortValue(value) {
  var displayed = parseInt(displayScreenRotation(value), 10);
  if (isFinite(displayed)) return (displayed + 360) % 360;
  var raw = parseInt(value, 10);
  return isFinite(raw) ? (raw + 360) % 360 : 999;
}

function sortScreenRotationOptions(options) {
  return (options || []).slice().sort(function (a, b) {
    return screenRotationSortValue(a) - screenRotationSortValue(b);
  });
}

function appendScreenRotationOption(select, opt) {
  var o = document.createElement("option");
  o.value = opt;
  o.textContent = displayScreenRotation(opt) + " deg";
  select.appendChild(o);
}

function screenRotationStartupRequired() {
  return !!(CFG.features && CFG.features.screenRotation);
}

function gridPreviewBlockedByRotationStartup() {
  return screenRotationStartupRequired() && !state.screenRotationInitialReady;
}

function clearInitialScreenRotationTimer() {
  if (!state.screenRotationInitialTimer) return;
  clearTimeout(state.screenRotationInitialTimer);
  state.screenRotationInitialTimer = null;
}

function startInitialScreenRotationCheck() {
  clearInitialScreenRotationTimer();
  state.pendingButtonOrderRaw = null;
  state.screenRotationInitialReady = !screenRotationStartupRequired();
  if (!state.screenRotationInitialReady) {
    state.screenRotationInitialTimer = setTimeout(resolveInitialScreenRotationCheck, SCREEN_ROTATION_STARTUP_FALLBACK_MS);
  }
}

function resolveInitialScreenRotationCheck() {
  if (state.screenRotationInitialReady) return;
  clearInitialScreenRotationTimer();
  state.screenRotationInitialReady = true;
  if (state.pendingButtonOrderRaw !== null) {
    applyButtonOrderValue(state.pendingButtonOrderRaw, true);
    state.pendingButtonOrderRaw = null;
  }
  if (els.previewMain) renderPreview();
}
