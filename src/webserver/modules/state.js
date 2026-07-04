// ── State ──────────────────────────────────────────────────────────────

var AUTO_TIMEZONE_OPTION = "Auto (Home Assistant)";
var FALLBACK_TIMEZONE_OPTION = "UTC (GMT+0)";
var NTP_SERVER_DEFAULTS = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"];
var LANGUAGE_LABELS = {
  cs: "Čeština (Czech)",
  da: "Dansk (Danish)",
  de: "Deutsch (German)",
  en: "English",
  es: "Español (Spanish)",
  fi: "Suomi (Finnish)",
  fr: "Français (French)",
  hu: "Magyar (Hungarian)",
  it: "Italiano (Italian)",
  nb: "Norsk bokmål (Norwegian Bokmål)",
  nl: "Nederlands (Dutch)",
  pl: "Polski (Polish)",
  pt: "Português (Portuguese)",
  "pt-br": "Português (Brasil) (Brazilian Portuguese)",
  ro: "Română (Romanian)",
  sk: "Slovenčina (Slovak)",
  sl: "Slovenščina (Slovenian)",
  sv: "Svenska (Swedish)",
  tr: "Türkçe (Turkish)",
  uk: "Українська (Ukrainian)"
};
var THEME_PRESETS = {
  Light: { on: "0073FF" },
  Dark: { on: WEB_UI_COLORS.primary },
};
var DEFAULT_COLOR_PRESET = THEME_PRESETS[defaultTheme()];

function defaultTheme() {
  return "Dark";
}

function voiceServicesSupported() {
  return !!(CFG.features && CFG.features.voiceServices);
}

function defaultTimezoneOptions() {
  var options = (CFG && Array.isArray(CFG.timezoneOptions)) ? CFG.timezoneOptions.slice() : [];
  return options;
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

function clockBarVisibleInPreview() {
  return !!state.clockBarOn;
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
  return EspControlModel.normalizeTemperatureUnit(value);
}

function normalizeLanguage(value) {
  var language = String(value == null ? "" : value).trim().toLowerCase();
  return language || "en";
}

function languageLabel(value) {
  value = normalizeLanguage(value);
  return LANGUAGE_LABELS[value] || value;
}

function languageOptionsWithFallback(options, selected) {
  var list = uniqueOptions((options && options.length ? options : ["en"]).map(normalizeLanguage));
  selected = normalizeLanguage(selected);
  if (list.indexOf(selected) === -1) list.unshift(selected);
  return list;
}

function appendLanguageOption(select, opt) {
  var o = document.createElement("option");
  o.value = normalizeLanguage(opt);
  o.textContent = languageLabel(opt);
  select.appendChild(o);
}

function syncLanguageSelect() {
  if (!els.setLanguage) return;
  state.languageOptions = languageOptionsWithFallback(state.languageOptions, state.language);
  els.setLanguage.innerHTML = "";
  state.languageOptions.forEach(function (opt) {
    appendLanguageOption(els.setLanguage, opt);
  });
  els.setLanguage.value = normalizeLanguage(state.language);
}

function timezonePrefersFahrenheit(timezone) {
  var tz = getTzId(effectiveTimezoneOptionForWeb(timezone || state.timezone));
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

var MAX_CLOCK_BAR_TEMPERATURES = 1;

function defaultClockBarTemperatureEntity(index) {
  if (index === 0) return "sensor.outdoor_temperature";
  return "";
}

function normalizeClockBarTemperatureEntries(value) {
  var input = Array.isArray(value) ? value : String(value || "").split(/[|,\n]/);
  return input.map(function (entry) {
    return String(entry || "").trim();
  }).slice(0, MAX_CLOCK_BAR_TEMPERATURES);
}

function normalizeClockBarTemperatureEntities(value) {
  var input = normalizeClockBarTemperatureEntries(value);
  var out = [];
  input.forEach(function (entry) {
    if (entry && out.indexOf(entry) === -1) out.push(entry);
  });
  return out.slice(0, MAX_CLOCK_BAR_TEMPERATURES);
}

function serializeClockBarTemperatureEntities(list) {
  return normalizeClockBarTemperatureEntities(list).join("|");
}

function legacyClockBarTemperatureEntities() {
  var list = [];
  if (state._outdoorOn && state.outdoorEntity) list.push(state.outdoorEntity);
  if (state._indoorOn && state.indoorEntity) list.push(state.indoorEntity);
  return normalizeClockBarTemperatureEntities(list);
}

function clockBarTemperatureEntries() {
  var list = normalizeClockBarTemperatureEntries(state.clockBarTemperatureEntities);
  if (!list.length && !state._clockBarTemperatureEntitiesReceived) return legacyClockBarTemperatureEntities();
  return list;
}

function clockBarTemperatureEntities() {
  return normalizeClockBarTemperatureEntities(clockBarTemperatureEntries());
}

function primaryClockBarTemperatureEntity() {
  return clockBarTemperatureEntities()[0] || state.outdoorEntity || "";
}

function clockBarTemperatureVisible() {
  return !!(state._outdoorOn && primaryClockBarTemperatureEntity());
}

function applyClockBarTemperatureEntities(list, postDevice) {
  state.clockBarTemperatureEntities = normalizeClockBarTemperatureEntries(list);
  state._clockBarTemperatureEntitiesReceived = true;
  var configured = clockBarTemperatureEntities();
  if (!state._clockBarTemperatureVisibilityReceived) {
    state._outdoorOn = configured.length > 0;
  }
  state._indoorOn = false;
  state.outdoorEntity = configured[0] || "";
  state.indoorEntity = "";
  if (postDevice) {
    postClockBarTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
    postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
    postSwitch(entityName("indoor_temp_enable"), state._indoorOn);
    postText(entityName("outdoor_temp_entity"), state.outdoorEntity);
    postText(entityName("indoor_temp_entity"), state.indoorEntity);
  }
  syncTemperatureUi();
  updateTempPreview();
  updateClockBarItemUi();
}

function saveClockBarTemperatureSettings(entity, degreeSymbolOn) {
  entity = String(entity || "").trim();
  state.clockBarTemperatureEntities = entity ? [entity] : [];
  state._clockBarTemperatureEntitiesReceived = true;
  state._clockBarTemperatureVisibilityReceived = true;
  state._outdoorOn = !!entity;
  state._indoorOn = false;
  state.outdoorEntity = entity;
  state.indoorEntity = "";
  state.temperatureDegreeSymbolOn = !!degreeSymbolOn;
  postClockBarTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
  postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
  postSwitch(entityName("indoor_temp_enable"), false);
  postText(entityName("outdoor_temp_entity"), state.outdoorEntity);
  postText(entityName("indoor_temp_entity"), "");
  postTemperatureDegreeSymbol(state.temperatureDegreeSymbolOn);
  syncTemperatureUi();
  syncClockBarUi();
}

function setClockBarItemVisible(item, visible) {
  visible = !!visible;
  if (isClockBarTemperatureItem(item)) {
    var entity = primaryClockBarTemperatureEntity();
    if (visible && !entity) {
      entity = defaultClockBarTemperatureEntity(0);
      state.clockBarTemperatureEntities = [entity];
      state._clockBarTemperatureEntitiesReceived = true;
      state.outdoorEntity = entity;
      postClockBarTemperatureEntities(entity);
      postText(entityName("outdoor_temp_entity"), entity);
    }
    state._clockBarTemperatureVisibilityReceived = true;
    state._outdoorOn = visible && !!entity;
    state._indoorOn = false;
    postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
    postSwitch(entityName("indoor_temp_enable"), false);
    postText(entityName("indoor_temp_entity"), "");
  } else if (item === "time") {
    state.clockBarTimeOn = visible;
    postClockBarTime(state.clockBarTimeOn);
  } else if (item === "voice" && voiceServicesSupported()) {
    state.voiceServicesOn = visible;
    postVoiceServices(state.voiceServicesOn);
  } else if (item === "network") {
    state.networkStatusOn = visible;
    postNetworkStatusIcon(state.networkStatusOn);
  }
  syncClockBarUi();
  syncTemperatureUi();
}

function appendScreenRotationOption(select, opt) {
  var o = document.createElement("option");
  o.value = opt;
  o.textContent = displayScreenRotation(opt) + " deg";
  select.appendChild(o);
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

function syncClockBarUi() {
  var visible = clockBarVisibleInPreview();
  if (!visible && state.clockBarSelectedItem) {
    state.clockBarSelectedItem = "";
    hideSettingsOverlay();
  }
  syncPreviewGridTop();
  if (els.topbar) els.topbar.className = "sp-topbar" + (visible ? "" : " sp-hidden");
  if (els.setClockBarToggle) els.setClockBarToggle.checked = !!state.clockBarOn;
  if (els.setClockBarTimeToggle) els.setClockBarTimeToggle.checked = !!state.clockBarTimeOn;
  if (els.setNetworkStatusToggle) {
    els.setNetworkStatusToggle.checked = !!state.networkStatusOn;
  }
  if (els.setVoiceServicesToggle) {
    els.setVoiceServicesToggle.checked = !!state.voiceServicesOn;
  }
  if (els.setClockBarBadge) {
    els.setClockBarBadge.className = "sp-card-badge" + (state.clockBarOn ? "" : " sp-hidden");
  }
  if (els.setTemperatureDegreeSymbolToggle) {
    els.setTemperatureDegreeSymbolToggle.checked = !!state.temperatureDegreeSymbolOn;
  }
  if (els.setSubpageChevronToggle) {
    els.setSubpageChevronToggle.checked = !!state.subpageChevronsOn;
  }
  updateClockBarItemUi();
  renderSelectionBar(ctx());
  updateNetworkPreview();
  updateVoicePreview();
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
var SCREEN_ROTATION_STARTUP_FALLBACK_MS = 1200;
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

function renderFirmwareVersion() {
  if (!els.fwVersionLabel) return;
  els.fwVersionLabel.innerHTML = '<span class="sp-fw-label">Installed </span>' +
    escHtml(firmwareVersionLabel());
}

function displayC6FirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  return version || "Unknown";
}

function c6FirmwareVersionLooksKnown(version) {
  version = String(version == null ? "" : version).trim();
  return /\d/.test(version);
}

function c6FirmwareUpdateKnownAvailable() {
  var current = String(state.c6FirmwareCurrentVersion || "").trim();
  var latest = String(state.c6FirmwareLatestVersion || "").trim();
  return c6FirmwareVersionLooksKnown(current) &&
    c6FirmwareVersionLooksKnown(latest) &&
    current !== latest;
}

function syncC6FirmwareUi() {
  var show = state.c6FirmwareUpdateControlsSupported === true;
  if (els.c6FirmwareCard) els.c6FirmwareCard.style.display = show ? "" : "none";
  if (els.c6FirmwareCurrent) {
    els.c6FirmwareCurrent.textContent = displayC6FirmwareVersion(state.c6FirmwareCurrentVersion);
  }
  if (els.c6FirmwareLatest) {
    els.c6FirmwareLatest.textContent = displayC6FirmwareVersion(state.c6FirmwareLatestVersion);
  }
  if (els.c6FirmwareStatus) {
    var cls = "sp-fw-status";
    var status = "";
    if (state.c6FirmwareInstalling) {
      status = "Installing WiFi firmware update\u2026";
      cls += " sp-update-installing";
    } else if (state.c6FirmwareChecking) {
      status = "Checking WiFi firmware\u2026";
    } else if (c6FirmwareUpdateKnownAvailable()) {
      status = "WiFi firmware update available.";
      cls += " sp-update-available";
    } else if (state.c6FirmwareUpdateAvailable) {
      status = state.c6FirmwareUpdateAvailable;
    }
    els.c6FirmwareStatus.className = cls;
    els.c6FirmwareStatus.textContent = status;
  }
  if (els.c6FirmwareUpdateBtn) {
    var busy = state.c6FirmwareChecking || state.c6FirmwareInstalling;
    els.c6FirmwareUpdateBtn.className = "sp-fw-btn" + (busy ? " sp-fw-btn-busy" : "");
    els.c6FirmwareUpdateBtn.disabled = busy || !show ||
      (c6FirmwareUpdateKnownAvailable() && !state.c6FirmwareInstallControlsSupported);
    if (state.c6FirmwareInstalling) {
      els.c6FirmwareUpdateBtn.textContent = "Installing\u2026";
    } else if (state.c6FirmwareChecking) {
      els.c6FirmwareUpdateBtn.textContent = "Checking\u2026";
    } else if (c6FirmwareUpdateKnownAvailable()) {
      els.c6FirmwareUpdateBtn.textContent = "Update WiFi Firmware";
    } else {
      els.c6FirmwareUpdateBtn.textContent = "Check for Update";
    }
  }
}

function setC6FirmwareCurrentVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return;
  state.c6FirmwareCurrentVersion = version;
  state.c6FirmwareUpdateControlsSupported = true;
  state.c6FirmwareChecking = false;
  state.c6FirmwareInstalling = false;
  syncC6FirmwareUi();
}

function setC6FirmwareLatestVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return;
  state.c6FirmwareLatestVersion = version;
  state.c6FirmwareUpdateControlsSupported = true;
  state.c6FirmwareChecking = false;
  syncC6FirmwareUi();
}

function setC6FirmwareUpdateAvailable(value) {
  value = String(value == null ? "" : value).trim();
  if (!value) return;
  state.c6FirmwareUpdateAvailable = value;
  state.c6FirmwareUpdateControlsSupported = true;
  state.c6FirmwareChecking = false;
  syncC6FirmwareUi();
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
  var info = selectedFirmwareInfo();
  return state.firmwareInstallControlsSupported === true &&
    !!info &&
    isSpecificFirmwareVersion(info.latest_version) &&
    !selectedFirmwareMatchesInstalled();
}

function firmwareVersionsSame(a, b) {
  return String(a == null ? "" : a).trim().toLowerCase() ===
    String(b == null ? "" : b).trim().toLowerCase();
}

function publicFirmwareManifestUrl() {
  return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/manifest.json";
}

function publicFirmwareVersionsUrl() {
  return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/versions.json";
}

function publicFirmwareAssetUrl(assetPath, baseUrl) {
  assetPath = String(assetPath || "").trim();
  if (!assetPath) return "";
  try {
    return new URL(assetPath, baseUrl || publicFirmwareManifestUrl()).href;
  } catch (err) {
    if (/^https?:\/\//i.test(assetPath)) return assetPath;
    return FIRMWARE_PUBLIC_MANIFEST_BASE + encodeURIComponent(DEVICE_ID) + "/" + assetPath.replace(/^\/+/, "");
  }
}

function firmwareInfoFromPublicManifest(data, baseUrl) {
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
        ota_url: publicFirmwareAssetUrl(otaPath, baseUrl || publicFirmwareManifestUrl()),
        ota_filename: expectedOta,
        ota_md5: String(ota.md5 || "").trim(),
      };
    }
  }
  return null;
}

function firmwareInfoFromPublicVersionEntry(entry, baseUrl) {
  if (!entry || typeof entry !== "object") return null;
  var version = String(entry.version || entry.latest_version || "").trim();
  if (!isSpecificFirmwareVersion(version)) return null;
  var ota = entry.ota && typeof entry.ota === "object" ? entry.ota : entry;
  var otaPath = String(ota.path || entry.ota_path || "").trim();
  var expectedOta = DEVICE_ID + ".ota.bin";
  if (!otaPath) return null;
  var filename = otaPath.split("/").pop();
  if (filename !== expectedOta) return null;
  return {
    latest_version: version,
    release_url: String(entry.release_url || ota.release_url || "").trim(),
    ota_url: publicFirmwareAssetUrl(otaPath, baseUrl || publicFirmwareVersionsUrl()),
    ota_filename: expectedOta,
    ota_md5: String(ota.md5 || entry.ota_md5 || "").trim(),
  };
}

function firmwareInfosFromPublicVersions(data, baseUrl) {
  if (!data || typeof data !== "object") return [];
  var entries = Array.isArray(data.versions) ? data.versions : [];
  var seen = {};
  var infos = [];
  for (var i = 0; i < entries.length; i++) {
    var info = firmwareInfoFromPublicVersionEntry(entries[i], baseUrl || publicFirmwareVersionsUrl());
    if (!info || seen[info.latest_version.toLowerCase()]) continue;
    seen[info.latest_version.toLowerCase()] = true;
    infos.push(info);
  }
  return infos;
}

function latestFirmwareInfoFromState() {
  if (!isSpecificFirmwareVersion(state.firmwareLatestVersion)) return null;
  return {
    latest_version: state.firmwareLatestVersion,
    release_url: state.firmwareReleaseUrl,
    ota_url: state.firmwareOtaUrl,
    ota_filename: state.firmwareOtaFilename || (DEVICE_ID + ".ota.bin"),
    ota_md5: state.firmwareOtaMd5,
  };
}

function findFirmwareVersionInfo(version) {
  version = String(version || "").trim();
  if (!version) return null;
  for (var i = 0; i < state.firmwareVersionOptions.length; i++) {
    var info = state.firmwareVersionOptions[i];
    if (firmwareVersionsSame(info.latest_version, version)) return info;
  }
  var latest = latestFirmwareInfoFromState();
  if (latest && firmwareVersionsSame(latest.latest_version, version)) return latest;
  return null;
}

function selectedFirmwareInfo() {
  return findFirmwareVersionInfo(state.firmwareSelectedVersion) ||
    (state.firmwareVersionOptions.length ? state.firmwareVersionOptions[0] : null) ||
    latestFirmwareInfoFromState();
}

function selectedFirmwareVersion() {
  var info = selectedFirmwareInfo();
  return info ? info.latest_version : "";
}

function selectedFirmwareIsLatest() {
  var version = selectedFirmwareVersion();
  return !version || !publicFirmwareReleaseKnown() ||
    firmwareVersionsSame(version, state.firmwareLatestVersion);
}

function selectedFirmwareMatchesInstalled() {
  var version = selectedFirmwareVersion();
  return isSpecificFirmwareVersion(version) &&
    isSpecificFirmwareVersion(state.firmwareVersion) &&
    firmwareVersionsSame(state.firmwareVersion, version);
}

function firmwareVersionSelectorVisible() {
  return state.firmwareVersionIndexLoaded && state.firmwareVersionOptions.length > 1;
}

function syncFirmwareVersionSelect() {
  if (!els.fwVersionSelect) return;
  var options = state.firmwareVersionOptions;
  els.fwVersionSelect.innerHTML = "";
  if (!options.length) {
    if (els.fwVersionField) els.fwVersionField.style.display = "none";
    return;
  }
  if (!findFirmwareVersionInfo(state.firmwareSelectedVersion)) {
    state.firmwareSelectedVersion = options[0].latest_version;
  }
  for (var i = 0; i < options.length; i++) {
    var info = options[i];
    var option = document.createElement("option");
    option.value = info.latest_version;
    option.textContent = info.latest_version +
      (i === 0 || firmwareVersionsSame(info.latest_version, state.firmwareLatestVersion) ? " (Latest)" : "");
    if (firmwareVersionsSame(info.latest_version, state.firmwareVersion)) {
      option.textContent += " (Installed)";
    }
    els.fwVersionSelect.appendChild(option);
  }
  els.fwVersionSelect.value = state.firmwareSelectedVersion;
  if (els.fwVersionField) {
    els.fwVersionField.style.display =
      firmwareUpdateControlsVisible() && firmwareVersionSelectorVisible() ? "" : "none";
  }
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
  syncFirmwareVersionSelect();
  renderFirmwareUpdateStatus();
  return true;
}

function setPublicFirmwareVersions(infos) {
  if (!Array.isArray(infos) || !infos.length) return false;
  state.firmwareVersionOptions = infos;
  state.firmwareVersionIndexLoaded = true;
  if (!state.firmwareSelectedVersion || !findFirmwareVersionInfo(state.firmwareSelectedVersion)) {
    state.firmwareSelectedVersion = infos[0].latest_version;
  }
  setPublicFirmwareInfo(infos[0]);
  syncFirmwareVersionSelect();
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
  var info = selectedFirmwareInfo() || latestFirmwareInfoFromState();
  var isLatest = selectedFirmwareIsLatest();
  var version = info && info.latest_version ? info.latest_version : state.firmwareLatestVersion;
  var releaseUrl = info && info.release_url ? info.release_url : state.firmwareReleaseUrl;
  var status = (isLatest ? "Latest public version: " : "Selected firmware version: ") + escHtml(version);
  if (releaseUrl) {
    status += ' <a href="' + escAttr(releaseUrl) + '" target="_blank" rel="noopener">release notes</a>';
  }
  return status;
}

function firmwareUpdateControlsVisible() {
  return state.firmwareUpdateControlsSupported === true;
}

function syncFirmwareUpdateUi() {
  var show = firmwareUpdateControlsVisible();
  if (els.fwActions) els.fwActions.style.display = show ? "" : "none";
  if (els.fwStatus) els.fwStatus.style.display = show ? "" : "none";
  if (els.fwVersionField) {
    els.fwVersionField.style.display = show && firmwareVersionSelectorVisible() ? "" : "none";
  }
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
  } else if (state.firmwareInstallError) {
    status = escHtml(state.firmwareInstallError);
    cls += " sp-update-error";
  } else if (firmwareInstallAvailable()) {
    status = publicFirmwareStatusHtml();
    cls += " sp-update-available";
  } else if (state.firmwareUpdateState === "NO UPDATE") {
    if (selectedFirmwareMatchesInstalled()) {
      inlineStatus = selectedFirmwareIsLatest() ? "Up to date" : "Installed";
    } else if (publicFirmwareReleaseKnown() &&
        isSpecificFirmwareVersion(state.firmwareVersion) &&
        !installedFirmwareMatchesPublicRelease()) {
      status = publicFirmwareStatusHtml();
    } else {
      inlineStatus = "Up to date";
    }
  } else if (publicFirmwareReleaseKnown()) {
    if (selectedFirmwareMatchesInstalled()) {
      inlineStatus = selectedFirmwareIsLatest() ? "Up to date" : "Installed";
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
    } else if (selectedFirmwareMatchesInstalled() && !selectedFirmwareIsLatest()) {
      els.fwCheckBtn.disabled = true;
      els.fwCheckBtn.textContent = "Installed";
    } else if (firmwareInstallAvailable()) {
      els.fwCheckBtn.disabled = false;
      els.fwCheckBtn.textContent = selectedFirmwareIsLatest() ? "Install Update" : "Install Version";
    } else {
      els.fwCheckBtn.disabled = state.firmwareChecking;
      els.fwCheckBtn.textContent = state.firmwareChecking ? "Checking\u2026" : "Check for Update";
    }
  }
  if (els.fwVersionSelect) {
    els.fwVersionSelect.disabled = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
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
  if (state.firmwareUpdateState) state.firmwareInstallError = "";
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

function isC6FirmwareCurrentEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "text_sensor/esp32-c6: current firmware" ||
    (domain === "text_sensor" && name === "esp32-c6: current firmware") ||
    (id.indexOf("text_sensor-") === 0 && id.indexOf("c6") !== -1 &&
      id.indexOf("current") !== -1 && id.indexOf("firmware") !== -1);
}

function isC6FirmwareLatestEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "text_sensor/esp32-c6: latest firmware" ||
    (domain === "text_sensor" && name === "esp32-c6: latest firmware") ||
    (id.indexOf("text_sensor-") === 0 && id.indexOf("c6") !== -1 &&
      id.indexOf("latest") !== -1 && id.indexOf("firmware") !== -1);
}

function isC6FirmwareUpdateAvailableEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "text_sensor/esp32-c6: update available" ||
    (domain === "text_sensor" && name === "esp32-c6: update available") ||
    (id.indexOf("text_sensor-") === 0 && id.indexOf("c6") !== -1 &&
      id.indexOf("update") !== -1 && id.indexOf("available") !== -1);
}

function isC6FirmwareCheckButtonEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "button/firmware esp32-c6: check for update" ||
    (domain === "button" && name === "firmware esp32-c6: check for update") ||
    (id.indexOf("button-") === 0 && id.indexOf("c6") !== -1 &&
      id.indexOf("check") !== -1 && id.indexOf("update") !== -1);
}

function isC6FirmwareInstallButtonEvent(id, d) {
  id = String(id || "").toLowerCase();
  var nameId = String(d.name_id || "").toLowerCase();
  var domain = String(d.domain || "").toLowerCase();
  var name = String(d.name || "").toLowerCase();
  return nameId === "button/firmware esp32-c6: install update" ||
    (domain === "button" && name === "firmware esp32-c6: install update") ||
    (id.indexOf("button-") === 0 && id.indexOf("c6") !== -1 &&
      id.indexOf("install") !== -1 && id.indexOf("update") !== -1);
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
