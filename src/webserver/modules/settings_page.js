// ── Settings Page ──────────────────────────────────────────────────────

function settingsStatusHeader(title) {
  var header = document.createElement("div");
  header.className = "sp-settings-status-header";

  var label = document.createElement("div");
  label.className = "sp-settings-status-title";
  label.textContent = title;
  header.appendChild(label);

  return header;
}

function appendSettingsSection(parent, title, cards) {
  var visibleCards = cards.filter(Boolean);
  if (!visibleCards.length) return;

  parent.appendChild(settingsStatusHeader(title));
  visibleCards.forEach(function (card) {
    parent.appendChild(card);
  });
}

function coverArtTrackOverlayDurationSupported() {
  return !!(CFG && CFG.coverArtSquareOverlay);
}

function infoPanel(id, text) {
  var panel = document.createElement("div");
  panel.className = "sp-info-panel";
  panel.id = id;
  panel.setAttribute("role", "note");
  var icon = document.createElement("span");
  icon.className = "mdi mdi-information-outline";
  icon.setAttribute("aria-hidden", "true");
  var message = document.createElement("span");
  message.textContent = text;
  panel.appendChild(icon);
  panel.appendChild(message);
  return panel;
}

function buildSettingsPage(parent) {
  var page = document.createElement("div");
  page.id = "sp-settings";
  page.className = "sp-page";

  var config = document.createElement("div");
  config.className = "sp-config fade-in";

  var appearBody = document.createElement("div");

  if (isEpaperPreview()) {
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
  } else {
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

  var appearanceCard = makeCollapsibleCard("Appearance", appearBody, true);

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
    renderPreview();
  });
  languageField.appendChild(languageSelect);
  languageBody.appendChild(languageField);
  var languageCard = makeCollapsibleCard("Language", languageBody, true);
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

  var brightnessManualTimes = condField();
  var dawnTime = createTimeInput("Dawn", "sp-set-brightness-dawn-time", state.brightnessDawnTime, "06:00", function (value) {
    state.brightnessDawnTime = normalizeTimeOfDay(value, "06:00");
    postBrightnessDawnTime(state.brightnessDawnTime);
    syncScreenScheduleUi();
  });
  brightnessManualTimes.appendChild(dawnTime.wrap);
  els.setBrightnessDawnTime = dawnTime.input;

  var duskTime = createTimeInput("Dusk", "sp-set-brightness-dusk-time", state.brightnessDuskTime, "18:00", function (value) {
    state.brightnessDuskTime = normalizeTimeOfDay(value, "18:00");
    postBrightnessDuskTime(state.brightnessDuskTime);
    syncScreenScheduleUi();
  });
  brightnessManualTimes.appendChild(duskTime.wrap);
  els.setBrightnessDuskTime = duskTime.input;
  blBody.appendChild(brightnessManualTimes);
  els.setBrightnessManualTimes = brightnessManualTimes;

  var sunInfo = document.createElement("div");
  sunInfo.className = "sp-sun-info";
  sunInfo.id = "sp-sun-info";
  blBody.appendChild(sunInfo);
  els.sunInfo = sunInfo;
  updateSunInfo();

  var backlightCard = makeCollapsibleCard("Backlight", blBody, true);

  var scheduleBody = document.createElement("div");
  scheduleBody.appendChild(infoPanel(
    "sp-night-schedule-info",
    "Time-based Night Schedule overrides screensaver presence wake and Media Cover Art while it is active. Use Sensor mode when you want presence to control the night schedule."
  ));
  scheduleBody.appendChild(fieldLabel("Mode"));
  var scheduleSegment = document.createElement("div");
  scheduleSegment.className = "sp-segment sp-screensaver-mode";
  var scheduleDisabledBtn = document.createElement("button");
  scheduleDisabledBtn.textContent = "Disabled";
  scheduleDisabledBtn.type = "button";
  var scheduleTimeBtn = document.createElement("button");
  scheduleTimeBtn.textContent = "Time";
  scheduleTimeBtn.type = "button";
  var scheduleSensorBtn = document.createElement("button");
  scheduleSensorBtn.textContent = "Sensor";
  scheduleSensorBtn.type = "button";
  scheduleSegment.appendChild(scheduleDisabledBtn);
  scheduleSegment.appendChild(scheduleTimeBtn);
  scheduleSegment.appendChild(scheduleSensorBtn);
  scheduleBody.appendChild(scheduleSegment);
  els.setScheduleModeButtons = {
    disabled: scheduleDisabledBtn,
    time: scheduleTimeBtn,
    sensor: scheduleSensorBtn,
  };

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

  var scheduleSensor = document.createElement("div");
  scheduleSensor.className = "sp-schedule-times";
  var schedulePresenceField = document.createElement("div");
  schedulePresenceField.className = "sp-field";
  schedulePresenceField.appendChild(fieldLabel("Presence Entity", "sp-set-schedule-presence"));
  var schedulePresInp = entityInput("sp-set-schedule-presence", state.presenceEntity, "Presence sensor entity", ["binary_sensor", "sensor"]);
  schedulePresenceField.appendChild(schedulePresInp);
  scheduleSensor.appendChild(schedulePresenceField);
  bindTextPost(schedulePresInp, entityName("presence_sensor_entity"), {});
  scheduleBody.appendChild(scheduleSensor);
  els.setScheduleSensor = scheduleSensor;
  els.setSchedulePresence = schedulePresInp;

  function setScheduleTrigger(trigger) {
    state._scheduleTriggerReceived = true;
    state.scheduleTrigger = normalizeScheduleTrigger(trigger, state.scheduleEnabled);
    state.scheduleEnabled = state.scheduleTrigger !== "disabled";
    postScreenScheduleTrigger(state.scheduleTrigger);
    postScreenScheduleEnabled(state.scheduleEnabled);
    syncScreenScheduleUi();
  }

  scheduleDisabledBtn.addEventListener("click", function () {
    setScheduleTrigger("disabled");
  });
  scheduleTimeBtn.addEventListener("click", function () {
    setScheduleTrigger("time");
  });
  scheduleSensorBtn.addEventListener("click", function () {
    setScheduleTrigger("sensor");
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

  var timeSettingsCard = makeCollapsibleCard("Time Settings", clockBody, true);

  var clockBarBody = document.createElement("div");

  var clockBar = toggleRow("Show Clock Bar", "sp-set-clock-bar", state.clockBarOn);
  clockBarBody.appendChild(clockBar.row);
  els.setClockBarToggle = clockBar.input;
  clockBar.input.addEventListener("change", function () {
    state.clockBarOn = this.checked;
    state._clockBarStateValues = { local: state.clockBarOn };
    syncClockBarUi();
    postClockBar(state.clockBarOn);
  });

  var clockBarBadge = document.createElement("span");
  clockBarBadge.setAttribute("aria-label", "Clock bar on");
  clockBarBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
  els.setClockBarBadge = clockBarBadge;
  syncClockBarUi();
  syncTemperatureUi();
  var clockBarCard = makeCollapsibleCard("Clock Bar", clockBarBody, true, clockBarBadge);

  var rotationCard = null;
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
    rotationCard = makeCollapsibleCard("Rotation", rotationBody, true);
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
  var temperatureCard = makeCollapsibleCard("Temperature Settings", tempBody, true);

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

  var coverArtBody = document.createElement("div");
  if (!isEpaperPreview()) {
    coverArtBody.appendChild(infoPanel(
      "sp-cover-art-info",
      "Media Cover Art overrides existing screensaver settings while the selected media player is playing."
    ));

    var coverArtToggle = toggleRow(
      "Show Cover Art",
      "sp-set-ss-cover-art-enable",
      state.coverArtScreensaverOn);
    coverArtBody.appendChild(coverArtToggle.row);
    coverArtToggle.input.addEventListener("change", function () {
      state.coverArtScreensaverOn = this.checked;
      if (!state.coverArtScreensaverOn && state.mediaPlayerSleepPreventionOn) {
        state.mediaPlayerSleepPreventionOn = false;
        syncMediaPlayerSleepPreventionUi();
        postSwitch(entityName("screen_saver_media_player_sleep_prevention"), false);
      }
      syncCoverArtScreensaverUi();
      postSwitch(entityName("screen_saver_cover_art"), state.coverArtScreensaverOn);
    });
    els.setCoverArtToggle = coverArtToggle.input;

    var sleepPreventionToggle = toggleRow(
      "Keep Screen Awake During Playback",
      "sp-set-ss-media-sleep-prevention",
      state.mediaPlayerSleepPreventionOn);
    coverArtBody.appendChild(sleepPreventionToggle.row);
    sleepPreventionToggle.input.addEventListener("change", function () {
      state.mediaPlayerSleepPreventionOn = this.checked;
      syncMediaPlayerSleepPreventionUi();
      syncCoverArtScreensaverUi();
      postSwitch(entityName("screen_saver_media_player_sleep_prevention"), state.mediaPlayerSleepPreventionOn);
    });
    els.setMediaPlayerSleepPreventionToggle = sleepPreventionToggle.input;

    var coverArtOptions = condField();
    var coverArtOnlyOptions = condField();

    var coverArtEntityField = document.createElement("div");
    coverArtEntityField.className = "sp-field";
    coverArtEntityField.appendChild(fieldLabel("Media Player", "sp-set-ss-cover-art-player"));
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
    coverArtDelayField.appendChild(fieldLabel("Show After", "sp-set-ss-cover-art-delay"));
    var coverArtDelaySelect = document.createElement("select");
    coverArtDelaySelect.className = "sp-select";
    coverArtDelaySelect.id = "sp-set-ss-cover-art-delay";
    [
      { label: "Immediately", value: 0 },
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
    coverArtOnlyOptions.appendChild(coverArtDelayField);
    els.setCoverArtDelay = coverArtDelaySelect;

    if (coverArtTrackOverlayDurationSupported()) {
      var trackOverlayField = document.createElement("div");
      trackOverlayField.className = "sp-field";
      trackOverlayField.appendChild(fieldLabel("Track Overlay Duration", "sp-set-ss-track-overlay"));
      var trackOverlaySelect = document.createElement("select");
      trackOverlaySelect.className = "sp-select";
      trackOverlaySelect.id = "sp-set-ss-track-overlay";
      [
        { label: "Never", value: 0 },
        { label: "3 seconds", value: 3 },
        { label: "5 seconds", value: 5 },
        { label: "10 seconds", value: 10 },
        { label: "15 seconds", value: 15 },
        { label: "20 seconds", value: 20 },
        { label: "30 seconds", value: 30 },
        { label: "60 seconds", value: 60 },
        { label: "Always", value: -1 },
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
      coverArtOnlyOptions.appendChild(trackOverlayField);
      els.setCoverArtTrackOverlayDuration = trackOverlaySelect;
    }

    var coverArtHideExternalInputToggle = toggleRow(
      "Hide for external sources",
      "sp-set-ss-cover-art-hide-external-input",
      state.coverArtHideExternalInputOn);
    coverArtOptions.appendChild(coverArtHideExternalInputToggle.row);
    coverArtHideExternalInputToggle.input.addEventListener("change", function () {
      state.coverArtHideExternalInputOn = this.checked;
      postCoverArtHideExternalInput(state.coverArtHideExternalInputOn);
    });
    els.setCoverArtHideExternalInputToggle = coverArtHideExternalInputToggle.input;

    els.setCoverArtOnlyOptions = coverArtOnlyOptions;
    coverArtOptions.appendChild(coverArtOnlyOptions);

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
  var presInp = entityInput("sp-set-presence", state.presenceEntity, "Presence sensor entity", ["binary_sensor", "sensor"]);
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
    postNumber(entityName("home_screen_timeout"), this.value);
  });
  idleBody.appendChild(hsSelect);
  els.setHSTimeout = hsSelect;
  var idleBadge = document.createElement("span");
  idleBadge.setAttribute("aria-label", "Idle on");
  idleBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
  els.setIdleBadge = idleBadge;
  syncIdleUi();
  var idleCard = makeCollapsibleCard("Idle", idleBody, true, idleBadge);
  var coverArtCard = null;
  if (!isEpaperPreview()) {
    var coverArtBadge = document.createElement("span");
    coverArtBadge.setAttribute("aria-label", "Media cover art on");
    coverArtBadge.innerHTML = '<span class="sp-card-badge-dot"></span><span>ON</span>';
    els.setCoverArtBadge = coverArtBadge;
    syncCoverArtScreensaverUi();
    coverArtCard = makeCollapsibleCard("Media Cover Art", coverArtBody, true, coverArtBadge);
  }

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
  var backupCard = makeCollapsibleCard("Backup", backupBody, true);

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
      var selectedInfo = selectedFirmwareInfo();
      var installingLatest = selectedFirmwareIsLatest();
      var updateReady = installingLatest && firmwareUpdateAvailable();
      state.firmwareInstallTargetVersion = selectedInfo && selectedInfo.latest_version ?
        selectedInfo.latest_version :
        state.firmwareLatestVersion;
      state.firmwareInstallPostPending = installingLatest && !updateReady;
      state.firmwareChecking = false;
      if (updateReady) {
        state.firmwareUpdateState = "INSTALLING";
        state.firmwareInstallStatus = "Installing update\u2026";
        renderFirmwareUpdateStatus();
        clearFirmwareWebOtaFallback();
        postFirmwareUpdateInstall();
        startFirmwareInstallRefresh();
      } else if (installingLatest) {
        state.firmwareUpdateState = "INSTALLING";
        state.firmwareInstallStatus = "Checking update before install\u2026";
        renderFirmwareUpdateStatus();
        postFirmwareUpdateCheck();
        scheduleFirmwareWebOtaFallback();
        startFirmwareInstallRefresh();
      } else {
        installPublicFirmwareViaWebOta(selectedInfo);
      }
      return;
    }
    state.firmwareChecking = true;
    renderFirmwareUpdateStatus();
    postFirmwareUpdateCheck();
    getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
      setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
    });
    getJsonQuietly(publicFirmwareVersionsUrl(), function (d) {
      setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
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

  var fwVersionField = document.createElement("div");
  fwVersionField.className = "sp-field sp-fw-version-field";
  fwVersionField.style.display = "none";
  fwVersionField.appendChild(fieldLabel("Install Version", "sp-set-firmware-version"));
  var fwVersionSelect = document.createElement("select");
  fwVersionSelect.className = "sp-select";
  fwVersionSelect.id = "sp-set-firmware-version";
  fwVersionSelect.addEventListener("change", function () {
    state.firmwareSelectedVersion = this.value;
    renderFirmwareUpdateStatus();
  });
  fwVersionField.appendChild(fwVersionSelect);
  fwBody.appendChild(fwVersionField);
  els.fwVersionField = fwVersionField;
  els.fwVersionSelect = fwVersionSelect;
  syncFirmwareVersionSelect();

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

  var firmwareCard = makeCollapsibleCard("Firmware", fwBody, true);

  var developerCard = null;
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
    developerCard = makeCollapsibleCard("Developer", devBody, true);
  }

  appendSettingsSection(config, "Display", [
    appearanceCard,
    backlightCard,
    clockBarCard,
    rotationCard,
  ]);
  appendSettingsSection(config, "Sleep & Schedule", [
    idleCard,
    screensaverCard,
    coverArtCard,
    scheduleCard,
  ]);
  appendSettingsSection(config, "System", [
    languageCard,
    timeSettingsCard,
    temperatureCard,
    backupCard,
    firmwareCard,
    developerCard,
  ]);

  page.appendChild(config);
  page.appendChild(buildApplyBar());

  parent.appendChild(page);
  els.settingsPage = page;
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
  syncClockBarUi();

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
  if (els.setSensorMediaPlayerSleepPreventionToggle) {
    els.setSensorMediaPlayerSleepPreventionToggle.checked = !!state.mediaPlayerSleepPreventionOn;
  }
}

function syncCoverArtScreensaverUi() {
  if (els.setCoverArtToggle) {
    els.setCoverArtToggle.checked = !!state.coverArtScreensaverOn;
  }
  if (els.setCoverArtOptions) {
    els.setCoverArtOptions.classList.toggle(
      "sp-visible",
      !!state.coverArtScreensaverOn || !!state.mediaPlayerSleepPreventionOn);
  }
  if (els.setCoverArtOnlyOptions) {
    els.setCoverArtOnlyOptions.classList.toggle("sp-visible", !!state.coverArtScreensaverOn);
  }
  if (els.setCoverArtBadge) {
    els.setCoverArtBadge.className = "sp-card-badge" + (state.coverArtScreensaverOn ? "" : " sp-hidden");
  }
  if (els.setCoverArtDelay) {
    var coverArtDelay = Math.max(0, parseFloat(state.coverArtDelay) || 0);
    state.coverArtDelay = coverArtDelay;
    setSelectValue(
      els.setCoverArtDelay,
      coverArtDelay,
      coverArtDelay > 0 ? formatDuration(coverArtDelay) : "Immediately");
  }
  if (els.setCoverArtTrackOverlayDuration) {
    var value = state.coverArtTrackOverlayDuration;
    setSelectValue(
      els.setCoverArtTrackOverlayDuration,
      value,
      value < 0 ? "Always" : value > 0 ? formatDuration(value) : "Never");
  }
  if (els.setCoverArtHideExternalInputToggle) {
    els.setCoverArtHideExternalInputToggle.checked = !!state.coverArtHideExternalInputOn;
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
  clockBrightnessField.className = "sp-clock-brightness-field";
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

function createTimeInput(label, id, initial, fallback, onChange) {
  var wrap = document.createElement("div");
  wrap.className = "sp-field";
  wrap.appendChild(fieldLabel(label, id));
  var input = document.createElement("input");
  input.type = "time";
  input.className = "sp-input";
  input.id = id;
  input.step = "60";
  input.value = normalizeTimeOfDay(initial, fallback);
  input.addEventListener("change", function () {
    var value = normalizeTimeOfDay(this.value, fallback);
    this.value = value;
    onChange(value);
  });
  wrap.appendChild(input);
  return { wrap: wrap, input: input };
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
