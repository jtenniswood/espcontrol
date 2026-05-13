// ── Build UI ───────────────────────────────────────────────────────────

function showBanner(msg, type) {
  if (!els.banner) return;
  els.banner.textContent = msg;
  els.banner.className = "sp-banner sp-" + type;
  if (type === "error" || type === "success" || type === "warning") {
    clearTimeout(els._bannerTimer);
    els._bannerTimer = setTimeout(function () {
      els.banner.className = "sp-banner";
    }, 6000);
  }
}

function buildUI() {
  var root = document.createElement("div");
  root.id = "sp-app";

  var banner = document.createElement("div");
  banner.className = "sp-banner";
  root.appendChild(banner);
  els.banner = banner;

  buildHeader(root);
  buildScreenPage(root);
  buildSettingsPage(root);

  var app = document.querySelector("esp-app");
  if (app) {
    app.parentNode.insertBefore(root, app);
  } else {
    document.body.insertBefore(root, document.body.firstChild);
  }
  els.root = root;
  switchTab("screen");
}
function buildHeader(parent) {
  var header = document.createElement("div");
  header.className = "sp-header";

  var brand = document.createElement("div");
  brand.className = "sp-brand";
  brand.textContent = "EspControl";
  header.appendChild(brand);

  var nav = document.createElement("nav");
  nav.className = "sp-nav";
  nav.setAttribute("aria-label", "Primary");

  var tabs = [
    { id: "screen", label: "Screen" },
    { id: "settings", label: "Settings" },
  ];

  tabs.forEach(function (t) {
    var tab = document.createElement("div");
    tab.className = "sp-tab";
    tab.setAttribute("role", "tab");
    tab.setAttribute("aria-selected", "false");
    tab.textContent = t.label;
    tab.addEventListener("click", function () { switchTab(t.id); });
    nav.appendChild(tab);
    els["tab_" + t.id] = tab;
  });

  var docsLink = document.createElement("a");
  docsLink.className = "sp-tab sp-tab-docs";
  docsLink.href = "https://jtenniswood.github.io/espcontrol/";
  docsLink.target = "_blank";
  docsLink.rel = "noopener";
  docsLink.innerHTML = 'Docs <span class="mdi mdi-arrow-top-right"></span>';
  nav.appendChild(docsLink);

  header.appendChild(nav);
  parent.appendChild(header);
}

function buildScreenPage(parent) {
  var page = document.createElement("div");
  page.id = "sp-screen";
  page.className = "sp-page";

  var selectionBar = document.createElement("div");
  selectionBar.className = "sp-selection-bar";
  els.selectionBar = selectionBar;
  page.appendChild(selectionBar);

  var wrap = document.createElement("div");
  wrap.className = "sp-wrap";
  wrap.innerHTML =
    '<div class="sp-screen">' +
    '<div class="sp-topbar">' +
    '<span class="sp-temp"></span>' +
    '<span class="sp-clock">--:--</span>' +
    '<span class="sp-network-preview mdi mdi-wifi-strength-4"></span>' +
    "</div>" +
    '<div class="sp-main"></div>' +
    "</div>";
  page.appendChild(wrap);

  els.topbar = wrap.querySelector(".sp-topbar");
  els.temp = wrap.querySelector(".sp-temp");
  els.clock = wrap.querySelector(".sp-clock");
  els.networkPreview = wrap.querySelector(".sp-network-preview");
  els.previewMain = wrap.querySelector(".sp-main");
  els.previewMain.setAttribute("role", "grid");
  els.previewMain.setAttribute("aria-label", "Button grid");

  var hint = document.createElement("div");
  hint.className = "sp-hint";
  hint.textContent = "tap to select \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
  els.previewHint = hint;
  page.appendChild(hint);

  var overlay = document.createElement("div");
  overlay.className = "sp-settings-overlay";
  var modal = document.createElement("div");
  modal.className = "sp-settings-modal";
  var closeBtn = document.createElement("button");
  closeBtn.className = "sp-settings-close";
  closeBtn.innerHTML = "&times;";
  closeBtn.addEventListener("click", closeSettings);
  modal.appendChild(closeBtn);
  var config = document.createElement("div");
  config.className = "sp-config";
  els.buttonSettings = config;
  modal.appendChild(config);
  overlay.appendChild(modal);
  overlay.addEventListener("click", function (e) {
    if (e.target === overlay) closeSettings();
  });
  page.appendChild(overlay);
  els.settingsOverlay = overlay;

  page.appendChild(buildApplyBar());

  parent.appendChild(page);
  els.screenPage = page;
}

// ── Settings Page ──────────────────────────────────────────────────────

function buildSettingsPage(parent) {
  var page = document.createElement("div");
  page.id = "sp-settings";
  page.className = "sp-page";

  var config = document.createElement("div");
  config.className = "sp-config fade-in";

  var appearBody = document.createElement("div");

  appearBody.appendChild(fieldLabel("Primary"));
  var onColor = colorField("sp-set-on-color", "FF8C00", function (hex) {
    postText("Button On Color", hex);
  });
  appearBody.appendChild(onColor);
  els.setOnColor = onColor;

  appearBody.appendChild(fieldLabel("Secondary"));
  var offColor = colorField("sp-set-off-color", "313131", function (hex) {
    postText("Button Off Color", hex);
  });
  appearBody.appendChild(offColor);
  els.setOffColor = offColor;

  appearBody.appendChild(fieldLabel("Tertiary"));
  var sensorColor = colorField("sp-set-sensor-color", "212121", function (hex) {
    postText("Sensor Card Color", hex);
  });
  appearBody.appendChild(sensorColor);
  els.setSensorColor = sensorColor;

  config.appendChild(makeCollapsibleCard("Appearance", appearBody, true));

  var blBody = document.createElement("div");

  var daySlider = createRangeSlider("Daytime Brightness", state.brightnessDayVal, "Screen: Daytime Brightness");
  blBody.appendChild(daySlider.wrap);
  els.setDayBrightness = daySlider.range;
  els.setDayBrightnessVal = daySlider.val;

  var nightSlider = createRangeSlider("Nighttime Brightness", state.brightnessNightVal, "Screen: Nighttime Brightness");
  blBody.appendChild(nightSlider.wrap);
  els.setNightBrightness = nightSlider.range;
  els.setNightBrightnessVal = nightSlider.val;

  var sunInfo = document.createElement("div");
  sunInfo.className = "sp-sun-info";
  sunInfo.id = "sp-sun-info";
  blBody.appendChild(sunInfo);
  els.sunInfo = sunInfo;
  updateSunInfo();

  config.appendChild(makeCollapsibleCard("Backlight", blBody, true));

  var scheduleBody = document.createElement("div");
  var scheduleToggle = toggleRow("Night Schedule", "sp-set-schedule-enabled", state.scheduleEnabled);
  scheduleBody.appendChild(scheduleToggle.row);
  els.setScheduleToggle = scheduleToggle.input;

  var scheduleTimes = document.createElement("div");
  scheduleTimes.className = "sp-schedule-times";

  var onHour = createHourSelect("Daytime", "sp-set-schedule-on-hour", state.scheduleOnHour, function (hour) {
    state.scheduleOnHour = hour;
    postScreenScheduleOnHour(hour);
    syncScreenScheduleUi();
  });
  scheduleTimes.appendChild(onHour.wrap);
  els.setScheduleOnHour = onHour.select;

  var offHour = createHourSelect("Night Time", "sp-set-schedule-off-hour", state.scheduleOffHour, function (hour) {
    state.scheduleOffHour = hour;
    postScreenScheduleOffHour(hour);
    syncScreenScheduleUi();
  });
  scheduleTimes.appendChild(offHour.wrap);
  els.setScheduleOffHour = offHour.select;

  var scheduleModeField = document.createElement("div");
  scheduleModeField.className = "sp-field";
  scheduleModeField.appendChild(fieldLabel("At Night Time", "sp-set-schedule-mode"));
  var scheduleModeSelect = document.createElement("select");
  scheduleModeSelect.className = "sp-select";
  scheduleModeSelect.id = "sp-set-schedule-mode";
  [
    { value: "screen_off", label: "Screen Off" },
    { value: "screen_dimmed", label: "Screen Dimmed" },
    { value: "clock", label: "Clock" },
  ].forEach(function (opt) {
    var option = document.createElement("option");
    option.value = opt.value;
    option.textContent = opt.label;
    scheduleModeSelect.appendChild(option);
  });
  scheduleModeSelect.addEventListener("change", function () {
    state.scheduleMode = normalizeScheduleMode(this.value);
    postScreenScheduleMode(state.scheduleMode);
    syncScreenScheduleUi();
  });
  scheduleModeField.appendChild(scheduleModeSelect);
  scheduleTimes.appendChild(scheduleModeField);
  els.setScheduleMode = scheduleModeSelect;

  var offScreenOptions = condField();
  var wakeTimeoutField = document.createElement("div");
  wakeTimeoutField.className = "sp-field";
  wakeTimeoutField.appendChild(fieldLabel("When Woken, Idle Time to Screen Off", "sp-set-schedule-wake-timeout"));
  var wakeTimeoutSelect = document.createElement("select");
  wakeTimeoutSelect.className = "sp-select";
  wakeTimeoutSelect.id = "sp-set-schedule-wake-timeout";
  var wakeTimeoutOptions = [
    { label: "10 seconds", value: 10 },
    { label: "30 seconds", value: 30 },
    { label: "1 minute", value: 60 },
    { label: "2 minutes", value: 120 },
    { label: "5 minutes", value: 300 },
    { label: "10 minutes", value: 600 },
    { label: "30 minutes", value: 1800 },
    { label: "1 hour", value: 3600 },
  ];
  wakeTimeoutOptions.forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt.value;
    o.textContent = opt.label;
    wakeTimeoutSelect.appendChild(o);
  });
  wakeTimeoutSelect.addEventListener("change", function () {
    state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(this.value);
    postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
    syncScreenScheduleUi();
  });
  wakeTimeoutField.appendChild(wakeTimeoutSelect);
  offScreenOptions.appendChild(wakeTimeoutField);
  els.setScheduleWakeTimeout = wakeTimeoutSelect;

  var wakeBrightnessSlider = createRangeSlider(
    "When Woken, Screen Brightness",
    state.scheduleWakeBrightness,
    postScreenScheduleWakeBrightness
  );
  wakeBrightnessSlider.range.id = "sp-set-schedule-wake-brightness";
  wakeBrightnessSlider.range.addEventListener("change", function () {
    state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(this.value);
    syncScreenScheduleUi();
  });
  offScreenOptions.appendChild(wakeBrightnessSlider.wrap);
  els.setScheduleWakeBrightness = wakeBrightnessSlider.range;
  els.setScheduleWakeBrightnessVal = wakeBrightnessSlider.val;
  scheduleTimes.appendChild(offScreenOptions);
  els.setScheduleOffOptions = offScreenOptions;

  var dimmedOptions = condField();
  var dimmedBrightnessSlider = createRangeSlider(
    "Dimmed Screen Brightness",
    state.scheduleDimmedBrightness,
    postScreenScheduleDimmedBrightness
  );
  dimmedBrightnessSlider.range.id = "sp-set-schedule-dimmed-brightness";
  dimmedBrightnessSlider.range.min = "1";
  dimmedBrightnessSlider.range.step = "1";
  dimmedBrightnessSlider.range.addEventListener("input", function () {
    state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(this.value);
    syncScreenScheduleUi();
  });
  dimmedOptions.appendChild(dimmedBrightnessSlider.wrap);
  scheduleTimes.appendChild(dimmedOptions);
  els.setScheduleDimmedOptions = dimmedOptions;
  els.setScheduleDimmedBrightness = dimmedBrightnessSlider.range;
  els.setScheduleDimmedBrightnessVal = dimmedBrightnessSlider.val;

  var clockOptions = condField();
  var clockBrightnessSlider = createRangeSlider(
    "Clock Brightness",
    state.scheduleClockBrightness,
    postScreenScheduleClockBrightness
  );
  clockBrightnessSlider.range.id = "sp-set-schedule-clock-brightness";
  clockBrightnessSlider.range.min = "1";
  clockBrightnessSlider.range.step = "1";
  clockBrightnessSlider.range.addEventListener("input", function () {
    state.scheduleClockBrightness = normalizeScheduleClockBrightness(this.value);
    syncScreenScheduleUi();
  });
  clockOptions.appendChild(clockBrightnessSlider.wrap);
  scheduleTimes.appendChild(clockOptions);
  els.setScheduleClockOptions = clockOptions;
  els.setScheduleClockBrightness = clockBrightnessSlider.range;
  els.setScheduleClockBrightnessVal = clockBrightnessSlider.val;

  scheduleBody.appendChild(scheduleTimes);
  els.setScheduleTimes = scheduleTimes;

  scheduleToggle.input.addEventListener("change", function () {
    state.scheduleEnabled = this.checked;
    postScreenScheduleEnabled(state.scheduleEnabled);
    syncScreenScheduleUi();
  });

  var scheduleBadge = document.createElement("span");
  scheduleBadge.setAttribute("aria-label", "Schedule on");
  scheduleBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
  els.setScheduleBadge = scheduleBadge;
  syncScreenScheduleUi();
  var scheduleCard = makeCollapsibleCard("Night Schedule", scheduleBody, true, scheduleBadge);

  var clockBody = document.createElement("div");

  var tzField = document.createElement("div");
  tzField.className = "sp-field";
  tzField.appendChild(fieldLabel("Timezone", "sp-set-timezone"));
  var tzSelect = document.createElement("select");
  tzSelect.className = "sp-select";
  tzSelect.id = "sp-set-timezone";
  if (state.timezoneOptions.length) {
    state.timezoneOptions.forEach(function (opt) {
      appendTimezoneOption(tzSelect, opt);
    });
  }
  tzSelect.value = state.timezone;
  tzSelect.addEventListener("change", function () {
    state.timezone = this.value;
    postSelect("Screen: Timezone", this.value);
    if (normalizeTemperatureUnit(state.temperatureUnit) === "Auto") {
      updateTempPreview();
      renderPreview();
    }
    updateClock();
  });
  tzField.appendChild(tzSelect);
  clockBody.appendChild(tzField);
  els.setTimezone = tzSelect;

  var cfField = document.createElement("div");
  cfField.className = "sp-field";
  cfField.appendChild(fieldLabel("Clock Format", "sp-set-clock-format"));
  var cfSelect = document.createElement("select");
  cfSelect.className = "sp-select";
  cfSelect.id = "sp-set-clock-format";
  state.clockFormatOptions.forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt;
    o.textContent = opt === "12h" ? "12-hour" : "24-hour";
    cfSelect.appendChild(o);
  });
  cfSelect.value = state.clockFormat;
  cfSelect.addEventListener("change", function () {
    postSelect("Screen: Clock Format", this.value);
  });
  cfField.appendChild(cfSelect);
  clockBody.appendChild(cfField);
  els.setClockFormat = cfSelect;

  var ntpField = document.createElement("div");
  ntpField.className = "sp-field";
  ntpField.appendChild(fieldLabel("NTP Servers", "sp-set-custom-ntp-servers"));
  state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
  var customNtpServers = toggleRow("Custom NTP Servers", "sp-set-custom-ntp-servers", state.customNtpServers);
  ntpField.appendChild(customNtpServers.row);
  els.setCustomNtpServersToggle = customNtpServers.input;
  customNtpServers.input.addEventListener("change", function () {
    state.customNtpServers = this.checked;
    if (!state.customNtpServers) {
      resetNtpServersToDefaults();
      postText("Screen: NTP Server 1", state.ntpServer1);
      postText("Screen: NTP Server 2", state.ntpServer2);
      postText("Screen: NTP Server 3", state.ntpServer3);
    }
    syncNtpServerUi();
  });

  var ntpList = document.createElement("div");
  ntpList.className = "sp-field-stack";
  els.setNtpServerFields = ntpList;

  function addNtpServerInput(id, stateKey, postName, placeholder, ariaLabel) {
    var input = textInput(id, state[stateKey], placeholder);
    input.setAttribute("aria-label", ariaLabel);
    input.addEventListener("blur", function () {
      var value = this.value.trim();
      this.value = value;
      state[stateKey] = value;
      state.customNtpServers = true;
      syncNtpServerUi();
      postText(postName, value);
    });
    input.addEventListener("keydown", function (e) {
      if (e.key === "Enter") this.blur();
    });
    ntpList.appendChild(input);
    return input;
  }

  els.setNtpServer1 = addNtpServerInput(
    "sp-set-ntp-server-1", "ntpServer1",
    "Screen: NTP Server 1", NTP_SERVER_DEFAULTS[0], "NTP Server 1");
  els.setNtpServer2 = addNtpServerInput(
    "sp-set-ntp-server-2", "ntpServer2",
    "Screen: NTP Server 2", NTP_SERVER_DEFAULTS[1], "NTP Server 2");
  els.setNtpServer3 = addNtpServerInput(
    "sp-set-ntp-server-3", "ntpServer3",
    "Screen: NTP Server 3", NTP_SERVER_DEFAULTS[2], "NTP Server 3");

  ntpField.appendChild(ntpList);
  syncNtpServerUi();
  clockBody.appendChild(ntpField);

  var monthNamesField = document.createElement("div");
  monthNamesField.className = "sp-field";
  monthNamesField.appendChild(fieldLabel("Advanced Date Labels", "sp-set-custom-month-names"));
  state.monthNames = normalizeMonthNames(state.monthNames);
  state.customMonthNames = state.customMonthNames || hasCustomMonthNames();
  var customMonthNames = toggleRow("Custom Month Names", "sp-set-custom-month-names", state.customMonthNames);
  monthNamesField.appendChild(customMonthNames.row);
  els.setCustomMonthNamesToggle = customMonthNames.input;
  customMonthNames.input.addEventListener("change", function () {
    state.customMonthNames = this.checked;
    if (!state.customMonthNames) {
      resetMonthNamesToDefaults();
      postText("Screen: Month Names", serializeMonthNames(state.monthNames));
      renderPreview();
    }
    syncMonthNameUi();
  });

  var monthList = document.createElement("div");
  monthList.className = "sp-field-stack";
  els.setMonthNameFields = monthList;
  els.setMonthNameInputs = [];

  function addMonthNameInput(index) {
    var input = textInput(
      "sp-set-month-name-" + (index + 1),
      state.monthNames[index],
      MONTH_NAME_DEFAULTS[index]
    );
    input.setAttribute("aria-label", MONTH_NAME_DEFAULTS[index] + " label");
    input.addEventListener("blur", function () {
      var names = normalizeMonthNames(state.monthNames);
      names[index] = this.value.trim() || MONTH_NAME_DEFAULTS[index];
      state.monthNames = names;
      this.value = names[index];
      state.customMonthNames = hasCustomMonthNames();
      syncMonthNameUi();
      postText("Screen: Month Names", serializeMonthNames(state.monthNames));
      renderPreview();
    });
    input.addEventListener("keydown", function (e) {
      if (e.key === "Enter") this.blur();
    });
    monthList.appendChild(input);
    els.setMonthNameInputs.push(input);
  }

  for (var monthIndex = 0; monthIndex < 12; monthIndex++) {
    addMonthNameInput(monthIndex);
  }
  monthNamesField.appendChild(monthList);
  syncMonthNameUi();
  clockBody.appendChild(monthNamesField);

  var timeSettingsCard = makeCollapsibleCard("Time Settings", clockBody, true);

  var clockBarBody = document.createElement("div");

  var clockBar = toggleRow("Show Clock Bar", "sp-set-clock-bar", state.clockBarOn);
  clockBarBody.appendChild(clockBar.row);
  els.setClockBarToggle = clockBar.input;
  clockBar.input.addEventListener("change", function () {
    state.clockBarOn = this.checked;
    syncClockBarUi();
    postClockBar(state.clockBarOn);
  });

  var networkStatus = toggleRow("Show Network Status Icon", "sp-set-network-status-icon", state.networkStatusOn);
  clockBarBody.appendChild(networkStatus.row);
  els.setNetworkStatusToggle = networkStatus.input;
  networkStatus.input.addEventListener("change", function () {
    state.networkStatusOn = this.checked;
    syncClockBarUi();
    postNetworkStatusIcon(state.networkStatusOn);
  });

  var outdoor = createEntityToggleSection("Outdoor Temperature", "sp-set-outdoor-toggle", state._outdoorOn,
    "Outdoor Temp Enable", "Outdoor Temp Entity", "Outdoor Temp Entity", "sensor.outdoor_temperature");
  clockBarBody.appendChild(outdoor.toggle.row);
  clockBarBody.appendChild(outdoor.field);
  els.setOutdoorToggle = outdoor.toggle.input;
  els.setOutdoorField = outdoor.field;
  els.setOutdoorEntity = outdoor.input;
  outdoor.toggle.input.addEventListener("change", function () {
    state._outdoorOn = this.checked;
    syncTemperatureUi();
    updateTempPreview();
  });

  var indoor = createEntityToggleSection("Indoor Temperature", "sp-set-indoor-toggle", state._indoorOn,
    "Indoor Temp Enable", "Indoor Temp Entity", "Indoor Temp Entity", "sensor.indoor_temperature");
  clockBarBody.appendChild(indoor.toggle.row);
  clockBarBody.appendChild(indoor.field);
  els.setIndoorToggle = indoor.toggle.input;
  els.setIndoorField = indoor.field;
  els.setIndoorEntity = indoor.input;
  indoor.toggle.input.addEventListener("change", function () {
    state._indoorOn = this.checked;
    syncTemperatureUi();
    updateTempPreview();
  });

  var degreeSymbol = toggleRow("Show Degree Symbol", "sp-set-temperature-degree-symbol", state.temperatureDegreeSymbolOn);
  clockBarBody.appendChild(degreeSymbol.row);
  els.setTemperatureDegreeSymbolToggle = degreeSymbol.input;
  degreeSymbol.input.addEventListener("change", function () {
    state.temperatureDegreeSymbolOn = this.checked;
    syncClockBarUi();
    postTemperatureDegreeSymbol(state.temperatureDegreeSymbolOn);
  });

  var clockBarBadge = document.createElement("span");
  clockBarBadge.setAttribute("aria-label", "Clock bar on");
  clockBarBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
  els.setClockBarBadge = clockBarBadge;
  syncClockBarUi();
  syncTemperatureUi();
  config.appendChild(makeCollapsibleCard("Clock Bar", clockBarBody, true, clockBarBadge));

  if (CFG.features && CFG.features.screenRotation) {
    var rotationBody = document.createElement("div");
    var rotField = document.createElement("div");
    rotField.className = "sp-field";
    rotField.appendChild(fieldLabel("Rotation", "sp-set-screen-rotation"));
    var rotSelect = document.createElement("select");
    rotSelect.className = "sp-select";
    rotSelect.id = "sp-set-screen-rotation";
    activeScreenRotationOptions().forEach(function (opt) {
      appendScreenRotationOption(rotSelect, opt);
    });
    rotSelect.value = state.screenRotation;
    rotSelect.addEventListener("change", function () {
      state.screenRotation = normalizeScreenRotation(this.value);
      syncPreviewOrientation();
      renderPreview();
      postSelect("Screen: Rotation", this.value);
    });
    rotField.appendChild(rotSelect);
    rotationBody.appendChild(rotField);
    config.appendChild(makeCollapsibleCard("Rotation", rotationBody, true));
    els.setScreenRotation = rotSelect;
  }

  var tempBody = document.createElement("div");

  var unitField = document.createElement("div");
  unitField.className = "sp-field";
  unitField.appendChild(fieldLabel("Temperature Unit", "sp-set-temperature-unit"));
  var unitSelect = document.createElement("select");
  unitSelect.className = "sp-select";
  unitSelect.id = "sp-set-temperature-unit";
  [
    ["Auto", "Auto (from timezone)"],
    ["\u00B0C", "Centigrade (\u00B0C)"],
    ["\u00B0F", "Fahrenheit (\u00B0F)"],
  ].forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt[0];
    o.textContent = opt[1];
    unitSelect.appendChild(o);
  });
  unitSelect.value = normalizeTemperatureUnit(state.temperatureUnit);
  unitSelect.addEventListener("change", function () {
    state.temperatureUnit = normalizeTemperatureUnit(this.value);
    postSelect("Screen: Temperature Unit", state.temperatureUnit);
    updateTempPreview();
    renderPreview();
  });
  unitField.appendChild(unitSelect);
  tempBody.appendChild(unitField);
  els.setTemperatureUnit = unitSelect;

  syncTemperatureUi();
  config.appendChild(makeCollapsibleCard("Temperature", tempBody, true));

  var ssBody = document.createElement("div");
  var ssMode = getActiveScreensaverMode();

  ssBody.appendChild(fieldLabel("Mode"));
  var segment = document.createElement("div");
  segment.className = "sp-segment sp-screensaver-mode";
  var disabledBtn = document.createElement("button");
  disabledBtn.textContent = "Disabled";
  disabledBtn.type = "button";
  var timerBtn = document.createElement("button");
  timerBtn.textContent = "Timer";
  timerBtn.type = "button";
  var sensorBtn = document.createElement("button");
  sensorBtn.textContent = "Sensor";
  sensorBtn.type = "button";
  segment.appendChild(disabledBtn);
  segment.appendChild(timerBtn);
  segment.appendChild(sensorBtn);
  ssBody.appendChild(segment);

  var timerPanel = document.createElement("div");

  var timeoutField = document.createElement("div");
  timeoutField.className = "sp-field";
  timeoutField.appendChild(fieldLabel("Timeout"));
  var timeoutSelect = document.createElement("select");
  timeoutSelect.className = "sp-select";
  timeoutSelect.id = "sp-set-ss-timeout";
  timeoutSelect.addEventListener("change", function () {
    var n = parseFloat(this.value);
    if (isFinite(n)) state.screensaverTimeout = n;
    postScreensaverTimeout(this.value);
  });
  timeoutField.appendChild(timeoutSelect);
  timerPanel.appendChild(timeoutField);

  var timerClockControls = createScreensaverThenControls("sp-set-clock-mode");
  timerPanel.appendChild(timerClockControls.clockField);
  timerPanel.appendChild(timerClockControls.dimBrightnessField);
  timerPanel.appendChild(timerClockControls.brightnessField);
  els.setClockSelect = timerClockControls.clockSelect;
  els.setClockField = timerClockControls.clockField;
  els.setDimBrightnessField = timerClockControls.dimBrightnessField;
  els.setDimBrightness = timerClockControls.dimBrightness;
  els.setDimBrightnessVal = timerClockControls.dimBrightnessVal;
  els.setClockBrightnessDay = timerClockControls.clockBrightnessDay;
  els.setClockBrightnessDayVal = timerClockControls.clockBrightnessDayVal;
  els.setClockBrightnessNight = timerClockControls.clockBrightnessNight;
  els.setClockBrightnessNightVal = timerClockControls.clockBrightnessNightVal;
  els.setClockBrightnessField = timerClockControls.brightnessField;

  var mediaPlayerToggle = toggleRow(
    "Keep Awake Media Player",
    "sp-set-ss-media-player-enable",
    state.mediaPlayerSleepPreventionOn);
  timerPanel.appendChild(mediaPlayerToggle.row);
  mediaPlayerToggle.input.addEventListener("change", function () {
    state.mediaPlayerSleepPreventionOn = this.checked;
    syncMediaPlayerSleepPreventionUi();
    postSwitch("Screen Saver: Media Player Sleep Prevention", state.mediaPlayerSleepPreventionOn);
  });
  els.setMediaPlayerSleepPreventionToggle = mediaPlayerToggle.input;

  var mediaPlayerField = document.createElement("div");
  mediaPlayerField.className = "sp-field sp-cond-field";
  mediaPlayerField.appendChild(fieldLabel("Media Player Entity", "sp-set-ss-media-player"));
  var mediaPlayerInp = textInput(
    "sp-set-ss-media-player",
    state.mediaPlayerSleepPreventionEntity,
    "e.g. media_player.living_room");
  mediaPlayerField.appendChild(mediaPlayerInp);
  timerPanel.appendChild(mediaPlayerField);
  bindTextPost(mediaPlayerInp, "Media Player Sleep Prevention Entity", {
    onBlur: function (value) { state.mediaPlayerSleepPreventionEntity = value; },
  });
  els.setMediaPlayerSleepPrevention = mediaPlayerInp;
  els.setMediaPlayerSleepPreventionField = mediaPlayerField;

  ssBody.appendChild(timerPanel);
  els.setSSTimeout = timeoutSelect;
  syncScreensaverTimeoutUi();

  var sensorPanel = document.createElement("div");
  var presenceField = document.createElement("div");
  presenceField.className = "sp-field";
  presenceField.appendChild(fieldLabel("Presence Entity", "sp-set-presence"));
  var presInp = entityInput("sp-set-presence", "", "Presence sensor entity", ["binary_sensor", "sensor"]);
  presenceField.appendChild(presInp);
  sensorPanel.appendChild(presenceField);
  bindTextPost(presInp, "Presence Sensor Entity", {});
  var sensorClockControls = createScreensaverThenControls("sp-set-sensor-clock-mode");
  sensorPanel.appendChild(sensorClockControls.clockField);
  sensorPanel.appendChild(sensorClockControls.dimBrightnessField);
  sensorPanel.appendChild(sensorClockControls.brightnessField);
  ssBody.appendChild(sensorPanel);
  els.setPresence = presInp;
  els.setSensorClockSelect = sensorClockControls.clockSelect;
  els.setSensorClockField = sensorClockControls.clockField;
  els.setSensorDimBrightnessField = sensorClockControls.dimBrightnessField;
  els.setSensorDimBrightness = sensorClockControls.dimBrightness;
  els.setSensorDimBrightnessVal = sensorClockControls.dimBrightnessVal;
  els.setSensorClockBrightnessDay = sensorClockControls.clockBrightnessDay;
  els.setSensorClockBrightnessDayVal = sensorClockControls.clockBrightnessDayVal;
  els.setSensorClockBrightnessNight = sensorClockControls.clockBrightnessNight;
  els.setSensorClockBrightnessNightVal = sensorClockControls.clockBrightnessNightVal;
  els.setSensorClockBrightnessField = sensorClockControls.brightnessField;
  syncClockScreensaverControls();
  syncMediaPlayerSleepPreventionUi();

  var ssBadge = document.createElement("span");
  ssBadge.setAttribute("aria-label", "Screensaver on");
  ssBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
  els.setScreensaverBadge = ssBadge;

  function setSsMode(mode) {
    ssMode = mode;
    disabledBtn.className = mode === "disabled" ? "active" : "";
    timerBtn.className = mode === "timer" ? "active" : "";
    sensorBtn.className = mode === "sensor" ? "active" : "";
    timerPanel.style.display = mode === "timer" ? "" : "none";
    sensorPanel.style.display = mode === "sensor" ? "" : "none";
    if (els.setScreensaverBadge) {
      els.setScreensaverBadge.className = "sp-card-badge" + (mode === "disabled" ? " sp-hidden" : "");
    }
  }
  disabledBtn.addEventListener("click", function () {
    setSsMode("disabled");
    state.screensaverMode = "disabled";
    postText("Screensaver Mode", "disabled");
  });
  timerBtn.addEventListener("click", function () {
    setSsMode("timer");
    state.screensaverMode = "timer";
    postText("Screensaver Mode", "timer");
  });
  sensorBtn.addEventListener("click", function () {
    setSsMode("sensor");
    state.screensaverMode = "sensor";
    postText("Screensaver Mode", "sensor");
  });
  els.setSsMode = setSsMode;
  setSsMode(ssMode);

  var screensaverCard = makeCollapsibleCard("Screensaver", ssBody, true, ssBadge);

  var idleBody = document.createElement("div");
  idleBody.appendChild(fieldLabel("Return Home After"));
  var hsSelect = document.createElement("select");
  hsSelect.className = "sp-select";
  hsSelect.id = "sp-set-hs-timeout";
  var hsOptions = [
    { label: "Disabled", value: 0 },
    { label: "10 seconds", value: 10 },
    { label: "20 seconds", value: 20 },
    { label: "30 seconds", value: 30 },
    { label: "1 minute", value: 60 },
    { label: "2 minutes", value: 120 },
    { label: "5 minutes", value: 300 },
  ];
  hsOptions.forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt.value;
    o.textContent = opt.label;
    if (opt.value === state.homeScreenTimeout) o.selected = true;
    hsSelect.appendChild(o);
  });
  hsSelect.addEventListener("change", function () {
    state.homeScreenTimeout = parseFloat(this.value) || 0;
    syncIdleUi();
    postNumber("Home Screen Timeout", this.value);
  });
  idleBody.appendChild(hsSelect);
  els.setHSTimeout = hsSelect;
  var idleBadge = document.createElement("span");
  idleBadge.setAttribute("aria-label", "Idle on");
  idleBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
  els.setIdleBadge = idleBadge;
  syncIdleUi();
  config.appendChild(makeCollapsibleCard("Idle", idleBody, true, idleBadge));
  config.appendChild(screensaverCard);
  config.appendChild(scheduleCard);

  var backupBody = document.createElement("div");

  var backupRow = document.createElement("div");
  backupRow.className = "sp-backup-btns";

  var exportBtn = document.createElement("button");
  exportBtn.className = "sp-backup-btn";
  exportBtn.innerHTML = '<span class="mdi mdi-download"></span>Export';
  exportBtn.addEventListener("click", exportConfig);
  backupRow.appendChild(exportBtn);

  var importBtn = document.createElement("button");
  importBtn.className = "sp-backup-btn";
  importBtn.innerHTML = '<span class="mdi mdi-upload"></span>Import';
  importBtn.addEventListener("click", importConfig);
  backupRow.appendChild(importBtn);

  backupBody.appendChild(backupRow);
  config.appendChild(timeSettingsCard);
  config.appendChild(makeCollapsibleCard("Backup", backupBody, true));

  var fwBody = document.createElement("div");

  var fwVersionRow = document.createElement("div");
  fwVersionRow.className = "sp-fw-row";
  var fwVersionLabel = document.createElement("span");
  fwVersionLabel.className = "sp-fw-version";
  fwVersionRow.appendChild(fwVersionLabel);
  els.fwVersionLabel = fwVersionLabel;
  renderFirmwareVersion();
  refreshFirmwareVersion();

  var fwActions = document.createElement("div");
  fwActions.className = "sp-fw-actions";
  els.fwActions = fwActions;
  var fwInlineStatus = document.createElement("span");
  fwInlineStatus.className = "sp-fw-inline-status";
  fwActions.appendChild(fwInlineStatus);
  els.fwInlineStatus = fwInlineStatus;

  var fwCheckBtn = document.createElement("button");
  fwCheckBtn.className = "sp-fw-btn";
  fwCheckBtn.textContent = "Check for Update";
  fwCheckBtn.addEventListener("click", function () {
    if (!firmwareUpdateControlsVisible()) return;
    if (firmwareInstallAvailable()) {
      var updateReady = firmwareUpdateAvailable();
      state.firmwareInstallTargetVersion = state.firmwareLatestVersion;
      state.firmwareInstallPostPending = !updateReady;
      state.firmwareUpdateState = "INSTALLING";
      state.firmwareChecking = false;
      renderFirmwareUpdateStatus();
      if (updateReady) {
        postFirmwareUpdateInstall();
      } else {
        postFirmwareUpdateCheck();
      }
      startFirmwareInstallRefresh();
      return;
    }
    state.firmwareChecking = true;
    renderFirmwareUpdateStatus();
    postFirmwareUpdateCheck();
    getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
      setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
    });
    setTimeout(function () {
      state.firmwareChecking = false;
      refreshFirmwareVersion();
      renderFirmwareUpdateStatus();
    }, 10000);
  });
  fwActions.appendChild(fwCheckBtn);
  fwVersionRow.appendChild(fwActions);
  els.fwCheckBtn = fwCheckBtn;
  fwBody.appendChild(fwVersionRow);

  var fwStatus = document.createElement("div");
  fwStatus.className = "sp-fw-status";
  fwBody.appendChild(fwStatus);
  els.fwStatus = fwStatus;
  renderFirmwareUpdateStatus();

  var autoUpdateToggle = toggleRow("Auto Update", "sp-set-auto-update", state.autoUpdate);
  fwBody.appendChild(autoUpdateToggle.row);
  autoUpdateToggle.input.addEventListener("change", function () {
    if (!firmwareUpdateControlsVisible()) {
      syncFirmwareUpdateUi();
      return;
    }
    postSwitch("Firmware: Auto Update", this.checked);
    syncFirmwareUpdateUi();
  });
  els.setAutoUpdateRow = autoUpdateToggle.row;
  els.setAutoUpdate = autoUpdateToggle.input;

  var freqWrap = document.createElement("div");
  freqWrap.style.display = state.autoUpdate ? "" : "none";
  var freqSelect = document.createElement("select");
  freqSelect.className = "sp-select";
  freqSelect.id = "sp-set-update-freq";
  state.updateFreqOptions.forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt;
    o.textContent = opt;
    freqSelect.appendChild(o);
  });
  freqSelect.value = state.updateFrequency;
  freqSelect.addEventListener("change", function () {
    if (!firmwareUpdateControlsVisible()) return;
    postSelect("Firmware: Update Frequency", this.value);
  });
  freqWrap.appendChild(freqSelect);
  fwBody.appendChild(freqWrap);
  els.updateFreqWrap = freqWrap;
  els.setUpdateFreq = freqSelect;
  syncFirmwareUpdateUi();

  config.appendChild(makeCollapsibleCard("Firmware", fwBody, true));

  if (developerExperimentalUrlFlag()) {
    var devBody = document.createElement("div");
    var experimentalToggle = toggleRow(
      "Developer/Experimental Features",
      "sp-set-developer-experimental-features",
      state.developerExperimentalFeatures
    );
    devBody.appendChild(experimentalToggle.row);
    experimentalToggle.input.addEventListener("change", function () {
      state.developerExperimentalFeatures = this.checked;
      postSwitch("Developer: Experimental Features", state.developerExperimentalFeatures);
      scheduleRender();
    });
    els.setDeveloperExperimentalFeatures = experimentalToggle.input;
    config.appendChild(makeCollapsibleCard("Developer", devBody, true));
  }

  page.appendChild(config);
  page.appendChild(buildApplyBar());

  parent.appendChild(page);
  els.settingsPage = page;
}

// ── Settings helpers ───────────────────────────────────────────────────

function makeCollapsibleCard(title, bodyElement, defaultCollapsed, badgeElement) {
  var card = document.createElement("div");
  card.className = "card";
  var header = document.createElement("div");
  header.className = "card-header";
  var h3 = document.createElement("h3");
  h3.textContent = title;
  var rightWrap = document.createElement("div");
  rightWrap.className = "card-header-right";
  var chevron = document.createElement("span");
  chevron.className = "card-chevron";
  chevron.innerHTML = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9l6 6 6-6"/></svg>';
  if (badgeElement) rightWrap.appendChild(badgeElement);
  rightWrap.appendChild(chevron);
  header.appendChild(h3);
  header.appendChild(rightWrap);
  var body = document.createElement("div");
  body.className = "card-body";
  body.appendChild(bodyElement);
  card.appendChild(header);
  card.appendChild(body);
  if (defaultCollapsed) card.classList.add("collapsed");
  header.onclick = function () { card.classList.toggle("collapsed"); };
  return card;
}

function fieldLabel(text, forId) {
  var el = document.createElement("label");
  el.className = "sp-field-label";
  el.textContent = text;
  if (forId) el.htmlFor = forId;
  return el;
}

function textInput(id, value, placeholder) {
  var el = document.createElement("input");
  el.type = "text";
  el.className = "sp-input";
  if (id) el.id = id;
  el.value = value;
  el.placeholder = placeholder || "";
  return el;
}

function colorField(id, value, onChange) {
  var row = document.createElement("div");
  row.className = "sp-color-row";

  var swatch = document.createElement("div");
  swatch.className = "sp-color-swatch";
  swatch.style.backgroundColor = "#" + (value.length === 6 ? value : "000000");

  var picker = document.createElement("input");
  picker.type = "color";
  picker.value = "#" + (value.length === 6 ? value : "000000");
  swatch.appendChild(picker);
  row.appendChild(swatch);

  var inp = document.createElement("input");
  inp.type = "text";
  inp.className = "sp-input";
  inp.id = id;
  inp.value = value;
  inp.placeholder = "6-digit hex e.g. FF8C00";
  row.appendChild(inp);

  picker.addEventListener("input", function () {
    var hex = this.value.replace("#", "").toUpperCase();
    inp.value = hex;
    swatch.style.backgroundColor = "#" + hex;
    onChange(hex);
  });

  inp.addEventListener("blur", function () {
    var hex = this.value.replace(/^#/, "").toUpperCase();
    if (/^[0-9A-F]{6}$/i.test(hex)) {
      swatch.style.backgroundColor = "#" + hex;
      picker.value = "#" + hex;
    }
    onChange(hex);
  });
  inp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });

  row._syncColor = function (hex) {
    if (document.activeElement !== inp) inp.value = hex;
    swatch.style.backgroundColor = "#" + (hex.length === 6 ? hex : "000000");
    picker.value = "#" + (hex.length === 6 ? hex : "000000");
  };

  return row;
}

function toggleRow(label, id, checked) {
  var row = document.createElement("div");
  row.className = "sp-toggle-row";
  var lbl = document.createElement("span");
  lbl.className = "sp-toggle-label";
  lbl.textContent = label;
  row.appendChild(lbl);
  var toggle = document.createElement("label");
  toggle.className = "sp-toggle";
  var inp = document.createElement("input");
  inp.type = "checkbox";
  inp.id = id;
  inp.checked = !!checked;
  var track = document.createElement("span");
  track.className = "sp-toggle-track";
  toggle.appendChild(inp);
  toggle.appendChild(track);
  row.appendChild(toggle);
  return { row: row, input: inp };
}

function condField() {
  var el = document.createElement("div");
  el.className = "sp-cond-field";
  return el;
}

function createRangeSlider(label, initial, postName) {
  var wrap = document.createElement("div");
  wrap.className = "sp-field";
  wrap.appendChild(fieldLabel(label));
  var row = document.createElement("div");
  row.className = "sp-range-row";
  var range = document.createElement("input");
  range.type = "range";
  range.className = "sp-range";
  range.min = "10";
  range.max = "100";
  range.step = "5";
  range.value = String(initial);
  var val = document.createElement("span");
  val.className = "sp-range-val";
  val.textContent = initial + "%";
  range.addEventListener("input", function () { val.textContent = this.value + "%"; });
  range.addEventListener("change", function () {
    if (typeof postName === "function") postName(this.value);
    else if (postName) postNumber(postName, this.value);
  });
  row.appendChild(range);
  row.appendChild(val);
  wrap.appendChild(row);
  return { wrap: wrap, range: range, val: val };
}

function syncClockScreensaverControls() {
  var mode = normalizeScreensaverAction(state.screensaverAction);
  var dayBrightness = Math.round(state.clockBrightnessDay) + "%";
  var nightBrightness = Math.round(state.clockBrightnessNight) + "%";
  var dimBrightness = Math.round(state.screensaverDimmedBrightness) + "%";
  var clockDisplay = mode === "clock" ? "" : "none";
  var dimDisplay = mode === "dim" ? "" : "none";

  state.clockScreensaverOn = mode === "clock";

  if (els.setClockSelect) els.setClockSelect.value = mode;
  if (els.setSensorClockSelect) els.setSensorClockSelect.value = mode;
  syncOptionalClockBrightness(els.setClockBrightnessField, els.setDimBrightnessField || els.setClockField, clockDisplay);
  syncOptionalClockBrightness(els.setSensorClockBrightnessField, els.setSensorDimBrightnessField || els.setSensorClockField, clockDisplay);
  syncOptionalClockBrightness(els.setDimBrightnessField, els.setClockField, dimDisplay);
  syncOptionalClockBrightness(els.setSensorDimBrightnessField, els.setSensorClockField, dimDisplay);
  if (els.setDimBrightness) {
    els.setDimBrightness.value = state.screensaverDimmedBrightness;
    els.setDimBrightnessVal.textContent = dimBrightness;
  }
  if (els.setSensorDimBrightness) {
    els.setSensorDimBrightness.value = state.screensaverDimmedBrightness;
    els.setSensorDimBrightnessVal.textContent = dimBrightness;
  }
  if (els.setClockBrightnessDay) {
    els.setClockBrightnessDay.value = state.clockBrightnessDay;
    els.setClockBrightnessDayVal.textContent = dayBrightness;
  }
  if (els.setClockBrightnessNight) {
    els.setClockBrightnessNight.value = state.clockBrightnessNight;
    els.setClockBrightnessNightVal.textContent = nightBrightness;
  }
  if (els.setSensorClockBrightnessDay) {
    els.setSensorClockBrightnessDay.value = state.clockBrightnessDay;
    els.setSensorClockBrightnessDayVal.textContent = dayBrightness;
  }
  if (els.setSensorClockBrightnessNight) {
    els.setSensorClockBrightnessNight.value = state.clockBrightnessNight;
    els.setSensorClockBrightnessNightVal.textContent = nightBrightness;
  }
}

function syncMediaPlayerSleepPreventionUi() {
  if (els.setMediaPlayerSleepPreventionToggle) {
    els.setMediaPlayerSleepPreventionToggle.checked = !!state.mediaPlayerSleepPreventionOn;
  }
  if (els.setMediaPlayerSleepPreventionField) {
    els.setMediaPlayerSleepPreventionField.classList.toggle("sp-visible", !!state.mediaPlayerSleepPreventionOn);
  }
}

function syncOptionalClockBrightness(field, previousField, display) {
  if (field) field.style.display = display;
  if (previousField) previousField.style.marginBottom = display === "none" ? "20px" : "";
}

function createScreensaverThenControls(selectId) {
  var clockField = document.createElement("div");
  clockField.className = "sp-field";
  clockField.appendChild(fieldLabel("Then", selectId));
  var clockSelect = document.createElement("select");
  clockSelect.className = "sp-select";
  clockSelect.id = selectId;
  [
    { value: "off", label: "Display Off" },
    { value: "dim", label: "Screen Dimmed" },
    { value: "clock", label: "Clock" },
  ].forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt.value;
    o.textContent = opt.label;
    clockSelect.appendChild(o);
  });
  clockSelect.value = normalizeScreensaverAction(state.screensaverAction);
  clockSelect.addEventListener("change", function () {
    state.screensaverAction = normalizeScreensaverAction(this.value);
    state.clockScreensaverOn = state.screensaverAction === "clock";
    syncClockScreensaverControls();
    postScreensaverAction(state.screensaverAction);
    postSwitch("Screen Saver: Clock", state.clockScreensaverOn);
  });
  clockField.appendChild(clockSelect);

  var dimBrightnessField = document.createElement("div");
  dimBrightnessField.style.display = normalizeScreensaverAction(state.screensaverAction) === "dim" ? "" : "none";
  var dimSlider = createRangeSlider("Dimmed Screen Brightness", state.screensaverDimmedBrightness, postScreensaverDimmedBrightness);
  dimSlider.range.min = "1";
  dimSlider.range.step = "1";
  dimSlider.range.addEventListener("input", function () {
    state.screensaverDimmedBrightness = normalizeScreensaverDimmedBrightness(this.value);
    syncClockScreensaverControls();
  });
  dimBrightnessField.appendChild(dimSlider.wrap);

  var clockBrightnessField = document.createElement("div");
  clockBrightnessField.style.display = normalizeScreensaverAction(state.screensaverAction) === "clock" ? "" : "none";
  var daySlider = createRangeSlider("Daytime Clock Brightness", state.clockBrightnessDay, postClockBrightnessDay);
  daySlider.range.min = "1";
  daySlider.range.step = "1";
  daySlider.range.addEventListener("input", function () {
    state.clockBrightnessDay = normalizeClockBrightness(this.value, 35);
    syncClockScreensaverControls();
  });
  clockBrightnessField.appendChild(daySlider.wrap);
  var nightSlider = createRangeSlider("Nighttime Clock Brightness", state.clockBrightnessNight, postClockBrightnessNight);
  nightSlider.range.min = "1";
  nightSlider.range.step = "1";
  nightSlider.range.addEventListener("input", function () {
    state.clockBrightnessNight = normalizeClockBrightness(this.value, state.clockBrightnessDay);
    syncClockScreensaverControls();
  });
  clockBrightnessField.appendChild(nightSlider.wrap);

  return {
    clockField: clockField,
    clockSelect: clockSelect,
    dimBrightnessField: dimBrightnessField,
    dimBrightness: dimSlider.range,
    dimBrightnessVal: dimSlider.val,
    brightnessField: clockBrightnessField,
    clockBrightnessDay: daySlider.range,
    clockBrightnessDayVal: daySlider.val,
    clockBrightnessNight: nightSlider.range,
    clockBrightnessNightVal: nightSlider.val,
  };
}

function createHourSelect(label, id, initial, onChange) {
  var wrap = document.createElement("div");
  wrap.className = "sp-field";
  wrap.appendChild(fieldLabel(label, id));
  var select = document.createElement("select");
  select.className = "sp-select";
  select.id = id;
  for (var h = 0; h < 24; h++) {
    var o = document.createElement("option");
    o.value = String(h);
    o.textContent = formatHour(h);
    select.appendChild(o);
  }
  select.value = String(normalizeHour(initial, 0));
  select.addEventListener("change", function () {
    onChange(normalizeHour(this.value, 0));
  });
  wrap.appendChild(select);
  return { wrap: wrap, select: select };
}

function createEntityToggleSection(label, id, checked, switchName, entityLabel, entityPostName, placeholder) {
  var toggle = toggleRow(label, id, checked);
  var field = condField();
  var inp = entityInput("", "", placeholder, ["sensor"]);
  field.appendChild(inp);
  toggle.input.addEventListener("change", function () { postSwitch(switchName, this.checked); });
  bindTextPost(inp, entityPostName, {});
  return { toggle: toggle, field: field, input: inp };
}

function buildApplyBar() {
  var bar = document.createElement("div");
  bar.className = "sp-apply-bar";
  var btn = document.createElement("button");
  btn.className = "sp-apply-btn";
  btn.textContent = "Apply Configuration";
  btn.addEventListener("click", function () {
    if (isConfigLocked()) return;
    if (document.activeElement && document.activeElement.blur) {
      document.activeElement.blur();
    }
    setConfigLocked(true, "Restarting device\u2026");
    setTimeout(function () {
      postButtonPress("Apply Configuration");
      waitForReboot();
    }, 600);
  });
  bar.appendChild(btn);
  var note = document.createElement("div");
  note.className = "sp-apply-note";
  note.textContent = "Restarts the device to apply changes";
  bar.appendChild(note);
  return bar;
}

function switchTab(tab) {
  state.activeTab = tab;
  ["screen", "settings"].forEach(function (t) {
    els["tab_" + t].className = "sp-tab" + (tab === t ? " active" : "");
    els["tab_" + t].setAttribute("aria-selected", tab === t ? "true" : "false");
  });
  els.screenPage.className = "sp-page" + (tab === "screen" ? " active" : "");
  els.settingsPage.className = "sp-page" + (tab === "settings" ? " active" : "");
}

function isConfigLocked() {
  return !!state.configLocked;
}

function syncConfigLockUi() {
  if (els.root) {
    els.root.classList.toggle("sp-config-locked", isConfigLocked());
  }
  if (els.previewMain) {
    els.previewMain.setAttribute("aria-disabled", isConfigLocked() ? "true" : "false");
  }
  if (els.root) {
    var text = "Waiting for device\u2026";
    if ((state.configLockReason || "").indexOf("Restarting") !== -1) text = "Restarting\u2026";
    els.root.querySelectorAll(".sp-apply-btn").forEach(function (btn) {
      btn.disabled = isConfigLocked();
      btn.textContent = isConfigLocked() ? text : "Apply Configuration";
    });
  }
  updatePreviewHint();
}

function setConfigLocked(locked, reason) {
  var nextLocked = !!locked;
  state.configLocked = nextLocked;
  state.configLockReason = nextLocked ? (reason || "Reconnecting to device\u2026") : "";

  if (nextLocked) {
    hideContextMenu();
    hideSettingsOverlay();
    state.settingsDraft = null;
    state.selectedSlots = [];
    state.lastClickedSlot = -1;
    state.subpageSelectedSlots = [];
    state.subpageLastClicked = -1;
    if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
    dragSrcPos = -1;
    previewDropIdx = -1;
    clearPlaceholder();
  }

  syncConfigLockUi();
  if (els.previewMain) renderPreview();
}

// ── Preview rendering (unified) ────────────────────────────────────────

function previewHtmlValue(typePreview, key, fallback) {
  return typePreview && Object.prototype.hasOwnProperty.call(typePreview, key)
    ? typePreview[key]
    : fallback;
}

function renderPreview() {
  var main = els.previewMain;
  main.innerHTML = "";
  var c = ctx();

  updatePreviewHint(c);

  for (var pos = 0; pos < c.maxSlots; pos++) {
    var slot = c.grid[pos];
    if (slot === -1) continue;

    if (slot === -2) {
      var backBtn = document.createElement("div");
      var bkSz = c.sizes[-2];
      var backLabel = c.isSub ? (getSubpage(state.editingSubpage).backLabel || "Back") : "Back";
      backBtn.className = "sp-btn sp-back-btn" + sizeClass(bkSz) +
        (c.selected.indexOf(-2) !== -1 ? " sp-selected" : "");
      backBtn.innerHTML =
        '<span class="sp-btn-icon sp-back-hit mdi mdi-chevron-left"></span>' +
        '<span class="sp-btn-label">' + escHtml(backLabel) + '</span>';
      backBtn.style.backgroundColor = "#" + (state.offColor.length === 6 ? state.offColor : "313131");
      backBtn.style.cursor = "pointer";
      backBtn.setAttribute("data-pos", pos);
      backBtn.draggable = !isConfigLocked();
      main.appendChild(backBtn);
    } else if (slot > 0) {
      var bIdx = slot - 1;
      if (c.isSub && bIdx >= c.buttons.length) continue;
      var b = c.buttons[bIdx];
      var iconName = resolveIcon(b);
      var label = b.label || b.entity || "Configure";
      var color = (b.type === "sensor" || b.type === "weather" || b.type === "weather_forecast" || b.type === "calendar" || b.type === "timezone")
        ? state.sensorColor : state.offColor;
      var previewTypeDef = BUTTON_TYPES[b.type || ""] || null;
      if (previewTypeDef && c.isSub && !previewTypeDef.allowInSubpage) previewTypeDef = null;
      if (previewTypeDef && previewTypeDef.experimental && !isExperimentalEnabled(previewTypeDef.experimental)) {
        previewTypeDef = null;
        iconName = "cog";
        label = "Configure";
      }
      var slotSz = c.sizes[slot];
      var typePreview = previewTypeDef && previewTypeDef.renderPreview
        ? previewTypeDef.renderPreview(b, { escHtml: escHtml, cardSize: slotSz || 1 })
        : null;

      var btn = document.createElement("div");
      btn.className = "sp-btn" + sizeClass(slotSz) +
        (c.selected.indexOf(slot) !== -1 ? " sp-selected" : "");
      btn.style.backgroundColor = "#" + (color.length === 6 ? color : "313131");
      btn.draggable = !isConfigLocked();
      btn.setAttribute("data-pos", pos);
      btn.setAttribute("data-slot", slot);
      var hasWhenOn = !typePreview && (b.sensor || (b.icon_on && b.icon_on !== "Auto"));
      var badgeIcon = b.sensor ? "gauge" : "swap-horizontal";
      var sensorBadge = hasWhenOn
        ? '<span class="sp-sensor-badge mdi mdi-' + badgeIcon + '"></span>'
        : '';
      var labelHtml = previewHtmlValue(typePreview, "labelHtml",
        '<span class="sp-btn-label">' + escHtml(label) + '</span>');
      var iconHtml = previewHtmlValue(typePreview, "iconHtml",
        '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>');
      btn.innerHTML =
        sensorBadge +
        iconHtml +
        labelHtml;
      main.appendChild(btn);
    } else {
      var empty = document.createElement("div");
      empty.className = "sp-empty-cell";
      empty.setAttribute("data-pos", pos);
      empty.innerHTML = '<span class="sp-add-pill"><span class="sp-add-icon mdi mdi-plus"></span></span>';
      main.appendChild(empty);
    }
  }
  renderSelectionBar(c);
}

// ── Button settings panel (unified) ────────────────────────────────────

function hideSettingsOverlay() {
  if (els.settingsOverlay) els.settingsOverlay.classList.remove("sp-visible");
}

function updatePreviewHint(c) {
  if (!els.previewHint) return;
  c = c || ctx();
  els.previewHint.style.display = "";
  if (isConfigLocked()) {
    els.previewHint.textContent = "Editing is paused while the device reconnects";
  } else if (c.selected.length > 1) {
    els.previewHint.textContent = c.selected.length + " buttons selected \u2022 right click to copy, cut, or delete";
  } else {
    els.previewHint.textContent = "tap to select \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
  }
}

function renderSelectionBar(c) {
  if (!els.selectionBar) return;
  c = c || ctx();
  els.selectionBar.innerHTML = "";
  if (isConfigLocked() || !c.selected.length) {
    els.selectionBar.className = "sp-selection-bar";
    return;
  }

  els.selectionBar.className = "sp-selection-bar sp-visible";

  var label = document.createElement("span");
  label.className = "sp-selection-label";
  label.textContent = c.selected.length === 1 ? "1 card selected" : c.selected.length + " cards selected";
  els.selectionBar.appendChild(label);

  var actions = document.createElement("div");
  actions.className = "sp-selection-actions";

  if (c.selected.length === 1) {
    var editBtn = document.createElement("button");
    editBtn.type = "button";
    editBtn.className = "sp-selection-btn sp-selection-btn-primary";
    editBtn.innerHTML = '<span class="mdi mdi-pencil"></span>Edit';
    editBtn.addEventListener("click", function (e) {
      e.preventDefault();
      e.stopPropagation();
      openSelectedCardSettings();
    });
    actions.appendChild(editBtn);
  }

  var menuBtn = document.createElement("button");
  menuBtn.type = "button";
  menuBtn.className = "sp-selection-btn";
  menuBtn.setAttribute("aria-label", "Card actions");
  menuBtn.innerHTML = '<span class="mdi mdi-dots-horizontal"></span>';
  menuBtn.addEventListener("click", function (e) {
    e.preventDefault();
    e.stopPropagation();
    showSelectionMenu(e);
  });
  actions.appendChild(menuBtn);
  els.selectionBar.appendChild(actions);
}

function closeSettings() {
  hideSettingsOverlay();
  _settingsDeferred = false;
  state.settingsDraft = null;
  ctx().setSelected([]);
  renderPreview();
}

function clearCardSelection() {
  var c = ctx();
  if (!c.selected.length && c.getLastClicked() < 0) return;
  c.setSelected([]);
  c.setLastClicked(-1);
  hideSettingsOverlay();
  renderPreview();
  renderButtonSettings();
}

function isSelectionControlTarget(target) {
  return !!(
    (els.previewMain && els.previewMain.contains(target)) ||
    (els.selectionBar && els.selectionBar.contains(target)) ||
    (els.settingsOverlay && els.settingsOverlay.contains(target)) ||
    (ctxMenu && ctxMenu.contains(target)) ||
    (target.closest && target.closest(".sp-ctx-menu"))
  );
}

function handleDocumentSelectionMouseDown(e) {
  if (e.button !== 0) return;
  if (isSelectionControlTarget(e.target)) return;
  clearCardSelection();
}

function openSelectedCardSettings() {
  if (isConfigLocked()) return;
  var c = ctx();
  if (c.selected.length !== 1) return;
  renderButtonSettings(true);
}

function openCardSettings(slot) {
  if (isConfigLocked()) return;
  var c = ctx();
  if ((slot > 0 || (slot === -2 && c.isSub)) && c.selected.indexOf(slot) === -1) {
    c.setSelected([slot]);
    c.setLastClicked(slot);
    renderPreview();
  }
  renderButtonSettings(true);
}

function renderBackButtonSettings(container, c) {
  if (!c.isSub || c.selected[0] !== -2) return false;
  if (els.settingsOverlay) els.settingsOverlay.classList.add("sp-visible");
  var sp = getSubpage(state.editingSubpage);

  var title = document.createElement("div");
  title.className = "sp-section-title";
  title.textContent = "Back Button";
  container.appendChild(title);

  var panel = document.createElement("div");
  panel.className = "sp-panel";
  var lf = document.createElement("div");
  lf.className = "sp-field";
  lf.appendChild(fieldLabel("Label", "sp-sp-inp-back-label"));
  var labelInp = textInput("sp-sp-inp-back-label", sp.backLabel || "Back", "Back");
  lf.appendChild(labelInp);
  panel.appendChild(lf);

  function saveBackLabel() {
    sp.backLabel = labelInp.value || "Back";
    saveSubpageConfig(state.editingSubpage);
    renderPreview();
  }
  labelInp.addEventListener("input", saveBackLabel);
  labelInp.addEventListener("change", saveBackLabel);
  labelInp.addEventListener("blur", saveBackLabel);
  labelInp.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      saveBackLabel();
      this.blur();
    }
  });

  var doneRow = document.createElement("div");
  doneRow.className = "sp-btn-row sp-btn-row--save";
  var doneBtn = document.createElement("button");
  doneBtn.type = "button";
  doneBtn.className = "sp-action-btn sp-save-btn";
  doneBtn.textContent = "Done";
  doneBtn.addEventListener("click", closeSettings);
  doneRow.appendChild(doneBtn);
  panel.appendChild(doneRow);

  container.appendChild(panel);
  return true;
}

function renderButtonSettings(forceOpen) {
  var container = els.buttonSettings;
  container.innerHTML = "";
  var c = ctx();

  if (isConfigLocked()) {
    hideSettingsOverlay();
    return;
  }

  if (c.selected.length === 0) {
    hideSettingsOverlay();
    return;
  }

  if (c.selected.length > 1) {
    hideSettingsOverlay();
    return;
  }

  if (!forceOpen && !isSettingsOpen()) {
    hideSettingsOverlay();
    return;
  }

  if (els.settingsOverlay) els.settingsOverlay.classList.add("sp-visible");

  if (renderBackButtonSettings(container, c)) return;

  var slot = c.selected[0];
  var bIdx = slot - 1;
  if (bIdx < 0 || bIdx >= c.buttons.length) return;
  var liveButton = c.buttons[bIdx];
  var draftKey = (c.isSub ? "sub:" + state.editingSubpage : "main") + ":" + slot;

  function cloneButtonConfig(src) {
    return {
      entity: src.entity || "",
      label: src.label || "",
      icon: src.icon || "Auto",
      icon_on: src.icon_on || "Auto",
      sensor: src.sensor || "",
      unit: src.unit || "",
      type: src.type || "",
      precision: src.precision || "",
      options: src.options || "",
      _whenOnActive: src._whenOnActive,
      _whenOnMode: src._whenOnMode,
    };
  }

  function copyButtonConfig(target, src) {
    target.entity = src.entity || "";
    target.label = src.label || "";
    target.icon = src.icon || "Auto";
    target.icon_on = src.icon_on || "Auto";
    target.sensor = src.sensor || "";
    target.unit = src.unit || "";
    target.type = src.type || "";
    target.precision = src.precision || "";
    target.options = src.options || "";
    target._whenOnActive = src._whenOnActive;
    target._whenOnMode = src._whenOnMode;
    normalizeButtonConfig(target);
  }

  if (!state.settingsDraft || state.settingsDraft.key !== draftKey) {
    state.settingsDraft = {
      key: draftKey,
      slot: slot,
      homeSlot: state.editingSubpage,
      isSub: c.isSub,
      dirty: false,
      button: cloneButtonConfig(liveButton),
    };
  }
  var b = state.settingsDraft.button;

  var title = document.createElement("div");
  title.className = "sp-section-title";
  title.textContent = "Settings";
  container.appendChild(title);

  var panel = document.createElement("div");
  panel.className = "sp-panel";

  var idPrefix = c.isSub ? "sp-sp-inp-" : "sp-inp-";
  var requiredFields = [];

  function markDraftDirty() {
    if (state.settingsDraft && state.settingsDraft.key === draftKey) {
      state.settingsDraft.dirty = true;
    }
  }

  function saveField(field, val) {
    markDraftDirty();
  }

  function fieldContainer(input) {
    return input && input.closest ? input.closest(".sp-field") : null;
  }

  function clearFieldError(input) {
    if (!input) return;
    input.classList.remove("sp-input-error");
    input.removeAttribute("aria-invalid");
    input.removeAttribute("aria-describedby");
    var field = fieldContainer(input);
    if (field) field.classList.remove("sp-field-invalid");
    var existing = field ? field.querySelector(".sp-field-error") : null;
    if (existing && existing.parentNode) existing.parentNode.removeChild(existing);
  }

  function showFieldError(input, message) {
    if (!input) return;
    var field = fieldContainer(input);
    input.classList.add("sp-input-error");
    input.setAttribute("aria-invalid", "true");
    if (field) field.classList.add("sp-field-invalid");
    var existing = field ? field.querySelector(".sp-field-error") : null;
    if (!existing && field) {
      existing = document.createElement("div");
      existing.className = "sp-field-error";
      existing.id = (input.id || "sp-field") + "-error";
      field.appendChild(existing);
    }
    if (existing) {
      existing.textContent = message || "Add an entity before saving.";
      input.setAttribute("aria-describedby", existing.id);
    }
  }

  function requireField(input, message, isActive) {
    if (!input) return;
    requiredFields.push({
      input: input,
      message: message || "Add an entity before saving.",
      isActive: isActive || function () { return true; },
    });
    function maybeClearError() {
      if (!isActive || isActive()) {
        if (String(input.value || "").trim()) clearFieldError(input);
      } else {
        clearFieldError(input);
      }
    }
    input.addEventListener("input", maybeClearError);
    input.addEventListener("change", maybeClearError);
  }

  function validateSettingsDraft() {
    var firstInvalid = null;
    for (var i = 0; i < requiredFields.length; i++) {
      var rule = requiredFields[i];
      if (rule.isActive && !rule.isActive()) {
        clearFieldError(rule.input);
        continue;
      }
      if (String(rule.input.value || "").trim()) {
        clearFieldError(rule.input);
        continue;
      }
      if (!firstInvalid) firstInvalid = rule.input;
      showFieldError(rule.input, rule.message);
    }
    if (!firstInvalid) return true;
    firstInvalid.focus();
    return false;
  }

  function validateConfigSize() {
    if (c.isSub) return true;
    if (serializeButtonConfig(b).length <= 255) return true;
    showBanner("Card settings are too large to save. Shorten confirmation text, labels, or entity IDs.", "error");
    return false;
  }

  function applySettingsDraft() {
    if (!state.settingsDraft || state.settingsDraft.key !== draftKey) return;
    copyButtonConfig(liveButton, state.settingsDraft.button);
    state.settingsDraft = null;
    if (c.isSub) {
      saveSubpageConfig(state.editingSubpage);
    } else {
      saveButtonConfig(slot);
    }
    renderPreview();
  }

  function bindField(input, field, rerender) {
    function syncValue() {
      if (b[field] === input.value) return;
      b[field] = input.value;
      markDraftDirty();
    }
    input.addEventListener("input", function () {
      syncValue();
    });
    input.addEventListener("change", syncValue);
    input.addEventListener("blur", syncValue);
    input.addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        syncValue();
        this.blur();
      }
    });
  }

  function makeIconPicker(pickerId, inputId, currentVal, onSelect, labelText) {
    var icf = document.createElement("div");
    icf.className = "sp-field";
    icf.appendChild(fieldLabel(labelText || "Icon", inputId));
    var picker = document.createElement("div");
    picker.className = "sp-icon-picker";
    if (pickerId) picker.id = pickerId;
    picker.innerHTML =
      '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(currentVal) + '"></span>' +
      '<input class="sp-icon-picker-input"' + (inputId ? ' id="' + inputId + '"' : '') +
      ' type="text" placeholder="Search icons\u2026" value="' + escAttr(currentVal) + '" autocomplete="off">' +
      '<div class="sp-icon-dropdown"></div>';
    icf.appendChild(picker);
    initIconPicker(picker, currentVal, onSelect);
    return icf;
  }

  function fieldWithControl(labelText, inputId, control) {
    var field = document.createElement("div");
    field.className = "sp-field";
    field.appendChild(fieldLabel(labelText, inputId));
    if (control) field.appendChild(control);
    return field;
  }

  function optionValue(option) {
    if (Array.isArray(option)) return option[0];
    if (option && typeof option === "object") return option.value;
    return option;
  }

  function optionLabel(option) {
    if (Array.isArray(option)) return option[1];
    if (option && typeof option === "object") return option.label;
    return option;
  }

  function selectField(labelText, inputId, options, value, onChange) {
    var select = document.createElement("select");
    select.className = "sp-select";
    if (inputId) select.id = inputId;
    (options || []).forEach(function (entry) {
      var option = document.createElement("option");
      option.value = optionValue(entry);
      option.textContent = optionLabel(entry);
      select.appendChild(option);
    });
    select.value = value || "";
    if (onChange) select.addEventListener("change", onChange);
    return {
      field: fieldWithControl(labelText, inputId, select),
      select: select,
    };
  }

  function entityField(labelText, inputId, value, placeholder, domains, bindName, rerender, requiredMessage) {
    var input = entityInput(inputId, value, placeholder, domains);
    var field = fieldWithControl(labelText, inputId, input);
    if (bindName) bindField(input, bindName, rerender);
    if (requiredMessage) requireField(input, requiredMessage);
    return {
      field: field,
      input: input,
    };
  }

  function textField(labelText, inputId, value, placeholder, bindName, rerender) {
    var input = textInput(inputId, value, placeholder);
    var field = fieldWithControl(labelText, inputId, input);
    if (bindName) bindField(input, bindName, rerender);
    return {
      field: field,
      input: input,
    };
  }

  function segmentControl(options, value, onSelect) {
    var segment = document.createElement("div");
    segment.className = "sp-segment";
    var buttons = {};
    (options || []).forEach(function (entry) {
      var optValue = optionValue(entry);
      var button = document.createElement("button");
      button.type = "button";
      button.textContent = optionLabel(entry);
      button.classList.toggle("active", optValue === value);
      button.addEventListener("click", function () {
        if (onSelect) onSelect(optValue, button);
      });
      segment.appendChild(button);
      buttons[optValue] = button;
    });
    return {
      segment: segment,
      buttons: buttons,
    };
  }

  function toggleSection(labelText, inputId, checked) {
    return {
      toggle: toggleRow(labelText, inputId, checked),
      section: condField(),
    };
  }

  function precisionField(inputId, value, onChange) {
    return selectField("Unit Precision", inputId, [
      ["0", "10"],
      ["1", "10.2"],
      ["2", "10.21"],
    ], value || "0", onChange);
  }

  var typeDef = BUTTON_TYPES[b.type || ""] || BUTTON_TYPES[""];
  if (typeDef && typeDef.experimental && !isExperimentalEnabled(typeDef.experimental)) {
    typeDef = BUTTON_TYPES[""];
  }
  {
    var typeOpts = [];
    var selectedTypeKey = (typeDef && typeDef.pickerKey) || (b.type || "");
    for (var k in BUTTON_TYPES) {
      var td = BUTTON_TYPES[k];
      if (td.pickerKey && td.pickerKey !== td.key) continue;
      if (c.isSub && !td.allowInSubpage) continue;
      if (td.isAvailable && !td.isAvailable({ isSub: c.isSub }) && selectedTypeKey !== td.key) continue;
      if (td.experimental && !isExperimentalEnabled(td.experimental)) continue;
      typeOpts.push([td.key, td.label]);
    }
    typeOpts.sort(function (a, b) {
      return a[1].localeCompare(b[1]);
    });
    var tf = document.createElement("div");
    tf.className = "sp-field";
    tf.appendChild(fieldLabel("Card", "sp-inp-type"));
    var typeSelect = document.createElement("select");
    typeSelect.className = "sp-select";
    typeSelect.id = "sp-inp-type";
    typeOpts.forEach(function (o) {
      var opt = document.createElement("option");
      opt.value = o[0];
      opt.textContent = o[1];
      if (selectedTypeKey === o[0]) opt.selected = true;
      typeSelect.appendChild(opt);
    });
    typeSelect.addEventListener("change", function () {
      var newType = this.value;
      b.type = newType;
      var td = BUTTON_TYPES[newType];
      if (td && td.onSelect) td.onSelect(b);
      saveField("type", newType);
      renderButtonSettings();
    });
    tf.appendChild(typeSelect);
    panel.appendChild(tf);
  }

  var typeHelpers = {
    makeIconPicker: makeIconPicker,
    iconPickerField: makeIconPicker,
    fieldWithControl: fieldWithControl,
    selectField: selectField,
    entityField: entityField,
    textField: textField,
    segmentControl: segmentControl,
    toggleSection: toggleSection,
    precisionField: precisionField,
    fieldLabel: fieldLabel,
    textInput: textInput,
    entityInput: entityInput,
    bindField: bindField,
    saveField: saveField,
    requireField: requireField,
    clearFieldError: clearFieldError,
    toggleRow: toggleRow,
    cardSize: c.sizes[slot] || 1,
    idPrefix: idPrefix,
  };

  if (typeDef && typeDef.renderSettingsBeforeLabel && (!c.isSub || typeDef.allowInSubpage)) {
    typeDef.renderSettingsBeforeLabel(panel, b, slot, typeHelpers);
  }

  if (!typeDef || !typeDef.hideLabel) {
    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(fieldLabel("Label", idPrefix + "label"));
    var labelPlaceholder = (typeDef && typeDef.labelPlaceholder) || "e.g. Kitchen";
    var labelInp = textInput(idPrefix + "label", b.label, labelPlaceholder);
    lf.appendChild(labelInp);
    panel.appendChild(lf);
    bindField(labelInp, "label", true);
  }

  if (typeDef && typeDef.renderSettings && (!c.isSub || typeDef.allowInSubpage)) {
    typeDef.renderSettings(panel, b, slot, typeHelpers);
  } else {
    // Toggle fallback: entity, icons, sensor data
    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(fieldLabel("Entity", idPrefix + "entity"));
    var entityInp = entityInput(idPrefix + "entity", b.entity, "e.g. light.kitchen", [
      "light", "switch", "input_boolean", "fan"
    ]);
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    bindField(entityInp, "entity", true);
    requireField(entityInp, "Add an entity before saving.");

    panel.appendChild(makeIconPicker(idPrefix + "icon-picker", idPrefix + "icon", b.icon || "Auto", function (opt) {
      b.icon = opt;
      saveField("icon", opt);
    }, "Off Icon"));

    // When-on section
    var hasIconOn = b.icon_on && b.icon_on !== "Auto";
    var hasSensor = !!b.sensor;
    var whenOnEnabled = hasIconOn || hasSensor || !!b._whenOnActive;
    var whenOnMode = b._whenOnMode || (hasSensor ? "sensor" : "icon");

    var whenOnToggle = toggleRow("Active Display", idPrefix + "whenon-toggle", whenOnEnabled);
    panel.appendChild(whenOnToggle.row);

    var whenOnCond = condField();
    if (whenOnEnabled) whenOnCond.classList.add("sp-visible");

    var seg = document.createElement("div");
    seg.className = "sp-segment";
    var btnIcon = document.createElement("button");
    btnIcon.type = "button";
    btnIcon.textContent = "On Icon";
    if (whenOnMode === "icon") btnIcon.classList.add("active");
    var btnSensor = document.createElement("button");
    btnSensor.type = "button";
    btnSensor.textContent = "Numeric";
    if (whenOnMode === "sensor") btnSensor.classList.add("active");
    seg.appendChild(btnIcon);
    seg.appendChild(btnSensor);
    whenOnCond.appendChild(seg);

    // Icon-on section
    var iconOnSection = condField();
    if (whenOnMode === "icon") iconOnSection.classList.add("sp-visible");
    var ionLabel = fieldLabel("On Icon", idPrefix + "icon-on");
    iconOnSection.appendChild(ionLabel);
    var iconOnVal = hasIconOn ? b.icon_on : "Auto";
    var iconOnPicker = document.createElement("div");
    iconOnPicker.className = "sp-icon-picker";
    iconOnPicker.id = idPrefix + "icon-on-picker";
    iconOnPicker.innerHTML =
      '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(iconOnVal) + '"></span>' +
      '<input class="sp-icon-picker-input" id="' + idPrefix + 'icon-on" type="text" ' +
      'placeholder="Search icons\u2026" value="' + escAttr(iconOnVal) + '" autocomplete="off">' +
      '<div class="sp-icon-dropdown"></div>';
    iconOnSection.appendChild(iconOnPicker);
    whenOnCond.appendChild(iconOnSection);

    initIconPicker(iconOnPicker, iconOnVal, function (opt) {
      b.icon_on = opt;
      saveField("icon_on", opt);
    });

    // Sensor section
    var sensorSection = condField();
    if (whenOnMode === "sensor") sensorSection.classList.add("sp-visible");

    var sf = document.createElement("div");
    sf.className = "sp-field";
    sf.appendChild(fieldLabel("Sensor Entity", idPrefix + "sensor"));
    var sensorInp = entityInput(idPrefix + "sensor", b.sensor, "e.g. sensor.printer_percent_complete", [
      "sensor", "binary_sensor", "text_sensor"
    ]);
    sf.appendChild(sensorInp);
    sensorSection.appendChild(sf);

    var uf = document.createElement("div");
    uf.className = "sp-field";
    uf.appendChild(fieldLabel("Unit", idPrefix + "unit"));
    var unitInp = textInput(idPrefix + "unit", b.unit, "e.g. %");
    unitInp.className = "sp-input";
    uf.appendChild(unitInp);
    sensorSection.appendChild(uf);

    var pf = document.createElement("div");
    pf.className = "sp-field";
    pf.appendChild(fieldLabel("Unit Precision", idPrefix + "precision"));
    var precisionSelect = document.createElement("select");
    precisionSelect.className = "sp-select";
    precisionSelect.id = idPrefix + "precision";
    var precOpts = [["0", "10"], ["1", "10.2"], ["2", "10.21"]];
    for (var pi = 0; pi < precOpts.length; pi++) {
      var opt = document.createElement("option");
      opt.value = precOpts[pi][0];
      opt.textContent = precOpts[pi][1];
      precisionSelect.appendChild(opt);
    }
    precisionSelect.value = b.precision || "0";
    precisionSelect.addEventListener("change", function () {
      b.precision = this.value === "0" ? "" : this.value;
      saveField("precision", b.precision);
    });
    pf.appendChild(precisionSelect);
    sensorSection.appendChild(pf);
    whenOnCond.appendChild(sensorSection);

    panel.appendChild(whenOnCond);

    bindField(sensorInp, "sensor", true);
    bindField(unitInp, "unit", false);

    function setWhenOnMode(mode) {
      whenOnMode = mode;
      b._whenOnActive = true;
      b._whenOnMode = mode;
      btnIcon.classList.toggle("active", mode === "icon");
      btnSensor.classList.toggle("active", mode === "sensor");
      iconOnSection.classList.toggle("sp-visible", mode === "icon");
      sensorSection.classList.toggle("sp-visible", mode === "sensor");
      if (mode === "icon") {
        sensorInp.value = "";
        unitInp.value = "";
        precisionSelect.value = "0";
        b.sensor = "";
        b.unit = "";
        b.precision = "";
        saveField("sensor", "");
        saveField("unit", "");
        saveField("precision", "");
      } else {
        b.icon_on = "Auto";
        saveField("icon_on", "Auto");
        var ionPreview = iconOnPicker.querySelector(".sp-icon-picker-preview");
        if (ionPreview) ionPreview.className = "sp-icon-picker-preview mdi mdi-cog";
        var ionInput = iconOnPicker.querySelector(".sp-icon-picker-input");
        if (ionInput) ionInput.value = "Auto";
      }
    }

    btnIcon.addEventListener("click", function () { setWhenOnMode("icon"); });
    btnSensor.addEventListener("click", function () { setWhenOnMode("sensor"); });

    whenOnToggle.input.addEventListener("change", function () {
      if (this.checked) {
        b._whenOnActive = true;
        whenOnCond.classList.add("sp-visible");
      } else {
        b._whenOnActive = false;
        b._whenOnMode = null;
        whenOnCond.classList.remove("sp-visible");
        sensorInp.value = "";
        unitInp.value = "";
        precisionSelect.value = "0";
        b.sensor = "";
        b.unit = "";
        b.precision = "";
        b.icon_on = "Auto";
        saveField("sensor", "");
        saveField("unit", "");
        saveField("precision", "");
        saveField("icon_on", "Auto");
      }
    });
  }

  var saveRow = document.createElement("div");
  saveRow.className = "sp-btn-row sp-btn-row--save";

  var delBtn = document.createElement("button");
  delBtn.className = "sp-action-btn sp-delete-btn";
  delBtn.innerHTML = '<span class="mdi mdi-trash-can-outline"></span>';
  delBtn.addEventListener("click", function () {
    state.settingsDraft = null;
    deleteSlot(slot);
  });
  saveRow.appendChild(delBtn);
  saveRow.classList.add("sp-has-delete");

  var rightGroup = document.createElement("div");
  rightGroup.className = "sp-btn-group-right";
  var editSubBtn = panel.querySelector(".sp-edit-subpage-btn");
  if (editSubBtn) rightGroup.appendChild(editSubBtn);
  var saveBtn = document.createElement("button");
  saveBtn.className = "sp-action-btn sp-save-btn";
  saveBtn.textContent = "Save";
  saveBtn.addEventListener("click", function () {
    if (!validateSettingsDraft()) return;
    if (!validateConfigSize()) return;
    applySettingsDraft();
    closeSettings();
  });
  rightGroup.appendChild(saveBtn);
  saveRow.appendChild(rightGroup);
  panel.appendChild(saveRow);

  container.appendChild(panel);
}

// ── Render debouncing ──────────────────────────────────────────────────

var _renderPending = false;
function scheduleRender() {
  if (_renderPending) return;
  _renderPending = true;
  requestAnimationFrame(function () {
    _renderPending = false;
    renderPreview();
    if (isSettingsOpen() || isSettingsFocused()) {
      _settingsDeferred = true;
    } else {
      renderButtonSettings();
    }
  });
}

var _settingsDeferred = false;
document.addEventListener("focusout", function (e) {
  if (!_settingsDeferred) return;
  if (e.relatedTarget && els.buttonSettings && els.buttonSettings.contains(e.relatedTarget)) return;
  requestAnimationFrame(function () {
    if (isSettingsOpen()) return;
    if (!isSettingsFocused()) {
      _settingsDeferred = false;
      renderButtonSettings();
    }
  });
});

document.addEventListener("keydown", function (e) {
  if (e.key === "Escape" && els.settingsOverlay &&
      els.settingsOverlay.classList.contains("sp-visible")) {
    closeSettings();
  }
});

// ── Icon picker (optimized) ────────────────────────────────────────────

function initIconPicker(picker, currentIcon, onSelect) {
  var input = picker.querySelector(".sp-icon-picker-input");
  var dropdown = picker.querySelector(".sp-icon-dropdown");
  var preview = picker.querySelector(".sp-icon-picker-preview");
  var highlighted = -1;
  var optionEls = null;
  var emptyEl = null;

  function ensureBuilt() {
    if (optionEls) return;
    optionEls = [];
    var frag = document.createDocumentFragment();
    ICON_OPTIONS.forEach(function (opt) {
      var row = document.createElement("div");
      row.className = "sp-icon-option" + (opt === currentIcon ? " sp-active" : "");
      row.innerHTML =
        '<span class="sp-icon-option-icon mdi mdi-' + iconSlug(opt) + '"></span>' +
        '<span class="sp-icon-option-label">' + escHtml(opt) + '</span>';
      row._lcName = opt.toLowerCase();
      row._optName = opt;
      row.addEventListener("mousedown", function (e) {
        e.preventDefault();
        selectOpt(opt);
      });
      frag.appendChild(row);
      optionEls.push(row);
    });
    emptyEl = document.createElement("div");
    emptyEl.className = "sp-icon-option sp-icon-option--empty";
    emptyEl.textContent = "No matches";
    emptyEl.style.display = "none";
    frag.appendChild(emptyEl);
    dropdown.appendChild(frag);
  }

  function filterOpts(filter) {
    ensureBuilt();
    highlighted = -1;
    var lc = (filter || "").toLowerCase();
    var hasMatch = false;
    for (var i = 0; i < optionEls.length; i++) {
      var match = !lc || optionEls[i]._lcName.indexOf(lc) !== -1;
      optionEls[i].style.display = match ? "" : "none";
      optionEls[i].classList.remove("sp-highlighted");
      if (match) hasMatch = true;
    }
    emptyEl.style.display = hasMatch ? "none" : "";
  }

  function setPickerIcon(opt) {
    currentIcon = opt;
    input.value = opt;
    preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(opt);
    if (optionEls) {
      for (var i = 0; i < optionEls.length; i++) {
        optionEls[i].classList.toggle("sp-active", optionEls[i]._optName === opt);
      }
    }
  }

  function selectOpt(opt) {
    setPickerIcon(opt);
    closePicker();
    onSelect(opt);
  }
  picker._setIcon = setPickerIcon;

  function openPicker() {
    if (currentIcon === "Auto") {
      input.value = "";
      filterOpts("");
    } else {
      filterOpts(currentIcon);
      setTimeout(function () { input.select(); }, 0);
    }
    picker.classList.add("sp-open");
  }

  function closePicker() {
    picker.classList.remove("sp-open");
    input.value = currentIcon;
    highlighted = -1;
  }

  function getVisible() {
    var vis = [];
    if (optionEls) {
      for (var i = 0; i < optionEls.length; i++) {
        if (optionEls[i].style.display !== "none") vis.push(optionEls[i]);
      }
    }
    return vis;
  }

  function highlightAt(idx) {
    var visible = getVisible();
    if (visible.length === 0) return;
    if (optionEls) optionEls.forEach(function (el) { el.classList.remove("sp-highlighted"); });
    if (idx < 0) idx = visible.length - 1;
    if (idx >= visible.length) idx = 0;
    highlighted = idx;
    visible[highlighted].classList.add("sp-highlighted");
    visible[highlighted].scrollIntoView({ block: "nearest" });
  }

  input.addEventListener("focus", openPicker);
  input.addEventListener("blur", closePicker);

  input.addEventListener("input", function () {
    filterOpts(this.value);
    var vis = getVisible();
    if (vis.length > 0) highlightAt(0);
  });

  input.addEventListener("keydown", function (e) {
    if (e.key === "ArrowDown") {
      e.preventDefault();
      if (!picker.classList.contains("sp-open")) { openPicker(); return; }
      highlightAt(highlighted + 1);
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      highlightAt(highlighted - 1);
    } else if (e.key === "Enter") {
      e.preventDefault();
      var visible = getVisible();
      if (highlighted >= 0 && highlighted < visible.length) {
        selectOpt(visible[highlighted]._optName);
      }
    } else if (e.key === "Tab") {
      var visible = getVisible();
      if (picker.classList.contains("sp-open") && highlighted >= 0 && highlighted < visible.length) {
        selectOpt(visible[highlighted]._optName);
      }
    } else if (e.key === "Escape") {
      e.preventDefault();
      closePicker();
      input.blur();
    }
  });
}

// ── Preview event delegation & drag ────────────────────────────────────

function resolveSpanPos(pos) {
  var c = ctx();
  if (c.grid[pos] === -1) {
    for (var anchor = 0; anchor < c.maxSlots; anchor++) {
      var slot = c.grid[anchor];
      if (!(slot > 0 || slot === -2)) continue;
      var cells = coveredCells(anchor, c.sizes[slot] || 1, c.maxSlots, false);
      if (cells.indexOf(pos) !== -1) return anchor;
    }
  }
  return pos;
}

function getCellFromEvent(e, container) {
  if (CFG.dragMode === "swap") {
    var rect = container.getBoundingClientRect();
    var col = Math.floor((e.clientX - rect.left) / (rect.width / GRID_COLS));
    var row = Math.floor((e.clientY - rect.top) / (rect.height / GRID_ROWS));
    col = Math.max(0, Math.min(col, GRID_COLS - 1));
    row = Math.max(0, Math.min(row, GRID_ROWS - 1));
    return resolveSpanPos(row * GRID_COLS + col);
  }
  var x = e.clientX, y = e.clientY;
  var children = container.children;
  var skip = 0;
  var bestDist = Infinity, bestPos = -1;
  for (var i = skip; i < children.length; i++) {
    var r = children[i].getBoundingClientRect();
    var pos = parseInt(children[i].getAttribute("data-pos"), 10);
    if (isNaN(pos)) continue;
    if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) return pos;
    var cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
    var d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
    if (d < bestDist) { bestDist = d; bestPos = pos; }
  }
  return bestPos;
}

function moveToCell(fromPos, toPos) {
  var c = ctx();
  toPos = resolveSpanPos(toPos);
  if (toPos >= c.maxSlots || c.grid[toPos] === -1) return;
  var grid = c.grid.slice();
  var movingSlot = grid[fromPos];
  clearSpans(grid, c.maxSlots);
  var targetSlot = grid[toPos];
  grid[toPos] = movingSlot;
  grid[fromPos] = targetSlot;
  applySpans(grid, c.sizes, c.maxSlots);
  if ((c.sizes[movingSlot] || 1) > 1 && !sizeFitsAt(toPos, c.sizes[movingSlot], c.maxSlots)) {
    delete c.sizes[movingSlot];
  }
  if (c.isSub) {
    getSubpage(state.editingSubpage).grid = grid;
  } else {
    state.grid = grid;
  }
}


function canPlaceSlotAt(grid, pos, size, maxSlots) {
  if (pos < 0 || pos >= maxSlots || grid[pos] !== 0) return false;
  if (!sizeFitsAt(pos, size, maxSlots)) return false;
  var cells = coveredCells(pos, size, maxSlots, false);
  for (var i = 0; i < cells.length; i++) {
    if (grid[cells[i]] !== 0) return false;
  }
  return true;
}

function findPlacementCell(grid, start, size, maxSlots) {
  for (var i = 0; i < maxSlots; i++) {
    var candidate = (start + i) % maxSlots;
    if (canPlaceSlotAt(grid, candidate, size, maxSlots)) return candidate;
  }
  return -1;
}

function findDuplicatePlacement(grid, start, size, maxSlots) {
  var targetSize = size || 1;
  var pos = findPlacementCell(grid, start, targetSize, maxSlots);
  if (pos >= 0) return { pos: pos, size: targetSize };
  if (targetSize !== 1) {
    pos = findPlacementCell(grid, start, 1, maxSlots);
    if (pos >= 0) return { pos: pos, size: 1 };
  }
  return { pos: -1, size: targetSize };
}

function placeSlotAt(grid, slot, pos, size) {
  grid[pos] = slot;
  markSpannedCells(grid, pos, size, grid.length);
}

function placeOrderedGridEntries(entries, sizes, maxSlots) {
  var grid = [];
  for (var i = 0; i < maxSlots; i++) grid.push(0);

  for (var j = 0; j < entries.length && j < maxSlots; j++) {
    var slot = entries[j];
    if (!(slot > 0 || slot === -2)) continue;

    var targetSize = sizes[slot] || 1;
    var place = j;
    if (!canPlaceSlotAt(grid, place, targetSize, maxSlots)) {
      place = findPlacementCell(grid, place, targetSize, maxSlots);
    }
    if (place < 0 && targetSize !== 1) {
      targetSize = 1;
      place = canPlaceSlotAt(grid, j, targetSize, maxSlots)
        ? j
        : findPlacementCell(grid, j, targetSize, maxSlots);
    }
    if (place < 0) continue;

    if (targetSize === 1) delete sizes[slot]; else sizes[slot] = targetSize;
    placeSlotAt(grid, slot, place, targetSize);
  }

  return grid;
}

function moveSelectedToCell(fromPos, toPos) {
  var c = ctx();
  toPos = resolveSpanPos(toPos);
  if (toPos < 0 || toPos >= c.maxSlots) return false;

  var sourceEntries = c.grid.slice();
  clearSpans(sourceEntries, c.maxSlots);

  var movingSlot = sourceEntries[fromPos];
  if (c.selected.length <= 1 || c.selected.indexOf(movingSlot) === -1) return false;

  var movingSlots = c.selected.slice();
  var targetSlot = sourceEntries[toPos];
  if (targetSlot > 0 && c.selected.indexOf(targetSlot) !== -1) return true;

  var entries = [];
  for (var i = 0; i < c.maxSlots; i++) {
    var entry = sourceEntries[i];
    if (entry > 0 && c.selected.indexOf(entry) !== -1) continue;
    entries.push(entry);
  }
  while (entries.length < c.maxSlots) entries.push(0);

  var insertPos;
  if (targetSlot > 0 || targetSlot === -2) {
    insertPos = entries.indexOf(targetSlot);
    insertPos = insertPos < 0 ? toPos : insertPos + 1;
  } else {
    insertPos = toPos;
    for (var r = 0; r < toPos; r++) {
      if (sourceEntries[r] > 0 && c.selected.indexOf(sourceEntries[r]) !== -1) insertPos--;
    }
  }
  insertPos = Math.max(0, Math.min(insertPos, entries.length));
  entries.splice.apply(entries, [insertPos, 0].concat(movingSlots));
  entries = entries.slice(0, c.maxSlots);

  var grid = placeOrderedGridEntries(entries, c.sizes, c.maxSlots);

  if (c.isSub) {
    getSubpage(state.editingSubpage).grid = grid;
  } else {
    state.grid = grid;
  }
  return true;
}

function clearPlaceholder() {
  if (previewPlaceholder) {
    previewPlaceholder.classList.remove("sp-drop-placeholder");
    previewPlaceholder = null;
  }
}

function clearTextSelection() {
  var selection = window.getSelection && window.getSelection();
  if (selection && selection.removeAllRanges) selection.removeAllRanges();
}

function setupPreviewEvents() {
  var container = els.previewMain;
  var pendingCellIdx = -1;

  function isBackExitTarget(e, target) {
    var icon = target.querySelector(".sp-back-hit");
    if (!icon) return false;
    var rect = icon.getBoundingClientRect();
    var pad = 12;
    return e.clientX >= rect.left - pad &&
      e.clientX <= rect.right + pad &&
      e.clientY >= rect.top - pad &&
      e.clientY <= rect.bottom + pad;
  }

  container.addEventListener("mousedown", function (e) {
    if (isConfigLocked()) return;
    if (!e.target.closest("[data-pos]")) return;
    if (e.shiftKey || e.ctrlKey || e.metaKey) e.preventDefault();
  });

  // Click delegation
  container.addEventListener("click", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      e.stopPropagation();
      return;
    }
    if (e.target.closest(".sp-subpage-badge")) {
      var btnEl = e.target.closest("[data-slot]");
      if (btnEl) {
        enterSubpage(parseInt(btnEl.getAttribute("data-slot"), 10));
        return;
      }
    }
    var target = e.target.closest("[data-pos]");
    if (!target) return;
    var pos = parseInt(target.getAttribute("data-pos"), 10);
    var c = ctx();
    var slot = c.grid[pos];
    if (slot > 0) {
      handleBtnClick(e, slot, pos);
    } else if (slot === -2) {
      if (didDrag) { didDrag = false; return; }
      if (isBackExitTarget(e, target)) {
        exitSubpage();
      } else {
        handleBtnClick(e, slot, pos);
      }
    } else if (slot === 0) {
      if (state.clipboard) {
        e.preventDefault();
        e.stopPropagation();
        showEmptySlotMenu(e, pos);
      } else {
        addSlot(pos);
      }
    }
  });

  // Context menu delegation
  container.addEventListener("contextmenu", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      return;
    }
    var target = e.target.closest("[data-pos]");
    if (!target) return;
    e.preventDefault();
    var pos = parseInt(target.getAttribute("data-pos"), 10);
    var c = ctx();
    var slot = c.grid[pos];
    if (slot > 0) {
      showContextMenu(e, slot);
    } else if (slot === -2) {
      showBackContextMenu(e);
    } else if (slot === 0) {
      showEmptySlotMenu(e, pos);
    }
  });

  // Drag delegation
  container.addEventListener("dragstart", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      return;
    }
    var target = e.target.closest(".sp-btn") || e.target.closest(".sp-back-btn");
    if (!target) return;
    var pos = parseInt(target.getAttribute("data-pos"), 10);
    dragSrcPos = pos;
    if (CFG.dragAnimation) dragSrcEl = target;
    dragIsSubpage = !!state.editingSubpage;
    didDrag = true;
    dragEnterCount = 0;
    container.classList.add("sp-drag-active");
    e.dataTransfer.effectAllowed = "move";
    e.dataTransfer.setData("text/plain", String(pos));
    if (CFG.dragAnimation) {
      requestAnimationFrame(function () { target.classList.add("sp-dragging"); });
    }
  });

  container.addEventListener("dragend", function () {
    dragSrcPos = -1;
    previewDropIdx = -1;
    dragIsSubpage = false;
    dragEnterCount = 0;
    clearPlaceholder();
    if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
    setTimeout(function () { container.classList.remove("sp-drag-active"); }, 50);
  });

  // Drop zone
  function updatePlaceholder(cellIdx) {
    if (cellIdx === previewDropIdx) return;
    previewDropIdx = cellIdx;
    clearPlaceholder();
    var target = container.querySelector('[data-pos="' + cellIdx + '"]');
    if (target) {
      previewPlaceholder = target;
      previewPlaceholder.classList.add("sp-drop-placeholder");
    }
  }

  container.addEventListener("dragenter", function (e) {
    if (isConfigLocked()) return;
    if (dragSrcPos < 0) return;
    e.preventDefault();
    dragEnterCount++;
  });

  container.addEventListener("dragover", function (e) {
    if (isConfigLocked()) return;
    if (dragSrcPos < 0) return;
    e.preventDefault();
    e.dataTransfer.dropEffect = "move";
    if (CFG.dragAnimation) {
      pendingCellIdx = getCellFromEvent(e, container);
      if (dragRafPending) return;
      dragRafPending = true;
      requestAnimationFrame(function () {
        dragRafPending = false;
        if (dragSrcPos < 0) return;
        updatePlaceholder(pendingCellIdx);
      });
    } else {
      updatePlaceholder(getCellFromEvent(e, container));
    }
  });

  container.addEventListener("dragleave", function () {
    dragEnterCount--;
    if (dragEnterCount <= 0) {
      dragEnterCount = 0;
      previewDropIdx = -1;
      clearPlaceholder();
    }
  });

  container.addEventListener("drop", function (e) {
    if (isConfigLocked()) {
      e.preventDefault();
      return;
    }
    e.preventDefault();
    dragEnterCount = 0;
    var toPos = previewDropIdx;
    previewDropIdx = -1;
    clearPlaceholder();
    if (dragSrcEl) { dragSrcEl.classList.remove("sp-dragging"); dragSrcEl = null; }
    var c = ctx();
    if (dragSrcPos < 0 || toPos < 0 || toPos >= c.maxSlots) { dragSrcPos = -1; dragIsSubpage = false; return; }
    if (dragSrcPos === toPos) { dragSrcPos = -1; dragIsSubpage = false; return; }
    if (!moveSelectedToCell(dragSrcPos, toPos)) moveToCell(dragSrcPos, toPos);
    renderPreview();
    renderButtonSettings();
    c.save();
    dragSrcPos = -1;
    dragIsSubpage = false;
  });
}

function handleBtnClick(e, slot, pos) {
  if (isConfigLocked()) return;
  if (didDrag) { didDrag = false; return; }
  var c = ctx();
  if (e.shiftKey || e.ctrlKey || e.metaKey) e.preventDefault();

  if (e.shiftKey && c.getLastClicked() > 0) {
    var anchorPos = c.grid.indexOf(c.getLastClicked());
    if (anchorPos !== -1) {
      var from = Math.min(anchorPos, pos);
      var to = Math.max(anchorPos, pos);
      var newSel = [];
      for (var i = from; i <= to; i++) {
        if (c.grid[i] > 0) newSel.push(c.grid[i]);
      }
      c.setSelected(newSel);
      renderPreview();
      hideSettingsOverlay();
      clearTextSelection();
      return;
    }
  }

  if (e.ctrlKey || e.metaKey) {
    var idx = c.selected.indexOf(slot);
    if (idx !== -1) {
      c.selected.splice(idx, 1);
    } else {
      c.selected.push(slot);
      c.setLastClicked(slot);
    }
    renderPreview();
    hideSettingsOverlay();
    clearTextSelection();
    return;
  }

  if (c.selected.length === 1 && c.selected[0] === slot) {
    c.setSelected([]);
    c.setLastClicked(-1);
  } else {
    c.setSelected([slot]);
    c.setLastClicked(slot);
  }
  renderPreview();
  renderButtonSettings();
}

function selectButton(slot) {
  if (isConfigLocked()) return;
  if (slot < 1) {
    state.selectedSlots = [];
  } else {
    state.selectedSlots = [slot];
    state.lastClickedSlot = slot;
  }
  renderPreview();
  renderButtonSettings();
}

// ── Button management (unified) ────────────────────────────────────────

function firstFreeSlot() {
  var used = {};
  state.grid.forEach(function (s) { if (s > 0) used[s] = true; });
  for (var i = 1; i <= NUM_SLOTS; i++) {
    if (!used[i]) return i;
  }
  return -1;
}

function firstFreeCell(afterPos) {
  var start = afterPos != null ? afterPos : 0;
  for (var i = 0; i < NUM_SLOTS; i++) {
    var candidate = (start + i) % NUM_SLOTS;
    if (state.grid[candidate] === 0) return candidate;
  }
  return -1;
}

function emptyButtonConfig(type) {
  return { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: type || "", precision: "", options: "" };
}

function addSlot(pos) {
  if (isConfigLocked()) return;
  var c = ctx();
  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    var newSlot = subpageFirstFreeSlot(sp);
    while (sp.buttons.length < newSlot) {
      sp.buttons.push(emptyButtonConfig());
    }
    sp.grid[pos] = newSlot;
    sp.order = serializeSubpageGrid(sp);
    saveSubpageConfig(state.editingSubpage);
    state.subpageSelectedSlots = [newSlot];
    state.subpageLastClicked = newSlot;
    renderPreview();
    renderButtonSettings(true);
  } else {
    var slot = firstFreeSlot();
    if (slot < 0) return;
    state.grid[pos] = slot;
    postText("Button Order", serializeGrid(state.grid));
    state.selectedSlots = [slot];
    state.lastClickedSlot = slot;
    renderPreview();
    renderButtonSettings(true);
  }
}

function addSubpageSlot(pos) {
  if (isConfigLocked()) return;
  var c = ctx();
  if (c.isSub) return;
  var slot = firstFreeSlot();
  if (slot < 0) return;
  state.buttons[slot - 1] = emptyButtonConfig("subpage");
  state.grid[pos] = slot;
  state.subpages[slot] = { order: [], buttons: [], grid: [], sizes: {} };
  buildSubpageGrid(state.subpages[slot]);
  postText("Button Order", serializeGrid(state.grid));
  saveButtonConfig(slot);
  saveSubpageEntity(slot);
  selectButton(slot);
}

function duplicateButton(srcSlot) {
  if (isConfigLocked()) return;
  var newSlot = firstFreeSlot();
  if (newSlot < 0) return;
  var srcSz = state.sizes[srcSlot] || 1;
  var srcPos = state.grid.indexOf(srcSlot);
  var placement = findDuplicatePlacement(state.grid, srcPos + 1, srcSz, NUM_SLOTS);
  if (placement.pos < 0) return;

  var src = state.buttons[srcSlot - 1];
  state.buttons[newSlot - 1] = {
    entity: src.entity, label: src.label, icon: src.icon,
    icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
    type: src.type || "", precision: src.precision || "",
    options: src.options || "",
  };

  if (placement.size === 1) delete state.sizes[newSlot]; else state.sizes[newSlot] = placement.size;
  placeSlotAt(state.grid, newSlot, placement.pos, placement.size);

  if (src.type === "subpage" && state.subpages[srcSlot]) {
    var spJson = serializeSubpageConfig(state.subpages[srcSlot]);
    var spCopy = parseSubpageConfig(spJson);
    spCopy.sizes = {};
    buildSubpageGrid(spCopy);
    state.subpages[newSlot] = spCopy;
  }
  postText("Button Order", serializeGrid(state.grid));
  saveButtonConfig(newSlot);
  saveSubpageEntity(newSlot);
  state.selectedSlots = [newSlot];
  state.lastClickedSlot = newSlot;
  renderPreview();
}

function duplicateSubpageButton(srcSlot) {
  if (isConfigLocked()) return;
  var homeSlot = state.editingSubpage;
  var sp = getSubpage(homeSlot);
  var newSlot = subpageFirstFreeSlot(sp);
  while (sp.buttons.length < newSlot) {
    sp.buttons.push(emptyButtonConfig());
  }
  var srcSz = sp.sizes[srcSlot] || 1;
  var srcPos = sp.grid.indexOf(srcSlot);
  var placement = findDuplicatePlacement(sp.grid, srcPos + 1, srcSz, NUM_SLOTS);
  if (placement.pos < 0) return;

  var src = sp.buttons[srcSlot - 1];
  sp.buttons[newSlot - 1] = {
    entity: src.entity, label: src.label, icon: src.icon,
    icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
    type: src.type || "", precision: src.precision || "",
    options: src.options || "",
  };

  if (placement.size === 1) delete sp.sizes[newSlot]; else sp.sizes[newSlot] = placement.size;
  placeSlotAt(sp.grid, newSlot, placement.pos, placement.size);

  sp.order = serializeSubpageGrid(sp);
  saveSubpageConfig(homeSlot);
  state.subpageSelectedSlots = [newSlot];
  state.subpageLastClicked = newSlot;
  renderPreview();
}

function deleteSlot(slot) {
  if (isConfigLocked()) return;
  var c = ctx();
  for (var i = 0; i < c.maxSlots; i++) {
    if (c.grid[i] === slot) {
      c.grid[i] = 0;
      var cells = coveredCells(i, c.sizes[slot] || 1, c.maxSlots, false);
      for (var ci = 0; ci < cells.length; ci++) {
        if (c.grid[cells[ci]] === -1) c.grid[cells[ci]] = 0;
      }
      break;
    }
  }
  delete c.sizes[slot];

  var selIdx = c.selected.indexOf(slot);
  if (selIdx !== -1) c.selected.splice(selIdx, 1);

  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    if (slot >= 1 && slot <= sp.buttons.length) {
      sp.buttons[slot - 1] = emptyButtonConfig();
    }
    sp.order = serializeSubpageGrid(sp);
    state.subpageLastClicked = -1;
    saveSubpageConfig(state.editingSubpage);
  } else {
    postText("Button Order", serializeGrid(state.grid));
    state.buttons[slot - 1] = emptyButtonConfig();
    delete state.subpages[slot];
    saveButtonConfig(slot);
    saveSubpageEntity(slot);
  }

  renderPreview();
  renderButtonSettings();
}

function deleteButtons(slots) {
  if (isConfigLocked()) return;
  var c = ctx();
  for (var i = 0; i < c.maxSlots; i++) {
    if (slots.indexOf(c.grid[i]) !== -1) {
      var cells = coveredCells(i, c.sizes[c.grid[i]] || 1, c.maxSlots, false);
      for (var ci = 0; ci < cells.length; ci++) {
        if (c.grid[cells[ci]] === -1) c.grid[cells[ci]] = 0;
      }
      c.grid[i] = 0;
    }
  }
  slots.forEach(function (slot) { delete c.sizes[slot]; });
  c.setSelected([]);
  c.setLastClicked(-1);
  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    slots.forEach(function (slot) {
      if (slot >= 1 && slot <= sp.buttons.length) {
        sp.buttons[slot - 1] = emptyButtonConfig();
      }
    });
    sp.order = serializeSubpageGrid(sp);
    saveSubpageConfig(state.editingSubpage);
  } else {
    slots.forEach(function (slot) {
      state.buttons[slot - 1] = emptyButtonConfig();
      delete state.subpages[slot];
      saveButtonConfig(slot);
      saveSubpageEntity(slot);
    });
    postText("Button Order", serializeGrid(state.grid));
  }
  renderPreview();
  renderButtonSettings();
}

// ── Context menu (unified) ─────────────────────────────────────────────

var ctxMenu = null;

function positionMenu(menu, e) {
  var w = menu.offsetWidth, h = menu.offsetHeight;
  var x = Math.max(4, Math.min(e.clientX, window.innerWidth - w - 4));
  var y = Math.max(4, Math.min(e.clientY, window.innerHeight - h - 4));
  menu.style.left = x + "px";
  menu.style.top = y + "px";
}

function addCtxItem(icon, text, handler, danger) {
  var item = document.createElement("div");
  item.className = "sp-ctx-item" + (danger ? " sp-ctx-danger" : "");
  item.innerHTML = '<span class="mdi mdi-' + icon + '"></span>' + escHtml(text);
  item.addEventListener("mousedown", function (ev) {
    ev.preventDefault();
    ev.stopPropagation();
    hideContextMenu();
    handler();
  });
  ctxMenu.appendChild(item);
}

function addCtxDivider() {
  var div = document.createElement("div");
  div.className = "sp-ctx-divider";
  ctxMenu.appendChild(div);
}

function addCtxSubmenu(icon, text, buildFn) {
  var wrapper = document.createElement("div");
  wrapper.className = "sp-ctx-item sp-ctx-sub";
  wrapper.innerHTML = '<span class="mdi mdi-' + icon + '"></span>' + escHtml(text);
  var sub = document.createElement("div");
  sub.className = "sp-ctx-submenu";
  buildFn(sub);
  wrapper.appendChild(sub);
  wrapper.addEventListener("mouseenter", function () {
    sub.style.left = "100%"; sub.style.right = "auto";
    var r = sub.getBoundingClientRect();
    if (r.right > window.innerWidth - 4) { sub.style.left = "auto"; sub.style.right = "100%"; }
  });
  wrapper.addEventListener("mousedown", function (ev) { ev.preventDefault(); ev.stopPropagation(); });
  ctxMenu.appendChild(wrapper);
}

function addSubItem(container, icon, text, handler, active) {
  var item = document.createElement("div");
  item.className = "sp-ctx-item";
  item.innerHTML = (active ? '<span class="sp-ctx-check mdi mdi-check"></span>' : '<span style="width:18px"></span>') + escHtml(text);
  item.addEventListener("mousedown", function (ev) {
    ev.preventDefault();
    ev.stopPropagation();
    hideContextMenu();
    handler();
  });
  container.appendChild(item);
}

function resizeSlot(slot, targetSz) {
  if (isConfigLocked()) return;
  var c = ctx();
  var slotPos = slot === -2 ? c.grid.indexOf(-2) : c.grid.indexOf(slot);
  if (slotPos < 0) return;
  var curSz = c.sizes[slot] || 1;
  if (curSz === targetSz) return;

  var oldCells = coveredCells(slotPos, curSz, c.maxSlots, false);
  for (var oi = 0; oi < oldCells.length; oi++) {
    if (c.grid[oldCells[oi]] === -1) c.grid[oldCells[oi]] = 0;
  }

  if (targetSz > 1 && !sizeFitsAt(slotPos, targetSz, c.maxSlots)) {
    delete c.sizes[slot];
    return;
  }
  var need = coveredCells(slotPos, targetSz, c.maxSlots, false);

  for (var i = 0; i < need.length; i++) {
    var p = need[i];
    if (c.grid[p] > 0 || c.grid[p] === -2) {
      if (c.isSub && c.grid[p] > 0) return;
      var displaced = c.grid[p];
      c.grid[p] = 0;
      if (c.isSub) {
        for (var j = 0; j < c.maxSlots; j++) { if (c.grid[j] === 0 && need.indexOf(j) === -1) { c.grid[j] = displaced; break; } }
      } else {
        var fc = firstFreeCell(p + 1);
        if (fc >= 0) c.grid[fc] = displaced;
      }
    }
  }
  for (var i = 0; i < need.length; i++) c.grid[need[i]] = -1;

  if (targetSz === 1) delete c.sizes[slot]; else c.sizes[slot] = targetSz;

  if (c.isSub) {
    var sp = getSubpage(state.editingSubpage);
    sp.order = serializeSubpageGrid(sp);
    saveSubpageConfig(state.editingSubpage);
  } else {
    postText("Button Order", serializeGrid(state.grid));
  }
  renderPreview();
  renderButtonSettings();
}


function addBulkCardMenuItems(slots) {
  addCtxItem("clipboard-outline", "Copy " + slots.length + " Cards", function () { copyButtons(slots); });
  addCtxItem("content-cut", "Cut " + slots.length + " Cards", function () { cutButtons(slots); });
  addCtxItem("delete", "Delete " + slots.length + " Cards", function () { deleteButtons(slots); }, true);
}

function addSingleCardMenuItems(slot) {
  var c = ctx();
  var b = c.buttons[slot - 1];
  addCtxItem("pencil", "Edit Card", function () { openCardSettings(slot); });

  var ctxTypeDef = BUTTON_TYPES[(b && b.type) || ""];
  if (ctxTypeDef && ctxTypeDef.contextMenuItems && (!c.isSub || ctxTypeDef.allowInSubpage)) {
    ctxTypeDef.contextMenuItems(slot, b, { addCtxItem: addCtxItem });
  }

  var sz = c.sizes[slot] || 1;
  addCtxSubmenu("arrow-expand-all", "Size", function (sub) {
    addSubItem(sub, "", "Single (1x1)", function () { resizeSlot(slot, 1); }, sz === 1);
    addSubItem(sub, "", "Tall (2x1)", function () { resizeSlot(slot, 2); }, sz === 2);
    addSubItem(sub, "", "Extra Tall (3x1)", function () { resizeSlot(slot, 5); }, sz === 5);
    addSubItem(sub, "", "Wide (1x2)", function () { resizeSlot(slot, 3); }, sz === 3);
    addSubItem(sub, "", "Extra Wide (1x3)", function () { resizeSlot(slot, 6); }, sz === 6);
    addSubItem(sub, "", "Large (2x2)", function () { resizeSlot(slot, 4); }, sz === 4);
  });

  addCtxDivider();
  addCtxItem("content-copy", "Duplicate", function () {
    if (c.isSub) { duplicateSubpageButton(slot); } else { duplicateButton(slot); }
  });

  addCtxItem("clipboard-outline", "Copy", function () { copySlot(slot); });
  addCtxItem("content-cut", "Cut", function () { cutSlot(slot); });
  addCtxItem("delete", "Delete", function () { deleteSlot(slot); }, true);
}

function showSelectionMenu(e) {
  if (isConfigLocked()) return;
  hideContextMenu();
  var c = ctx();
  if (!c.selected.length) return;

  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";
  if (c.selected.length > 1) {
    addBulkCardMenuItems(c.selected.slice());
  } else {
    addSingleCardMenuItems(c.selected[0]);
  }
  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function showContextMenu(e, slot) {
  if (isConfigLocked()) return;
  hideContextMenu();
  var c = ctx();

  if (c.selected.indexOf(slot) === -1) {
    if (c.selected.length > 1) {
      c.selected.push(slot);
    } else {
      c.setSelected([slot]);
      c.setLastClicked(slot);
    }
    renderPreview();
    renderButtonSettings();
    c = ctx();
  }

  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";

  if (c.selected.length > 1 && c.selected.indexOf(slot) !== -1) {
    addBulkCardMenuItems(c.selected.slice());
  } else {
    addSingleCardMenuItems(slot);
  }

  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function showBackContextMenu(e) {
  if (isConfigLocked()) return;
  hideContextMenu();
  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";
  var sp = getSubpage(state.editingSubpage);
  var bkSz = sp.sizes[-2] || 1;
  addCtxItem("pencil", "Edit Label", function () { openCardSettings(-2); });
  addCtxItem("keyboard-return", "Exit Subpage", function () { exitSubpage(); });
  addCtxDivider();
  addCtxSubmenu("arrow-expand-all", "Size", function (sub) {
    addSubItem(sub, "", "Single (1x1)", function () { resizeSlot(-2, 1); }, bkSz === 1);
    addSubItem(sub, "", "Tall (2x1)", function () { resizeSlot(-2, 2); }, bkSz === 2);
    addSubItem(sub, "", "Extra Tall (3x1)", function () { resizeSlot(-2, 5); }, bkSz === 5);
    addSubItem(sub, "", "Wide (1x2)", function () { resizeSlot(-2, 3); }, bkSz === 3);
    addSubItem(sub, "", "Extra Wide (1x3)", function () { resizeSlot(-2, 6); }, bkSz === 6);
    addSubItem(sub, "", "Large (2x2)", function () { resizeSlot(-2, 4); }, bkSz === 4);
  });
  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function showEmptySlotMenu(e, pos) {
  if (isConfigLocked()) return;
  hideContextMenu();
  ctxMenu = document.createElement("div");
  ctxMenu.className = "sp-ctx-menu";
  var c = ctx();
  if (state.clipboard) {
    var count = state.clipboard.buttons.length;
    addCtxItem("content-paste", count > 1 ? "Paste " + count + " Cards" : "Paste", function () {
      if (c.isSub) {
        pasteSubpageButton(pos);
      } else {
        pasteButton(pos);
      }
    });
    addCtxDivider();
  }
  addCtxItem("plus", "Create Card", function () { addSlot(pos); });
  if (!c.isSub) {
    addCtxItem("folder-plus", "Create Subpage", function () { addSubpageSlot(pos); });
  }
  document.body.appendChild(ctxMenu);
  positionMenu(ctxMenu, e);
}

function hideContextMenu() {
  if (ctxMenu && ctxMenu.parentNode) {
    ctxMenu.parentNode.removeChild(ctxMenu);
  }
  ctxMenu = null;
}

// ── Cut / Paste ────────────────────────────────────────────────────────

function buildClipboardEntry(slot) {
  var c = ctx();
  var src = c.buttons[slot - 1];
  var entry = {
    entity: src.entity, label: src.label, icon: src.icon,
    icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
    type: src.type || "", precision: src.precision || "",
    options: src.options || "",
    subpageConfig: null, size: c.sizes[slot] || 1,
  };
  if (!c.isSub && src.type === "subpage" && state.subpages[slot]) {
    entry.subpageConfig = serializeSubpageConfig(state.subpages[slot]);
  }
  return entry;
}

function copySlot(slot) {
  state.clipboard = { buttons: [buildClipboardEntry(slot)] };
}

function copyButtons(slots) {
  var entries = [];
  slots.forEach(function (slot) { entries.push(buildClipboardEntry(slot)); });
  state.clipboard = { buttons: entries };
}

function cutSlot(slot) {
  if (isConfigLocked()) return;
  copySlot(slot);
  deleteSlot(slot);
}

function cutButtons(slots) {
  if (isConfigLocked()) return;
  copyButtons(slots);
  deleteButtons(slots);
}

function pasteButton(pos) {
  if (isConfigLocked()) return;
  if (!state.clipboard) return;
  var entries = state.clipboard.buttons;
  var lastSlot = -1;
  for (var i = 0; i < entries.length; i++) {
    var newSlot = firstFreeSlot();
    if (newSlot < 0) break;
    var e = entries[i];
    var placement = findDuplicatePlacement(state.grid, pos, e.size || 1, NUM_SLOTS);
    if (placement.pos < 0) break;
    var cell = placement.pos;
    var placeSize = placement.size;
    state.buttons[newSlot - 1] = {
      entity: e.entity, label: e.label, icon: e.icon,
      icon_on: e.icon_on, sensor: e.sensor, unit: e.unit,
      type: e.type || "", precision: e.precision || "",
      options: e.options || "",
    };
    if (placeSize === 1) delete state.sizes[newSlot]; else state.sizes[newSlot] = placeSize;
    placeSlotAt(state.grid, newSlot, cell, placeSize);
    if (e.subpageConfig) {
      var spCopy = parseSubpageConfig(e.subpageConfig);
      spCopy.sizes = {};
      buildSubpageGrid(spCopy);
      state.subpages[newSlot] = spCopy;
    }
    saveButtonConfig(newSlot);
    saveSubpageEntity(newSlot);
    lastSlot = newSlot;
  }
  postText("Button Order", serializeGrid(state.grid));
  state.clipboard = null;
  state.selectedSlots = [];
  renderPreview();
}

function pasteSubpageButton(pos) {
  if (isConfigLocked()) return;
  if (!state.clipboard) return;
  var homeSlot = state.editingSubpage;
  var sp = getSubpage(homeSlot);
  var maxPos = NUM_SLOTS;
  var entries = state.clipboard.buttons;
  var lastSlot = -1;
  for (var i = 0; i < entries.length; i++) {
    var e = entries[i];
    var placement = findDuplicatePlacement(sp.grid, pos, e.size || 1, maxPos);
    if (placement.pos < 0) break;
    var cell = placement.pos;
    var placeSize = placement.size;
    var newSlot = subpageFirstFreeSlot(sp);
    while (sp.buttons.length < newSlot) {
      sp.buttons.push(emptyButtonConfig());
    }
    sp.buttons[newSlot - 1] = {
      entity: e.entity, label: e.label, icon: e.icon,
      icon_on: e.icon_on, sensor: e.sensor, unit: e.unit,
      type: e.type || "", precision: e.precision || "",
      options: e.options || "",
    };
    if (placeSize === 1) delete sp.sizes[newSlot]; else sp.sizes[newSlot] = placeSize;
    placeSlotAt(sp.grid, newSlot, cell, placeSize);
    lastSlot = newSlot;
  }
  sp.order = serializeSubpageGrid(sp);
  state.clipboard = null;
  saveSubpageConfig(homeSlot);
  state.subpageSelectedSlots = [];
  renderPreview();
}
