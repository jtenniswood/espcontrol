// ── Settings Page ──────────────────────────────────────────────────────

function buildSettingsPage(parent) {
  var page = document.createElement("div");
  page.id = "sp-settings";
  page.className = "sp-page";

  var config = document.createElement("div");
  config.className = "sp-config fade-in";
  var displayBody = settingsGroupBody();
  var sleepScheduleBody = settingsGroupBody();
  var systemBody = settingsGroupBody();

  var appearBody = document.createElement("div");

  var themeField = document.createElement("div");
  themeField.className = "sp-field";
  themeField.appendChild(fieldLabel("Theme", "sp-set-theme"));
  var themeSelect = document.createElement("select");
  themeSelect.className = "sp-select";
  themeSelect.id = "sp-set-theme";
  state.themeOptions.forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt;
    o.textContent = opt;
    themeSelect.appendChild(o);
  });
  themeSelect.value = normalizeTheme(state.theme);
  themeSelect.addEventListener("change", function () {
    applyThemePreset(this.value, true);
  });
  themeField.appendChild(themeSelect);
  appearBody.appendChild(themeField);
  els.setTheme = themeSelect;

  if (!isEpaperPreview()) {
    appearBody.appendChild(fieldLabel("Primary"));
    var onColor = colorField("sp-set-on-color", "0073FF", function (hex) {
      postText(entityName("button_on_color"), hex);
    });
    appearBody.appendChild(onColor);
    els.setOnColor = onColor;

    appearBody.appendChild(fieldLabel("Secondary"));
    var offColor = colorField("sp-set-off-color", "CECECE", function (hex) {
      postText(entityName("button_off_color"), hex);
    });
    appearBody.appendChild(offColor);
    els.setOffColor = offColor;

    appearBody.appendChild(fieldLabel("Tertiary"));
    var sensorColor = colorField("sp-set-sensor-color", "DEDEDE", function (hex) {
      postText(entityName("sensor_card_color"), hex);
    });
    appearBody.appendChild(sensorColor);
    els.setSensorColor = sensorColor;
  }

  appendSettingsGroupSection(displayBody, "Appearance", appearBody);

  var languageBody = document.createElement("div");
  var languageField = document.createElement("div");
  languageField.className = "sp-field";
  languageField.appendChild(fieldLabel("Language", "sp-set-language"));
  var languageSelect = document.createElement("select");
  languageSelect.className = "sp-select";
  languageSelect.id = "sp-set-language";
  state.languageOptions = languageOptionsWithFallback(state.languageOptions, state.language);
  state.languageOptions.forEach(function (opt) {
    appendLanguageOption(languageSelect, opt);
  });
  languageSelect.value = normalizeLanguage(state.language);
  languageSelect.addEventListener("change", function () {
    state.language = normalizeLanguage(this.value);
    postSelect(entityName("screen_language"), state.language);
  });
  languageField.appendChild(languageSelect);
  languageBody.appendChild(languageField);
  appendSettingsGroupSection(systemBody, "Language", languageBody);
  els.setLanguage = languageSelect;

  var blBody = document.createElement("div");

  var daySlider = createRangeSlider("Daytime Brightness", state.brightnessDayVal, entityName("screen_daytime_brightness"));
  blBody.appendChild(daySlider.wrap);
  els.setDayBrightness = daySlider.range;
  els.setDayBrightnessVal = daySlider.val;

  var nightSlider = createRangeSlider("Nighttime Brightness", state.brightnessNightVal, entityName("screen_nighttime_brightness"));
  blBody.appendChild(nightSlider.wrap);
  els.setNightBrightness = nightSlider.range;
  els.setNightBrightnessVal = nightSlider.val;

  var autoBrightnessToggle = toggleRow("Automatic Brightness", "sp-set-automatic-brightness", state.automaticBrightnessEnabled);
  blBody.appendChild(autoBrightnessToggle.row);
  els.setAutomaticBrightnessToggle = autoBrightnessToggle.input;
  autoBrightnessToggle.input.addEventListener("change", function () {
    state.automaticBrightnessEnabled = this.checked;
    postAutomaticBrightnessEnabled(state.automaticBrightnessEnabled);
    syncScreenScheduleUi();
  });

  var sunInfo = document.createElement("div");
  sunInfo.className = "sp-sun-info";
  sunInfo.id = "sp-sun-info";
  blBody.appendChild(sunInfo);
  els.sunInfo = sunInfo;
  updateSunInfo();

  appendSettingsGroupSection(displayBody, "Backlight", blBody);

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
  clockOptions.appendChild(fieldLabel("Clock Text Colour"));
  var clockTextColor = colorField(
    "sp-set-schedule-clock-text-color",
    state.scheduleClockTextColor,
    function (hex) {
      state.scheduleClockTextColor = normalizeHexColor(hex, "FFFFFF");
      postText(entityName("screen_schedule_clock_text_color"), state.scheduleClockTextColor);
    }
  );
  clockOptions.appendChild(clockTextColor);
  scheduleTimes.appendChild(clockOptions);
  els.setScheduleClockOptions = clockOptions;
  els.setScheduleClockBrightness = clockBrightnessSlider.range;
  els.setScheduleClockBrightnessVal = clockBrightnessSlider.val;
  els.setScheduleClockTextColor = clockTextColor;

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

  var clockBody = document.createElement("div");

  var tzField = document.createElement("div");
  tzField.className = "sp-field";
  tzField.appendChild(fieldLabel("Timezone", "sp-set-timezone"));
  var tzSelect = document.createElement("select");
  tzSelect.className = "sp-select";
  tzSelect.id = "sp-set-timezone";
  state.timezoneOptions = timezoneOptionsWithFallback(state.timezoneOptions, state.timezone);
  state.timezoneOptions.forEach(function (opt) {
    appendTimezoneOption(tzSelect, opt);
  });
  tzSelect.value = state.timezone;
  tzSelect.addEventListener("change", function () {
    state.timezone = this.value;
    postSelect(entityName("screen_timezone"), this.value);
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
    postSelect(entityName("screen_clock_format"), this.value);
  });
  cfField.appendChild(cfSelect);
  clockBody.appendChild(cfField);
  els.setClockFormat = cfSelect;

  var ntpField = document.createElement("div");
  ntpField.className = "sp-field";
  state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
  var customNtpServers = toggleRow("Custom NTP Servers", "sp-set-custom-ntp-servers", state.customNtpServers);
  ntpField.appendChild(customNtpServers.row);
  els.setCustomNtpServersToggle = customNtpServers.input;
  customNtpServers.input.addEventListener("change", function () {
    state.customNtpServers = this.checked;
    if (!state.customNtpServers) {
      resetNtpServersToDefaults();
      postText(entityName("screen_ntp_server_1"), state.ntpServer1);
      postText(entityName("screen_ntp_server_2"), state.ntpServer2);
      postText(entityName("screen_ntp_server_3"), state.ntpServer3);
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
    entityName("screen_ntp_server_1"), NTP_SERVER_DEFAULTS[0], "NTP Server 1");
  els.setNtpServer2 = addNtpServerInput(
    "sp-set-ntp-server-2", "ntpServer2",
    entityName("screen_ntp_server_2"), NTP_SERVER_DEFAULTS[1], "NTP Server 2");
  els.setNtpServer3 = addNtpServerInput(
    "sp-set-ntp-server-3", "ntpServer3",
    entityName("screen_ntp_server_3"), NTP_SERVER_DEFAULTS[2], "NTP Server 3");

  ntpField.appendChild(ntpList);
  syncNtpServerUi();
  clockBody.appendChild(ntpField);

  var monthNamesField = document.createElement("div");
  monthNamesField.className = "sp-field";
  state.monthNames = normalizeMonthNames(state.monthNames);
  state.customMonthNames = state.customMonthNames || hasCustomMonthNames();
  var customMonthNames = toggleRow("Custom Month Names", "sp-set-custom-month-names", state.customMonthNames);
  monthNamesField.appendChild(customMonthNames.row);
  els.setCustomMonthNamesToggle = customMonthNames.input;
  customMonthNames.input.addEventListener("change", function () {
    state.customMonthNames = this.checked;
    if (!state.customMonthNames) {
      resetMonthNamesToDefaults();
      postText(entityName("screen_month_names"), serializeMonthNames(state.monthNames));
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
      postText(entityName("screen_month_names"), serializeMonthNames(state.monthNames));
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

  appendSettingsGroupSection(systemBody, "Time Settings", clockBody);

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
    entityName("outdoor_temp_enable"), entityName("outdoor_temp_entity"), "Outdoor Temp Entity", "sensor.outdoor_temperature");
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
    entityName("indoor_temp_enable"), entityName("indoor_temp_entity"), "Indoor Temp Entity", "sensor.indoor_temperature");
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
  appendSettingsGroupSection(displayBody, "Clock Bar", clockBarBody, clockBarBadge);

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
      postSelect(entityName("screen_rotation"), this.value);
    });
    rotField.appendChild(rotSelect);
    rotationBody.appendChild(rotField);
    appendSettingsGroupSection(displayBody, "Rotation", rotationBody);
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
    postSelect(entityName("screen_temperature_unit"), state.temperatureUnit);
    updateTempPreview();
    renderPreview();
  });
  unitField.appendChild(unitSelect);
  tempBody.appendChild(unitField);
  els.setTemperatureUnit = unitSelect;

  syncTemperatureUi();
  appendSettingsGroupSection(displayBody, "Temperature", tempBody);

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
    postSwitch(entityName("screen_saver_media_player_sleep_prevention"), state.mediaPlayerSleepPreventionOn);
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
  bindTextPost(mediaPlayerInp, entityName("media_player_sleep_prevention_entity"), {
    onBlur: function (value) { state.mediaPlayerSleepPreventionEntity = value; },
  });
  els.setMediaPlayerSleepPrevention = mediaPlayerInp;
  els.setMediaPlayerSleepPreventionField = mediaPlayerField;

  var coverArtBody = document.createElement("div");
  if (!isEpaperPreview()) {
    var coverArtToggle = toggleRow(
      "Cover Art While Playing",
      "sp-set-ss-cover-art-enable",
      state.coverArtScreensaverOn);
    coverArtBody.appendChild(coverArtToggle.row);
    coverArtToggle.input.addEventListener("change", function () {
      state.coverArtScreensaverOn = this.checked;
      syncCoverArtScreensaverUi();
      postSwitch(entityName("screen_saver_cover_art"), state.coverArtScreensaverOn);
    });
    els.setCoverArtToggle = coverArtToggle.input;

    var coverArtOptions = condField();

    var coverArtEntityField = document.createElement("div");
    coverArtEntityField.className = "sp-field";
    coverArtEntityField.appendChild(fieldLabel("Cover Art Media Player", "sp-set-ss-cover-art-player"));
    var coverArtEntityInp = entityInput(
      "sp-set-ss-cover-art-player",
      state.coverArtMediaPlayerEntity,
      "e.g. media_player.living_room",
      ["media_player"]);
    coverArtEntityField.appendChild(coverArtEntityInp);
    coverArtOptions.appendChild(coverArtEntityField);
    bindTextPost(coverArtEntityInp, entityName("screen_saver_cover_art_entity"), {
      onBlur: function (value) { state.coverArtMediaPlayerEntity = value; },
    });
    els.setCoverArtMediaPlayer = coverArtEntityInp;

    var coverArtDelayField = document.createElement("div");
    coverArtDelayField.className = "sp-field";
    coverArtDelayField.appendChild(fieldLabel("Show Cover Art After", "sp-set-ss-cover-art-delay"));
    var coverArtDelaySelect = document.createElement("select");
    coverArtDelaySelect.className = "sp-select";
    coverArtDelaySelect.id = "sp-set-ss-cover-art-delay";
    [
      { label: "5 seconds", value: 5 },
      { label: "10 seconds", value: 10 },
      { label: "30 seconds", value: 30 },
      { label: "1 minute", value: 60 },
      { label: "5 minutes", value: 300 },
    ].forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      coverArtDelaySelect.appendChild(o);
    });
    coverArtDelaySelect.addEventListener("change", function () {
      state.coverArtDelay = parseFloat(this.value) || 0;
      postNumber(entityName("screen_saver_cover_art_delay"), state.coverArtDelay);
    });
    coverArtDelayField.appendChild(coverArtDelaySelect);
    coverArtOptions.appendChild(coverArtDelayField);
    els.setCoverArtDelay = coverArtDelaySelect;

    var trackOverlayField = document.createElement("div");
    trackOverlayField.className = "sp-field";
    trackOverlayField.appendChild(fieldLabel("Track Overlay Duration", "sp-set-ss-track-overlay"));
    var trackOverlaySelect = document.createElement("select");
    trackOverlaySelect.className = "sp-select";
    trackOverlaySelect.id = "sp-set-ss-track-overlay";
    [
      { label: "Hidden", value: 0 },
      { label: "3 seconds", value: 3 },
      { label: "5 seconds", value: 5 },
      { label: "10 seconds", value: 10 },
      { label: "30 seconds", value: 30 },
      { label: "1 minute", value: 60 },
    ].forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      trackOverlaySelect.appendChild(o);
    });
    trackOverlaySelect.addEventListener("change", function () {
      state.coverArtTrackOverlayDuration = parseFloat(this.value) || 0;
      postNumber(entityName("screen_saver_track_overlay_duration"), state.coverArtTrackOverlayDuration);
    });
    trackOverlayField.appendChild(trackOverlaySelect);
    coverArtOptions.appendChild(trackOverlayField);
    els.setCoverArtTrackOverlayDuration = trackOverlaySelect;

    var coverArtProgressBarToggle = toggleRow(
      "Show Track Progress Bar",
      "sp-set-ss-cover-art-progress-bar",
      state.coverArtProgressBarOn);
    coverArtOptions.appendChild(coverArtProgressBarToggle.row);
    coverArtProgressBarToggle.input.addEventListener("change", function () {
      state.coverArtProgressBarOn = this.checked;
      postSwitch(entityName("screen_saver_cover_art_progress_bar"), state.coverArtProgressBarOn);
    });
    els.setCoverArtProgressBarToggle = coverArtProgressBarToggle.input;

    var coverArtOpenSubpageToggle = toggleRow(
      "Open Media Subpage While Playing",
      "sp-set-ss-cover-art-open-media",
      state.coverArtOpenMediaSubpageOn);
    coverArtOptions.appendChild(coverArtOpenSubpageToggle.row);
    coverArtOpenSubpageToggle.input.addEventListener("change", function () {
      state.coverArtOpenMediaSubpageOn = this.checked;
      syncCoverArtScreensaverUi();
      postSwitch(entityName("screen_saver_open_media_subpage"), state.coverArtOpenMediaSubpageOn);
    });
    els.setCoverArtOpenMediaSubpageToggle = coverArtOpenSubpageToggle.input;

    var coverArtSubpageField = document.createElement("div");
    coverArtSubpageField.className = "sp-field sp-cond-field";
    coverArtSubpageField.appendChild(fieldLabel("Media Subpage", "sp-set-ss-cover-art-media-subpage"));
    var coverArtSubpageSelect = document.createElement("select");
    coverArtSubpageSelect.className = "sp-select";
    coverArtSubpageSelect.id = "sp-set-ss-cover-art-media-subpage";
    coverArtSubpageSelect.addEventListener("change", function () {
      state.coverArtMediaSubpageTarget = this.value || "";
      postText(entityName("screen_saver_media_subpage"), state.coverArtMediaSubpageTarget);
    });
    coverArtSubpageField.appendChild(coverArtSubpageSelect);
    coverArtOptions.appendChild(coverArtSubpageField);
    els.setCoverArtMediaSubpageField = coverArtSubpageField;
    els.setCoverArtMediaSubpage = coverArtSubpageSelect;

    els.setCoverArtOptions = coverArtOptions;
    coverArtBody.appendChild(coverArtOptions);
  }

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
  bindTextPost(presInp, entityName("presence_sensor_entity"), {});
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
  syncCoverArtScreensaverUi();

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
    postText(entityName("screensaver_mode"), "disabled");
  });
  timerBtn.addEventListener("click", function () {
    setSsMode("timer");
    state.screensaverMode = "timer";
    postText(entityName("screensaver_mode"), "timer");
  });
  sensorBtn.addEventListener("click", function () {
    setSsMode("sensor");
    state.screensaverMode = "sensor";
    postText(entityName("screensaver_mode"), "sensor");
  });
  els.setSsMode = setSsMode;
  setSsMode(ssMode);

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
    postNumber(entityName("home_screen_timeout"), this.value);
  });
  idleBody.appendChild(hsSelect);
  els.setHSTimeout = hsSelect;
  var idleBadge = document.createElement("span");
  idleBadge.setAttribute("aria-label", "Idle on");
  idleBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
  els.setIdleBadge = idleBadge;
  syncIdleUi();
  appendSettingsGroupSection(sleepScheduleBody, "Idle", idleBody, idleBadge);
  appendSettingsGroupSection(sleepScheduleBody, "Screensaver", ssBody, ssBadge);
  if (!isEpaperPreview()) {
    appendSettingsGroupSection(sleepScheduleBody, "Media Cover Art", coverArtBody);
  }
  appendSettingsGroupSection(sleepScheduleBody, "Night Schedule", scheduleBody, scheduleBadge);

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
  appendSettingsGroupSection(systemBody, "Backup", backupBody);

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
      state.firmwareInstallStatus = updateReady ? "Installing update\u2026" : "Checking update before install\u2026";
      state.firmwareChecking = false;
      renderFirmwareUpdateStatus();
      if (updateReady) {
        clearFirmwareWebOtaFallback();
        postFirmwareUpdateInstall();
      } else {
        postFirmwareUpdateCheck();
        scheduleFirmwareWebOtaFallback();
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
    postSwitch(entityName("firmware_auto_update"), this.checked);
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
    postSelect(entityName("firmware_update_frequency"), this.value);
  });
  freqWrap.appendChild(freqSelect);
  fwBody.appendChild(freqWrap);
  els.updateFreqWrap = freqWrap;
  els.setUpdateFreq = freqSelect;
  syncFirmwareUpdateUi();

  appendSettingsGroupSection(systemBody, "Firmware", fwBody);

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
      postDeveloperExperimentalFeatures(state.developerExperimentalFeatures);
      scheduleRender();
    });
    els.setDeveloperExperimentalFeatures = experimentalToggle.input;
    appendSettingsGroupSection(systemBody, "Developer", devBody);
  }

  config.appendChild(makeCollapsibleCard("Display", displayBody, true));
  config.appendChild(makeCollapsibleCard("Sleep & Schedule", sleepScheduleBody, true));
  config.appendChild(makeCollapsibleCard("System", systemBody, true));

  page.appendChild(config);
  page.appendChild(buildApplyBar());

  parent.appendChild(page);
  els.settingsPage = page;
}

function settingsGroupBody() {
  var body = document.createElement("div");
  body.className = "sp-settings-group";
  return body;
}

function appendSettingsGroupSection(parent, title, bodyElement, badgeElement) {
  var section = document.createElement("section");
  section.className = "sp-settings-section";

  var header = document.createElement("div");
  header.className = "sp-settings-section-header";

  var heading = document.createElement("h4");
  heading.textContent = title;
  header.appendChild(heading);
  if (badgeElement) header.appendChild(badgeElement);

  section.appendChild(header);
  section.appendChild(bodyElement);
  parent.appendChild(section);
}

// ── Settings sync helpers ───────────────────────────────────────────

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

function syncCoverArtScreensaverUi() {
  syncCoverArtSubpageOptions();
  if (els.setCoverArtToggle) {
    els.setCoverArtToggle.checked = !!state.coverArtScreensaverOn;
  }
  if (els.setCoverArtOptions) {
    els.setCoverArtOptions.classList.toggle("sp-visible", !!state.coverArtScreensaverOn);
  }
  if (els.setCoverArtDelay) {
    if (state.coverArtDelay < 5) {
      state.coverArtDelay = 10;
      postNumber(entityName("screen_saver_cover_art_delay"), state.coverArtDelay);
    }
    setSelectValue(els.setCoverArtDelay, state.coverArtDelay, formatDuration(state.coverArtDelay));
  }
  if (els.setCoverArtTrackOverlayDuration) {
    var value = state.coverArtTrackOverlayDuration;
    setSelectValue(
      els.setCoverArtTrackOverlayDuration,
      value,
      value > 0 ? formatDuration(value) : "Hidden");
  }
  if (els.setCoverArtProgressBarToggle) {
    els.setCoverArtProgressBarToggle.checked = !!state.coverArtProgressBarOn;
  }
  if (els.setCoverArtOpenMediaSubpageToggle) {
    els.setCoverArtOpenMediaSubpageToggle.checked = !!state.coverArtOpenMediaSubpageOn;
  }
  if (els.setCoverArtMediaSubpageField) {
    els.setCoverArtMediaSubpageField.classList.toggle("sp-visible", !!state.coverArtOpenMediaSubpageOn);
  }
}

function syncCoverArtSubpageOptions() {
  if (!els.setCoverArtMediaSubpage) return;

  var select = els.setCoverArtMediaSubpage;
  var current = state.coverArtMediaSubpageTarget || "";
  select.innerHTML = "";

  var foundCurrent = current === "";
  for (var i = 0; i < state.buttons.length; i++) {
    var b = state.buttons[i] || {};
    if (b.type !== "subpage") continue;
    var slot = i + 1;
    var label = (b.label || "").trim() || ("Subpage " + slot);
    var opt = document.createElement("option");
    opt.value = "slot:" + slot;
    opt.textContent = label;
    if (opt.value === current) foundCurrent = true;
    select.appendChild(opt);
  }

  if (current && !foundCurrent) {
    var saved = document.createElement("option");
    saved.value = current;
    saved.textContent = current + " (saved)";
    select.appendChild(saved);
  }

  select.value = current && foundCurrent ? current : "";
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
    postSwitch(entityName("screen_saver_clock"), state.clockScreensaverOn);
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
