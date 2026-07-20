import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppStateEventHandlersModule(): GlobalDescriptors {
    // ── State Event Handlers ──────────────────────────────────────────
    function createSseHandlers(this: any) {
        return {
            "text-button_order": function (this: any, val?: any) {
                if (gridPreviewBlockedByRotationStartup()) {
                    orderReceived = !!(val && val.trim());
                    state.pendingButtonOrderRaw = val;
                    return;
                }
                applyButtonOrderValue(val);
            },
            "text-button_on_color": function (this: any, val?: any) {
                state.onColor = val;
                syncColorUi();
                renderPreview();
            },
            "text-button_off_color": function (this: any) { },
            "text-sensor_card_color": function (this: any) { },
            "switch-indoor_temp_enable": function (this: any, val?: any, d?: any) {
                state._clockBarTemperatureVisibilityReceived = true;
                state._indoorOn = d.value === true || val === "ON";
                syncTemperatureUi();
                updateTempPreview();
                updateClockBarItemUi();
            },
            "switch-outdoor_temp_enable": function (this: any, val?: any, d?: any) {
                state._clockBarTemperatureVisibilityReceived = true;
                state._outdoorOn = d.value === true || val === "ON";
                syncTemperatureUi();
                updateTempPreview();
                updateClockBarItemUi();
            },
            "switch-screen__clock_bar": function (this: any, val?: any, d?: any, key?: any) {
                if (applyClockBarStateValue(state, val, d, key))
                    syncClockBarUi();
            },
            "text-clock_bar_temperature_entities": function (this: any, val?: any) {
                applyClockBarTemperatureEntities(normalizeClockBarTemperatureEntities(val), false);
            },
            "switch-screen__clock_bar_time": function (this: any, val?: any, d?: any) {
                state.clockBarTimeOn = d.value === true || val === "ON";
                syncClockBarUi();
            },
            "switch-screen__network_status_icon": function (this: any, val?: any, d?: any) {
                state.networkStatusOn = d.value === true || val === "ON";
                syncClockBarUi();
            },
            "switch-screen__battery_status": function (this: any, val?: any, d?: any) {
                state.batteryStatusOn = d.value === true || val === "ON";
                syncClockBarUi();
            },
            "number-agenda_view__days_ahead": function (this: any, val?: any) {
                var n: any = parseInt(val, 10);
                state.agendaViewDays = Number.isFinite(n) && n >= 1 ? n : 30;
                syncInput(els.setAgendaViewDays, String(state.agendaViewDays));
            },
            "switch-agenda_view__hide_empty_days": function (this: any, val?: any, d?: any) {
                state.agendaViewHideEmpty = d.value === true || val === "ON";
                if (els.setAgendaViewHideEmpty)
                    els.setAgendaViewHideEmpty.checked = state.agendaViewHideEmpty;
            },
            "switch-voice_services": function (this: any, val?: any, d?: any) {
                state.voiceServicesOn = d.value === true || val === "ON";
                syncClockBarUi();
            },
            "switch-alarm_delay__audio": function (this: any, val?: any, d?: any) {
                state.alarmDelayAudioOn = d.value === true || val === "ON";
                syncAlarmDelayAudioUi();
            },
            "switch-alarm_delay__tts": function (this: any, val?: any, d?: any) {
                state.alarmDelayTtsOn = d.value === true || val === "ON";
                syncAlarmDelayAudioUi();
            },
            "text-alarm_delay__entry_announcement": function (this: any, val?: any) {
                state.alarmDelayEntryAnnouncement = normalizeAlarmDelayAnnouncement(val, DEFAULT_ALARM_DELAY_ENTRY_ANNOUNCEMENT);
                syncAlarmDelayAudioUi();
            },
            "text-alarm_delay__exit_announcement": function (this: any, val?: any) {
                state.alarmDelayExitAnnouncement = normalizeAlarmDelayAnnouncement(val, DEFAULT_ALARM_DELAY_EXIT_ANNOUNCEMENT);
                syncAlarmDelayAudioUi();
            },
            "number-alarm_delay__beep_volume": function (this: any, val?: any) {
                state.alarmDelayBeepVolume = normalizeAlarmDelayBeepVolume(val);
                syncAlarmDelayAudioUi();
            },
            "number-alarm_delay__final_countdown": function (this: any, val?: any) {
                state.alarmDelayFinalCountdown = normalizeAlarmDelayFinalCountdown(val);
                syncAlarmDelayAudioUi();
            },
            "switch-screen__temperature_degree_symbol": function (this: any, val?: any, d?: any) {
                state.temperatureDegreeSymbolOn = d.value === true || val === "ON";
                syncClockBarUi();
            },
            "switch-screen__subpage_chevron": function (this: any, val?: any, d?: any) {
                state.subpageChevronsOn = d.value === true || val === "ON";
                syncClockBarUi();
                renderPreview();
            },
            "text-indoor_temp_entity": function (this: any, val?: any) {
                state.indoorEntity = val;
                syncInput(els.setIndoorEntity, val);
                if (!state._clockBarTemperatureEntitiesReceived) {
                    syncTemperatureUi();
                    updateTempPreview();
                    updateClockBarItemUi();
                }
            },
            "text-outdoor_temp_entity": function (this: any, val?: any) {
                state.outdoorEntity = val;
                syncInput(els.setOutdoorEntity, val);
                if (!state._clockBarTemperatureEntitiesReceived) {
                    syncTemperatureUi();
                    updateTempPreview();
                    updateClockBarItemUi();
                }
            },
            "select-screen__temperature_unit": function (this: any, val?: any, d?: any) {
                state.temperatureUnit = normalizeTemperatureUnit(d.value || val);
                if (els.setTemperatureUnit)
                    els.setTemperatureUnit.value = state.temperatureUnit;
                updateTempPreview();
                renderPreview();
            },
            "number-screensaver_timeout": function (this: any, val?: any, d?: any) {
                applyScreensaverTimeoutState(d);
            },
            "number-home_screen_timeout": function (this: any, val?: any) {
                state.homeScreenTimeout = parseFloat(val) || 0;
                syncIdleUi();
            },
            "switch-screen_saver__clock": function (this: any, val?: any, d?: any) {
                state.clockScreensaverOn = d.value === true || val === "ON";
                if (!state._screensaverActionReceived) {
                    state.screensaverAction = state.clockScreensaverOn ? "clock" : "off";
                }
                syncClockScreensaverControls();
            },
            "switch-screen_saver__media_player_sleep_prevention": function (this: any, val?: any, d?: any) {
                state.mediaPlayerSleepPreventionOn = d.value === true || val === "ON";
                syncMediaPlayerSleepPreventionUi();
            },
            "switch-screen_saver__cover_art": function (this: any, val?: any, d?: any) {
                state.coverArtScreensaverOn = d.value === true || val === "ON";
                syncCoverArtScreensaverUi();
            },
            "switch-screen_saver__hide_cover_art_on_external_input": function (this: any, val?: any, d?: any) {
                state.coverArtHideExternalInputOn = d.value === true || val === "ON";
                syncCoverArtScreensaverUi();
            },
            "number-screen_saver__clock_brightness": function (this: any, val?: any) {
                if (state.clockBrightnessSplitReceived)
                    return;
                var brightness: any = normalizeClockBrightness(val, 35);
                state.clockBrightnessDay = brightness;
                state.clockBrightnessNight = brightness;
                syncClockScreensaverControls();
            },
            "number-screen_saver__daytime_clock_brightness": function (this: any, val?: any) {
                state.clockBrightnessSplitReceived = true;
                state.clockBrightnessDay = normalizeClockBrightness(val, 35);
                syncClockScreensaverControls();
            },
            "number-screen_saver__nighttime_clock_brightness": function (this: any, val?: any) {
                state.clockBrightnessSplitReceived = true;
                state.clockBrightnessNight = normalizeClockBrightness(val, state.clockBrightnessDay);
                syncClockScreensaverControls();
            },
            "select-screen_saver__action": function (this: any, val?: any, d?: any) {
                state._screensaverActionReceived = true;
                state.screensaverAction = normalizeScreensaverAction(d.value || val);
                state.clockScreensaverOn = state.screensaverAction === "clock";
                syncClockScreensaverControls();
            },
            "number-screen_saver__dimmed_brightness": function (this: any, val?: any) {
                state.screensaverDimmedBrightness = normalizeScreensaverDimmedBrightness(val);
                syncClockScreensaverControls();
            },
            "switch-screen_saver__photos_show_agenda": function (this: any, val?: any, d?: any) {
                state.photosShowAgenda = d.value === true || val === "ON";
                if (els.setPhotosShowAgenda) els.setPhotosShowAgenda.checked = state.photosShowAgenda;
                if (els.syncPhotoAgendaFields) els.syncPhotoAgendaFields();
            },
            "text-screen_saver__photos_agenda_entities": function (this: any, val?: any) {
                state.photosAgendaEntities = val || "";
                if (els.setPhotosAgendaCalendars)
                    els.setPhotosAgendaCalendars.setValue(state.photosAgendaEntities);
            },
            "select-screen_saver__photos_agenda_style": function (this: any, val?: any) {
                state.photosAgendaStyle = val || "Next Event";
                syncInput(els.setPhotosAgendaStyle, state.photosAgendaStyle);
            },
            "number-screen_saver__photos_agenda_opacity": function (this: any, val?: any) {
                var n: any = parseFloat(val);
                state.photosAgendaOpacity = Number.isFinite(n) ? n : 45;
                syncInput(els.setPhotosAgendaOpacity, String(state.photosAgendaOpacity));
            },
            "number-screen_saver__photos_agenda_limit": function (this: any, val?: any) {
                var n: any = parseInt(val, 10);
                state.photosAgendaLimit = Number.isFinite(n) && n >= 1 ? n : 5;
                syncInput(els.setPhotosAgendaLimit, String(state.photosAgendaLimit));
            },
            "switch-screen_saver__photos_agenda_hide_empty": function (this: any, val?: any, d?: any) {
                state.photosAgendaHideEmpty = d.value === true || val === "ON";
                if (els.setPhotosAgendaHideEmpty)
                    els.setPhotosAgendaHideEmpty.checked = state.photosAgendaHideEmpty;
            },
            "number-screen_saver__photos_agenda_days": function (this: any, val?: any) {
                var n: any = parseInt(val, 10);
                state.photosAgendaDays = Number.isFinite(n) && n >= 1 ? n : 14;
                syncInput(els.setPhotosAgendaDays, String(state.photosAgendaDays));
            },
            "text-screen_saver__photos_folder": function (this: any, val?: any) {
                state.photosFolder = val || "";
                syncInput(els.setPhotosFolder, state.photosFolder);
            },
            "number-screen_saver__photo_duration": function (this: any, val?: any) {
                var n: any = parseInt(val, 10);
                state.photosInterval = Number.isFinite(n) && n >= 5 ? n : 30;
                syncInput(els.setPhotosInterval, String(state.photosInterval));
            },
            "switch-screen_saver__shuffle_photos": function (this: any, val?: any, d?: any) {
                state.photosShuffle = d.value === true || val === "ON";
                if (els.setPhotosShuffle) els.setPhotosShuffle.checked = state.photosShuffle;
            },
            "switch-screen_saver__photos_show_clock": function (this: any, val?: any, d?: any) {
                state.photosShowDatetime = d.value === true || val === "ON";
                if (els.setPhotosShowDatetime) els.setPhotosShowDatetime.checked = state.photosShowDatetime;
            },
            "switch-screen_saver__photos_show_date": function (this: any, val?: any, d?: any) {
                state.photosShowDate = d.value === true || val === "ON";
                if (els.setPhotosShowDate) els.setPhotosShowDate.checked = state.photosShowDate;
            },
            "switch-screen_saver__photos_show_weather": function (this: any, val?: any, d?: any) {
                state.photosShowWeather = d.value === true || val === "ON";
                if (els.setPhotosShowWeather) els.setPhotosShowWeather.checked = state.photosShowWeather;
                syncClockScreensaverControls();
            },
            "text-screen_saver__photos_weather_entity": function (this: any, val?: any) {
                state.photosWeatherEntity = val || "";
                syncInput(els.setPhotosWeatherEntity, state.photosWeatherEntity);
            },
            "text-presence_sensor_entity": function (this: any, val?: any) {
                state.presenceEntity = val;
                syncInput(els.setPresence, val);
                syncInput(els.setSchedulePresence, val);
                if (state.screensaverMode === "") {
                    if (els.setSsMode)
                        els.setSsMode(getActiveScreensaverMode());
                }
            },
            "text-media_player_sleep_prevention_entity": function (this: any, val?: any) {
                state.mediaPlayerSleepPreventionEntity = val;
                if (!state.coverArtMediaPlayerEntity) {
                    state.coverArtMediaPlayerEntity = val;
                    syncInput(els.setCoverArtMediaPlayer, val);
                }
            },
            "text-screen_saver__cover_art_entity": function (this: any, val?: any) {
                state.coverArtMediaPlayerEntity = val;
                if (!state.mediaPlayerSleepPreventionEntity)
                    state.mediaPlayerSleepPreventionEntity = val;
                syncInput(els.setCoverArtMediaPlayer, val);
            },
            "text-screen_saver__external_source_media_entity": function (this: any, val?: any) {
                state.coverArtSecondaryMediaPlayerEntity = val;
                syncInput(els.setCoverArtSecondaryMediaPlayer, val);
            },
            "text-screen_saver__cover_art_conditions": function (this: any, val?: any) {
                state.coverArtAttributeConditions = val;
                syncInput(els.setCoverArtConditions, val);
            },
            "number-screen_saver__cover_art_delay": function (this: any, val?: any) {
                state.coverArtDelay = normalizeCoverArtDelay(val);
                syncCoverArtScreensaverUi();
            },
            "number-screen_saver__track_overlay_duration": function (this: any, val?: any) {
                state.coverArtTrackOverlayDuration = parseFloat(val) || 0;
                syncCoverArtScreensaverUi();
            },
            "select-home_assistant_artwork_protocol": function (this: any, val?: any, d?: any) {
                state.homeAssistantArtworkProtocol = normalizeHomeAssistantArtworkProtocol(d.value || val);
                syncCoverArtScreensaverUi();
            },
            "number-home_assistant_artwork_port": function (this: any, val?: any) {
                state.coverArtHomeAssistantPort = normalizeHomeAssistantArtworkPort(val);
                syncCoverArtScreensaverUi();
            },
            "text-screensaver_mode": function (this: any, val?: any) {
                state._screensaverModeReceived = true;
                state.screensaverMode = val === "sensor" || val === "timer" || val === "disabled" ? val : "disabled";
                if (els.setSsMode)
                    els.setSsMode(getActiveScreensaverMode());
            },
            "number-screen__daytime_brightness": function (this: any, val?: any) {
                state.brightnessDayVal = parseFloat(val) || 100;
                if (els.setDayBrightness) {
                    els.setDayBrightness.value = state.brightnessDayVal;
                    els.setDayBrightnessVal.textContent = Math.round(state.brightnessDayVal) + "%";
                }
            },
            "number-screen__nighttime_brightness": function (this: any, val?: any) {
                state.brightnessNightVal = parseFloat(val) || 75;
                if (els.setNightBrightness) {
                    els.setNightBrightness.value = state.brightnessNightVal;
                    els.setNightBrightnessVal.textContent = Math.round(state.brightnessNightVal) + "%";
                }
            },
            "switch-screen__automatic_brightness": function (this: any, val?: any, d?: any) {
                state.automaticBrightnessEnabled = d.value === true || val === "ON";
                syncScreenScheduleUi();
            },
            "text-screen__brightness_dawn_time": function (this: any, val?: any) {
                state.brightnessDawnTime = normalizeTimeOfDay(val, "06:00");
                syncScreenScheduleUi();
            },
            "text-screen__brightness_dusk_time": function (this: any, val?: any) {
                state.brightnessDuskTime = normalizeTimeOfDay(val, "18:00");
                syncScreenScheduleUi();
            },
            "switch-screen__schedule_enabled": function (this: any, val?: any, d?: any) {
                state.scheduleEnabled = d.value === true || val === "ON";
                if (!state._scheduleTriggerReceived) {
                    state.scheduleTrigger = state.scheduleEnabled ? "time" : "disabled";
                }
                syncScreenScheduleUi();
            },
            "text-screen__schedule_trigger": function (this: any, val?: any, d?: any) {
                state._scheduleTriggerReceived = true;
                state.scheduleTrigger = normalizeScheduleTrigger(d.value || val, state.scheduleEnabled);
                state.scheduleEnabled = state.scheduleTrigger !== "disabled";
                syncScreenScheduleUi();
            },
            "select-screen__schedule_sensor_activation": function (this: any, val?: any, d?: any) {
                state.scheduleSensorActivation = normalizeScheduleSensorActivation(d.value || val);
                syncScreenScheduleUi();
            },
            "number-screen__schedule_on_hour": function (this: any, val?: any) {
                state.scheduleOnHour = normalizeHour(val, 6);
                syncScreenScheduleUi();
            },
            "number-screen__schedule_off_hour": function (this: any, val?: any) {
                state.scheduleOffHour = normalizeHour(val, 23);
                syncScreenScheduleUi();
            },
            "select-screen__schedule_mode": function (this: any, val?: any, d?: any) {
                state.scheduleMode = normalizeScheduleMode(d.value || val);
                syncScreenScheduleUi();
            },
            "number-screen__schedule_wake_timeout": function (this: any, val?: any) {
                state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(val);
                syncScreenScheduleUi();
            },
            "number-screen__schedule_wake_brightness": function (this: any, val?: any) {
                state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(val);
                syncScreenScheduleUi();
            },
            "number-screen__schedule_dimmed_brightness": function (this: any, val?: any) {
                state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(val);
                syncScreenScheduleUi();
            },
            "number-screen__schedule_clock_brightness": function (this: any, val?: any) {
                state.scheduleClockBrightness = normalizeScheduleClockBrightness(val);
                syncScreenScheduleUi();
            },
            "text-screen__schedule_clock_text_color": function (this: any, val?: any) {
                state.scheduleClockTextColor = normalizeHexColor(val, "FFFFFF");
                syncScreenScheduleUi();
            },
            "select-screen__timezone": function (this: any, val?: any, d?: any) {
                state.timezone = d.value || val || state.timezone;
                if (Array.isArray(d.option)) {
                    state.timezoneOptions = timezoneOptionsWithFallback(d.option, state.timezone, true);
                    if (els.setTimezone) {
                        els.setTimezone.innerHTML = "";
                        state.timezoneOptions.forEach(function (this: any, opt?: any) {
                            appendTimezoneOption(els.setTimezone, opt);
                        });
                    }
                }
                if (els.setTimezone)
                    els.setTimezone.value = state.timezone;
                if (normalizeTemperatureUnit(state.temperatureUnit) === "Auto") {
                    updateTempPreview();
                    renderPreview();
                }
                updateClock();
            },
            "text_sensor-screen__active_timezone": function (this: any, val?: any, d?: any) {
                state.activeTimezone = d.value || val || FALLBACK_TIMEZONE_OPTION;
                if (isHomeAssistantAutoTimezone(state.timezone)) {
                    if (normalizeTemperatureUnit(state.temperatureUnit) === "Auto")
                        updateTempPreview();
                    renderPreview();
                    updateClock();
                }
            },
            "select-screen__language": function (this: any, val?: any, d?: any) {
                state.language = normalizeLanguage(d.value || val || state.language);
                if (d.option && Array.isArray(d.option)) {
                    state.languageOptions = languageOptionsWithFallback(d.option, state.language);
                }
                syncLanguageSelect();
                renderPreview();
            },
            "select-screen__clock_format": function (this: any, val?: any, d?: any) {
                state.clockFormat = d.value || val || state.clockFormat;
                if (d.option && Array.isArray(d.option)) {
                    state.clockFormatOptions = d.option;
                    if (els.setClockFormat) {
                        els.setClockFormat.innerHTML = "";
                        d.option.forEach(function (this: any, opt?: any) {
                            var o: any = document.createElement("option");
                            o.value = opt;
                            o.textContent = opt === "12h" ? "12-hour" : "24-hour";
                            els.setClockFormat.appendChild(o);
                        });
                    }
                }
                if (els.setClockFormat)
                    els.setClockFormat.value = state.clockFormat;
                updateClock();
            },
            "text-screen__ntp_server_1": function (this: any, val?: any) {
                state.ntpServer1 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[0]);
                state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
                syncNtpServerUi();
            },
            "text-screen__ntp_server_2": function (this: any, val?: any) {
                state.ntpServer2 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[1]);
                state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
                syncNtpServerUi();
            },
            "text-screen__ntp_server_3": function (this: any, val?: any) {
                state.ntpServer3 = normalizeNtpServer(val, NTP_SERVER_DEFAULTS[2]);
                state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
                syncNtpServerUi();
            },
            "select-screen__rotation": function (this: any, val?: any, d?: any) {
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
            "text_sensor-screen__sunrise": function (this: any, val?: any) {
                state.sunrise = val;
                updateSunInfo();
            },
            "text_sensor-screen__sunset": function (this: any, val?: any) {
                state.sunset = val;
                updateSunInfo();
            },
            "text_sensor-network_transport": function (this: any, val?: any) {
                state.networkTransport = normalizeNetworkTransport(val);
                updateNetworkPreview();
                syncFirmwareUpdateUi();
            },
            "sensor-wifi_strength": function (this: any, val?: any) {
                state.networkTransport = "wifi";
                state.wifiStrengthPercent = normalizeWifiStrengthPercent(val);
                updateNetworkPreview();
                syncFirmwareUpdateUi();
            },
            "text_sensor-firmware__version": function (this: any, val?: any) {
                setFirmwareVersion(val);
            },
            "text_sensor-firmware_version": function (this: any, val?: any) {
                setFirmwareVersion(val);
            },
            "update-firmware__update": function (this: any, val?: any, d?: any) {
                setFirmwareUpdateInfo(d);
            },
            "switch-firmware__auto_update": function (this: any, val?: any, d?: any) {
                state.firmwareUpdateControlsSupported = true;
                state.autoUpdate = d.value === true || val === "ON";
                if (els.setAutoUpdate)
                    els.setAutoUpdate.checked = state.autoUpdate;
                syncFirmwareUpdateUi();
            },
            "select-firmware__update_frequency": function (this: any, val?: any, d?: any) {
                state.firmwareUpdateControlsSupported = true;
                state.updateFrequency = d.value || val || state.updateFrequency;
                if (els.setUpdateFreq)
                    els.setUpdateFreq.value = state.updateFrequency;
                if (d.option && Array.isArray(d.option)) {
                    state.updateFreqOptions = d.option;
                }
                syncFirmwareUpdateUi();
            },
            "switch-wifi_firmware__auto_update": function (this: any, val?: any, d?: any) {
                state.c6FirmwareUpdateControlsSupported = true;
                state.c6FirmwareAutoUpdateSupported = true;
                state.c6FirmwareAutoUpdate = d.value === true || val === "ON";
                syncC6FirmwareUi();
            },
            "text_sensor-esp32_c6__current_firmware": function (this: any, val?: any) {
                setC6FirmwareCurrentVersion(val);
            },
            "text_sensor-c6_update_current_firmware": function (this: any, val?: any) {
                setC6FirmwareCurrentVersion(val);
            },
            "text_sensor-esp32_c6__latest_firmware": function (this: any, val?: any) {
                setC6FirmwareLatestVersion(val);
            },
            "text_sensor-c6_update_latest_firmware": function (this: any, val?: any) {
                setC6FirmwareLatestVersion(val);
            },
            "text_sensor-esp32_c6__update_available": function (this: any, val?: any) {
                setC6FirmwareUpdateAvailable(val);
            },
            "text_sensor-c6_update_available": function (this: any, val?: any) {
                setC6FirmwareUpdateAvailable(val);
            },
            "button-firmware_esp32_c6__install_update": function (this: any) {
                state.c6FirmwareUpdateControlsSupported = true;
                state.c6FirmwareInstallControlsSupported = true;
                syncC6FirmwareUi();
            },
            "button-firmware_esp32_c6__check_for_update": function (this: any) {
                state.c6FirmwareUpdateControlsSupported = true;
                syncC6FirmwareUi();
            },
        };
    }
    return {
        "createSseHandlers": staticGlobal(createSseHandlers),
    };
}
