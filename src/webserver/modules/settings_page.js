// ── Settings Page ──────────────────────────────────────────────────────
// @web-module-requires: state, language_state, environment_state, screen_rotation_state, screen_schedule_state, screen_schedule_post_api, ntp_state, appearance_state, idle_state, artwork_state, artwork_post_api, screensaver_state, firmware_version_state, clock_bar_state, clock_bar_post_api, entity_state, firmware_update_state, screensaver_timeout, c6_firmware_ui, api, public_firmware_install, state_loader_api, controls, controls_shell

function buildSettingsPage(parent) {
  var page = document.createElement("div");
  page.id = "sp-settings";
  page.className = "sp-page";

  var config = document.createElement("div");
  config.className = "sp-config fade-in";

  var appearBody = document.createElement("div");

  var onColor = colorField("sp-set-on-color", DEFAULT_COLOR_PRESET.on, function (hex) {
    postText(entityName("button_on_color"), hex);
  });
  appearBody.appendChild(onColor);
  els.setOnColor = onColor;

  var appearanceResetButton = createActionButton(
    "sp-icon-button sp-card-header-action",
    "",
    "restore",
    "Reset colours to defaults"
  );
  appearanceResetButton.title = "Reset colours";
  appearanceResetButton.addEventListener("click", function (event) {
    event.stopPropagation();
    resetAppearanceColors(true);
  });
  var appearanceCard = makeCollapsibleCard("Appearance", appearBody, true, null, appearanceResetButton);

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

  var scheduleCard = buildScreenScheduleSettingsCard();

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

  var timeSettingsCard = makeCollapsibleCard("Time", clockBody, true);

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

  var clockBarBadge = statusBadge("Clock bar on");
  els.setClockBarBadge = clockBarBadge;
  syncClockBarUi();
  syncTemperatureUi();
  var clockBarCard = makeCollapsibleCard("Clock Bar", clockBarBody, true, clockBarBadge);

  var voiceServicesCard = null;
  if (CFG.features && CFG.features.voiceServices) {
    var voiceServicesBody = document.createElement("div");
    var voiceServices = toggleRow("Voice Services", "sp-set-voice-services", state.voiceServicesOn);
    voiceServicesBody.appendChild(voiceServices.row);
    els.setVoiceServicesToggle = voiceServices.input;
    voiceServices.input.addEventListener("change", function () {
      state.voiceServicesOn = this.checked;
      syncClockBarUi();
      postVoiceServices(state.voiceServicesOn);
    });
    voiceServicesCard = makeCollapsibleCard("Voice Services", voiceServicesBody, true);
    els.voiceServicesCard = voiceServicesCard;
  }

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
  var temperatureCard = makeCollapsibleCard("Temperature", tempBody, true);

  var ssBody = document.createElement("div");
  var ssMode = getActiveScreensaverMode();

  ssBody.appendChild(fieldLabel("Mode"));
  var ssModeSegment = segmentControl([
    ["disabled", "Disabled"],
    ["timer", "Timer"],
    ["sensor", "Sensor"],
  ], ssMode, function (mode) {
    setSsMode(mode);
    state.screensaverMode = mode;
    postScreensaverMode(mode);
  }, "sp-segment sp-screensaver-mode");
  var disabledBtn = ssModeSegment.buttons.disabled;
  var timerBtn = ssModeSegment.buttons.timer;
  var sensorBtn = ssModeSegment.buttons.sensor;
  ssBody.appendChild(ssModeSegment.segment);

  var timerPanel = document.createElement("div");

  var timeoutControl = selectField("Timeout", "sp-set-ss-timeout", [], state.screensaverTimeout, function () {
    var n = parseFloat(this.value);
    if (isFinite(n)) state.screensaverTimeout = n;
    postScreensaverTimeout(this.value);
  });
  var timeoutSelect = timeoutControl.select;
  timerPanel.appendChild(timeoutControl.field);

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

  var coverArtCard = buildCoverArtSettingsCard();

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
  bindTextPost(presInp, entityName("presence_sensor_entity"), {
    post: postPresenceSensorEntity,
  });
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

  var ssBadge = statusBadge("Screensaver on");
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
    postHomeScreenTimeout(this.value);
  });
  idleBody.appendChild(hsSelect);
  els.setHSTimeout = hsSelect;
  var idleBadge = statusBadge("Idle on");
  els.setIdleBadge = idleBadge;
  syncIdleUi();
  var idleCard = makeCollapsibleCard("Idle", idleBody, true, idleBadge);

  var systemSettingsCards = buildSystemSettingsCards();

  appendSettingsSection(config, "Display", [
    appearanceCard,
    backlightCard,
    clockBarCard,
    coverArtCard,
    voiceServicesCard,
    rotationCard,
  ]);
  appendSettingsSection(config, "Sleep & Schedule", [
    idleCard,
    screensaverCard,
    scheduleCard,
  ]);
  appendSettingsSection(config, "Preferences", [
    languageCard,
    timeSettingsCard,
    temperatureCard,
  ]);
  appendSettingsSection(config, "System", [
    systemSettingsCards.backupCard,
    systemSettingsCards.firmwareCard,
    systemSettingsCards.wifiFirmwareCard,
    systemSettingsCards.homeAssistantSettingsCard,
  ]);

  page.appendChild(config);
  page.appendChild(buildApplyBar());

  parent.appendChild(page);
  els.settingsPage = page;
}
