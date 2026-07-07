// ── SSE ────────────────────────────────────────────────────────────────

var SSE_ALIAS_GROUPS = {
  clockBar: ["switch-screen__clock_bar", "switch-screen_clock_bar", "switch-clock_bar_enabled"],
  clockBarLayout: ["text-screen__clock_bar_layout", "text-screen_clock_bar_layout", "text-clock_bar_layout"],
  clockBarTime: ["switch-screen__clock_bar_time", "switch-screen_clock_bar_time", "switch-clock_bar_time_enabled"],
  clockBarTemperatureEntities: ["text-clock_bar_temperature_entities", "text-clock_bar__temperature_entities"],
  networkStatus: ["switch-screen__network_status_icon", "switch-screen_network_status_icon", "switch-network_status_enabled"],
  voiceServices: ["switch-voice_services", "switch-voice_services_enabled"],
  temperatureDegreeSymbol: ["switch-screen__temperature_degree_symbol", "switch-screen_temperature_degree_symbol", "switch-temperature_degree_symbol_enabled"],
  subpageChevron: ["switch-screen__subpage_chevron", "switch-screen_subpage_chevron", "switch-subpage_chevrons_enabled"],
  screensaverTimeout: ["number-screensaver_timeout", "number-screen_saver__timeout", "number-screen_saver_timeout"],
  coverArt: ["switch-screen_saver__cover_art", "switch-screen_saver_cover_art", "switch-screensaver_cover_art"],
  coverArtEntity: ["text-screen_saver__cover_art_entity", "text-screen_saver_cover_art_entity", "text-cover_art_media_player_entity"],
  coverArtConditions: ["text-screen_saver__cover_art_conditions", "text-screen_saver_cover_art_conditions", "text-cover_art_attribute_conditions"],
  coverArtDelay: ["number-screen_saver__cover_art_delay", "number-screen_saver_cover_art_delay", "number-cover_art_delay"],
  trackOverlayDuration: ["number-screen_saver__track_overlay_duration", "number-screen_saver_track_overlay_duration", "number-track_overlay_duration", "number-screen_saver__show_track_overlay"],
  coverArtHideExternalInput: ["switch-screen_saver__hide_cover_art_on_external_input", "switch-screen_saver_hide_cover_art_on_external_input", "switch-hide_cover_art_on_external_input", "switch-cover_art_hide_external_input", "switch-screen_saver__hide_for_external_sources"],
  screensaverPinRequired: ["switch-screensaver__pin_required", "switch-screensaver_pin_required", "switch-screensaver__require_pin_after_wake"],
  screensaverPin: ["text-screensaver__pin", "text-screensaver_pin"],
  homeAssistantArtworkPort: ["number-home_assistant_artwork_port"],
  scheduleTrigger: ["text-screen__schedule_trigger", "text-screen_schedule_trigger", "text-schedule_trigger"],
  scheduleWakeTimeout: ["number-screen__schedule_wake_timeout", "number-screen_schedule_wake_timeout", "number-schedule_wake_timeout"],
  scheduleWakeBrightness: ["number-screen__schedule_wake_brightness", "number-screen_schedule_wake_brightness", "number-schedule_wake_brightness"],
  scheduleDimmedBrightness: ["number-screen__schedule_dimmed_brightness", "number-screen_schedule_dimmed_brightness", "number-schedule_dimmed_brightness"],
  scheduleClockBrightness: ["number-screen__schedule_clock_brightness", "number-screen_schedule_clock_brightness", "number-schedule_clock_brightness"],
  scheduleClockTextColor: ["text-screen__schedule_clock_text_color", "text-screen_schedule_clock_text_color", "text-schedule_clock_text_color"],
  screenTheme: ["select-screen__theme", "select-screen_theme"],
  screenLanguage: ["select-screen__language", "select-screen_language"],
  ntpServer1: ["text-screen__ntp_server_1", "text-ntp_server_1"],
  ntpServer2: ["text-screen__ntp_server_2", "text-ntp_server_2"],
  ntpServer3: ["text-screen__ntp_server_3", "text-ntp_server_3"],
};

function applyClockBarStateValue(val, d, matchedKey) {
  var keys = entityStateKeys(d);
  uniquePush(keys, matchedKey);
  var nextOn = d && d.value === true || val === "ON";
  var sourceKey = matchedKey || keys[0] || "clock_bar";
  if (!state._clockBarStateValues) state._clockBarStateValues = {};
  state._clockBarStateValues[sourceKey] = nextOn;

  var previous = state.clockBarOn;
  state.clockBarOn = Object.keys(state._clockBarStateValues).some(function (key) {
    return state._clockBarStateValues[key] === true;
  });
  return state.clockBarOn !== previous;
}

function isRemovedLegacyStateEvent(id, d) {
  var keys = entityStateKeys(d || {});
  uniquePush(keys, id);
  return keys.indexOf("text-screen_saver__cover_art_fallback_server") !== -1 ||
    keys.indexOf("text-screen_saver_cover_art_fallback_server") !== -1 ||
    keys.indexOf("text-cover_art_fallback_server") !== -1;
}

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
      if (gridPreviewBlockedByRotationStartup()) {
        orderReceived = !!(val && val.trim());
        state.pendingButtonOrderRaw = val;
        return;
      }
      applyButtonOrderValue(val);
    },
    "select-screen__theme": function (val, d) {
      syncThemeFromDevice(d.value || val, d.option);
    },
    "text-button_on_color": function (val) {
      state.onColor = val;
      syncColorUi();
      renderPreview();
    },
    "text-button_off_color": function (val) {
      state.offColor = val;
      syncColorUi();
      renderPreview();
    },
    "text-sensor_card_color": function (val) {
      state.sensorColor = val;
      syncColorUi();
      renderPreview();
    },
    "switch-indoor_temp_enable": function (val, d) {
      state._clockBarTemperatureVisibilityReceived = true;
      state._indoorOn = d.value === true || val === "ON";
      syncTemperatureUi();
      updateTempPreview();
      updateClockBarItemUi();
    },
    "switch-outdoor_temp_enable": function (val, d) {
      state._clockBarTemperatureVisibilityReceived = true;
      state._outdoorOn = d.value === true || val === "ON";
      syncTemperatureUi();
      updateTempPreview();
      updateClockBarItemUi();
    },
    "switch-screen__clock_bar": function (val, d, key) {
      if (applyClockBarStateValue(val, d, key)) syncClockBarUi();
    },
    "text-screen__clock_bar_layout": function (val) {
      applyClockBarLayoutValue(CLOCK_BAR_FIXED_LAYOUT_STRING);
    },
    "text-clock_bar_temperature_entities": function (val) {
      applyClockBarTemperatureEntities(normalizeClockBarTemperatureEntities(val), false);
    },
    "switch-screen__clock_bar_time": function (val, d) {
      state.clockBarTimeOn = d.value === true || val === "ON";
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
    "switch-screen__subpage_chevron": function (val, d) {
      state.subpageChevronsOn = d.value === true || val === "ON";
      syncClockBarUi();
      renderPreview();
    },
    "text-indoor_temp_entity": function (val) {
      state.indoorEntity = val;
      syncInput(els.setIndoorEntity, val);
      if (!state._clockBarTemperatureEntitiesReceived) {
        syncTemperatureUi();
        updateTempPreview();
        updateClockBarItemUi();
      }
    },
    "text-outdoor_temp_entity": function (val) {
      state.outdoorEntity = val;
      syncInput(els.setOutdoorEntity, val);
      if (!state._clockBarTemperatureEntitiesReceived) {
        syncTemperatureUi();
        updateTempPreview();
        updateClockBarItemUi();
      }
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
    "switch-screen_saver__cover_art": function (val, d) {
      state.coverArtScreensaverOn = d.value === true || val === "ON";
      syncCoverArtScreensaverUi();
    },
    "switch-screen_saver__hide_cover_art_on_external_input": function (val, d) {
      state.coverArtHideExternalInputOn = d.value === true || val === "ON";
      syncCoverArtScreensaverUi();
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
    "switch-screensaver__pin_required": function (val, d) {
      state.screensaverPinRequired = d.value === true || val === "ON";
      syncScreensaverPinUi();
    },
    "text-screensaver__pin": function (val) {
      state.screensaverPinSet = normalizePin(val).length > 0;
      syncScreensaverPinUi();
    },
    "number-screen_saver__dimmed_brightness": function (val) {
      state.screensaverDimmedBrightness = normalizeScreensaverDimmedBrightness(val);
      syncClockScreensaverControls();
    },
    "text-presence_sensor_entity": function (val) {
      state.presenceEntity = val;
      syncInput(els.setPresence, val);
      syncInput(els.setSchedulePresence, val);
      if (state.screensaverMode === "") {
        if (els.setSsMode) els.setSsMode(getActiveScreensaverMode());
      }
    },
    "text-media_player_sleep_prevention_entity": function (val) {
      state.mediaPlayerSleepPreventionEntity = val;
      if (!state.coverArtMediaPlayerEntity) {
        state.coverArtMediaPlayerEntity = val;
        syncInput(els.setCoverArtMediaPlayer, val);
      }
    },
    "text-screen_saver__cover_art_entity": function (val) {
      state.coverArtMediaPlayerEntity = val;
      syncInput(els.setCoverArtMediaPlayer, val);
    },
    "text-screen_saver__cover_art_conditions": function (val) {
      state.coverArtAttributeConditions = val;
      syncInput(els.setCoverArtConditions, val);
    },
    "number-screen_saver__cover_art_delay": function (val) {
      state.coverArtDelay = parseFloat(val) || 0;
      syncCoverArtScreensaverUi();
    },
    "number-screen_saver__track_overlay_duration": function (val) {
      state.coverArtTrackOverlayDuration = parseFloat(val) || 0;
      syncCoverArtScreensaverUi();
    },
    "number-home_assistant_artwork_port": function (val) {
      state.coverArtHomeAssistantPort = normalizeHomeAssistantArtworkPort(val);
      syncCoverArtScreensaverUi();
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
    "text-screen__brightness_dawn_time": function (val) {
      state.brightnessDawnTime = normalizeTimeOfDay(val, "06:00");
      syncScreenScheduleUi();
    },
    "text-screen__brightness_dusk_time": function (val) {
      state.brightnessDuskTime = normalizeTimeOfDay(val, "18:00");
      syncScreenScheduleUi();
    },
    "switch-screen__schedule_enabled": function (val, d) {
      state.scheduleEnabled = d.value === true || val === "ON";
      if (!state._scheduleTriggerReceived) {
        state.scheduleTrigger = state.scheduleEnabled ? "time" : "disabled";
      }
      syncScreenScheduleUi();
    },
    "text-screen__schedule_trigger": function (val, d) {
      state._scheduleTriggerReceived = true;
      state.scheduleTrigger = normalizeScheduleTrigger(d.value || val, state.scheduleEnabled);
      state.scheduleEnabled = state.scheduleTrigger !== "disabled";
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
    "text-screen__schedule_clock_text_color": function (val) {
      state.scheduleClockTextColor = normalizeHexColor(val, "FFFFFF");
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
    "select-screen__language": function (val, d) {
      state.language = normalizeLanguage(d.value || val || state.language);
      if (d.option && Array.isArray(d.option)) {
        state.languageOptions = languageOptionsWithFallback(d.option, state.language);
      }
      syncLanguageSelect();
      renderPreview();
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
    "select-screen__rotation": function (val, d) {
      state.screenRotation = normalizeScreenRotation(d.value || val || state.screenRotation);
      if (d.option && Array.isArray(d.option)) {
        state.screenRotationDeviceOptions = d.option;
        state.screenRotationOptions = d.option;
      }
      syncScreenRotationSelect();
      syncPreviewOrientation();
      resolveInitialScreenRotationCheck();
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
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockBarLayout, sseHandlers["text-screen__clock_bar_layout"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockBarTime, sseHandlers["switch-screen__clock_bar_time"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockBarTemperatureEntities, sseHandlers["text-clock_bar_temperature_entities"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.networkStatus, sseHandlers["switch-screen__network_status_icon"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.voiceServices, sseHandlers["switch-voice_services"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.temperatureDegreeSymbol, sseHandlers["switch-screen__temperature_degree_symbol"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.subpageChevron, sseHandlers["switch-screen__subpage_chevron"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screensaverTimeout, sseHandlers["number-screensaver_timeout"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArt, sseHandlers["switch-screen_saver__cover_art"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtEntity, sseHandlers["text-screen_saver__cover_art_entity"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtConditions, sseHandlers["text-screen_saver__cover_art_conditions"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtDelay, sseHandlers["number-screen_saver__cover_art_delay"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.trackOverlayDuration, sseHandlers["number-screen_saver__track_overlay_duration"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtHideExternalInput, sseHandlers["switch-screen_saver__hide_cover_art_on_external_input"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screensaverPinRequired, sseHandlers["switch-screensaver__pin_required"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screensaverPin, sseHandlers["text-screensaver__pin"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.homeAssistantArtworkPort, sseHandlers["number-home_assistant_artwork_port"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleTrigger, sseHandlers["text-screen__schedule_trigger"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleWakeTimeout, sseHandlers["number-screen__schedule_wake_timeout"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleWakeBrightness, sseHandlers["number-screen__schedule_wake_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleDimmedBrightness, sseHandlers["number-screen__schedule_dimmed_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleClockBrightness, sseHandlers["number-screen__schedule_clock_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleClockTextColor, sseHandlers["text-screen__schedule_clock_text_color"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screenTheme, sseHandlers["select-screen__theme"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screenLanguage, sseHandlers["select-screen__language"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer1, sseHandlers["text-screen__ntp_server_1"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer2, sseHandlers["text-screen__ntp_server_2"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer3, sseHandlers["text-screen__ntp_server_3"]);

  var ssePatterns = [
    {
      re: /^text-button_(\d+)_config$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
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
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].main = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_2$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext2 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_3$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext3 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_4$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext4 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_5$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext5 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_6$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
        if (!state.subpageRaw[slot]) state.subpageRaw[slot] = { main: "", ext: "", ext2: "", ext3: "", ext4: "", ext5: "", ext6: "", ext7: "" };
        state.subpageRaw[slot].ext6 = val || "";
        applySubpageRaw(slot);
      },
    },
    {
      re: /^text-subpage_(\d+)_config_ext_7$/,
      fn: function (m, val) {
        var slot = parseInt(m[1], 10);
        if (slot < 1 || slot > TOTAL_SLOTS) return;
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
      if (sseHandlers[keys[ki]]) { sseHandlers[keys[ki]](val, d, keys[ki]); return; }
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
    if (isRemovedLegacyStateEvent(id, d)) return;

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
