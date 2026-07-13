// ── State Event Handlers ──────────────────────────────────────────
// @web-module-requires: state, language_state, environment_state, screen_rotation_state, screen_schedule_state, ntp_state, appearance_state, idle_state, artwork_state, screensaver_state, firmware_version_state, clock_bar_state, entity_state, firmware_update_state, screensaver_timeout, c6_firmware_ui, api, app_status_preview

function createSseHandlers() {
  return {
    "text-button_order": function (val) {
      if (gridPreviewBlockedByRotationStartup()) {
        orderReceived = !!(val && val.trim());
        state.pendingButtonOrderRaw = val;
        return;
      }
      applyButtonOrderValue(val);
    },
    "text-button_on_color": function (val) {
      state.onColor = val;
      syncColorUi();
      renderPreview();
    },
    "text-button_off_color": function () {},
    "text-sensor_card_color": function () {},
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
      if (applyClockBarStateValue(state, val, d, key)) syncClockBarUi();
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
    "switch-voice_services": function (val, d) {
      state.voiceServicesOn = d.value === true || val === "ON";
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
      if (!state.mediaPlayerSleepPreventionEntity) state.mediaPlayerSleepPreventionEntity = val;
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
    "select-home_assistant_artwork_protocol": function (val, d) {
      state.homeAssistantArtworkProtocol = normalizeHomeAssistantArtworkProtocol(d.value || val);
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
        state.timezoneOptions = timezoneOptionsWithFallback(d.option, state.timezone, true);
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
    "text_sensor-screen__active_timezone": function (val, d) {
      state.activeTimezone = d.value || val || FALLBACK_TIMEZONE_OPTION;
      if (isHomeAssistantAutoTimezone(state.timezone)) {
        if (normalizeTemperatureUnit(state.temperatureUnit) === "Auto") updateTempPreview();
        renderPreview();
        updateClock();
      }
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
    "text_sensor-esp32_c6__current_firmware": function (val) {
      setC6FirmwareCurrentVersion(val);
    },
    "text_sensor-c6_update_current_firmware": function (val) {
      setC6FirmwareCurrentVersion(val);
    },
    "text_sensor-esp32_c6__latest_firmware": function (val) {
      setC6FirmwareLatestVersion(val);
    },
    "text_sensor-c6_update_latest_firmware": function (val) {
      setC6FirmwareLatestVersion(val);
    },
    "text_sensor-esp32_c6__update_available": function (val) {
      setC6FirmwareUpdateAvailable(val);
    },
    "text_sensor-c6_update_available": function (val) {
      setC6FirmwareUpdateAvailable(val);
    },
    "button-firmware_esp32_c6__install_update": function () {
      state.c6FirmwareUpdateControlsSupported = true;
      state.c6FirmwareInstallControlsSupported = true;
      syncC6FirmwareUi();
    },
    "button-firmware_esp32_c6__check_for_update": function () {
      state.c6FirmwareUpdateControlsSupported = true;
      syncC6FirmwareUi();
    },
  };
}
