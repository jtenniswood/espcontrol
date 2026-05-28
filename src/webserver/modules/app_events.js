// ── SSE ────────────────────────────────────────────────────────────────

var SSE_ALIAS_GROUPS = {
  clockBar: ["switch-screen__clock_bar", "switch-screen_clock_bar", "switch-clock_bar_enabled"],
  networkStatus: ["switch-screen__network_status_icon", "switch-screen_network_status_icon", "switch-network_status_enabled"],
  temperatureDegreeSymbol: ["switch-screen__temperature_degree_symbol", "switch-screen_temperature_degree_symbol", "switch-temperature_degree_symbol_enabled"],
  screensaverTimeout: ["number-screensaver_timeout", "number-screen_saver__timeout", "number-screen_saver_timeout"],
  scheduleWakeTimeout: ["number-screen__schedule_wake_timeout", "number-screen_schedule_wake_timeout", "number-schedule_wake_timeout"],
  scheduleWakeBrightness: ["number-screen__schedule_wake_brightness", "number-screen_schedule_wake_brightness", "number-schedule_wake_brightness"],
  scheduleDimmedBrightness: ["number-screen__schedule_dimmed_brightness", "number-screen_schedule_dimmed_brightness", "number-schedule_dimmed_brightness"],
  scheduleClockBrightness: ["number-screen__schedule_clock_brightness", "number-screen_schedule_clock_brightness", "number-schedule_clock_brightness"],
  ntpServer1: ["text-screen__ntp_server_1", "text-ntp_server_1"],
  ntpServer2: ["text-screen__ntp_server_2", "text-ntp_server_2"],
  ntpServer3: ["text-screen__ntp_server_3", "text-ntp_server_3"],
  monthNames: ["text-screen__month_names", "text-month_names"],
  developerExperimentalFeatures: ["switch-developer__experimental_features", "switch-developer_experimental_features"],
};

function connectEvents() {
  if (_eventSource) { _eventSource.close(); _eventSource = null; }

  function markConnected() {
    state.selectedSlots = [];
    state.lastClickedSlot = -1;
    state.editingSubpage = null;
    state.subpageSelectedSlots = [];
    state.subpageLastClicked = -1;
    orderReceived = false;
    setConfigLocked(false);
    if (els.banner) els.banner.className = "sp-banner";
    els.root.querySelectorAll(".sp-apply-btn").forEach(function (btn) {
      btn.disabled = false;
      btn.textContent = "Apply Configuration";
    });
    clearTimeout(migrationTimer);
    migrationTimer = setTimeout(scheduleMigration, 5000);
    clearTimeout(sliderMigrationTimer);
    pendingSliderSubpageMigrations = {};
    refreshFirmwareVersion();
    refreshScreensaverTimeout();
  }

  function handleDisconnected(source) {
    setConfigLocked(true, "Reconnecting to device\u2026");
    showBanner("Reconnecting to device\u2026", "offline");
    if (source.readyState === 2) {
      source.close();
      _eventSource = null;
      setTimeout(connectEvents, 5000);
    }
  }

  function addSseAliases(handlers, names, fn) {
    for (var i = 0; i < names.length; i++) handlers[names[i]] = fn;
  }

  var sseHandlers = {
    "text-button_order": function (val) {
      orderReceived = !!(val && val.trim());
      state.sizes = {};
      state.grid = parseOrder(val);
      state.selectedSlots = state.selectedSlots.filter(function (s) {
        return state.grid.indexOf(s) !== -1;
      });
      scheduleRender();
    },
    "text-button_on_color": function (val) {
      state.onColor = val;
      if (els.setOnColor && els.setOnColor._syncColor) els.setOnColor._syncColor(val);
      renderPreview();
    },
    "text-button_off_color": function (val) {
      state.offColor = val;
      if (els.setOffColor && els.setOffColor._syncColor) els.setOffColor._syncColor(val);
      renderPreview();
    },
    "text-sensor_card_color": function (val) {
      state.sensorColor = val;
      if (els.setSensorColor && els.setSensorColor._syncColor) els.setSensorColor._syncColor(val);
      renderPreview();
    },
    "switch-indoor_temp_enable": function (val, d) {
      state._indoorOn = d.value === true || val === "ON";
      syncTemperatureUi();
      updateTempPreview();
    },
    "switch-outdoor_temp_enable": function (val, d) {
      state._outdoorOn = d.value === true || val === "ON";
      syncTemperatureUi();
      updateTempPreview();
    },
    "switch-screen__clock_bar": function (val, d) {
      state.clockBarOn = d.value === true || val === "ON";
      syncClockBarUi();
    },
    "switch-screen__network_status_icon": function (val, d) {
      state.networkStatusOn = d.value === true || val === "ON";
      syncClockBarUi();
    },
    "switch-screen__temperature_degree_symbol": function (val, d) {
      state.temperatureDegreeSymbolOn = d.value === true || val === "ON";
      syncClockBarUi();
    },
    "text-indoor_temp_entity": function (val) {
      state.indoorEntity = val;
      syncInput(els.setIndoorEntity, val);
    },
    "text-outdoor_temp_entity": function (val) {
      state.outdoorEntity = val;
      syncInput(els.setOutdoorEntity, val);
    },
    "select-screen__temperature_unit": function (val, d) {
      state.temperatureUnit = normalizeTemperatureUnit(d.value || val);
      if (els.setTemperatureUnit) els.setTemperatureUnit.value = state.temperatureUnit;
      updateTempPreview();
      renderPreview();
    },
    "number-screensaver_timeout": function (val, d) {
      applyScreensaverTimeoutState(d);
    },
    "number-home_screen_timeout": function (val) {
      state.homeScreenTimeout = parseFloat(val) || 0;
      syncIdleUi();
    },
    "switch-screen_saver__clock": function (val, d) {
      state.clockScreensaverOn = d.value === true || val === "ON";
      if (!state._screensaverActionReceived) {
        state.screensaverAction = state.clockScreensaverOn ? "clock" : "off";
      }
      syncClockScreensaverControls();
    },
    "switch-screen_saver__media_player_sleep_prevention": function (val, d) {
      state.mediaPlayerSleepPreventionOn = d.value === true || val === "ON";
      syncMediaPlayerSleepPreventionUi();
    },
    "number-screen_saver__clock_brightness": function (val) {
      if (state.clockBrightnessSplitReceived) return;
      var brightness = normalizeClockBrightness(val, 35);
      state.clockBrightnessDay = brightness;
      state.clockBrightnessNight = brightness;
      syncClockScreensaverControls();
    },
    "number-screen_saver__daytime_clock_brightness": function (val) {
      state.clockBrightnessSplitReceived = true;
      state.clockBrightnessDay = normalizeClockBrightness(val, 35);
      syncClockScreensaverControls();
    },
    "number-screen_saver__nighttime_clock_brightness": function (val) {
      state.clockBrightnessSplitReceived = true;
      state.clockBrightnessNight = normalizeClockBrightness(val, state.clockBrightnessDay);
      syncClockScreensaverControls();
    },
    "select-screen_saver__action": function (val, d) {
      state._screensaverActionReceived = true;
      state.screensaverAction = normalizeScreensaverAction(d.value || val);
      state.clockScreensaverOn = state.screensaverAction === "clock";
      syncClockScreensaverControls();
    },
    "number-screen_saver__dimmed_brightness": function (val) {
      state.screensaverDimmedBrightness = normalizeScreensaverDimmedBrightness(val);
      syncClockScreensaverControls();
    },
    "text-presence_sensor_entity": function (val) {
      state.presenceEntity = val;
      syncInput(els.setPresence, val);
      if (state.screensaverMode === "") {
        if (els.setSsMode) els.setSsMode(getActiveScreensaverMode());
      }
    },
    "text-media_player_sleep_prevention_entity": function (val) {
      state.mediaPlayerSleepPreventionEntity = val;
      syncInput(els.setMediaPlayerSleepPrevention, val);
    },
    "text-screensaver_mode": function (val) {
      state._screensaverModeReceived = true;
      state.screensaverMode = val === "sensor" || val === "timer" || val === "disabled" ? val : "disabled";
      if (els.setSsMode) els.setSsMode(getActiveScreensaverMode());
    },
    "number-screen__daytime_brightness": function (val) {
      state.brightnessDayVal = parseFloat(val) || 100;
      if (els.setDayBrightness) {
        els.setDayBrightness.value = state.brightnessDayVal;
        els.setDayBrightnessVal.textContent = Math.round(state.brightnessDayVal) + "%";
      }
    },
    "number-screen__nighttime_brightness": function (val) {
      state.brightnessNightVal = parseFloat(val) || 75;
      if (els.setNightBrightness) {
        els.setNightBrightness.value = state.brightnessNightVal;
        els.setNightBrightnessVal.textContent = Math.round(state.brightnessNightVal) + "%";
      }
    },
    "switch-screen__automatic_brightness": function (val, d) {
      state.automaticBrightnessEnabled = d.value === true || val === "ON";
      syncScreenScheduleUi();
    },
    "switch-screen__schedule_enabled": function (val, d) {
      state.scheduleEnabled = d.value === true || val === "ON";
      syncScreenScheduleUi();
    },
    "number-screen__schedule_on_hour": function (val) {
      state.scheduleOnHour = normalizeHour(val, 6);
      syncScreenScheduleUi();
    },
    "number-screen__schedule_off_hour": function (val) {
      state.scheduleOffHour = normalizeHour(val, 23);
      syncScreenScheduleUi();
    },
    "select-screen__schedule_mode": function (val, d) {
      state.scheduleMode = normalizeScheduleMode(d.value || val);
      syncScreenScheduleUi();
    },
    "number-screen__schedule_wake_timeout": function (val) {
      state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(val);
      syncScreenScheduleUi();
    },
    "number-screen__schedule_wake_brightness": function (val) {
      state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(val);
      syncScreenScheduleUi();
    },
    "number-screen__schedule_dimmed_brightness": function (val) {
      state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(val);
      syncScreenScheduleUi();
    },
    "number-screen__schedule_clock_brightness": function (val) {
      state.scheduleClockBrightness = normalizeScheduleClockBrightness(val);
      syncScreenScheduleUi();
    },
    "select-screen__timezone": function (val, d) {
      state.timezone = d.value || val || state.timezone;
      if (Array.isArray(d.option)) {
        state.timezoneOptions = timezoneOptionsWithFallback(d.option, state.timezone);
        if (els.setTimezone) {
          els.setTimezone.innerHTML = "";
          state.timezoneOptions.forEach(function (opt) {
            appendTimezoneOption(els.setTimezone, opt);
          });
        }
      }
      if (els.setTimezone) els.setTimezone.value = state.timezone;
      if (normalizeTemperatureUnit(state.temperatureUnit) === "Auto") {
        updateTempPreview();
        renderPreview();
      }
      updateClock();
    },
    "select-screen__clock_format": function (val, d) {
      state.clockFormat = d.value || val || state.clockFormat;
      if (d.option && Array.isArray(d.option)) {
        state.clockFormatOptions = d.option;
        if (els.setClockFormat) {
          els.setClockFormat.innerHTML = "";
          d.option.forEach(function (opt) {
            var o = document.createElement("option");
            o.value = opt;
            o.textContent = opt === "12h" ? "12-hour" : "24-hour";
            els.setClockFormat.appendChild(o);
          });
        }
      }
      if (els.setClockFormat) els.setClockFormat.value = state.clockFormat;
      updateClock();
    },
    "text-screen__ntp_server_1": function (val) {
      state.ntpServer1 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[0]);
      state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
      syncNtpServerUi();
    },
    "text-screen__ntp_server_2": function (val) {
      state.ntpServer2 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[1]);
      state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
      syncNtpServerUi();
    },
    "text-screen__ntp_server_3": function (val) {
      state.ntpServer3 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[2]);
      state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
      syncNtpServerUi();
    },
    "text-screen__month_names": function (val) {
      state.monthNames = normalizeMonthNames(val);
      state.customMonthNames = hasCustomMonthNames();
      syncMonthNameUi();
      renderPreview();
    },
    "select-screen__rotation": function (val, d) {
      state.screenRotation = normalizeScreenRotation(d.value || val || state.screenRotation);
      if (d.option && Array.isArray(d.option)) {
        state.screenRotationDeviceOptions = d.option;
        state.screenRotationOptions = d.option;
      }
      syncScreenRotationSelect();
      syncPreviewOrientation();
      renderPreview();
    },
    "text_sensor-screen__sunrise": function (val) {
      state.sunrise = val;
      updateSunInfo();
    },
    "text_sensor-screen__sunset": function (val) {
      state.sunset = val;
      updateSunInfo();
    },
    "text_sensor-network_transport": function (val) {
      state.networkTransport = normalizeNetworkTransport(val);
      updateNetworkPreview();
      syncFirmwareUpdateUi();
    },
    "sensor-wifi_strength": function (val) {
      state.networkTransport = "wifi";
      state.wifiStrengthPercent = normalizeWifiStrengthPercent(val);
      updateNetworkPreview();
      syncFirmwareUpdateUi();
    },
    "text_sensor-firmware__version": function (val) {
      setFirmwareVersion(val);
    },
    "text_sensor-firmware_version": function (val) {
      setFirmwareVersion(val);
    },
    "update-firmware__update": function (val, d) {
      setFirmwareUpdateInfo(d);
    },
    "switch-firmware__auto_update": function (val, d) {
      state.firmwareUpdateControlsSupported = true;
      state.autoUpdate = d.value === true || val === "ON";
      if (els.setAutoUpdate) els.setAutoUpdate.checked = state.autoUpdate;
      syncFirmwareUpdateUi();
    },
    "switch-developer__experimental_features": function (val, d) {
      state.developerExperimentalFeatures = d.value === true || val === "ON";
      if (els.setDeveloperExperimentalFeatures) {
        els.setDeveloperExperimentalFeatures.checked = state.developerExperimentalFeatures;
      }
      syncScreenRotationSelect();
      scheduleRender();
    },
    "select-firmware__update_frequency": function (val, d) {
      state.firmwareUpdateControlsSupported = true;
      state.updateFrequency = d.value || val || state.updateFrequency;
      if (els.setUpdateFreq) els.setUpdateFreq.value = state.updateFrequency;
      if (d.option && Array.isArray(d.option)) {
        state.updateFreqOptions = d.option;
      }
      syncFirmwareUpdateUi();
    },
  };

  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockBar, sseHandlers["switch-screen__clock_bar"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.networkStatus, sseHandlers["switch-screen__network_status_icon"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.temperatureDegreeSymbol, sseHandlers["switch-screen__temperature_degree_symbol"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screensaverTimeout, sseHandlers["number-screensaver_timeout"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleWakeTimeout, sseHandlers["number-screen__schedule_wake_timeout"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleWakeBrightness, sseHandlers["number-screen__schedule_wake_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleDimmedBrightness, sseHandlers["number-screen__schedule_dimmed_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleClockBrightness, sseHandlers["number-screen__schedule_clock_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer1, sseHandlers["text-screen__ntp_server_1"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer2, sseHandlers["text-screen__ntp_server_2"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer3, sseHandlers["text-screen__ntp_server_3"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.monthNames, sseHandlers["text-screen__month_names"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.developerExperimentalFeatures, sseHandlers["switch-developer__experimental_features"]);

  var ssePatterns = [
    {
      re: /^text-button_(\d+)_config$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        var b = state.buttons[slot - 1];
        var migrateConfig = buttonConfigNeedsMigration(val || "");
        var parsed = parseButtonConfig(val || "");
        b.entity = parsed.entity;
        b.label = parsed.label;
        b.icon = parsed.icon;
        b.icon_on = parsed.icon_on;
        b.sensor = parsed.sensor;
        b.unit = parsed.unit;
        b.type = parsed.type;
        b.precision = parsed.precision;
        b.options = parsed.options;
        if (migrateConfig) saveButtonConfig(slot);
        scheduleRender();
      },
    },
    {
      re: /^text-subpage_(\d+)_config$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].main = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_2$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext2 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_3$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext3 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_4$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext4 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_5$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext5 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_6$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext6 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_7$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > NUM_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext7 = val || "";
        applySubpageRaw(slot);
      },
    },
  ];

  function handleState(d) {
    rememberEntityPostPath(d);
    var keys = entityStateKeys(d);
    var id = keys[0] || d.id;
    var val = d.state != null ? String(d.state) : "";

    for (var ki = 0; ki < keys.length; ki++) {
      if (sseHandlers[keys[ki]]) { sseHandlers[keys[ki]](val, d); return; }
    }
    if (isFirmwareVersionEvent(id, d)) {
      setFirmwareVersion(val);
      return;
    }
    if (isFirmwareUpdateEvent(id, d)) {
      setFirmwareUpdateInfo(d);
      return;
    }
    if (isFirmwareInstallButtonEvent(id, d)) {
      state.firmwareUpdateControlsSupported = true;
      state.firmwareInstallControlsSupported = true;
      renderFirmwareUpdateStatus();
      return;
    }
    if (isFirmwareCheckButtonEvent(id, d)) {
      state.firmwareUpdateControlsSupported = true;
      renderFirmwareUpdateStatus();
      return;
    }

    for (var i = 0; i < ssePatterns.length; i++) {
      for (var pk = 0; pk < keys.length; pk++) {
        var m = keys[pk].match(ssePatterns[i].re);
        if (m) { ssePatterns[i].fn(m, val, d); return; }
      }
    }

    console.log("[state] unhandled:", id, val);
  }

  if (!eventStreamEnabled()) {
    loadInitialState(handleState, markConnected);
    return;
  }

  var source = new EventSource("/events");
  _eventSource = source;

  source.addEventListener("open", markConnected);
  source.addEventListener("error", function () {
    handleDisconnected(source);
  });
  source.addEventListener("state", function (e) {
    var d;
    try { d = JSON.parse(e.data); } catch (_) { return; }
    handleState(d);
  });

}
