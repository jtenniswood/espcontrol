// Screensaver timeout options and UI syncing.
// @web-module-requires: state, screen_schedule_state

var SCREENSAVER_TIMEOUT_OPTIONS = [
  { label: "10 seconds", value: 10 },
  { label: "30 seconds", value: 30 },
  { label: "1 minute", value: 60 },
  { label: "5 minutes", value: 300 },
  { label: "10 minutes", value: 600 },
  { label: "15 minutes", value: 900 },
  { label: "20 minutes", value: 1200 },
  { label: "30 minutes", value: 1800 },
  { label: "45 minutes", value: 2700 },
  { label: "1 hour", value: 3600 },
];

function readNumberMeta(d, keys, fallback) {
  for (var i = 0; i < keys.length; i++) {
    if (d[keys[i]] == null) continue;
    var n = parseFloat(d[keys[i]]);
    if (isFinite(n)) return n;
  }
  return fallback;
}

function syncScreensaverTimeoutLimits(d) {
  state.screensaverTimeoutMin = readNumberMeta(d, ["min", "min_value"], state.screensaverTimeoutMin);
  state.screensaverTimeoutMax = readNumberMeta(d, ["max", "max_value"], state.screensaverTimeoutMax);
  state.screensaverTimeoutLimitsLoaded = true;
}

function screensaverTimeoutSupported(value) {
  var n = parseFloat(value);
  if (!isFinite(n)) return false;
  if (!state.screensaverTimeoutLimitsLoaded) {
    return n > 0 && n <= state.screensaverTimeoutMax;
  }
  return n >= state.screensaverTimeoutMin && n <= state.screensaverTimeoutMax;
}

function syncScreensaverTimeoutUi() {
  var select = els.setSSTimeout;
  if (!select) return;
  var current = String(state.screensaverTimeout);
  select.innerHTML = "";
  SCREENSAVER_TIMEOUT_OPTIONS.forEach(function (opt) {
    if (!screensaverTimeoutSupported(opt.value)) return;
    var o = document.createElement("option");
    o.value = opt.value;
    o.textContent = opt.label;
    select.appendChild(o);
  });
  if (screensaverTimeoutSupported(state.screensaverTimeout)) {
    setSelectValue(select, state.screensaverTimeout, formatDuration(state.screensaverTimeout));
    select.value = current;
  }
}

function applyScreensaverTimeoutState(d) {
  if (!d) return;
  syncScreensaverTimeoutLimits(d);
  var n = parseFloat(d.value != null ? d.value : d.state);
  if (!isFinite(n)) return;
  state.screensaverTimeout = n;
  syncScreensaverTimeoutUi();
}
