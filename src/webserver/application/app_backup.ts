import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppBackupModule(): GlobalDescriptors {
    // ── Export / Import ────────────────────────────────────────────────────
    function backupExportScreenSizeSlug(this: any, value?: any) {
        value = String(value || "").trim().toLowerCase();
        if (!value)
            return "screen";
        value = value.replace(/\binches\b/g, "inch").replace(/\bin\b/g, "inch");
        value = value.replace(/[^a-z0-9.]+/g, "-").replace(/^-+|-+$/g, "");
        return value || "screen";
    }
    function backupExportFileDate(this: any, value?: any) {
        return value.getFullYear() + "-" +
            String(value.getMonth() + 1).padStart(2, "0") + "-" +
            String(value.getDate()).padStart(2, "0");
    }
    function backupExportFileName(this: any, value?: any) {
        var date: any = value || new Date();
        return "espcontrol-" + backupExportScreenSizeSlug(CFG.screenSize) + "-" +
            backupExportFileDate(date) + ".zip";
    }
    function backupZipCrc32(this: any, bytes?: any) {
        var crc: any = 0xFFFFFFFF;
        for (var i: any = 0; i < bytes.length; i++) {
            crc ^= bytes[i];
            for (var bit: any = 0; bit < 8; bit++)
                crc = (crc >>> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        return (crc ^ 0xFFFFFFFF) >>> 0;
    }
    function backupZipU16(this: any, value?: any) {
        return new Uint8Array([value & 255, (value >>> 8) & 255]);
    }
    function backupZipU32(this: any, value?: any) {
        return new Uint8Array([value & 255, (value >>> 8) & 255, (value >>> 16) & 255, (value >>> 24) & 255]);
    }
    function backupCreateZip(this: any, entries?: any) {
        var chunks: any = [];
        var central: any = [];
        var offset: any = 0;
        entries.forEach(function (this: any, entry?: any) {
            var name: any = new TextEncoder().encode(entry.name);
            var body: any = entry.bytes instanceof Uint8Array ? entry.bytes : new Uint8Array(entry.bytes);
            var crc: any = backupZipCrc32(body);
            var local: any = [backupZipU32(0x04034b50), backupZipU16(20), backupZipU16(0), backupZipU16(0),
                backupZipU16(0), backupZipU16(0), backupZipU32(crc), backupZipU32(body.length),
                backupZipU32(body.length), backupZipU16(name.length), backupZipU16(0), name, body];
            chunks.push.apply(chunks, local);
            var centralEntry: any = [backupZipU32(0x02014b50), backupZipU16(20), backupZipU16(20),
                backupZipU16(0), backupZipU16(0), backupZipU16(0), backupZipU16(0), backupZipU32(crc),
                backupZipU32(body.length), backupZipU32(body.length), backupZipU16(name.length), backupZipU16(0),
                backupZipU16(0), backupZipU16(0), backupZipU16(0), backupZipU32(0), backupZipU32(offset), name];
            central.push.apply(central, centralEntry);
            offset += 30 + name.length + body.length;
        });
        var centralSize: any = central.reduce(function (this: any, total?: any, part?: any) { return total + part.length; }, 0);
        chunks.push.apply(chunks, central);
        chunks.push(backupZipU32(0x06054b50), backupZipU16(0), backupZipU16(0),
            backupZipU16(entries.length), backupZipU16(entries.length), backupZipU32(centralSize),
            backupZipU32(offset), backupZipU16(0));
        return new Blob(chunks, { type: "application/zip" });
    }
    function backupReadStoredZip(this: any, buffer?: any) {
        var bytes: any = new Uint8Array(buffer);
        var view: any = new DataView(buffer);
        var entries: any = {};
        var offset: any = 0;
        while (offset + 4 <= bytes.length && view.getUint32(offset, true) === 0x04034b50) {
            if (offset + 30 > bytes.length)
                throw new Error("Invalid ZIP backup.");
            var flags: any = view.getUint16(offset + 6, true);
            var compression: any = view.getUint16(offset + 8, true);
            var size: any = view.getUint32(offset + 18, true);
            var nameLength: any = view.getUint16(offset + 26, true);
            var extraLength: any = view.getUint16(offset + 28, true);
            if ((flags & 8) || compression !== 0)
                throw new Error("Unsupported ZIP backup format.");
            var nameStart: any = offset + 30;
            var dataStart: any = nameStart + nameLength + extraLength;
            var dataEnd: any = dataStart + size;
            if (dataEnd > bytes.length)
                throw new Error("Invalid ZIP backup.");
            var name: any = new TextDecoder().decode(bytes.slice(nameStart, nameStart + nameLength));
            entries[name] = bytes.slice(dataStart, dataEnd);
            offset = dataEnd;
        }
        if (!entries["backup.json"])
            throw new Error("ZIP backup is missing backup.json.");
        return entries;
    }
    function backupDownload(this: any, blob?: any, name?: any) {
        var url: any = URL.createObjectURL(blob);
        var a: any = document.createElement("a");
        a.href = url;
        a.download = name;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }
    function backupImageArchiveEntries(this: any) {
        return cardImageBackupAssetProvider.createArchiveEntries();
    }
    function backupRestoreArchivedImages(this: any, entries?: any) {
        return cardImageBackupAssetProvider.restoreArchiveEntries(entries);
    }
    function backupRemapImportedImageReferences(this: any, backupPlan?: any, idMap?: any) {
        cardImageBackupAssetProvider.remapImportedReferences(backupPlan || {}, idMap || {});
    }
    function backupCommitArchivedImages(this: any) {
        return cardImageBackupAssetProvider.commitRestore
            ? cardImageBackupAssetProvider.commitRestore()
            : Promise.resolve();
    }
    function backupRollbackArchivedImages(this: any) {
        return cardImageBackupAssetProvider.rollbackRestore
            ? cardImageBackupAssetProvider.rollbackRestore()
            : Promise.resolve();
    }
    function backupCardConfigurationSnapshot(this: any) {
        var subpages: any = {};
        Object.keys(state.subpages || {}).forEach(function (key) {
            var subpage: any = state.subpages[key];
            if (subpage)
                subpages[key] = parseSubpageConfig(serializeSubpageConfig(subpage));
        });
        return {
            onColor: state.onColor,
            order: serializeGrid(state.grid),
            buttons: (state.buttons || []).map(function (button) {
                return backupNormalizeButtonConfig(button);
            }),
            subpages: subpages,
        };
    }
    function backupRestoreCardConfigurationSnapshot(this: any, snapshot?: any) {
        resetPostQueueError();
        postText(entityName("button_on_color"), snapshot.onColor);
        for (var index: any = 0; index < NUM_SLOTS; index++) {
            state.buttons[index] = backupNormalizeButtonConfig(snapshot.buttons[index]);
            saveButtonConfig(index + 1);
        }
        state.subpages = snapshot.subpages || {};
        state.subpageRaw = {};
        for (var slot: any = 1; slot <= NUM_SLOTS; slot++)
            saveSubpageEntity(String(slot));
        postText(entityName("button_order"), snapshot.order);
        applyImportedButtonOrder(snapshot.order, {});
        state.onColor = snapshot.onColor;
        return postQueueIdle().then(function () {
            if (postQueueHadError())
                throw new Error("The previous card layout could not be fully restored.");
            renderPreview();
            renderButtonSettings();
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
                firmware_auto_update: !!state.autoUpdate,
                firmware_update_frequency: state.updateFrequency,
                screensaver_action: normalizeScreensaverAction(state.screensaverAction),
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
                schedule_sensor_activation: normalizeScheduleSensorActivation(state.scheduleSensorActivation),
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
        backupImageArchiveEntries()
            .then(function (this: any, imageEntries?: any) {
                imageEntries.unshift({ name: "backup.json", bytes: new TextEncoder().encode(JSON.stringify(data, null, 2)) });
                backupDownload(backupCreateZip(imageEntries), backupExportFileName());
                showBanner("Backup exported with " + Math.max(0, imageEntries.length - 2) + " image" +
                    (imageEntries.length === 3 ? "" : "s") + ".", "success");
            })
            .catch(function (this: any, err?: any) {
                showBanner(err && err.message || "Could not create backup with images.", "error");
            });
    }
    function importConfig(this: any) {
        var input: any = document.createElement("input");
        input.type = "file";
        input.accept = ".json,.zip";
        input.style.display = "none";
        var importPostThrottleMs: any = 75;
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
            function processImportText(this: any, importText?: any, zipEntries?: any) {
                var data: any;
                try {
                    data = JSON.parse(importText);
                }
                catch (_) {
                    showBanner("Invalid file \u2014 could not parse JSON", "error");
                    cleanupInput();
                    return;
                }
                var backupPlan: any;
                try {
                    backupPlan = planBackupImport(data, { device: DEVICE_ID, slots: NUM_SLOTS });
                }
                catch (e) {
                    showBanner((e as any).backupMessage || "Invalid config file \u2014 missing required fields", "error");
                    cleanupInput();
                    return;
                }
                for (var warningIdx: any = 0; warningIdx < backupPlan.warnings.length; warningIdx++) {
                    showBanner(backupPlan.warnings[warningIdx], "warning");
                }
                var cardConfigurationSnapshot: any = backupCardConfigurationSnapshot();
                return backupRestoreArchivedImages(zipEntries).then(function (this: any, imageIdMap?: any) {
                backupRemapImportedImageReferences(backupPlan, imageIdMap);
                setPostThrottle(importPostThrottleMs);
                resetPostQueueError();
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
                }
                for (var subpageSlot: any = 1; subpageSlot <= NUM_SLOTS; subpageSlot++)
                    saveSubpageEntity(String(subpageSlot));
                postText(entityName("button_order"), backupPlan.button_order);
                applyImportedButtonOrder(backupPlan.button_order, backupPlan.importedSizes);
                state.onColor = backupPlan.config.button_on_color;
                if (els.setOnColor && els.setOnColor._syncColor)
                    els.setOnColor._syncColor(state.onColor);
                if (backupPlan.settings) {
                    var s: any = backupPlan.settings;
                    var importedSettings: any = EspControlModel.normalizeBackupPanelSettings(s, {
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
                        autoUpdate: state.autoUpdate,
                        updateFrequency: state.updateFrequency,
                        updateFrequencyOptions: state.updateFreqOptions,
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
                    postClockBarTime(importedSettings.clockBarTime);
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
                    if (firmwareUpdateControlsVisible()) {
                        postFirmwareAutoUpdate(importedSettings.autoUpdate);
                        postFirmwareUpdateFrequency(importedSettings.updateFrequency);
                    }
                    var importedScreensaverAction: any = importedSettings.screensaverAction;
                    var importedScreensaverDimmedBrightness: any = importedSettings.screensaverDimmedBrightness;
                    var importedClockBrightnessDay: any = importedSettings.clockBrightnessDay;
                    var importedClockBrightnessNight: any = importedSettings.clockBrightnessNight;
                    postScreensaverAction(importedScreensaverAction);
                    postClockScreensaver(importedScreensaverAction === "clock");
                    postClockBrightnessDay(importedClockBrightnessDay);
                    postClockBrightnessNight(importedClockBrightnessNight);
                    postScreensaverDimmedBrightness(importedScreensaverDimmedBrightness);
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
                    state.autoUpdate = importedSettings.autoUpdate;
                    state.updateFrequency = importedSettings.updateFrequency;
                    state.screensaverAction = importedScreensaverAction;
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
                    syncAlarmDelayAudioUi();
                    if (els.setTemperatureUnit)
                        els.setTemperatureUnit.value = state.temperatureUnit;
                    syncInput(els.setPresence, state.presenceEntity);
                    syncMediaPlayerSleepPreventionUi();
                    syncInput(els.setCoverArtMediaPlayer, state.coverArtMediaPlayerEntity);
                    syncInput(els.setCoverArtSecondaryMediaPlayer, state.coverArtSecondaryMediaPlayerEntity);
                    syncInput(els.setCoverArtConditions, state.coverArtAttributeConditions);
                    syncCoverArtScreensaverUi();
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
                    });
                    state.brightnessDayVal = importedScreenSettings.brightnessDayVal;
                    state.brightnessNightVal = importedScreenSettings.brightnessNightVal;
                    state.automaticBrightnessEnabled = importedScreenSettings.automaticBrightnessEnabled;
                    state.brightnessDawnTime = importedScreenSettings.brightnessDawnTime;
                    state.brightnessDuskTime = importedScreenSettings.brightnessDuskTime;
                    state.scheduleTrigger = importedScreenSettings.scheduleTrigger;
                    state.scheduleEnabled = importedScreenSettings.scheduleEnabled;
                    state.scheduleSensorActivation = importedScreenSettings.scheduleSensorActivation;
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
                    postScreenScheduleSensorActivation(state.scheduleSensorActivation);
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
                setPostThrottle(0);
                cleanupInput();
                return postQueueIdle().then(function (this: any) {
                    if (postQueueHadError())
                        throw new Error("Some restored settings could not be saved. Staged images were rolled back.");
                    return backupCommitArchivedImages().then(function () {
                        showBanner("Configuration imported successfully", "success");
                    });
                }).catch(function (this: any, err?: any) {
                    return backupRestoreCardConfigurationSnapshot(cardConfigurationSnapshot).catch(function () {
                        // Device rollback below still removes any partially saved staged references.
                    }).then(function () { return backupRollbackArchivedImages(); }).catch(function () {
                        throw new Error("Restore recovery is incomplete. Restart the display before trying again.");
                    }).then(function () { throw err; });
                });
                }).catch(function (this: any, err?: any) {
                    showBanner(err && err.message || "Could not restore backup images.", "error");
                    cleanupInput();
                });
            }
            var selectedFile: any = input.files[0];
            if (/\.zip$/i.test(selectedFile.name || "")) {
                selectedFile.arrayBuffer()
                    .then(backupReadStoredZip)
                    .then(function (this: any, entries?: any) {
                        return processImportText(new TextDecoder().decode(entries["backup.json"]), entries);
                    })
                    .catch(function (this: any, err?: any) {
                        showBanner(err && err.message || "Invalid ZIP backup", "error");
                        cleanupInput();
                    });
            }
            else {
                reader.onload = function () { processImportText(reader.result); };
                reader.readAsText(selectedFile);
            }
        });
        document.body.appendChild(input);
        input.click();
    }
    return {
        "backupExportScreenSizeSlug": staticGlobal(backupExportScreenSizeSlug),
        "backupExportFileDate": staticGlobal(backupExportFileDate),
        "backupExportFileName": staticGlobal(backupExportFileName),
        "backupCommitArchivedImages": staticGlobal(backupCommitArchivedImages),
        "backupRollbackArchivedImages": staticGlobal(backupRollbackArchivedImages),
        "backupCardConfigurationSnapshot": staticGlobal(backupCardConfigurationSnapshot),
        "backupRestoreCardConfigurationSnapshot": staticGlobal(backupRestoreCardConfigurationSnapshot),
        "backupZipCrc32": staticGlobal(backupZipCrc32),
        "backupZipU16": staticGlobal(backupZipU16),
        "backupZipU32": staticGlobal(backupZipU32),
        "backupCreateZip": staticGlobal(backupCreateZip),
        "backupReadStoredZip": staticGlobal(backupReadStoredZip),
        "backupDownload": staticGlobal(backupDownload),
        "backupImageArchiveEntries": staticGlobal(backupImageArchiveEntries),
        "backupRestoreArchivedImages": staticGlobal(backupRestoreArchivedImages),
        "backupRemapImportedImageReferences": staticGlobal(backupRemapImportedImageReferences),
        "exportConfig": staticGlobal(exportConfig),
        "importConfig": staticGlobal(importConfig),
    };
}
