// ── State ──────────────────────────────────────────────────────────────

var NTP_SERVER_DEFAULTS = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"];
var MONTH_NAME_DEFAULTS = [
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"
];

var state = {
  grid: [],
  sizes: {},
  buttons: [],
  onColor: "FF8C00",
  offColor: "313131",
  sensorColor: "212121",
  selectedSlots: [],
  lastClickedSlot: -1,
  activeTab: "screen",
  _indoorOn: false,
  _outdoorOn: false,
  _indoorVal: null,
  _outdoorVal: null,
  indoorEntity: "",
  outdoorEntity: "",
  temperatureUnit: "Auto",
  clockBarOn: false,
  networkStatusOn: true,
  networkTransport: "wifi",
  wifiStrengthPercent: 100,
  temperatureDegreeSymbolOn: true,
  presenceEntity: "",
  mediaPlayerSleepPreventionOn: false,
  mediaPlayerSleepPreventionEntity: "",
  screensaverMode: "disabled",
  _screensaverModeReceived: false,
  screensaverAction: "off",
  _screensaverActionReceived: false,
  clockScreensaverOn: false,
  clockBrightnessDay: 35,
  clockBrightnessNight: 35,
  clockBrightnessSplitReceived: false,
  screensaverDimmedBrightness: 10,
  screensaverTimeout: 300,
  screensaverTimeoutMin: 60,
  screensaverTimeoutMax: 3600,
  screensaverTimeoutLimitsLoaded: false,
  homeScreenTimeout: 60,
  brightnessDayVal: 100,
  brightnessNightVal: 75,
  scheduleEnabled: false,
  scheduleOnHour: 6,
  scheduleOffHour: 23,
  scheduleMode: "screen_off",
  scheduleWakeTimeout: 60,
  scheduleWakeBrightness: 10,
  scheduleDimmedBrightness: 10,
  scheduleClockBrightness: 10,
  timezone: "UTC (GMT+0)",
  timezoneOptions: [],
  clockFormat: "24h",
  clockFormatOptions: ["12h", "24h"],
  customNtpServers: false,
  ntpServer1: NTP_SERVER_DEFAULTS[0],
  ntpServer2: NTP_SERVER_DEFAULTS[1],
  ntpServer3: NTP_SERVER_DEFAULTS[2],
  customMonthNames: false,
  monthNames: MONTH_NAME_DEFAULTS.slice(),
  screenRotation: "0",
  screenRotationOptions: (CFG.features && CFG.features.screenRotationOptions) || ["0", "90", "180", "270"],
  screenRotationDeviceOptions: null,
  sunrise: "",
  sunset: "",
  firmwareVersion: "",
  firmwareLatestVersion: "",
  firmwareUpdateState: "",
  firmwareReleaseUrl: "",
  firmwareChecking: false,
  firmwareVersionRefreshPending: false,
  firmwareInstallTargetVersion: "",
  firmwareInstallPostPending: false,
  firmwareInstallStatus: "",
  firmwareUpdateControlsSupported: false,
  firmwareInstallControlsSupported: false,
  firmwareOtaUrl: "",
  firmwareOtaFilename: "",
  firmwareOtaMd5: "",
  autoUpdate: true,
  updateFrequency: "Daily",
  updateFreqOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
  developerExperimentalFeatures: false,
  configLocked: false,
  configLockReason: "",
  subpages: {},
  subpageRaw: {},
  subpageSavePending: {},
  editingSubpage: null,
  subpageSelectedSlots: [],
  subpageLastClicked: -1,
  clipboard: null,
  settingsDraft: null,
  entityPostPaths: {},
  entityNames: {},
};

for (var i = 0; i < NUM_SLOTS; i++) {
  state.grid.push(0);
  state.buttons.push({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" });
}

function getActiveScreensaverMode() {
  if (state.screensaverMode === "sensor") return "sensor";
  if (state.screensaverMode === "timer") return "timer";
  return "disabled";
}

function normalizeScreenRotation(value) {
  value = String(value == null ? "" : value);
  return allScreenRotationOptions().indexOf(value) !== -1 ? value : "0";
}

function uniqueOptions(options) {
  var out = [];
  (options || []).forEach(function (opt) {
    opt = String(opt);
    if (out.indexOf(opt) < 0) out.push(opt);
  });
  return out;
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

function normalizeTemperatureUnit(value) {
  var unit = String(value == null ? "" : value).trim().toLowerCase();
  if (unit === "f" || unit === "\u00B0f" || unit === "fahrenheit") return "\u00B0F";
  if (unit === "c" || unit === "\u00B0c" || unit === "celsius" || unit === "centigrade") return "\u00B0C";
  return "Auto";
}

function timezonePrefersFahrenheit(timezone) {
  var tz = getTzId(timezone || state.timezone);
  var fahrenheitZones = {
    "America/Adak": true,
    "America/Anchorage": true,
    "America/Boise": true,
    "America/Chicago": true,
    "America/Denver": true,
    "America/Detroit": true,
    "America/Juneau": true,
    "America/Los_Angeles": true,
    "America/New_York": true,
    "America/Phoenix": true,
    "America/Puerto_Rico": true,
    "Pacific/Guam": true,
    "Pacific/Honolulu": true,
    "Pacific/Pago_Pago": true,
  };
  return !!fahrenheitZones[tz];
}

function temperatureUnitSymbol() {
  var unit = normalizeTemperatureUnit(state.temperatureUnit);
  if (unit === "\u00B0F") return "\u00B0F";
  if (unit === "\u00B0C") return "\u00B0C";
  return timezonePrefersFahrenheit(state.timezone) ? "\u00B0F" : "\u00B0C";
}

function clockBarTemperatureUnitSymbol() {
  return state.temperatureDegreeSymbolOn ? "\u00B0" : "";
}

function appendScreenRotationOption(select, opt) {
  var o = document.createElement("option");
  o.value = opt;
  o.textContent = displayScreenRotation(opt) + " deg";
  select.appendChild(o);
}

function normalizeHour(value, fallback) {
  var n = parseInt(value, 10);
  if (!isFinite(n)) return fallback;
  if (n < 0) return 0;
  if (n > 23) return 23;
  return n;
}

function normalizeScheduleWakeTimeout(value) {
  var n = parseFloat(value);
  if (!isFinite(n) || n <= 0) return 60;
  if (n < 10) return 10;
  if (n > 3600) return 3600;
  return Math.round(n);
}

function normalizeScheduleWakeBrightness(value) {
  var n = parseFloat(value);
  if (!isFinite(n) || n <= 0) return 10;
  if (n < 10) return 10;
  if (n > 100) return 100;
  return Math.round(n);
}

function normalizeScheduleClockBrightness(value) {
  var n = parseFloat(value);
  if (!isFinite(n) || n <= 0) return 10;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

function normalizeScheduleDimmedBrightness(value) {
  var n = parseFloat(value);
  if (!isFinite(n) || n <= 0) return 10;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

function normalizeScheduleMode(value) {
  var v = String(value || "").toLowerCase().replace(/[\s-]+/g, "_");
  if (v === "screen_dimmed" || v === "dimmed" || v === "always_on" || v === "always") {
    return "screen_dimmed";
  }
  if (v === "clock") return "clock";
  return "screen_off";
}

function normalizeScreensaverAction(value) {
  var v = String(value || "").toLowerCase().replace(/[\s-]+/g, "_");
  if (v === "screen_dimmed" || v === "dimmed" || v === "dim") return "dim";
  if (v === "clock") return "clock";
  return "off";
}

function screensaverActionOption(value) {
  var action = normalizeScreensaverAction(value);
  if (action === "dim") return "Screen Dimmed";
  if (action === "clock") return "Clock";
  return "Display Off";
}

function scheduleModeOption(value) {
  var mode = normalizeScheduleMode(value);
  if (mode === "screen_dimmed") return "Screen Dimmed";
  if (mode === "clock") return "Clock";
  return "Screen off";
}

function normalizeClockBrightness(value, fallback) {
  var n = parseFloat(value);
  if (!isFinite(n) || n <= 0) return fallback;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

function normalizeScreensaverDimmedBrightness(value) {
  var n = parseFloat(value);
  if (!isFinite(n) || n <= 0) return 10;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

function normalizeNtpServer(value, fallback) {
  var v = String(value == null ? "" : value).trim();
  return v || fallback;
}

function normalizeMonthNames(value) {
  var parts = Array.isArray(value)
    ? value.slice()
    : String(value == null ? "" : value).split(",");
  var out = [];
  for (var i = 0; i < 12; i++) {
    var text = String(parts[i] == null ? "" : parts[i]).trim();
    out.push(text || MONTH_NAME_DEFAULTS[i]);
  }
  return out;
}

function serializeMonthNames(value) {
  return normalizeMonthNames(value).join(",");
}

function hasCustomMonthNames() {
  var names = normalizeMonthNames(state.monthNames);
  for (var i = 0; i < 12; i++) {
    if (names[i] !== MONTH_NAME_DEFAULTS[i]) return true;
  }
  return false;
}

function resetMonthNamesToDefaults() {
  state.monthNames = MONTH_NAME_DEFAULTS.slice();
}

function monthNameForIndex(index) {
  var names = normalizeMonthNames(state.monthNames);
  return names[index] || MONTH_NAME_DEFAULTS[index] || "";
}

function hasCustomNtpServers() {
  return normalizeNtpServer(state.ntpServer1, NTP_SERVER_DEFAULTS[0]) !== NTP_SERVER_DEFAULTS[0] ||
    normalizeNtpServer(state.ntpServer2, NTP_SERVER_DEFAULTS[1]) !== NTP_SERVER_DEFAULTS[1] ||
    normalizeNtpServer(state.ntpServer3, NTP_SERVER_DEFAULTS[2]) !== NTP_SERVER_DEFAULTS[2];
}

function resetNtpServersToDefaults() {
  state.ntpServer1 = NTP_SERVER_DEFAULTS[0];
  state.ntpServer2 = NTP_SERVER_DEFAULTS[1];
  state.ntpServer3 = NTP_SERVER_DEFAULTS[2];
}

function formatDuration(seconds) {
  seconds = normalizeScheduleWakeTimeout(seconds);
  if (seconds < 60) return seconds + " second" + (seconds === 1 ? "" : "s");
  if (seconds % 60 === 0) {
    var minutes = seconds / 60;
    return minutes + " minute" + (minutes === 1 ? "" : "s");
  }
  return seconds + " seconds";
}

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

function setSelectValue(select, value, label) {
  if (!select) return;
  value = String(value);
  var found = false;
  for (var i = 0; i < select.options.length; i++) {
    if (select.options[i].value === value) {
      found = true;
      break;
    }
  }
  if (!found) {
    var o = document.createElement("option");
    o.value = value;
    o.textContent = label || value;
    select.appendChild(o);
  }
  select.value = value;
}

function formatHour(hour) {
  hour = normalizeHour(hour, 0);
  var suffix = hour < 12 ? "AM" : "PM";
  var h = hour % 12;
  if (h === 0) h = 12;
  return h + ":00 " + suffix;
}

function syncScreenScheduleUi() {
  state.scheduleOnHour = normalizeHour(state.scheduleOnHour, 6);
  state.scheduleOffHour = normalizeHour(state.scheduleOffHour, 23);
  state.scheduleMode = normalizeScheduleMode(state.scheduleMode);
  state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(state.scheduleWakeTimeout);
  state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(state.scheduleWakeBrightness);
  state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(state.scheduleDimmedBrightness);
  state.scheduleClockBrightness = normalizeScheduleClockBrightness(state.scheduleClockBrightness);
  if (els.setScheduleToggle) els.setScheduleToggle.checked = !!state.scheduleEnabled;
  if (els.setScheduleOnHour) els.setScheduleOnHour.value = String(state.scheduleOnHour);
  if (els.setScheduleOffHour) els.setScheduleOffHour.value = String(state.scheduleOffHour);
  if (els.setScheduleMode) {
    setSelectValue(els.setScheduleMode, state.scheduleMode, scheduleModeOption(state.scheduleMode));
  }
  setSelectValue(els.setScheduleWakeTimeout, state.scheduleWakeTimeout, formatDuration(state.scheduleWakeTimeout));
  if (els.setScheduleWakeBrightness) {
    els.setScheduleWakeBrightness.value = state.scheduleWakeBrightness;
    els.setScheduleWakeBrightnessVal.textContent = Math.round(state.scheduleWakeBrightness) + "%";
  }
  if (els.setScheduleDimmedBrightness) {
    els.setScheduleDimmedBrightness.value = state.scheduleDimmedBrightness;
    els.setScheduleDimmedBrightnessVal.textContent = Math.round(state.scheduleDimmedBrightness) + "%";
  }
  if (els.setScheduleClockBrightness) {
    els.setScheduleClockBrightness.value = state.scheduleClockBrightness;
    els.setScheduleClockBrightnessVal.textContent = Math.round(state.scheduleClockBrightness) + "%";
  }
  if (els.setScheduleOffOptions) {
    els.setScheduleOffOptions.className =
      "sp-cond-field" + (state.scheduleMode === "screen_off" ? " sp-visible" : "");
  }
  if (els.setScheduleDimmedOptions) {
    els.setScheduleDimmedOptions.className =
      "sp-cond-field" + (state.scheduleMode === "screen_dimmed" ? " sp-visible" : "");
  }
  if (els.setScheduleClockOptions) {
    els.setScheduleClockOptions.className =
      "sp-cond-field" + (state.scheduleMode === "clock" ? " sp-visible" : "");
  }
  if (els.setScheduleTimes) {
    els.setScheduleTimes.className = "sp-schedule-times" + (state.scheduleEnabled ? "" : " sp-hidden");
  }
  if (els.setScheduleBadge) {
    els.setScheduleBadge.className = "sp-card-badge" + (state.scheduleEnabled ? "" : " sp-hidden");
  }
}

function syncTemperatureUi() {
  if (els.setIndoorToggle) els.setIndoorToggle.checked = !!state._indoorOn;
  if (els.setIndoorField) {
    els.setIndoorField.className = "sp-cond-field" + (state._indoorOn ? " sp-visible" : "");
  }
  if (els.setOutdoorToggle) els.setOutdoorToggle.checked = !!state._outdoorOn;
  if (els.setOutdoorField) {
    els.setOutdoorField.className = "sp-cond-field" + (state._outdoorOn ? " sp-visible" : "");
  }
}

function syncNtpServerUi() {
  if (els.setCustomNtpServersToggle) {
    els.setCustomNtpServersToggle.checked = !!state.customNtpServers;
  }
  if (els.setNtpServerFields) {
    els.setNtpServerFields.className =
      "sp-field-stack" + (state.customNtpServers ? "" : " sp-hidden");
  }
  syncInput(els.setNtpServer1, state.ntpServer1);
  syncInput(els.setNtpServer2, state.ntpServer2);
  syncInput(els.setNtpServer3, state.ntpServer3);
}

function syncMonthNameUi() {
  state.monthNames = normalizeMonthNames(state.monthNames);
  state.customMonthNames = !!state.customMonthNames || hasCustomMonthNames();
  if (els.setCustomMonthNamesToggle) {
    els.setCustomMonthNamesToggle.checked = !!state.customMonthNames;
  }
  if (els.setMonthNameFields) {
    els.setMonthNameFields.className =
      "sp-field-stack" + (state.customMonthNames ? "" : " sp-hidden");
  }
  if (els.setMonthNameInputs) {
    for (var i = 0; i < els.setMonthNameInputs.length; i++) {
      syncInput(els.setMonthNameInputs[i], state.monthNames[i]);
    }
  }
}

function syncClockBarUi() {
  syncPreviewGridTop();
  if (els.topbar) els.topbar.className = "sp-topbar" + (state.clockBarOn ? "" : " sp-hidden");
  if (els.setClockBarToggle) els.setClockBarToggle.checked = !!state.clockBarOn;
  if (els.setNetworkStatusToggle) {
    els.setNetworkStatusToggle.checked = !!state.networkStatusOn;
  }
  if (els.setClockBarBadge) {
    els.setClockBarBadge.className = "sp-card-badge" + (state.clockBarOn ? "" : " sp-hidden");
  }
  if (els.setTemperatureDegreeSymbolToggle) {
    els.setTemperatureDegreeSymbolToggle.checked = !!state.temperatureDegreeSymbolOn;
  }
  updateNetworkPreview();
  updateTempPreview();
}

function syncIdleUi() {
  state.homeScreenTimeout = parseFloat(state.homeScreenTimeout) || 0;
  if (els.setHSTimeout) els.setHSTimeout.value = String(state.homeScreenTimeout);
  if (els.setIdleBadge) {
    els.setIdleBadge.className = "sp-card-badge" +
      (state.homeScreenTimeout > 0 ? "" : " sp-hidden");
  }
}

var els = {};
var dragSrcPos = -1;
var didDrag = false;
var previewPlaceholder = null;
var previewDropIdx = -1;
var dragRafPending = CFG.dragAnimation ? false : null;
var dragSrcEl = CFG.dragAnimation ? null : null;
var dragIsSubpage = false;
var dragEnterCount = 0;
var orderReceived = false;
var migrationTimer = null;
var sliderMigrationTimer = null;
var pendingSliderSubpageMigrations = {};
var _eventSource = null;
var firmwareInstallRefreshTimer = null;
var firmwareInstallRefreshUntil = 0;
var firmwareWebOtaFallbackTimer = null;
var FIRMWARE_VERSION_METADATA_PATH = "/espcontrol/version";
var FIRMWARE_PUBLIC_MANIFEST_BASE = "https://jtenniswood.github.io/espcontrol/firmware/";
var FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS = 12000;
var FIRMWARE_CHECKING_VERSION_LABEL = "Checking version...";
var FIRMWARE_DEV_VERSION_LABEL = "Dev build";
var FIRMWARE_UNKNOWN_VERSION_LABEL = "Version unknown";

// ── Utilities ──────────────────────────────────────────────────────────

function escHtml(s) {
  return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;")
    .replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function renderFirmwareVersion() {
  if (!els.fwVersionLabel) return;
  els.fwVersionLabel.innerHTML = '<span class="sp-fw-label">Installed </span>' +
    escHtml(firmwareVersionLabel());
}

function isSpecificFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  return /^v[0-9]+(\.[0-9]+){2}([-+][0-9A-Za-z.-]+)?$/i.test(version);
}

function setFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return;
  if (isSpecificFirmwareVersion(state.firmwareVersion) && !isSpecificFirmwareVersion(version)) return;
  state.firmwareVersion = displayFirmwareVersion(version);
  renderFirmwareVersion();
  renderFirmwareUpdateStatus();
  stopFirmwareInstallRefreshIfComplete();
}

function displayFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return FIRMWARE_UNKNOWN_VERSION_LABEL;
  if (version === FIRMWARE_UNKNOWN_VERSION_LABEL) return FIRMWARE_UNKNOWN_VERSION_LABEL;
  return isSpecificFirmwareVersion(version) ? version : FIRMWARE_DEV_VERSION_LABEL;
}

function firmwareVersionFromMetadata(data) {
  if (!data) return "";
  return String(data.firmware_version || data.project_version || data.version || data.current_version || "").trim();
}

function firmwareVersionLabel() {
  if (!state.firmwareVersion && state.firmwareVersionRefreshPending) {
    return FIRMWARE_CHECKING_VERSION_LABEL;
  }
  return displayFirmwareVersion(state.firmwareVersion);
}

function firmwareUpdateAvailable() {
  return state.firmwareUpdateState === "UPDATE AVAILABLE" &&
    isSpecificFirmwareVersion(state.firmwareLatestVersion);
}

function publicFirmwareInstallAvailable() {
  return publicFirmwareReleaseKnown() && !installedFirmwareMatchesPublicRelease();
}

function firmwareInstallAvailable() {
  return state.firmwareInstallControlsSupported === true &&
    (firmwareUpdateAvailable() || publicFirmwareInstallAvailable());
}

function firmwareVersionsSame(a, b) {
  return String(a == null ? "" : a).trim().toLowerCase() ===
    String(b == null ? "" : b).trim().toLowerCase();
}

function publicFirmwareManifestUrl() {
  return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/manifest.json";
}

function publicFirmwareAssetUrl(assetPath) {
  assetPath = String(assetPath || "").trim();
  if (!assetPath) return "";
  try {
    return new URL(assetPath, publicFirmwareManifestUrl()).href;
  } catch (err) {
    if (/^https?:\/\//i.test(assetPath)) return assetPath;
    return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/" + assetPath.replace(/^\/+/, "");
  }
}

function firmwareInfoFromPublicManifest(data) {
  if (!data || typeof data !== "object") return null;
  var version = String(data.version || "").trim();
  if (!isSpecificFirmwareVersion(version)) return null;
  var builds = Array.isArray(data.builds) ? data.builds : [];
  var expectedOta = DEVICE_ID + ".ota.bin";
  for (var i = 0; i < builds.length; i++) {
    var build = builds[i] || {};
    var ota = build.ota || {};
    var otaPath = String(ota.path || "").trim();
    if (otaPath === expectedOta) {
      return {
        latest_version: version,
        release_url: String(ota.release_url || "").trim(),
        ota_url: publicFirmwareAssetUrl(otaPath),
        ota_filename: expectedOta,
        ota_md5: String(ota.md5 || "").trim(),
      };
    }
  }
  return null;
}

function setPublicFirmwareInfo(info) {
  if (!info) return false;
  var latest = String(info.latest_version || "").trim();
  if (!isSpecificFirmwareVersion(latest)) return false;
  state.firmwareLatestVersion = latest;
  if (info.release_url) state.firmwareReleaseUrl = String(info.release_url).trim();
  if (info.ota_url) state.firmwareOtaUrl = String(info.ota_url).trim();
  if (info.ota_filename) state.firmwareOtaFilename = String(info.ota_filename).trim();
  if (info.ota_md5) state.firmwareOtaMd5 = String(info.ota_md5).trim();
  if (state.firmwareUpdateState === "NO UPDATE" &&
      !isSpecificFirmwareVersion(state.firmwareVersion)) {
    setFirmwareVersion(latest);
  }
  renderFirmwareUpdateStatus();
  return true;
}

function publicFirmwareReleaseKnown() {
  return isSpecificFirmwareVersion(state.firmwareLatestVersion);
}

function installedFirmwareMatchesPublicRelease() {
  return publicFirmwareReleaseKnown() &&
    isSpecificFirmwareVersion(state.firmwareVersion) &&
    firmwareVersionsSame(state.firmwareVersion, state.firmwareLatestVersion);
}

function publicFirmwareStatusHtml() {
  var status = "Latest public version: " + escHtml(state.firmwareLatestVersion);
  if (state.firmwareReleaseUrl) {
    status += ' <a href="' + escAttr(state.firmwareReleaseUrl) + '" target="_blank" rel="noopener">release notes</a>';
  }
  return status;
}

function firmwareUpdateControlsVisible() {
  var transport = String(state.networkTransport == null ? "" : state.networkTransport).trim().toLowerCase();
  return state.firmwareUpdateControlsSupported === true && transport !== "ethernet";
}

function syncFirmwareUpdateUi() {
  var show = firmwareUpdateControlsVisible();
  if (els.fwActions) els.fwActions.style.display = show ? "" : "none";
  if (els.fwStatus) els.fwStatus.style.display = show ? "" : "none";
  if (els.setAutoUpdateRow) els.setAutoUpdateRow.style.display = show ? "" : "none";
  if (els.updateFreqWrap) {
    els.updateFreqWrap.style.display = show && state.autoUpdate ? "" : "none";
  }
}

function renderFirmwareUpdateStatus() {
  if (!els.fwStatus) return;
  var cls = "sp-fw-status";
  var status = "";
  var inlineStatus = "";
  if (state.firmwareUpdateState === "INSTALLING") {
    status = state.firmwareInstallStatus || "Installing update\u2026";
    cls += " sp-update-installing";
  } else if (firmwareUpdateAvailable() || publicFirmwareInstallAvailable()) {
    status = publicFirmwareStatusHtml();
    cls += " sp-update-available";
  } else if (state.firmwareUpdateState === "NO UPDATE") {
    if (publicFirmwareReleaseKnown() &&
        isSpecificFirmwareVersion(state.firmwareVersion) &&
        !installedFirmwareMatchesPublicRelease()) {
      status = publicFirmwareStatusHtml();
    } else {
      inlineStatus = "Up to date";
    }
  } else if (publicFirmwareReleaseKnown()) {
    if (installedFirmwareMatchesPublicRelease()) {
      inlineStatus = "Up to date";
    } else {
      status = publicFirmwareStatusHtml();
    }
  } else if (state.firmwareChecking) {
    status = "Checking public firmware\u2026";
  }
  els.fwStatus.className = cls;
  els.fwStatus.innerHTML = status;
  if (els.fwInlineStatus) {
    els.fwInlineStatus.className = "sp-fw-inline-status" + (inlineStatus ? " sp-visible" : "");
    els.fwInlineStatus.textContent = inlineStatus;
  }
  if (els.fwCheckBtn) {
    var isBusy = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
    els.fwCheckBtn.className = "sp-fw-btn" + (isBusy ? " sp-fw-btn-busy" : "");
    if (state.firmwareUpdateState === "INSTALLING") {
      els.fwCheckBtn.disabled = true;
      els.fwCheckBtn.textContent = "Installing\u2026";
    } else if (firmwareInstallAvailable()) {
      els.fwCheckBtn.disabled = false;
      els.fwCheckBtn.textContent = "Install Update";
    } else {
      els.fwCheckBtn.disabled = state.firmwareChecking;
      els.fwCheckBtn.textContent = state.firmwareChecking ? "Checking\u2026" : "Check for Update";
    }
  }
  syncFirmwareUpdateUi();
}

function setFirmwareUpdateInfo(d) {
  state.firmwareUpdateControlsSupported = true;
  state.firmwareInstallControlsSupported = true;
  var latest = d.latest_version || d.value || "";
  var updateState = String(d.state || state.firmwareUpdateState || "").trim().toUpperCase();
  if (d.current_version) setFirmwareVersion(d.current_version);
  if (latest) state.firmwareLatestVersion = String(latest).trim();
  var installWindowActive = !!state.firmwareInstallTargetVersion &&
    Date.now() < firmwareInstallRefreshUntil;
  if (state.firmwareInstallPostPending) {
    if (installWindowActive && updateState === "UPDATE AVAILABLE") {
      state.firmwareInstallPostPending = false;
      clearFirmwareWebOtaFallback();
      state.firmwareInstallStatus = "Installing update\u2026";
      postFirmwareUpdateInstall();
      updateState = "INSTALLING";
    } else if (!installWindowActive || (updateState === "NO UPDATE" && !publicFirmwareInstallAvailable())) {
      state.firmwareInstallPostPending = false;
    }
  }
  if (installWindowActive && updateState === "UPDATE AVAILABLE") {
    updateState = "INSTALLING";
  }
  state.firmwareUpdateState = updateState;
  state.firmwareReleaseUrl = d.release_url || state.firmwareReleaseUrl || "";
  if (state.firmwareUpdateState === "NO UPDATE" &&
      !isSpecificFirmwareVersion(state.firmwareVersion) &&
      isSpecificFirmwareVersion(state.firmwareLatestVersion)) {
    setFirmwareVersion(state.firmwareLatestVersion);
  }
  if (state.firmwareUpdateState) state.firmwareChecking = false;
  if (state.firmwareUpdateState === "INSTALLING") {
    startFirmwareInstallRefresh();
  } else {
    stopFirmwareInstallRefreshIfComplete();
  }
  renderFirmwareUpdateStatus();
}

function firmwareVersionMatches(version, expected) {
  return String(version == null ? "" : version).trim() ===
    String(expected == null ? "" : expected).trim();
}

function stopFirmwareInstallRefresh() {
  if (firmwareInstallRefreshTimer) clearTimeout(firmwareInstallRefreshTimer);
  firmwareInstallRefreshTimer = null;
  firmwareInstallRefreshUntil = 0;
  clearFirmwareWebOtaFallback();
  state.firmwareInstallTargetVersion = "";
  state.firmwareInstallPostPending = false;
  state.firmwareInstallStatus = "";
}

function stopFirmwareInstallRefreshIfComplete() {
  var target = state.firmwareInstallTargetVersion;
  if (!target || state.firmwareUpdateState !== "NO UPDATE") return false;
  if (isSpecificFirmwareVersion(target) && !firmwareVersionMatches(state.firmwareVersion, target)) {
    setFirmwareVersion(target);
  }
  stopFirmwareInstallRefresh();
  return true;
}

function pollFirmwareInstallRefresh() {
  firmwareInstallRefreshTimer = null;
  refreshFirmwareVersion();
  if (stopFirmwareInstallRefreshIfComplete()) return;
  if (Date.now() >= firmwareInstallRefreshUntil) {
    stopFirmwareInstallRefresh();
    return;
  }
  firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
}

function startFirmwareInstallRefresh() {
  if (!state.firmwareInstallTargetVersion && isSpecificFirmwareVersion(state.firmwareLatestVersion)) {
    state.firmwareInstallTargetVersion = state.firmwareLatestVersion;
  }
  firmwareInstallRefreshUntil = Date.now() + 180000;
  if (firmwareInstallRefreshTimer) clearTimeout(firmwareInstallRefreshTimer);
  firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
}

function clearFirmwareWebOtaFallback() {
  if (firmwareWebOtaFallbackTimer) clearTimeout(firmwareWebOtaFallbackTimer);
  firmwareWebOtaFallbackTimer = null;
}

function scheduleFirmwareWebOtaFallback() {
  clearFirmwareWebOtaFallback();
  firmwareWebOtaFallbackTimer = setTimeout(function () {
    firmwareWebOtaFallbackTimer = null;
    if (!state.firmwareInstallPostPending) return;
    if (firmwareUpdateAvailable()) return;
    if (!publicFirmwareInstallAvailable()) return;
    installPublicFirmwareViaWebOta();
  }, FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS);
}

function isFirmwareVersionEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "text_sensor/firmware: version" ||
    (domain === "text_sensor" && name === "firmware: version") ||
    (id.indexOf("text_sensor-") === 0 && id.indexOf("firmware") !== -1 && id.indexOf("version") !== -1);
}

function isFirmwareUpdateEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "update/firmware: update" ||
    (domain === "update" && name === "firmware: update") ||
    (id.indexOf("update-") === 0 && id.indexOf("firmware") !== -1 && id.indexOf("update") !== -1);
}

function isFirmwareCheckButtonEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "button/firmware: check for update" ||
    (domain === "button" && name === "firmware: check for update") ||
    (id.indexOf("button-") === 0 && id.indexOf("firmware") !== -1 &&
      id.indexOf("check") !== -1 && id.indexOf("update") !== -1);
}

function isFirmwareInstallButtonEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "button/firmware: install update" ||
    (domain === "button" && name === "firmware: install update") ||
    (id.indexOf("button-") === 0 && id.indexOf("firmware") !== -1 &&
      id.indexOf("install") !== -1 && id.indexOf("update") !== -1);
}

function escAttr(s) {
  return s.replace(/&/g, "&amp;").replace(/"/g, "&quot;")
    .replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function isSettingsFocused() {
  var ae = document.activeElement;
  return ae && els.buttonSettings && els.buttonSettings.contains(ae);
}

function isSettingsOpen() {
  return !!(els.settingsOverlay && els.settingsOverlay.classList.contains("sp-visible"));
}
