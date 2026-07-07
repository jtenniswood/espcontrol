// ── Export / Import ────────────────────────────────────────────────────

function backupExportScreenSizeSlug(value) {
  value = String(value || "").trim().toLowerCase();
  if (!value) return "screen";
  value = value.replace(/\binches\b/g, "inch").replace(/\bin\b/g, "inch");
  value = value.replace(/[^a-z0-9.]+/g, "-").replace(/^-+|-+$/g, "");
  return value || "screen";
}

function backupExportFileDate(value) {
  return value.getFullYear() + "-" +
    String(value.getMonth() + 1).padStart(2, "0") + "-" +
    String(value.getDate()).padStart(2, "0");
}

function backupExportFileName(value) {
  var date = value || new Date();
  return "espcontrol-" + backupExportScreenSizeSlug(CFG.screenSize) + "-" +
    backupExportFileDate(date) + ".json";
}

function exportConfig() {
  var data = createBackupConfig({
    device: DEVICE_ID,
    slots: NUM_SLOTS,
    exported_at: new Date().toISOString(),
    grid: state.grid,
    sizes: state.sizes,
    button_order: serializeGrid(state.grid),
    button_on_color: state.onColor,
    button_off_color: state.offColor,
    sensor_card_color: state.sensorColor,
    buttons: state.buttons,
    subpages: state.subpages,
    settings: {
      indoor_temp_enable: state._indoorOn,
      outdoor_temp_enable: state._outdoorOn,
      clock_bar_temperature_entities: serializeClockBarTemperatureEntities(clockBarTemperatureEntities()),
      theme: normalizeTheme(state.theme),
      indoor_temp_entity: state.indoorEntity,
      outdoor_temp_entity: state.outdoorEntity,
      temperature_unit: normalizeTemperatureUnit(state.temperatureUnit),
      clock_bar: state.clockBarOn,
      clock_bar_layout: CLOCK_BAR_FIXED_LAYOUT_STRING,
      clock_bar_time: state.clockBarTimeOn,
      network_status_icon: state.networkStatusOn,
      temperature_degree_symbol: state.temperatureDegreeSymbolOn,
      subpage_chevron: state.subpageChevronsOn,
      timezone: state.timezone,
      language: normalizeLanguage(state.language),
      clock_format: state.clockFormat,
      ntp_server_1: state.ntpServer1,
      ntp_server_2: state.ntpServer2,
      ntp_server_3: state.ntpServer3,
      screensaver_mode: getActiveScreensaverMode(),
      presence_sensor_entity: state.presenceEntity,
      media_player_sleep_prevention: state.mediaPlayerSleepPreventionOn,
      media_player_sleep_prevention_entity: state.mediaPlayerSleepPreventionEntity,
      cover_art_screensaver: state.coverArtScreensaverOn,
      cover_art_media_player_entity: state.coverArtMediaPlayerEntity,
      cover_art_attribute_conditions: state.coverArtAttributeConditions,
      cover_art_delay: state.coverArtDelay,
      cover_art_track_overlay_duration: state.coverArtTrackOverlayDuration,
      cover_art_hide_external_input: state.coverArtHideExternalInputOn,
      home_assistant_artwork_port: normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort),
      screensaver_action: normalizeScreensaverAction(state.screensaverAction),
      screensaver_pin_required: !!state.screensaverPinRequired,
      clock_screensaver: state.clockScreensaverOn,
      clock_brightness: state.clockBrightnessDay,
      clock_brightness_day: state.clockBrightnessDay,
      clock_brightness_night: state.clockBrightnessNight,
      screensaver_dimmed_brightness: normalizeScreensaverDimmedBrightness(state.screensaverDimmedBrightness),
      screensaver_timeout: state.screensaverTimeout,
      home_screen_timeout: state.homeScreenTimeout,
      screen_rotation: state.screenRotation,
    },
    screen: {
      brightness_day: Math.round(state.brightnessDayVal),
      brightness_night: Math.round(state.brightnessNightVal),
      automatic_brightness: !!state.automaticBrightnessEnabled,
      brightness_dawn_time: normalizeTimeOfDay(state.brightnessDawnTime, "06:00"),
      brightness_dusk_time: normalizeTimeOfDay(state.brightnessDuskTime, "18:00"),
      schedule_trigger: normalizeScheduleTrigger(state.scheduleTrigger, state.scheduleEnabled),
      schedule_enabled: !!state.scheduleEnabled,
      schedule_on_hour: normalizeHour(state.scheduleOnHour, 6),
      schedule_off_hour: normalizeHour(state.scheduleOffHour, 23),
      schedule_mode: normalizeScheduleMode(state.scheduleMode),
      schedule_wake_timeout: normalizeScheduleWakeTimeout(state.scheduleWakeTimeout),
      schedule_wake_brightness: normalizeScheduleWakeBrightness(state.scheduleWakeBrightness),
      schedule_dimmed_brightness: normalizeScheduleDimmedBrightness(state.scheduleDimmedBrightness),
      schedule_clock_brightness: normalizeScheduleClockBrightness(state.scheduleClockBrightness),
      schedule_clock_text_color: normalizeHexColor(state.scheduleClockTextColor, "FFFFFF"),
    },
  });

  var json = JSON.stringify(data, null, 2);
  var blob = new Blob([json], { type: "application/json" });
  var url = URL.createObjectURL(blob);
  var name = backupExportFileName();
  var a = document.createElement("a");
  a.href = url;
  a.download = name;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

function importConfig() {
  var input = document.createElement("input");
  input.type = "file";
  input.accept = ".json";
  input.style.display = "none";

  function cleanupInput() {
    if (input.parentNode) input.parentNode.removeChild(input);
  }

  input.addEventListener("cancel", cleanupInput);
  input.addEventListener("change", function () {
    if (!input.files || !input.files[0]) {
      cleanupInput();
      return;
    }
    var reader = new FileReader();
    reader.onerror = function () {
      cleanupInput();
      showBanner("Invalid file \u2014 could not read backup", "error");
    };
    reader.onload = function () {
      var data;
      try { data = JSON.parse(reader.result); } catch (_) {
        showBanner("Invalid file \u2014 could not parse JSON", "error");
        cleanupInput();
        return;
      }

      var backupPlan;
      try {
        backupPlan = planBackupImport(data, { device: DEVICE_ID, slots: NUM_SLOTS });
      } catch (e) {
        showBanner(e.backupMessage || "Invalid config file \u2014 missing required fields", "error");
        cleanupInput();
        return;
      }
      for (var warningIdx = 0; warningIdx < backupPlan.warnings.length; warningIdx++) {
        showBanner(backupPlan.warnings[warningIdx], "warning");
      }

      postText(entityName("button_on_color"), backupPlan.config.button_on_color);
      postText(entityName("button_off_color"), backupPlan.config.button_off_color);
      postText(entityName("sensor_card_color"), backupPlan.config.sensor_card_color);

      for (var i = 0; i < NUM_SLOTS; i++) {
        var b = backupPlan.buttons[i];
        var n = i + 1;
        state.buttons[i] = backupNormalizeButtonConfig(b);
        saveButtonConfig(n);
      }

      state.subpages = {};
      state.subpageRaw = {};
      for (var subpageKey in backupPlan.subpages) {
        state.subpages[subpageKey] = backupPlan.subpages[subpageKey];
        saveSubpageEntity(subpageKey);
      }

      postText(entityName("button_order"), backupPlan.button_order);
      applyImportedButtonOrder(backupPlan.button_order, backupPlan.importedSizes);
      state.onColor = backupPlan.config.button_on_color;
      state.offColor = backupPlan.config.button_off_color;
      state.sensorColor = backupPlan.config.sensor_card_color;

      if (els.setOnColor && els.setOnColor._syncColor) els.setOnColor._syncColor(state.onColor);
      if (els.setOffColor && els.setOffColor._syncColor) els.setOffColor._syncColor(state.offColor);
      if (els.setSensorColor && els.setSensorColor._syncColor) els.setSensorColor._syncColor(state.sensorColor);

      if (backupPlan.settings) {
        var s = backupPlan.settings;
        var importedSettings = EspControlModel.normalizeBackupPanelSettings(s, {
          timezone: state.timezone,
          language: state.language,
          clockBarLayout: CLOCK_BAR_FIXED_LAYOUT_STRING,
          clockFormat: state.clockFormat,
          clockFormatOptions: state.clockFormatOptions,
          ntpDefaults: NTP_SERVER_DEFAULTS,
          ntpServer1: state.ntpServer1,
          ntpServer2: state.ntpServer2,
          ntpServer3: state.ntpServer3,
          coverArtHomeAssistantPort: state.coverArtHomeAssistantPort,
          screenRotationOptions: allScreenRotationOptions(),
        });

        state._clockBarTemperatureVisibilityReceived = true;
        state._outdoorOn = importedSettings.outdoorTempEnable;
        state._indoorOn = importedSettings.indoorTempEnable;
        applyClockBarTemperatureEntities(importedSettings.clockBarTemperatureEntities, false);
        postClockBarTemperatureEntities(serializeClockBarTemperatureEntities(importedSettings.clockBarTemperatureEntities));
        postSwitch(entityName("outdoor_temp_enable"), importedSettings.outdoorTempEnable);
        postSwitch(entityName("indoor_temp_enable"), importedSettings.indoorTempEnable);
        postText(entityName("outdoor_temp_entity"), importedSettings.outdoorTempEntity);
        postText(entityName("indoor_temp_entity"), importedSettings.indoorTempEntity);
        postClockBar(importedSettings.clockBar);
        applyClockBarLayoutValue(CLOCK_BAR_FIXED_LAYOUT_STRING);
        postClockBarLayout(CLOCK_BAR_FIXED_LAYOUT_STRING);
        postClockBarTime(importedSettings.clockBarTime);
        postNetworkStatusIcon(importedSettings.networkStatusIcon);
        postTemperatureDegreeSymbol(importedSettings.temperatureDegreeSymbol);
        postSubpageChevron(importedSettings.subpageChevron);
        var importedTimezone = importedSettings.timezone;
        var importedTheme = normalizeTheme(s.theme || state.theme);
        var importedTemperatureUnit = importedSettings.temperatureUnit;
        var importedLanguage = importedSettings.language;
        var importedClockFormat = importedSettings.clockFormat;
        var hasNtpServer1 = importedSettings.hasNtpServer1;
        var hasNtpServer2 = importedSettings.hasNtpServer2;
        var hasNtpServer3 = importedSettings.hasNtpServer3;
        var importedNtpServer1 = importedSettings.ntpServer1;
        var importedNtpServer2 = importedSettings.ntpServer2;
        var importedNtpServer3 = importedSettings.ntpServer3;
        if (s.timezone) postSelect(entityName("screen_timezone"), importedTimezone);
        if (s.language) postSelect(entityName("screen_language"), importedLanguage);
        if (s.theme && isEpaperPreview()) postSelect(entityName("screen_theme"), importedTheme);
        postSelect(entityName("screen_temperature_unit"), importedTemperatureUnit);
        if (s.clock_format) postSelect(entityName("screen_clock_format"), importedClockFormat);
        if (hasNtpServer1) {
          postText(entityName("screen_ntp_server_1"), importedNtpServer1);
        }
        if (hasNtpServer2) {
          postText(entityName("screen_ntp_server_2"), importedNtpServer2);
        }
        if (hasNtpServer3) {
          postText(entityName("screen_ntp_server_3"), importedNtpServer3);
        }
        var importedScreensaverMode = importedSettings.screensaverMode;
        postText(entityName("screensaver_mode"), importedScreensaverMode);
        postText(entityName("presence_sensor_entity"), importedSettings.presenceSensorEntity);
        postSwitch(entityName("screen_saver_media_player_sleep_prevention"), importedSettings.mediaPlayerSleepPrevention);
        postText(entityName("media_player_sleep_prevention_entity"), importedSettings.mediaPlayerSleepPreventionEntity);
        postSwitch(entityName("screen_saver_cover_art"), importedSettings.coverArtScreensaver);
        postText(entityName("screen_saver_cover_art_entity"), importedSettings.coverArtMediaPlayerEntity);
        postText(entityName("screen_saver_cover_art_conditions"), importedSettings.coverArtAttributeConditions);
        postCoverArtDelay(importedSettings.coverArtDelay);
        postCoverArtTrackOverlayDuration(importedSettings.coverArtTrackOverlayDuration);
        postCoverArtHideExternalInput(importedSettings.coverArtHideExternalInput);
        postHomeAssistantArtworkPort(importedSettings.coverArtHomeAssistantPort);
        var importedScreensaverAction = importedSettings.screensaverAction;
        var importedScreensaverDimmedBrightness = importedSettings.screensaverDimmedBrightness;
        var importedClockBrightnessDay = importedSettings.clockBrightnessDay;
        var importedClockBrightnessNight = importedSettings.clockBrightnessNight;
        postScreensaverAction(importedScreensaverAction);
        postSwitch(entityName("screen_saver_clock"), importedScreensaverAction === "clock");
        postText(entityName("screensaver_pin"), "");
        postSwitch(entityName("screensaver_pin_required"), importedSettings.screensaverPinRequired);
        postClockBrightnessDay(importedClockBrightnessDay);
        postClockBrightnessNight(importedClockBrightnessNight);
        postScreensaverDimmedBrightness(importedScreensaverDimmedBrightness);
        postScreensaverTimeout(importedSettings.screensaverTimeout);
        postNumber(entityName("home_screen_timeout"), importedSettings.homeScreenTimeout);
        var importedScreenRotation = importedSettings.screenRotation;
        if (CFG.features && CFG.features.screenRotation) postSelect(entityName("screen_rotation"), importedScreenRotation);
        state.clockBarTemperatureEntities = importedSettings.clockBarTemperatureEntities;
        state._clockBarTemperatureEntitiesReceived = true;
        state._indoorOn = importedSettings.indoorTempEnable;
        state._outdoorOn = importedSettings.outdoorTempEnable;
        state.indoorEntity = importedSettings.indoorTempEntity;
        state.outdoorEntity = importedSettings.outdoorTempEntity;
        state.temperatureUnit = importedTemperatureUnit;
        state.clockBarOn = importedSettings.clockBar;
        state.clockBarTimeOn = importedSettings.clockBarTime;
        state.networkStatusOn = importedSettings.networkStatusIcon;
        state.temperatureDegreeSymbolOn = importedSettings.temperatureDegreeSymbol;
        state.subpageChevronsOn = importedSettings.subpageChevron;
        state.timezone = importedTimezone;
        state.language = importedLanguage;
        state.theme = importedTheme;
        state.clockFormat = importedClockFormat;
        state.ntpServer1 = importedNtpServer1;
        state.ntpServer2 = importedNtpServer2;
        state.ntpServer3 = importedNtpServer3;
        state.customNtpServers = hasCustomNtpServers();
        state.screensaverMode = importedScreensaverMode;
        state._screensaverModeReceived = true;
        state.presenceEntity = importedSettings.presenceSensorEntity;
        state.mediaPlayerSleepPreventionOn = importedSettings.mediaPlayerSleepPrevention;
        state.mediaPlayerSleepPreventionEntity = importedSettings.mediaPlayerSleepPreventionEntity;
        state.coverArtScreensaverOn = importedSettings.coverArtScreensaver;
        state.coverArtMediaPlayerEntity = importedSettings.coverArtMediaPlayerEntity;
        state.coverArtAttributeConditions = importedSettings.coverArtAttributeConditions;
        state.coverArtDelay = importedSettings.coverArtDelay;
        state.coverArtTrackOverlayDuration = importedSettings.coverArtTrackOverlayDuration;
        state.coverArtHideExternalInputOn = importedSettings.coverArtHideExternalInput;
        state.coverArtHomeAssistantPort = importedSettings.coverArtHomeAssistantPort;
        state.screensaverAction = importedScreensaverAction;
        state.screensaverPinRequired = importedSettings.screensaverPinRequired;
        state.screensaverPinSet = false;
        state._screensaverActionReceived = true;
        state.clockScreensaverOn = importedScreensaverAction === "clock";
        state.clockBrightnessDay = importedClockBrightnessDay;
        state.clockBrightnessNight = importedClockBrightnessNight;
        state.screensaverDimmedBrightness = importedScreensaverDimmedBrightness;
        state.screensaverTimeout = importedSettings.screensaverTimeout;
        state.homeScreenTimeout = importedSettings.homeScreenTimeout;
        state.screenRotation = importedScreenRotation;

        syncTemperatureUi();
        syncClockBarUi();
        if (els.setTemperatureUnit) els.setTemperatureUnit.value = state.temperatureUnit;
        syncInput(els.setPresence, state.presenceEntity);
        syncMediaPlayerSleepPreventionUi();
        syncInput(els.setCoverArtMediaPlayer, state.coverArtMediaPlayerEntity);
        syncInput(els.setCoverArtConditions, state.coverArtAttributeConditions);
        syncCoverArtScreensaverUi();
        syncThemeUi();
        if (els.setTimezone) els.setTimezone.value = state.timezone;
        syncLanguageSelect();
        if (els.setClockFormat) els.setClockFormat.value = state.clockFormat;
        syncNtpServerUi();
        syncClockScreensaverControls();
        syncScreensaverPinUi();
        syncScreensaverTimeoutUi();
        syncIdleUi();
        if (els.setScreenRotation) els.setScreenRotation.value = state.screenRotation;
        syncPreviewOrientation();
        if (els.setSsMode) els.setSsMode(getActiveScreensaverMode());
        updateTempPreview();

      }

      var screenSettings = backupPlan.screen;
      if (screenSettings) {
        var importedScreenSettings = EspControlModel.normalizeBackupScreenSettings(screenSettings, {
          scheduleWakeBrightness: state.scheduleWakeBrightness,
          scheduleDimmedBrightness: state.scheduleDimmedBrightness,
          scheduleClockBrightness: state.scheduleClockBrightness,
          scheduleClockTextColor: state.scheduleClockTextColor,
        });
        state.brightnessDayVal = importedScreenSettings.brightnessDayVal;
        state.brightnessNightVal = importedScreenSettings.brightnessNightVal;
        state.automaticBrightnessEnabled = importedScreenSettings.automaticBrightnessEnabled;
        state.brightnessDawnTime = importedScreenSettings.brightnessDawnTime;
        state.brightnessDuskTime = importedScreenSettings.brightnessDuskTime;
        state.scheduleTrigger = importedScreenSettings.scheduleTrigger;
        state.scheduleEnabled = importedScreenSettings.scheduleEnabled;
        state.scheduleOnHour = importedScreenSettings.scheduleOnHour;
        state.scheduleOffHour = importedScreenSettings.scheduleOffHour;
        state.scheduleMode = importedScreenSettings.scheduleMode;
        state.scheduleWakeTimeout = importedScreenSettings.scheduleWakeTimeout;
        state.scheduleWakeBrightness = importedScreenSettings.scheduleWakeBrightness;
        state.scheduleDimmedBrightness = importedScreenSettings.scheduleDimmedBrightness;
        state.scheduleClockBrightness = importedScreenSettings.scheduleClockBrightness;
        state.scheduleClockTextColor = importedScreenSettings.scheduleClockTextColor;

        postNumber(entityName("screen_daytime_brightness"), state.brightnessDayVal);
        postNumber(entityName("screen_nighttime_brightness"), state.brightnessNightVal);
        postAutomaticBrightnessEnabled(state.automaticBrightnessEnabled);
        postBrightnessDawnTime(state.brightnessDawnTime);
        postBrightnessDuskTime(state.brightnessDuskTime);
        postScreenScheduleTrigger(state.scheduleTrigger);
        postScreenScheduleOnHour(state.scheduleOnHour);
        postScreenScheduleOffHour(state.scheduleOffHour);
        postScreenScheduleMode(state.scheduleMode);
        postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
        postScreenScheduleWakeBrightness(state.scheduleWakeBrightness);
        postScreenScheduleDimmedBrightness(state.scheduleDimmedBrightness);
        postScreenScheduleClockBrightness(state.scheduleClockBrightness);
        postText(entityName("screen_schedule_clock_text_color"), state.scheduleClockTextColor);
        postScreenScheduleEnabled(state.scheduleEnabled);

        if (els.setDayBrightness) {
          els.setDayBrightness.value = state.brightnessDayVal;
          els.setDayBrightnessVal.textContent = Math.round(state.brightnessDayVal) + "%";
        }
        if (els.setNightBrightness) {
          els.setNightBrightness.value = state.brightnessNightVal;
          els.setNightBrightnessVal.textContent = Math.round(state.brightnessNightVal) + "%";
        }
        syncScreenScheduleUi();
      }

      state.selectedSlots = [];
      state.lastClickedSlot = -1;
      renderPreview();
      renderButtonSettings();
      switchTab("screen");
      showBanner("Configuration imported successfully", "success");
      cleanupInput();
    };
    reader.readAsText(input.files[0]);
  });

  document.body.appendChild(input);
  input.click();
}
