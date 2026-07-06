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

function openVoiceServicesSettings() {
  if (isConfigLocked() || !els.voiceServicesCard) return;
  switchTab("settings");
  els.voiceServicesCard.classList.remove("collapsed");
  els.voiceServicesCard.scrollIntoView({ block: "center", behavior: "smooth" });
  if (els.setVoiceServicesToggle) {
    window.setTimeout(function () { els.setVoiceServicesToggle.focus(); }, 150);
  }
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

function statusBadge(label) {
  var badge = document.createElement("span");
  badge.setAttribute("aria-label", label);
  badge.appendChild(textSpan("", "sp-card-badge-dot"));
  badge.appendChild(textSpan("ON"));
  return badge;
}

function inlineDisclosure(title, bodyElement, defaultOpen) {
  var panel = document.createElement("div");
  panel.className = "sp-disclosure" + (defaultOpen ? " sp-open" : "");
  var button = document.createElement("button");
  button.type = "button";
  button.className = "sp-disclosure-button";
  button.setAttribute("aria-expanded", defaultOpen ? "true" : "false");
  var label = document.createElement("span");
  label.textContent = title;
  var chevron = createDisclosureChevron("sp-disclosure-chevron");
  button.appendChild(label);
  button.appendChild(chevron);
  var body = document.createElement("div");
  body.className = "sp-disclosure-body";
  body.appendChild(bodyElement);
  button.addEventListener("click", function () {
    var open = !panel.classList.contains("sp-open");
    panel.classList.toggle("sp-open", open);
    button.setAttribute("aria-expanded", open ? "true" : "false");
  });
  panel.appendChild(button);
  panel.appendChild(body);
  return panel;
}

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

  var scheduleBody = document.createElement("div");
  scheduleBody.appendChild(infoPanel(
    "sp-night-schedule-info",
    "Time-based Night Schedule overrides screensaver presence wake and Media Cover Art while it is active. Use Sensor mode when you want presence to control the night schedule."
  ));
  scheduleBody.appendChild(fieldLabel("Mode"));
  var scheduleModeSegment = segmentControl([
    ["disabled", "Disabled"],
    ["time", "Time"],
    ["sensor", "Sensor"],
  ], state.scheduleTrigger, function (mode) {
    setScheduleTrigger(mode);
  }, "sp-segment sp-screensaver-mode");
  scheduleBody.appendChild(scheduleModeSegment.segment);
  els.setScheduleModeButtons = {
    disabled: scheduleModeSegment.buttons.disabled,
    time: scheduleModeSegment.buttons.time,
    sensor: scheduleModeSegment.buttons.sensor,
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

  var scheduleModeControl = selectField("At Night Time", "sp-set-schedule-mode", [
    { value: "screen_off", label: "Screen Off" },
    { value: "screen_dimmed", label: "Screen Dimmed" },
    { value: "clock", label: "Clock" },
  ], state.scheduleMode, function () {
    state.scheduleMode = normalizeScheduleMode(this.value);
    postScreenScheduleMode(state.scheduleMode);
    syncScreenScheduleUi();
  });
  var scheduleModeSelect = scheduleModeControl.select;
  scheduleTimes.appendChild(scheduleModeControl.field);
  els.setScheduleMode = scheduleModeSelect;

  var offScreenOptions = condField();
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
  var wakeTimeoutControl = selectField(
    "When Woken, Idle Time to Screen Off",
    "sp-set-schedule-wake-timeout",
    wakeTimeoutOptions,
    state.scheduleWakeTimeout,
    function () {
    state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(this.value);
    postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
    syncScreenScheduleUi();
  });
  var wakeTimeoutSelect = wakeTimeoutControl.select;
  offScreenOptions.appendChild(wakeTimeoutControl.field);
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
  bindTextPost(schedulePresInp, entityName("presence_sensor_entity"), {
    post: postPresenceSensorEntity,
  });
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

  var scheduleBadge = statusBadge("Schedule on");
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

  var coverArtBody = document.createElement("div");
  var coverArtToggle = toggleRow(
      "Show Cover Art",
      "sp-set-ss-cover-art-enable",
      state.coverArtScreensaverOn);
    coverArtBody.appendChild(coverArtToggle.row);
    coverArtToggle.input.addEventListener("change", function () {
      state.coverArtScreensaverOn = this.checked;
      syncCoverArtScreensaverUi();
      postCoverArtScreensaver(state.coverArtScreensaverOn);
    });
    els.setCoverArtToggle = coverArtToggle.input;

    var coverArtOptions = condField();
    var coverArtOnlyOptions = condField();
    var coverArtAdvancedBody = document.createElement("div");

    var sleepPreventionToggle = toggleRow(
      "Keep Screen Awake During Playback",
      "sp-set-ss-media-sleep-prevention",
      state.mediaPlayerSleepPreventionOn);
    coverArtOptions.appendChild(sleepPreventionToggle.row);
    sleepPreventionToggle.input.addEventListener("change", function () {
      state.mediaPlayerSleepPreventionOn = this.checked;
      syncMediaPlayerSleepPreventionUi();
      syncCoverArtScreensaverUi();
      postMediaPlayerSleepPrevention(state.mediaPlayerSleepPreventionOn);
    });
    els.setMediaPlayerSleepPreventionToggle = sleepPreventionToggle.input;

    var coverArtEntityField = document.createElement("div");
    coverArtEntityField.className = "sp-field";
    coverArtEntityField.appendChild(fieldLabel("Media Player Entity", "sp-set-ss-cover-art-player"));
    var coverArtEntityInp = entityInput(
      "sp-set-ss-cover-art-player",
      state.coverArtMediaPlayerEntity,
      "e.g. media_player.living_room",
      ["media_player"]);
    coverArtEntityField.appendChild(coverArtEntityInp);
    coverArtOnlyOptions.appendChild(coverArtEntityField);
    bindTextPost(coverArtEntityInp, entityName("screen_saver_cover_art_entity"), {
      onBlur: function (value) {
        state.coverArtMediaPlayerEntity = value;
        state.mediaPlayerSleepPreventionEntity = value;
      },
      post: function (value) {
        postCoverArtMediaPlayerEntity(value);
        postMediaPlayerSleepPreventionEntity(value);
      },
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
      postCoverArtDelay(state.coverArtDelay);
    });
    coverArtDelayField.appendChild(coverArtDelaySelect);
    coverArtOnlyOptions.appendChild(coverArtDelayField);
    els.setCoverArtDelay = coverArtDelaySelect;

    var coverArtTouchPauseField = document.createElement("div");
    coverArtTouchPauseField.className = "sp-field";
    coverArtTouchPauseField.appendChild(fieldLabel("After Touch, Show Again", "sp-set-ss-cover-art-touch-pause"));
    var coverArtTouchPauseSelect = document.createElement("select");
    coverArtTouchPauseSelect.className = "sp-select";
    coverArtTouchPauseSelect.id = "sp-set-ss-cover-art-touch-pause";
    [
      { label: "Immediately", value: 0 },
      { label: "1 minute", value: 60 },
      { label: "2 minutes", value: 120 },
      { label: "3 minutes", value: 180 },
      { label: "5 minutes", value: 300 },
    ].forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      coverArtTouchPauseSelect.appendChild(o);
    });
    coverArtTouchPauseSelect.addEventListener("change", function () {
      state.coverArtTouchPause = parseFloat(this.value) || 0;
      postCoverArtTouchPause(state.coverArtTouchPause);
    });
    coverArtTouchPauseField.appendChild(coverArtTouchPauseSelect);
    coverArtOnlyOptions.appendChild(coverArtTouchPauseField);
    els.setCoverArtTouchPause = coverArtTouchPauseSelect;

    if (coverArtTrackOverlayDurationSupported()) {
      var trackOverlayField = document.createElement("div");
      trackOverlayField.className = "sp-field";
      trackOverlayField.appendChild(fieldLabel("Show Track Details For", "sp-set-ss-track-overlay"));
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
        postCoverArtTrackOverlayDuration(state.coverArtTrackOverlayDuration);
      });
      trackOverlayField.appendChild(trackOverlaySelect);
      coverArtOnlyOptions.appendChild(trackOverlayField);
      els.setCoverArtTrackOverlayDuration = trackOverlaySelect;
    }

    var coverArtHideExternalInputToggle = toggleRow(
      "Hide for external source inputs",
      "sp-set-ss-cover-art-hide-external-input",
      state.coverArtHideExternalInputOn);
    coverArtAdvancedBody.appendChild(coverArtHideExternalInputToggle.row);
    coverArtHideExternalInputToggle.input.addEventListener("change", function () {
      state.coverArtHideExternalInputOn = this.checked;
      postCoverArtHideExternalInput(state.coverArtHideExternalInputOn);
    });
    els.setCoverArtHideExternalInputToggle = coverArtHideExternalInputToggle.input;

    state.coverArtFilteringEnabled = !!state.coverArtAttributeConditions;
    var coverArtFilterToggle = toggleRow(
      "Advanced Filtering",
      "sp-set-ss-cover-art-filtering",
      state.coverArtFilteringEnabled);
    coverArtAdvancedBody.appendChild(coverArtFilterToggle.row);
    coverArtFilterToggle.input.addEventListener("change", function () {
      state.coverArtFilteringEnabled = this.checked;
      if (!state.coverArtFilteringEnabled) {
        state.coverArtAttributeConditions = "";
        syncInput(els.setCoverArtConditions, "");
        postCoverArtConditions("");
      }
      syncCoverArtScreensaverUi();
    });
    els.setCoverArtFilterToggle = coverArtFilterToggle.input;

    var coverArtFilterOptions = condField();
    var coverArtConditionsField = document.createElement("div");
    coverArtConditionsField.className = "sp-field";
    coverArtConditionsField.appendChild(fieldLabel("Only Show When", "sp-set-ss-cover-art-conditions"));
    var coverArtConditionsInp = document.createElement("input");
    coverArtConditionsInp.className = "sp-input";
    coverArtConditionsInp.id = "sp-set-ss-cover-art-conditions";
    coverArtConditionsInp.type = "text";
    coverArtConditionsInp.maxLength = 240;
    coverArtConditionsInp.placeholder = "app_id=com.apple.TVMusic; media_content_type=music";
    coverArtConditionsInp.value = state.coverArtAttributeConditions || "";
    coverArtConditionsField.appendChild(coverArtConditionsInp);
    coverArtFilterOptions.appendChild(coverArtConditionsField);
    coverArtAdvancedBody.appendChild(coverArtFilterOptions);
    bindTextPost(coverArtConditionsInp, entityName("screen_saver_cover_art_conditions"), {
      onBlur: function (value) {
        state.coverArtAttributeConditions = value;
        state.coverArtFilteringEnabled = !!value || state.coverArtFilteringEnabled;
        syncCoverArtScreensaverUi();
      },
      post: postCoverArtConditions,
    });
    els.setCoverArtConditions = coverArtConditionsInp;
    els.setCoverArtFilterOptions = coverArtFilterOptions;

    coverArtOnlyOptions.appendChild(inlineDisclosure(
      "Advanced Options",
      coverArtAdvancedBody,
      !!state.coverArtAttributeConditions || !state.coverArtHideExternalInputOn));

    els.setCoverArtOnlyOptions = coverArtOnlyOptions;
    coverArtOptions.appendChild(coverArtOnlyOptions);
    els.setCoverArtOptions = coverArtOptions;
    coverArtBody.appendChild(coverArtOptions);

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
  var coverArtBadge = statusBadge("Media cover art on");
  els.setCoverArtBadge = coverArtBadge;
  syncCoverArtScreensaverUi();
  var coverArtCard = makeCollapsibleCard("Cover Art", coverArtBody, true, coverArtBadge);

  var backupBody = document.createElement("div");

  var backupRow = document.createElement("div");
  backupRow.className = "sp-backup-btns";

  var exportBtn = createActionButton("sp-backup-btn", "Export", "download");
  exportBtn.addEventListener("click", exportConfig);
  backupRow.appendChild(exportBtn);

  var importBtn = createActionButton("sp-backup-btn", "Import", "upload");
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

  var fwCheckBtn = createActionButton("sp-fw-btn", "Check for Update");
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
    state.autoUpdate = this.checked;
    postFirmwareAutoUpdate(state.autoUpdate);
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
    state.updateFrequency = this.value;
    postFirmwareUpdateFrequency(state.updateFrequency);
  });
  freqWrap.appendChild(freqSelect);
  fwBody.appendChild(freqWrap);
  els.updateFreqWrap = freqWrap;
  els.setUpdateFreq = freqSelect;
  syncFirmwareUpdateUi();

  var firmwareCard = makeCollapsibleCard("Firmware", fwBody, true);

  var wifiFirmwareBody = document.createElement("div");
  var c6CurrentRow = document.createElement("div");
  c6CurrentRow.className = "sp-fw-row sp-fw-info-row";
  var c6CurrentLabel = document.createElement("span");
  c6CurrentLabel.className = "sp-fw-label";
  c6CurrentLabel.textContent = "Current C6 Firmware";
  var c6CurrentValue = document.createElement("span");
  c6CurrentValue.className = "sp-fw-version";
  c6CurrentRow.appendChild(c6CurrentLabel);
  c6CurrentRow.appendChild(c6CurrentValue);
  wifiFirmwareBody.appendChild(c6CurrentRow);
  els.c6FirmwareCurrent = c6CurrentValue;

  var c6LatestRow = document.createElement("div");
  c6LatestRow.className = "sp-fw-row sp-fw-info-row";
  var c6LatestLabel = document.createElement("span");
  c6LatestLabel.className = "sp-fw-label";
  c6LatestLabel.textContent = "Available C6 Firmware";
  var c6LatestValue = document.createElement("span");
  c6LatestValue.className = "sp-fw-version";
  c6LatestRow.appendChild(c6LatestLabel);
  c6LatestRow.appendChild(c6LatestValue);
  wifiFirmwareBody.appendChild(c6LatestRow);
  els.c6FirmwareLatest = c6LatestValue;

  var c6Actions = document.createElement("div");
  c6Actions.className = "sp-fw-actions sp-fw-actions-full";
  var c6UpdateBtn = document.createElement("button");
  c6UpdateBtn.className = "sp-fw-btn";
  c6UpdateBtn.textContent = "Check for Update";
  c6UpdateBtn.addEventListener("click", function () {
    if (!state.c6FirmwareUpdateControlsSupported) return;
    if (c6FirmwareUpdateKnownAvailable() && state.c6FirmwareInstallControlsSupported) {
      state.c6FirmwareInstalling = true;
      state.c6FirmwareChecking = false;
      syncC6FirmwareUi();
      postC6FirmwareUpdateInstall();
      setTimeout(function () {
        refreshFirmwareVersion();
      }, 5000);
      return;
    }
    state.c6FirmwareChecking = true;
    syncC6FirmwareUi();
    postC6FirmwareUpdateCheck();
    setTimeout(function () {
      state.c6FirmwareChecking = false;
      refreshFirmwareVersion();
      syncC6FirmwareUi();
    }, 10000);
  });
  c6Actions.appendChild(c6UpdateBtn);
  wifiFirmwareBody.appendChild(c6Actions);
  els.c6FirmwareUpdateBtn = c6UpdateBtn;

  var c6Status = document.createElement("div");
  c6Status.className = "sp-fw-status";
  wifiFirmwareBody.appendChild(c6Status);
  els.c6FirmwareStatus = c6Status;
  var wifiFirmwareCard = makeCollapsibleCard("WiFi", wifiFirmwareBody, true);
  els.c6FirmwareCard = wifiFirmwareCard;
  syncC6FirmwareUi();

  var homeAssistantSettingsBody = document.createElement("div");
  var haProtocolField = document.createElement("div");
  haProtocolField.className = "sp-field";
  haProtocolField.appendChild(fieldLabel("Home Assistant Protocol", "sp-set-ha-artwork-protocol"));
  var haProtocolSelect = document.createElement("select");
  haProtocolSelect.className = "sp-select";
  haProtocolSelect.id = "sp-set-ha-artwork-protocol";
  ["http", "https"].forEach(function (option) {
    var item = document.createElement("option");
    item.value = option;
    item.textContent = option;
    haProtocolSelect.appendChild(item);
  });
  haProtocolSelect.value = normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
  haProtocolSelect.addEventListener("change", function () {
    state.homeAssistantArtworkProtocol = normalizeHomeAssistantArtworkProtocol(this.value);
    this.value = state.homeAssistantArtworkProtocol;
    postHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
  });
  haProtocolField.appendChild(haProtocolSelect);
  homeAssistantSettingsBody.appendChild(haProtocolField);
  els.setHomeAssistantArtworkProtocol = haProtocolSelect;

  var haPortField = document.createElement("div");
  haPortField.className = "sp-field";
  haPortField.appendChild(fieldLabel("Home Assistant Port", "sp-set-ha-artwork-port"));
  var haPortInput = document.createElement("input");
  haPortInput.className = "sp-input sp-input--no-stepper";
  haPortInput.id = "sp-set-ha-artwork-port";
  haPortInput.type = "number";
  haPortInput.min = "1";
  haPortInput.max = "65535";
  haPortInput.step = "1";
  haPortInput.inputMode = "numeric";
  haPortInput.value = String(normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort));
  haPortInput.addEventListener("change", function () {
    state.coverArtHomeAssistantPort = normalizeHomeAssistantArtworkPort(this.value);
    this.value = String(state.coverArtHomeAssistantPort);
    postHomeAssistantArtworkPort(state.coverArtHomeAssistantPort);
  });
  haPortField.appendChild(haPortInput);
  homeAssistantSettingsBody.appendChild(haPortField);
  els.setCoverArtHomeAssistantPort = haPortInput;
  var homeAssistantSettingsCard = makeCollapsibleCard(
    "Home Assistant Settings",
    homeAssistantSettingsBody,
    true);

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
    backupCard,
    firmwareCard,
    wifiFirmwareCard,
    homeAssistantSettingsCard,
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
      !!state.coverArtScreensaverOn);
  }
  if (els.setCoverArtOnlyOptions) {
    els.setCoverArtOnlyOptions.classList.toggle(
      "sp-visible",
      !!state.coverArtScreensaverOn);
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
  if (els.setCoverArtTouchPause) {
    var coverArtTouchPause = Math.max(0, parseFloat(state.coverArtTouchPause) || 0);
    state.coverArtTouchPause = coverArtTouchPause;
    setSelectValue(
      els.setCoverArtTouchPause,
      coverArtTouchPause,
      coverArtTouchPause > 0 ? formatDuration(coverArtTouchPause) : "Immediately");
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
  if (els.setHomeAssistantArtworkProtocol) {
    els.setHomeAssistantArtworkProtocol.value =
      normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
  }
  if (els.setCoverArtHomeAssistantPort) {
    els.setCoverArtHomeAssistantPort.value = String(
      normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort));
  }
  if (els.setCoverArtFilterToggle) {
    state.coverArtFilteringEnabled = !!state.coverArtFilteringEnabled || !!state.coverArtAttributeConditions;
    els.setCoverArtFilterToggle.checked = !!state.coverArtFilteringEnabled;
  }
  if (els.setCoverArtFilterOptions) {
    els.setCoverArtFilterOptions.classList.toggle("sp-visible", !!state.coverArtFilteringEnabled);
  }
  syncInput(els.setCoverArtConditions, state.coverArtAttributeConditions || "");
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
    postClockScreensaver(state.clockScreensaverOn);
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
