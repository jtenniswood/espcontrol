// ── State ──────────────────────────────────────────────────────────────
// @web-module-requires: state_defaults, firmware_metadata

function voiceServicesSupported() {
  return !!(CFG.features && CFG.features.voiceServices);
}

function isHomeAssistantAutoTimezone(value) {
  return String(value || "") === AUTO_TIMEZONE_OPTION;
}

function effectiveTimezoneOptionForWeb(value) {
  if (!isHomeAssistantAutoTimezone(value)) return value;
  var active = String(state && state.activeTimezone || "").trim();
  return active && !isHomeAssistantAutoTimezone(active) ? active : FALLBACK_TIMEZONE_OPTION;
}

function timezoneOptionsWithFallback(options, selected, preserveSelectedAuto) {
  var list = Array.isArray(options) && options.length ? options.slice() : defaultTimezoneOptions();
  var supportsAuto = list.indexOf(AUTO_TIMEZONE_OPTION) !== -1;
  if (selected && list.indexOf(selected) === -1 &&
      (!isHomeAssistantAutoTimezone(selected) || supportsAuto || preserveSelectedAuto)) {
    list.unshift(selected);
  }
  return list;
}

var state = {
  grid: [],
  sizes: {},
  buttons: [],
  theme: defaultTheme(),
  onColor: DEFAULT_COLOR_PRESET.on,
  selectedSlots: [],
  lastClickedSlot: -1,
  clockBarSelectedItem: "",
  activeTab: "screen",
  _indoorOn: false,
  _outdoorOn: false,
  _indoorVal: null,
  _outdoorVal: null,
  indoorEntity: "",
  outdoorEntity: "",
  clockBarTemperatureEntities: [],
  _clockBarTemperatureEntitiesReceived: false,
  _clockBarTemperatureVisibilityReceived: false,
  temperatureUnit: "Auto",
  clockBarOn: false,
  _clockBarStateValues: {},
  clockBarTimeOn: true,
  networkStatusOn: true,
  voiceServicesOn: false,
  networkTransport: "wifi",
  wifiStrengthPercent: 100,
  temperatureDegreeSymbolOn: true,
  subpageChevronsOn: true,
  presenceEntity: "",
  mediaPlayerSleepPreventionOn: false,
  mediaPlayerSleepPreventionEntity: "",
  coverArtScreensaverOn: false,
  coverArtMediaPlayerEntity: "",
  coverArtAttributeConditions: "",
  coverArtFilteringEnabled: false,
  coverArtDelay: 10,
  coverArtTouchPause: 120,
  coverArtTrackOverlayDuration: 5,
  coverArtHideExternalInputOn: true,
  homeAssistantArtworkProtocol: "http",
  coverArtHomeAssistantPort: 8123,
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
  automaticBrightnessEnabled: true,
  brightnessDawnTime: "06:00",
  brightnessDuskTime: "18:00",
  scheduleTrigger: "disabled",
  _scheduleTriggerReceived: false,
  scheduleEnabled: false,
  scheduleOnHour: 6,
  scheduleOffHour: 23,
  scheduleMode: "screen_off",
  scheduleWakeTimeout: 60,
  scheduleWakeBrightness: 10,
  scheduleDimmedBrightness: 10,
  scheduleClockBrightness: 10,
  scheduleClockTextColor: "FFFFFF",
  timezone: AUTO_TIMEZONE_OPTION,
  activeTimezone: FALLBACK_TIMEZONE_OPTION,
  timezoneOptions: defaultTimezoneOptions(),
  language: "en",
  languageOptions: ["en", "cs", "da", "de", "es", "fi", "fr", "hu", "it", "nb", "nl", "pl", "pt", "pt-br", "ro", "sk", "sl", "sv", "tr", "uk"],
  clockFormat: "24h",
  clockFormatOptions: ["12h", "24h"],
  customNtpServers: false,
  ntpServer1: NTP_SERVER_DEFAULTS[0],
  ntpServer2: NTP_SERVER_DEFAULTS[1],
  ntpServer3: NTP_SERVER_DEFAULTS[2],
  screenRotation: (CFG.features && CFG.features.screenRotationDefault) || "0",
  screenRotationOptions: (CFG.features && CFG.features.screenRotationOptions) || ["0", "90", "180", "270"],
  screenRotationDeviceOptions: null,
  screenRotationInitialReady: !(CFG.features && CFG.features.screenRotation),
  pendingButtonOrderRaw: null,
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
  firmwareInstallError: "",
  firmwareUpdateControlsSupported: false,
  firmwareInstallControlsSupported: false,
  firmwareOtaUrl: "",
  firmwareOtaFilename: "",
  firmwareOtaMd5: "",
  firmwareVersionOptions: [],
  firmwareSelectedVersion: "",
  firmwareVersionIndexLoaded: false,
  c6FirmwareCurrentVersion: "",
  c6FirmwareLatestVersion: "",
  c6FirmwareUpdateAvailable: "",
  c6FirmwareUpdateControlsSupported: false,
  c6FirmwareInstallControlsSupported: false,
  c6FirmwareChecking: false,
  c6FirmwareInstalling: false,
  autoUpdate: true,
  updateFrequency: "Daily",
  updateFreqOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
  configLocked: false,
  configLockReason: "",
  clockBarDragItem: "",
  clockBarTempRestoreIndoor: false,
  clockBarTempRestoreOutdoor: true,
  clockBarTempRestoreEntities: [],
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

for (var i = 0; i < TOTAL_SLOTS; i++) {
  state.grid.push(0);
  state.buttons.push({ entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" });
}

function getActiveScreensaverMode() {
  if (state.screensaverMode === "sensor") return "sensor";
  if (state.screensaverMode === "timer") return "timer";
  return "disabled";
}

function uniqueOptions(options) {
  var out = [];
  (options || []).forEach(function (opt) {
    opt = String(opt);
    if (out.indexOf(opt) < 0) out.push(opt);
  });
  return out;
}

function normalizeTemperatureUnit(value) {
  return EspControlModel.normalizeTemperatureUnit(value);
}

function normalizeHour(value, fallback) {
  return EspControlModel.normalizeHour(value, fallback);
}

function normalizeTimeOfDay(value, fallback) {
  return EspControlModel.normalizeTimeOfDay(value, fallback);
}

function normalizeScheduleWakeTimeout(value) {
  return EspControlModel.normalizeScheduleWakeTimeout(value);
}

function normalizeScheduleWakeBrightness(value) {
  return EspControlModel.normalizeScheduleWakeBrightness(value);
}

function normalizeScheduleClockBrightness(value) {
  return EspControlModel.normalizeScheduleClockBrightness(value);
}

function normalizeHexColor(value, fallback) {
  return EspControlModel.normalizeHexColor(value, fallback);
}

function normalizeScheduleDimmedBrightness(value) {
  return EspControlModel.normalizeScheduleDimmedBrightness(value);
}

function normalizeScheduleMode(value) {
  return EspControlModel.normalizeScheduleMode(value);
}

function normalizeScheduleTrigger(value, scheduleEnabled) {
  return EspControlModel.normalizeScheduleTrigger(value, scheduleEnabled);
}

function normalizeScreensaverAction(value) {
  return EspControlModel.normalizeScreensaverAction(value);
}

function screensaverActionOption(value) {
  return EspControlModel.screensaverActionOption(value);
}

function scheduleModeOption(value) {
  return EspControlModel.scheduleModeOption(value);
}

function normalizeClockBrightness(value, fallback) {
  return EspControlModel.normalizeClockBrightness(value, fallback);
}

function normalizeScreensaverDimmedBrightness(value) {
  return EspControlModel.normalizeScreensaverDimmedBrightness(value);
}

function normalizeNtpServer(value, fallback) {
  return EspControlModel.normalizeNtpServer(value, fallback);
}

function monthNameForIndex(index) {
  var monthIndex = parseInt(index, 10);
  if (!isFinite(monthIndex) || monthIndex < 0 || monthIndex > 11) return "Date";
  try {
    return new Intl.DateTimeFormat(normalizeLanguage(state.language), { month: "long" })
      .format(new Date(Date.UTC(2000, monthIndex, 1)));
  } catch (_) {
    return new Intl.DateTimeFormat("en", { month: "long" })
      .format(new Date(Date.UTC(2000, monthIndex, 1)));
  }
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

function normalizeHomeAssistantArtworkPort(value) {
  var port = parseInt(value, 10);
  if (!isFinite(port)) return 8123;
  if (port < 1) return 1;
  if (port > 65535) return 65535;
  return port;
}

function normalizeHomeAssistantArtworkProtocol(value) {
  value = String(value == null ? "" : value).trim().toLowerCase();
  return value === "https" ? "https" : "http";
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
  state.scheduleTrigger = normalizeScheduleTrigger(state.scheduleTrigger, state.scheduleEnabled);
  state.scheduleEnabled = state.scheduleTrigger !== "disabled";
  state.scheduleOnHour = normalizeHour(state.scheduleOnHour, 6);
  state.scheduleOffHour = normalizeHour(state.scheduleOffHour, 23);
  state.scheduleMode = normalizeScheduleMode(state.scheduleMode);
  state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(state.scheduleWakeTimeout);
  state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(state.scheduleWakeBrightness);
  state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(state.scheduleDimmedBrightness);
  state.scheduleClockBrightness = normalizeScheduleClockBrightness(state.scheduleClockBrightness);
  state.brightnessDawnTime = normalizeTimeOfDay(state.brightnessDawnTime, "06:00");
  state.brightnessDuskTime = normalizeTimeOfDay(state.brightnessDuskTime, "18:00");
  if (els.setAutomaticBrightnessToggle) {
    els.setAutomaticBrightnessToggle.checked = !!state.automaticBrightnessEnabled;
  }
  if (els.setBrightnessDawnTime) els.setBrightnessDawnTime.value = state.brightnessDawnTime;
  if (els.setBrightnessDuskTime) els.setBrightnessDuskTime.value = state.brightnessDuskTime;
  if (els.setBrightnessManualTimes) {
    els.setBrightnessManualTimes.className =
      "sp-cond-field" + (!state.automaticBrightnessEnabled ? " sp-visible" : "");
  }
  if (els.setScheduleToggle) els.setScheduleToggle.checked = !!state.scheduleEnabled;
  if (els.setScheduleModeButtons) {
    els.setScheduleModeButtons.disabled.className = state.scheduleTrigger === "disabled" ? "active" : "";
    els.setScheduleModeButtons.time.className = state.scheduleTrigger === "time" ? "active" : "";
    els.setScheduleModeButtons.sensor.className = state.scheduleTrigger === "sensor" ? "active" : "";
  }
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
  if (els.setScheduleClockTextColor && els.setScheduleClockTextColor._syncColor) {
    els.setScheduleClockTextColor._syncColor(state.scheduleClockTextColor);
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
    els.setScheduleTimes.className = "sp-schedule-times" + (state.scheduleTrigger === "time" ? "" : " sp-hidden");
  }
  if (els.setScheduleSensor) {
    els.setScheduleSensor.className = "sp-schedule-times" + (state.scheduleTrigger === "sensor" ? "" : " sp-hidden");
  }
  if (els.setScheduleBadge) {
    els.setScheduleBadge.className = "sp-card-badge" + (state.scheduleEnabled ? "" : " sp-hidden");
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

function normalizeTheme(value) {
  return THEME_PRESETS[value] ? value : defaultTheme();
}

function syncThemeUi() {
  if (els.root) els.root.setAttribute("data-screen-theme", normalizeTheme(state.theme).toLowerCase());
}

function syncColorUi() {
  if (els.setOnColor && els.setOnColor._syncColor) els.setOnColor._syncColor(state.onColor);
}

function resetAppearanceColors(postChanges) {
  state.onColor = DEFAULT_COLOR_PRESET.on;
  syncColorUi();
  renderPreview();
  if (postChanges) {
    postText(entityName("button_on_color"), state.onColor);
  }
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
var FIRMWARE_CHECKING_VERSION_LABEL = "Checking version...";
var FIRMWARE_DEV_VERSION_LABEL = "Dev build";
var FIRMWARE_UNKNOWN_VERSION_LABEL = "Version unknown";

// ── Utilities ──────────────────────────────────────────────────────────

function escHtml(s) {
  return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;")
    .replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function mdiIcon(icon, className) {
  var iconName = String(icon || "cog").trim();
  var span = document.createElement("span");
  span.className = className || "mdi";
  span.classList.add("mdi-" + (/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(iconName) ? iconName : iconSlug(iconName)));
  return span;
}

function textSpan(text, className) {
  var span = document.createElement("span");
  if (className) span.className = className;
  span.textContent = text == null ? "" : String(text);
  return span;
}

function renderFirmwareVersion() {
  if (!els.fwVersionLabel) return;
  els.fwVersionLabel.innerHTML = '<span class="sp-fw-label">Installed </span>' +
    escHtml(firmwareVersionLabel());
}

function setFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return;
  if (isSpecificFirmwareVersion(state.firmwareVersion) && !isSpecificFirmwareVersion(version)) return;
  state.firmwareVersion = displayFirmwareVersion(version);
  renderFirmwareVersion();
  syncFirmwareVersionSelect();
  renderFirmwareUpdateStatus();
  stopFirmwareInstallRefreshIfComplete();
}

function displayFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return FIRMWARE_UNKNOWN_VERSION_LABEL;
  if (version === FIRMWARE_UNKNOWN_VERSION_LABEL) return FIRMWARE_UNKNOWN_VERSION_LABEL;
  return isSpecificFirmwareVersion(version) ? version : FIRMWARE_DEV_VERSION_LABEL;
}

function firmwareVersionLabel() {
  if (!state.firmwareVersion && state.firmwareVersionRefreshPending) {
    return FIRMWARE_CHECKING_VERSION_LABEL;
  }
  return displayFirmwareVersion(state.firmwareVersion);
}

function escAttr(s) {
  return String(s == null ? "" : s).replace(/&/g, "&amp;").replace(/"/g, "&quot;")
    .replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function isSettingsFocused() {
  var ae = document.activeElement;
  return ae && els.buttonSettings && els.buttonSettings.contains(ae);
}

function isSettingsOpen() {
  return !!(els.settingsOverlay && els.settingsOverlay.classList.contains("sp-visible"));
}
