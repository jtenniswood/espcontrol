import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { createBackupImportController } from "../features/backup_import_controller";
import { createBackupExportController } from "../features/backup_export_controller";
import { createBackupRestoreController } from "../features/backup_restore_controller";
export function installAppBackupModule(): GlobalDescriptors {
    // ── Export / Import ────────────────────────────────────────────────────
    var backupExportController: any = createBackupExportController({
        "serializeButtonConfig": function (button: any) { return serializeButtonConfig(button); },
        "serializeSubpageConfig": function (subpage: any) { return serializeSubpageConfig(subpage); },
    });
    function backupExportScreenSizeSlug(this: any, value?: any) {
        return backupExportController.screenSizeSlug(value);
    }
    function backupExportFileDate(this: any, value?: any) {
        return backupExportController.fileDate(value);
    }
    function backupExportFileName(this: any, value?: any) {
        return backupExportController.fileName(CFG.screenSize, value);
    }
    function normalizeImportedPanelSettings(this: any, settings?: any) {
        if (!settings)
            return null;
        return EspControlModel.normalizeBackupPanelSettings(settings, {
            timezone: state.timezone,
            language: state.language,
            clockFormat: state.clockFormat,
            clockFormatOptions: state.clockFormatOptions,
            ntpDefaults: NTP_SERVER_DEFAULTS,
            ntpServer1: state.ntpServer1,
            ntpServer2: state.ntpServer2,
            ntpServer3: state.ntpServer3,
            coverArtHomeAssistantProtocol: state.homeAssistantArtworkProtocol,
            coverArtHomeAssistantPort: state.coverArtHomeAssistantPort,
            coverArtHomeAssistantBaseUrl: state.coverArtHomeAssistantBaseUrl,
            autoUpdate: state.autoUpdate,
            updateFrequency: state.updateFrequency,
            updateFrequencyOptions: state.updateFreqOptions,
            screenRotationOptions: allScreenRotationOptions(),
        });
    }
    function gridColsForImportedSettings(this: any, importedSettings?: any) {
        var rotation: any = importedSettings ? importedSettings.screenRotation : state.screenRotation;
        var layout: any = isPortraitRotation(rotation) && CFG.portrait ? CFG.portrait : CFG;
        return layout.cols || CFG.cols;
    }
    var backupImportController: any = createBackupImportController({
        "normalizeBackup": function (data: any) { return normalizeBackupConfig(data); },
        "normalizeSettings": function (settings: any) { return normalizeImportedPanelSettings(settings); },
        "gridColsForSettings": function (settings: any) { return gridColsForImportedSettings(settings); },
        "getGridCols": function () { return GRID_COLS; },
        "setGridCols": function (gridCols: any) { GRID_COLS = gridCols; },
        "planBackupImport": function (data: any, target: any) { return planBackupImport(data, target); },
    });
    var backupRestoreController: any = createBackupRestoreController({
        "plan": function (backup: any, target: any) { return backupImportController.plan(backup, target); },
        "warnings": function (plannedImport: any) { return plannedImport.backupPlan.warnings; },
        "showBanner": showBanner,
        "setPostThrottle": setPostThrottle,
        "resetPostQueueError": resetPostQueueError,
        "postQueueIdle": postQueueIdle,
        "postQueueHadError": postQueueHadError,
    });
    function downloadBackupConfig(this: any, data?: any) {
        var json: any = JSON.stringify(data, null, 2);
        var blob: any = new Blob([json], { type: "application/json" });
        var url: any = URL.createObjectURL(blob);
        var a: any = document.createElement("a");
        a.href = url;
        a.download = backupExportFileName();
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }
    function addNativeConfigToBackup(this: any, data?: any) {
        return backupExportController.addNativeConfig(data, {
            "deviceProfile": DEVICE_ID,
            "buttons": state.buttons,
            "subpages": state.subpages,
            "buttonOrder": data.button_order,
            "buttonOnColor": data.button_on_color,
        });
    }
    function exportConfig(this: any) {
        var data: any = createBackupConfig({
            device: DEVICE_ID,
            slots: NUM_SLOTS,
            exported_at: new Date().toISOString(),
            grid: state.grid,
            sizes: state.sizes,
            button_order: serializeGrid(state.grid),
            button_on_color: state.onColor,
            buttons: state.buttons,
            subpages: state.subpages,
            settings: {
                indoor_temp_enable: state._indoorOn,
                outdoor_temp_enable: state._outdoorOn,
                clock_bar_temperature_entities: serializeClockBarTemperatureEntities(clockBarTemperatureEntities()),
                indoor_temp_entity: state.indoorEntity,
                outdoor_temp_entity: state.outdoorEntity,
                temperature_unit: normalizeTemperatureUnit(state.temperatureUnit),
                clock_bar: state.clockBarOn,
                clock_bar_time: state.clockBarTimeOn,
                clock_bar_night_mode: state.clockBarNightModeOn,
                network_status_icon: state.networkStatusOn,
                voice_services: state.voiceServicesOn,
                alarm_delay_audio: state.alarmDelayAudioOn,
                alarm_delay_tts: state.alarmDelayTtsOn,
                alarm_delay_entry_announcement: state.alarmDelayEntryAnnouncement,
                alarm_delay_exit_announcement: state.alarmDelayExitAnnouncement,
                alarm_delay_beep_volume: state.alarmDelayBeepVolume,
                alarm_delay_final_countdown: state.alarmDelayFinalCountdown,
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
                media_player_sleep_prevention_entity: state.mediaPlayerSleepPreventionEntity || state.coverArtMediaPlayerEntity,
                cover_art_screensaver: state.coverArtScreensaverOn,
                cover_art_media_player_entity: state.coverArtMediaPlayerEntity,
                cover_art_secondary_media_player_entity: state.coverArtSecondaryMediaPlayerEntity,
                cover_art_attribute_conditions: state.coverArtAttributeConditions,
                cover_art_delay: state.coverArtDelay,
                cover_art_track_overlay_duration: state.coverArtTrackOverlayDuration,
                cover_art_hide_external_input: state.coverArtHideExternalInputOn,
                home_assistant_artwork_protocol: normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol),
                home_assistant_artwork_port: normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort),
                home_assistant_artwork_base_url: normalizeHomeAssistantArtworkBaseUrl(state.coverArtHomeAssistantBaseUrl),
                firmware_auto_update: !!state.autoUpdate,
                firmware_update_frequency: state.updateFrequency,
                screensaver_action: normalizeScreensaverAction(state.screensaverAction),
                clock_screensaver: state.clockScreensaverOn,
                clock_brightness: state.clockBrightnessDay,
                clock_brightness_day: state.clockBrightnessDay,
                clock_brightness_night: state.clockBrightnessNight,
                screensaver_dimmed_brightness: normalizeScreensaverDimmedBrightness(state.screensaverDimmedBrightness),
                screensaver_dimmed_brightness_day: normalizeScreensaverDimmedBrightness(state.screensaverDimmedBrightnessDay),
                screensaver_dimmed_brightness_night: normalizeScreensaverDimmedBrightness(state.screensaverDimmedBrightnessNight),
                screensaver_timeout: state.screensaverTimeout,
                home_screen_timeout: state.homeScreenTimeout,
                screen_rotation: state.screenRotation,
            },
            screen: {
                brightness_day: Math.round(state.brightnessDayVal),
                brightness_night: Math.round(state.brightnessNightVal),
                brightness_mode: normalizeBrightnessMode(state.brightnessMode),
                manual_brightness: Math.round(state.manualBrightnessVal),
                brightness_dawn_time: normalizeTimeOfDay(state.brightnessDawnTime, "06:00"),
                brightness_dusk_time: normalizeTimeOfDay(state.brightnessDuskTime, "18:00"),
                schedule_trigger: normalizeScheduleTrigger(state.scheduleTrigger, state.scheduleEnabled),
                schedule_enabled: !!state.scheduleEnabled,
                schedule_sensor_activation: normalizeScheduleSensorActivation(state.scheduleSensorActivation),
                schedule_sensor_entity: state.scheduleSensorEntity,
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
        downloadBackupConfig(addNativeConfigToBackup(data));
    }
    function importConfig(this: any) {
        var input: any = document.createElement("input");
        input.type = "file";
        input.accept = ".json";
        input.style.display = "none";
        function cleanupInput(this: any) {
            if (input.parentNode)
                input.parentNode.removeChild(input);
        }
        input.addEventListener("cancel", cleanupInput);
        input.addEventListener("change", function (this: any) {
            if (!input.files || !input.files[0]) {
                cleanupInput();
                return;
            }
            var reader: any = new FileReader();
            reader.onerror = function (this: any) {
                cleanupInput();
                showBanner("Invalid file \u2014 could not read backup", "error");
            };
            reader.onload = function (this: any) {
                var data: any;
                try {
                    data = JSON.parse(reader.result);
                }
                catch (_) {
                    showBanner("Invalid file \u2014 could not parse JSON", "error");
                    cleanupInput();
                    return;
                }
                function applyBackupRestorePlan(this: any, plannedImport: any) {
                var importedSettings: any = plannedImport.importedSettings;
                var importedGridCols: any = plannedImport.importedGridCols;
                var backupPlan: any = plannedImport.backupPlan;
                postText(entityName("button_on_color"), backupPlan.config.button_on_color);
                for (var i: any = 0; i < NUM_SLOTS; i++) {
                    var b: any = backupPlan.buttons[i];
                    var n: any = i + 1;
                    state.buttons[i] = backupNormalizeButtonConfig(b);
                    saveButtonConfig(n);
                }
                state.subpages = {};
                state.subpageRaw = {};
                for (var subpageKey in backupPlan.subpages) {
                    state.subpages[subpageKey] = backupPlan.subpages[subpageKey];
                    saveSubpageEntity(subpageKey);
                }
                var activeGridCols: any = GRID_COLS;
                GRID_COLS = importedGridCols;
                var normalizedButtonOrder: any;
                try {
                    normalizedButtonOrder = applyImportedButtonOrder(backupPlan.button_order, backupPlan.importedSizes);
                }
                finally {
                    GRID_COLS = activeGridCols;
                }
                postText(entityName("button_order"), normalizedButtonOrder);
                state.onColor = backupPlan.config.button_on_color;
                if (els.setOnColor && els.setOnColor._syncColor)
                    els.setOnColor._syncColor(state.onColor);
                if (backupPlan.settings) {
                    var s: any = backupPlan.settings;
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
                    postClockBarTime(importedSettings.clockBarTime);
                    postClockBarNightMode(importedSettings.clockBarNightMode);
                    postNetworkStatusIcon(importedSettings.networkStatusIcon);
                    if (CFG.features && CFG.features.voiceServices)
                        postVoiceServices(importedSettings.voiceServices);
                    if (CFG.features && CFG.features.alarmDelayAudio) {
                        postAlarmDelayAudio(importedSettings.alarmDelayAudio);
                        postAlarmDelayTts(importedSettings.alarmDelayTts);
                        postAlarmDelayEntryAnnouncement(importedSettings.alarmDelayEntryAnnouncement);
                        postAlarmDelayExitAnnouncement(importedSettings.alarmDelayExitAnnouncement);
                        postAlarmDelayBeepVolume(importedSettings.alarmDelayBeepVolume);
                        postAlarmDelayFinalCountdown(importedSettings.alarmDelayFinalCountdown);
                    }
                    postTemperatureDegreeSymbol(importedSettings.temperatureDegreeSymbol);
                    postSubpageChevron(importedSettings.subpageChevron);
                    var importedTimezone: any = importedSettings.timezone;
                    var importedTemperatureUnit: any = importedSettings.temperatureUnit;
                    var importedLanguage: any = importedSettings.language;
                    var importedClockFormat: any = importedSettings.clockFormat;
                    var hasNtpServer1: any = importedSettings.hasNtpServer1;
                    var hasNtpServer2: any = importedSettings.hasNtpServer2;
                    var hasNtpServer3: any = importedSettings.hasNtpServer3;
                    var importedNtpServer1: any = importedSettings.ntpServer1;
                    var importedNtpServer2: any = importedSettings.ntpServer2;
                    var importedNtpServer3: any = importedSettings.ntpServer3;
                    if (s.timezone)
                        postSelect(entityName("screen_timezone"), importedTimezone);
                    if (s.language)
                        postSelect(entityName("screen_language"), importedLanguage);
                    postSelect(entityName("screen_temperature_unit"), importedTemperatureUnit);
                    if (s.clock_format)
                        postSelect(entityName("screen_clock_format"), importedClockFormat);
                    if (hasNtpServer1) {
                        postText(entityName("screen_ntp_server_1"), importedNtpServer1);
                    }
                    if (hasNtpServer2) {
                        postText(entityName("screen_ntp_server_2"), importedNtpServer2);
                    }
                    if (hasNtpServer3) {
                        postText(entityName("screen_ntp_server_3"), importedNtpServer3);
                    }
                    var importedScreensaverMode: any = importedSettings.screensaverMode;
                    postScreensaverMode(importedScreensaverMode);
                    postPresenceSensorEntity(importedSettings.presenceSensorEntity);
                    postMediaPlayerSleepPrevention(importedSettings.mediaPlayerSleepPrevention);
                    postMediaPlayerSleepPreventionEntity(importedSettings.mediaPlayerSleepPreventionEntity);
                    postCoverArtScreensaver(importedSettings.coverArtScreensaver);
                    postCoverArtMediaPlayerEntity(importedSettings.coverArtMediaPlayerEntity);
                    postCoverArtSecondaryMediaPlayerEntity(importedSettings.coverArtSecondaryMediaPlayerEntity);
                    postCoverArtConditions(importedSettings.coverArtAttributeConditions);
                    postCoverArtDelay(importedSettings.coverArtDelay);
                    postCoverArtTrackOverlayDuration(importedSettings.coverArtTrackOverlayDuration);
                    postCoverArtHideExternalInput(importedSettings.coverArtHideExternalInput);
                    postHomeAssistantArtworkProtocol(importedSettings.coverArtHomeAssistantProtocol);
                    postHomeAssistantArtworkPort(importedSettings.coverArtHomeAssistantPort);
                    postHomeAssistantArtworkBaseUrl(importedSettings.coverArtHomeAssistantBaseUrl);
                    if (firmwareUpdateControlsVisible()) {
                        postFirmwareAutoUpdate(importedSettings.autoUpdate);
                        postFirmwareUpdateFrequency(importedSettings.updateFrequency);
                    }
                    var importedScreensaverAction: any = importedSettings.screensaverAction;
                    var importedScreensaverDimmedBrightness: any = importedSettings.screensaverDimmedBrightness;
                    var importedScreensaverDimmedBrightnessDay: any = importedSettings.screensaverDimmedBrightnessDay;
                    var importedScreensaverDimmedBrightnessNight: any = importedSettings.screensaverDimmedBrightnessNight;
                    var importedClockBrightnessDay: any = importedSettings.clockBrightnessDay;
                    var importedClockBrightnessNight: any = importedSettings.clockBrightnessNight;
                    postScreensaverAction(importedScreensaverAction);
                    postClockScreensaver(importedScreensaverAction === "clock");
                    postClockBrightnessDay(importedClockBrightnessDay);
                    postClockBrightnessNight(importedClockBrightnessNight);
                    postScreensaverDimmedBrightness(importedScreensaverDimmedBrightness);
                    postScreensaverDimmedBrightnessDay(importedScreensaverDimmedBrightnessDay);
                    postScreensaverDimmedBrightnessNight(importedScreensaverDimmedBrightnessNight);
                    postScreensaverTimeout(importedSettings.screensaverTimeout);
                    postHomeScreenTimeout(importedSettings.homeScreenTimeout);
                    var importedScreenRotation: any = importedSettings.screenRotation;
                    if (CFG.features && CFG.features.screenRotation)
                        postSelect(entityName("screen_rotation"), importedScreenRotation);
                    state.clockBarTemperatureEntities = importedSettings.clockBarTemperatureEntities;
                    state._clockBarTemperatureEntitiesReceived = true;
                    state._indoorOn = importedSettings.indoorTempEnable;
                    state._outdoorOn = importedSettings.outdoorTempEnable;
                    state.indoorEntity = importedSettings.indoorTempEntity;
                    state.outdoorEntity = importedSettings.outdoorTempEntity;
                    state.temperatureUnit = importedTemperatureUnit;
                    state.clockBarOn = importedSettings.clockBar;
                    state.clockBarTimeOn = importedSettings.clockBarTime;
                    state.clockBarNightModeOn = importedSettings.clockBarNightMode;
                    state.networkStatusOn = importedSettings.networkStatusIcon;
                    state.voiceServicesOn = importedSettings.voiceServices;
                    state.alarmDelayAudioOn = importedSettings.alarmDelayAudio;
                    state.alarmDelayTtsOn = importedSettings.alarmDelayTts;
                    state.alarmDelayEntryAnnouncement = importedSettings.alarmDelayEntryAnnouncement;
                    state.alarmDelayExitAnnouncement = importedSettings.alarmDelayExitAnnouncement;
                    state.alarmDelayBeepVolume = importedSettings.alarmDelayBeepVolume;
                    state.alarmDelayFinalCountdown = importedSettings.alarmDelayFinalCountdown;
                    state.temperatureDegreeSymbolOn = importedSettings.temperatureDegreeSymbol;
                    state.subpageChevronsOn = importedSettings.subpageChevron;
                    state.timezone = importedTimezone;
                    state.language = importedLanguage;
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
                    state.coverArtSecondaryMediaPlayerEntity = importedSettings.coverArtSecondaryMediaPlayerEntity;
                    state.coverArtAttributeConditions = importedSettings.coverArtAttributeConditions;
                    state.coverArtDelay = importedSettings.coverArtDelay;
                    state.coverArtTrackOverlayDuration = importedSettings.coverArtTrackOverlayDuration;
                    state.coverArtHideExternalInputOn = importedSettings.coverArtHideExternalInput;
                    state.homeAssistantArtworkProtocol = importedSettings.coverArtHomeAssistantProtocol;
                    state.coverArtHomeAssistantPort = importedSettings.coverArtHomeAssistantPort;
                    state.coverArtHomeAssistantBaseUrl = importedSettings.coverArtHomeAssistantBaseUrl;
                    state.autoUpdate = importedSettings.autoUpdate;
                    state.updateFrequency = importedSettings.updateFrequency;
                    state.screensaverAction = importedScreensaverAction;
                    state._screensaverActionReceived = true;
                    state.clockScreensaverOn = importedScreensaverAction === "clock";
                    state.clockBrightnessDay = importedClockBrightnessDay;
                    state.clockBrightnessNight = importedClockBrightnessNight;
                    state.screensaverDimmedBrightness = importedScreensaverDimmedBrightness;
                    state.screensaverDimmedBrightnessDay = importedScreensaverDimmedBrightnessDay;
                    state.screensaverDimmedBrightnessNight = importedScreensaverDimmedBrightnessNight;
                    state.screensaverTimeout = importedSettings.screensaverTimeout;
                    state.homeScreenTimeout = importedSettings.homeScreenTimeout;
                    state.screenRotation = importedScreenRotation;
                    syncTemperatureUi();
                    syncClockBarUi();
                    syncAlarmDelayAudioUi();
                    if (els.setTemperatureUnit)
                        els.setTemperatureUnit.value = state.temperatureUnit;
                    syncInput(els.setPresence, state.presenceEntity);
                    syncInput(els.setSchedulePresence, state.scheduleSensorEntity);
                    syncMediaPlayerSleepPreventionUi();
                    syncInput(els.setCoverArtMediaPlayer, state.coverArtMediaPlayerEntity);
                    syncInput(els.setCoverArtSecondaryMediaPlayer, state.coverArtSecondaryMediaPlayerEntity);
                    syncInput(els.setCoverArtConditions, state.coverArtAttributeConditions);
                    syncCoverArtScreensaverUi();
                    syncInput(els.setCoverArtHomeAssistantBaseUrl, state.coverArtHomeAssistantBaseUrl);
                    if (els.setAutoUpdate)
                        els.setAutoUpdate.checked = state.autoUpdate;
                    if (els.setUpdateFreq)
                        els.setUpdateFreq.value = state.updateFrequency;
                    syncFirmwareUpdateUi();
                    if (els.setTimezone)
                        els.setTimezone.value = state.timezone;
                    syncLanguageSelect();
                    if (els.setClockFormat)
                        els.setClockFormat.value = state.clockFormat;
                    syncNtpServerUi();
                    syncClockScreensaverControls();
                    syncScreensaverTimeoutUi();
                    syncIdleUi();
                    if (els.setScreenRotation)
                        els.setScreenRotation.value = state.screenRotation;
                    syncPreviewOrientation();
                    if (els.setSsMode)
                        els.setSsMode(getActiveScreensaverMode());
                    updateTempPreview();
                }
                var screenSettings: any = backupPlan.screen;
                if (screenSettings) {
                    var importedScreenSettings: any = EspControlModel.normalizeBackupScreenSettings(screenSettings, {
                        scheduleWakeBrightness: state.scheduleWakeBrightness,
                        scheduleDimmedBrightness: state.scheduleDimmedBrightness,
                        scheduleClockBrightness: state.scheduleClockBrightness,
                        scheduleClockTextColor: state.scheduleClockTextColor,
                        scheduleSensorActivation: state.scheduleSensorActivation,
                        manualBrightnessVal: state.manualBrightnessVal,
                    }, importedSettings ? importedSettings.presenceSensorEntity : state.presenceEntity);
                    state.brightnessDayVal = importedScreenSettings.brightnessDayVal;
                    state.brightnessNightVal = importedScreenSettings.brightnessNightVal;
                    state.brightnessMode = importedScreenSettings.brightnessMode;
                    state.manualBrightnessVal = importedScreenSettings.manualBrightnessVal;
                    state.brightnessDawnTime = importedScreenSettings.brightnessDawnTime;
                    state.brightnessDuskTime = importedScreenSettings.brightnessDuskTime;
                    state.scheduleTrigger = importedScreenSettings.scheduleTrigger;
                    state.scheduleEnabled = importedScreenSettings.scheduleEnabled;
                    state.scheduleSensorActivation = importedScreenSettings.scheduleSensorActivation;
                    state.scheduleSensorEntity = importedScreenSettings.scheduleSensorEntity;
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
                    postBrightnessMode(state.brightnessMode);
                    if (state.brightnessMode === "manual")
                        postDisplayBacklightBrightness(state.manualBrightnessVal);
                    postBrightnessDawnTime(state.brightnessDawnTime);
                    postBrightnessDuskTime(state.brightnessDuskTime);
                    postScreenScheduleTrigger(state.scheduleTrigger);
                    postScreenScheduleSensorActivation(state.scheduleSensorActivation);
                    postScreenScheduleSensorEntity(state.scheduleSensorEntity);
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
                }
                backupRestoreController.restore(data, { device: DEVICE_ID, slots: NUM_SLOTS }, applyBackupRestorePlan);
                cleanupInput();
            };
            reader.readAsText(input.files[0]);
        });
        document.body.appendChild(input);
        input.click();
    }
    return {
        "backupExportScreenSizeSlug": staticGlobal(backupExportScreenSizeSlug),
        "backupExportFileDate": staticGlobal(backupExportFileDate),
        "backupExportFileName": staticGlobal(backupExportFileName),
        "downloadBackupConfig": staticGlobal(downloadBackupConfig),
        "addNativeConfigToBackup": staticGlobal(addNativeConfigToBackup),
        "normalizeImportedPanelSettings": staticGlobal(normalizeImportedPanelSettings),
        "gridColsForImportedSettings": staticGlobal(gridColsForImportedSettings),
        "exportConfig": staticGlobal(exportConfig),
        "importConfig": staticGlobal(importConfig),
    };
}
