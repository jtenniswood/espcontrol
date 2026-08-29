import { state } from "../state/app_instance";
import { NTP_SERVER_DEFAULTS } from "../state/app_state";
import { WEB_UI_COLORS } from "../state/ui_tokens";
import {
    normalizeBrightnessMode,
    normalizeLanguage,
    normalizeTemperatureUnit,
    normalizeTimeOfDay,
} from "../model/settings";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { CoreFeature } from "./core";
import type { ApplicationLayoutState } from "./application_context";
import { appendLanguageOption, languageOptionsWithFallback } from "./language_state";
import { hasCustomNtpServers, resetNtpServersToDefaults, syncNtpServerUi } from "./ntp_state";
import { syncIdleUi } from "./idle_state";
import { getActiveScreensaverMode } from "./screensaver_state";
import type { EnvironmentStateFeature } from "./environment_state";
import type { ScreenScheduleStateFeature } from "./screen_schedule_state";
import type { ScreensaverTimeoutFeature } from "./screensaver_timeout";
import type { ScreenRotationFeature } from "./screen_rotation_state";
import type { AppearanceFeature } from "./appearance_state";
import type { ClockBarFeature } from "./clock_bar_state";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { ApplicationApiFeature } from "./api";
import type { AppStatusPreviewFeature } from "./app_status_preview";
import type { ArtworkPostApiFeature } from "./artwork_post_api";
import type { ScreenSchedulePostApiFeature } from "./screen_schedule_post_api";
import type { ClockBarPostApiFeature } from "./clock_bar_post_api";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { SettingsPageHelpersFeature } from "./settings_page_helpers";
import type { SettingsScheduleSectionFeature } from "./settings_schedule_section";
import type { SettingsCoverArtSectionFeature } from "./settings_cover_art_section";
import type { SettingsSystemSectionFeature } from "./settings_system_section";
import type { PreviewRenderFeature } from "./preview_render";

export interface SettingsPageFeature {
    buildSettingsPage(...args: any[]): any;
}

export function createSettingsPageFeature(codec: Pick<ConfigCodecFeature, "bindTextPost">, runtime: UiRuntimeState, core: Pick<CoreFeature, "syncPreviewOrientation">, layout: ApplicationLayoutState, environment: EnvironmentStateFeature, schedule: ScreenScheduleStateFeature, screensaverTimeout: ScreensaverTimeoutFeature, screenRotation: ScreenRotationFeature, appearance: AppearanceFeature, clockBar: ClockBarFeature, entityState: Pick<EntityStateFeature, "entityName" | "entityInput">, shell: Pick<ControlsShellFeature, "createActionButton" | "buildApplyBar">, requestApi: Pick<ApplicationApiFeature, "postText" | "postSelect" | "postScreensaverMode" | "postScreensaverTimeout" | "postHomeScreenTimeout">, statusPreview: Pick<AppStatusPreviewFeature, "appendTimezoneOption" | "syncInput" | "updateClock" | "updateSunInfo" | "updateTempPreview">, artworkPostApi: Pick<ArtworkPostApiFeature, "postPresenceSensorEntity">, schedulePostApi: Pick<ScreenSchedulePostApiFeature, "postBrightnessMode" | "postDisplayBacklightBrightness" | "postBrightnessDawnTime" | "postBrightnessDuskTime">, clockBarPostApi: Pick<ClockBarPostApiFeature, "postClockBar" | "postClockBarNightMode" | "postBatteryStatus" | "postVoiceServices">, fields: Pick<ControlsFieldsFeature, "colorField" | "condField" | "createRangeSlider" | "fieldLabel" | "makeCollapsibleCard" | "segmentControl" | "selectField" | "textInput" | "toggleRow">, helpers: Pick<SettingsPageHelpersFeature, "appendSettingsSection" | "buildAlarmDelayAudioSettingsCard" | "createScreensaverThenControls" | "createTimeInput" | "statusBadge" | "syncClockScreensaverControls" | "syncCoverArtScreensaverUi" | "syncMediaPlayerSleepPreventionUi">, scheduleSection: SettingsScheduleSectionFeature, coverArtSection: SettingsCoverArtSectionFeature, systemSection: SettingsSystemSectionFeature, preview: Pick<PreviewRenderFeature, "render">): SettingsPageFeature {
    const { render: renderPreview } = preview;
    const { appendSettingsSection, buildAlarmDelayAudioSettingsCard, createScreensaverThenControls, createTimeInput, statusBadge, syncClockScreensaverControls, syncCoverArtScreensaverUi, syncMediaPlayerSleepPreventionUi } = helpers;
    const { buildScreenScheduleSettingsCard } = scheduleSection;
    const { buildCoverArtSettingsCard } = coverArtSection;
    const { buildSystemSettingsCards } = systemSection;
    const { colorField, condField, createRangeSlider, fieldLabel, makeCollapsibleCard, segmentControl, selectField, textInput, toggleRow } = fields;
    const { createActionButton, buildApplyBar } = shell;
    const { entityName, entityInput } = entityState;
    const { postText, postSelect, postScreensaverMode, postScreensaverTimeout, postHomeScreenTimeout } = requestApi;
    const { bindTextPost } = codec;
    const { appendTimezoneOption, syncInput, updateClock, updateSunInfo, updateTempPreview } = statusPreview;
    const { syncPreviewOrientation } = core;
    const { postPresenceSensorEntity } = artworkPostApi;
    const { postBrightnessMode, postDisplayBacklightBrightness, postBrightnessDawnTime, postBrightnessDuskTime } = schedulePostApi;
    const { postClockBar, postClockBarNightMode, postBatteryStatus, postVoiceServices } = clockBarPostApi;
    const els = runtime.els;
    const { timezoneOptionsWithFallback, voiceServicesUiState, setVoiceServicesEnabled } = environment;
    const { syncUi: syncScreenScheduleUi } = schedule;
    const { syncUi: syncScreensaverTimeoutUi } = screensaverTimeout;
    const { normalize: normalizeScreenRotation, activeOptions: activeScreenRotationOptions, appendOption: appendScreenRotationOption } = screenRotation;
    const { resetColors: resetAppearanceColors } = appearance;
    const {
        controllerState: clockBarControllerState,
        applyControllerState: applyClockBarControllerState,
        setEnabled: setClockBarEnabled,
        setNightModeEnabled,
        syncUi: syncClockBarUi,
        syncTemperatureUi,
    } = clockBar;
    // ── Settings Page ──────────────────────────────────────────────────────
    function buildSettingsPage(this: any, parent?: any) {
        var page: any = document.createElement("div");
        page.id = "sp-settings";
        page.className = "sp-page";
        var config: any = document.createElement("div");
        config.className = "sp-config fade-in";
        var appearBody: any = document.createElement("div");
        var onColor: any = colorField("sp-set-on-color", WEB_UI_COLORS.primary, function (this: any, hex?: any) {
            postText(entityName("button_on_color"), hex);
        });
        appearBody.appendChild(onColor);
        els.setOnColor = onColor;
        var appearanceResetButton: any = createActionButton("sp-icon-button sp-card-header-action", "", "restore", "Reset colours to defaults");
        appearanceResetButton.title = "Reset colours";
        appearanceResetButton.addEventListener("click", function (this: any, event?: any) {
            event.stopPropagation();
            resetAppearanceColors(true);
        });
        var appearanceCard: any = makeCollapsibleCard("Appearance", appearBody, true, null, appearanceResetButton);
        var languageBody: any = document.createElement("div");
        var languageField: any = document.createElement("div");
        languageField.className = "sp-field";
        languageField.appendChild(fieldLabel("Language", "sp-set-language"));
        var languageSelect: any = document.createElement("select");
        languageSelect.className = "sp-select";
        languageSelect.id = "sp-set-language";
        state.languageOptions = languageOptionsWithFallback(state.languageOptions, state.language);
        state.languageOptions.forEach(function (this: any, opt?: any) {
            appendLanguageOption(languageSelect, opt);
        });
        languageSelect.value = normalizeLanguage(state.language);
        languageSelect.addEventListener("change", function (this: any) {
            state.language = normalizeLanguage(this.value);
            postSelect(entityName("screen_language"), state.language);
            renderPreview();
        });
        languageField.appendChild(languageSelect);
        languageBody.appendChild(languageField);
        var languageCard: any = makeCollapsibleCard("Language", languageBody, true);
        els.setLanguage = languageSelect;
        var blBody: any = document.createElement("div");
        var brightnessModeSegment: any = segmentControl([
            ["manual", "Manual"],
            ["sunrise_sunset", "Automatic"],
            ["fixed_times", "Timed"],
        ], normalizeBrightnessMode(state.brightnessMode), function (this: any, mode?: any) {
            state.brightnessMode = normalizeBrightnessMode(mode);
            postBrightnessMode(state.brightnessMode);
            syncScreenScheduleUi();
        }, "sp-segment sp-segment-scroll sp-brightness-mode-segment");
        blBody.appendChild(brightnessModeSegment.segment);
        els.setBrightnessModeButtons = brightnessModeSegment.buttons;
        var brightnessManualField: any = condField();
        var manualSlider: any = createRangeSlider("Brightness", state.manualBrightnessVal, function (this: any, value?: any) {
            state.manualBrightnessVal = parseFloat(value) || 100;
            postDisplayBacklightBrightness(state.manualBrightnessVal);
        });
        brightnessManualField.appendChild(manualSlider.wrap);
        blBody.appendChild(brightnessManualField);
        els.setManualBrightnessField = brightnessManualField;
        els.setManualBrightness = manualSlider.range;
        els.setManualBrightnessVal = manualSlider.val;
        var brightnessAutomaticFields: any = condField();
        var daySlider: any = createRangeSlider("Daytime Brightness", state.brightnessDayVal, entityName("screen_daytime_brightness"));
        brightnessAutomaticFields.appendChild(daySlider.wrap);
        els.setDayBrightness = daySlider.range;
        els.setDayBrightnessVal = daySlider.val;
        var nightSlider: any = createRangeSlider("Nighttime Brightness", state.brightnessNightVal, entityName("screen_nighttime_brightness"));
        brightnessAutomaticFields.appendChild(nightSlider.wrap);
        els.setNightBrightness = nightSlider.range;
        els.setNightBrightnessVal = nightSlider.val;
        var brightnessManualTimes: any = condField();
        var dawnTime: any = createTimeInput("Dawn", "sp-set-brightness-dawn-time", state.brightnessDawnTime, "06:00", function (this: any, value?: any) {
            state.brightnessDawnTime = normalizeTimeOfDay(value, "06:00");
            postBrightnessDawnTime(state.brightnessDawnTime);
            syncScreenScheduleUi();
        });
        brightnessManualTimes.appendChild(dawnTime.wrap);
        els.setBrightnessDawnTime = dawnTime.input;
        var duskTime: any = createTimeInput("Dusk", "sp-set-brightness-dusk-time", state.brightnessDuskTime, "18:00", function (this: any, value?: any) {
            state.brightnessDuskTime = normalizeTimeOfDay(value, "18:00");
            postBrightnessDuskTime(state.brightnessDuskTime);
            syncScreenScheduleUi();
        });
        brightnessManualTimes.appendChild(duskTime.wrap);
        els.setBrightnessDuskTime = duskTime.input;
        brightnessAutomaticFields.appendChild(brightnessManualTimes);
        els.setBrightnessManualTimes = brightnessManualTimes;
        var sunInfo: any = document.createElement("div");
        sunInfo.className = "sp-sun-info";
        sunInfo.id = "sp-sun-info";
        brightnessAutomaticFields.appendChild(sunInfo);
        els.sunInfo = sunInfo;
        blBody.appendChild(brightnessAutomaticFields);
        els.setBrightnessAutomaticFields = brightnessAutomaticFields;
        updateSunInfo();
        var backlightCard: any = makeCollapsibleCard("Backlight", blBody, true);
        var scheduleCard: any = buildScreenScheduleSettingsCard();
        var clockBody: any = document.createElement("div");
        var tzField: any = document.createElement("div");
        tzField.className = "sp-field";
        tzField.appendChild(fieldLabel("Timezone", "sp-set-timezone"));
        var tzSelect: any = document.createElement("select");
        tzSelect.className = "sp-select";
        tzSelect.id = "sp-set-timezone";
        state.timezoneOptions = timezoneOptionsWithFallback(state.timezoneOptions, state.timezone);
        state.timezoneOptions.forEach(function (this: any, opt?: any) {
            appendTimezoneOption(tzSelect, opt);
        });
        tzSelect.value = state.timezone;
        tzSelect.addEventListener("change", function (this: any) {
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
        var cfField: any = document.createElement("div");
        cfField.className = "sp-field";
        cfField.appendChild(fieldLabel("Clock Format", "sp-set-clock-format"));
        var cfSelect: any = document.createElement("select");
        cfSelect.className = "sp-select";
        cfSelect.id = "sp-set-clock-format";
        state.clockFormatOptions.forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt;
            o.textContent = opt === "12h" ? "12-hour" : "24-hour";
            cfSelect.appendChild(o);
        });
        cfSelect.value = state.clockFormat;
        cfSelect.addEventListener("change", function (this: any) {
            postSelect(entityName("screen_clock_format"), this.value);
        });
        cfField.appendChild(cfSelect);
        clockBody.appendChild(cfField);
        els.setClockFormat = cfSelect;
        var ntpField: any = document.createElement("div");
        ntpField.className = "sp-field";
        state.customNtpServers = state.customNtpServers || hasCustomNtpServers();
        var customNtpServers: any = toggleRow("Custom NTP Servers", "sp-set-custom-ntp-servers", state.customNtpServers);
        ntpField.appendChild(customNtpServers.row);
        els.setCustomNtpServersToggle = customNtpServers.input;
        customNtpServers.input.addEventListener("change", function (this: any) {
            state.customNtpServers = this.checked;
            if (!state.customNtpServers) {
                resetNtpServersToDefaults();
                postText(entityName("screen_ntp_server_1"), state.ntpServer1);
                postText(entityName("screen_ntp_server_2"), state.ntpServer2);
                postText(entityName("screen_ntp_server_3"), state.ntpServer3);
            }
            syncNtpServerUi(runtime, syncInput);
        });
        var ntpList: any = document.createElement("div");
        ntpList.className = "sp-field-stack";
        els.setNtpServerFields = ntpList;
        function addNtpServerInput(this: any, id: any, stateKey: "ntpServer1" | "ntpServer2" | "ntpServer3", postName: any, placeholder: any, ariaLabel: any) {
            var input: any = textInput(id, state[stateKey], placeholder);
            input.setAttribute("aria-label", ariaLabel);
            input.addEventListener("blur", function (this: any) {
                var value: any = this.value.trim();
                this.value = value;
                state[stateKey] = value;
                state.customNtpServers = true;
                syncNtpServerUi(runtime, syncInput);
                postText(postName, value);
            });
            input.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key === "Enter")
                    this.blur();
            });
            ntpList.appendChild(input);
            return input;
        }
        els.setNtpServer1 = addNtpServerInput("sp-set-ntp-server-1", "ntpServer1", entityName("screen_ntp_server_1"), NTP_SERVER_DEFAULTS[0], "NTP Server 1");
        els.setNtpServer2 = addNtpServerInput("sp-set-ntp-server-2", "ntpServer2", entityName("screen_ntp_server_2"), NTP_SERVER_DEFAULTS[1], "NTP Server 2");
        els.setNtpServer3 = addNtpServerInput("sp-set-ntp-server-3", "ntpServer3", entityName("screen_ntp_server_3"), NTP_SERVER_DEFAULTS[2], "NTP Server 3");
        ntpField.appendChild(ntpList);
        syncNtpServerUi(runtime, syncInput);
        clockBody.appendChild(ntpField);
        var timeSettingsCard: any = makeCollapsibleCard("Time", clockBody, true);
        var clockBarBody: any = document.createElement("div");
        var clockBar: any = toggleRow("Show Clock Bar", "sp-set-clock-bar", state.clockBarOn);
        clockBarBody.appendChild(clockBar.row);
        els.setClockBarToggle = clockBar.input;
        clockBar.input.addEventListener("change", function (this: any) {
            setClockBarEnabled(this.checked);
            state._clockBarStateValues = { local: state.clockBarOn };
            syncClockBarUi();
            postClockBar(state.clockBarOn);
        });
        var clockBarNightMode: any = toggleRow("Show Night Mode Icon", "sp-set-clock-bar-night-mode", state.clockBarNightModeOn);
        clockBarBody.appendChild(clockBarNightMode.row);
        els.setClockBarNightModeToggle = clockBarNightMode.input;
        clockBarNightMode.input.addEventListener("change", function (this: any) {
            setNightModeEnabled(this.checked);
            syncClockBarUi();
            postClockBarNightMode(state.clockBarNightModeOn);
        });
        var clockBarBadge: any = statusBadge("Clock bar on");
        els.setClockBarBadge = clockBarBadge;
        syncClockBarUi();
        syncTemperatureUi();
        var clockBarCard: any = makeCollapsibleCard("Clock Bar", clockBarBody, true, clockBarBadge);
        var voiceServicesCard: any = null;
        if (voiceServicesUiState().settingsVisible) {
            var voiceServicesBody: any = document.createElement("div");
            var voiceServices: any = toggleRow("Voice Services", "sp-set-voice-services", state.voiceServicesOn);
            voiceServicesBody.appendChild(voiceServices.row);
            els.setVoiceServicesToggle = voiceServices.input;
            voiceServices.input.addEventListener("change", function (this: any) {
                setVoiceServicesEnabled(this.checked);
                syncClockBarUi();
                postVoiceServices(state.voiceServicesOn);
            });
            voiceServicesCard = makeCollapsibleCard("Voice Services", voiceServicesBody, true);
            els.voiceServicesCard = voiceServicesCard;
        }
        var batteryStatusCard: any = null;
        if (layout.config.features && layout.config.features.battery) {
            var batteryStatusBody: any = document.createElement("div");
            var batteryStatus: any = toggleRow("Enable battery support", "sp-set-battery-status", state.batteryStatusOn);
            batteryStatusBody.appendChild(batteryStatus.row);
            els.setBatteryStatusToggle = batteryStatus.input;
            batteryStatus.input.addEventListener("change", function (this: any) {
                state.batteryStatusOn = this.checked;
                syncClockBarUi();
                postBatteryStatus(state.batteryStatusOn);
            });
            var batteryStatusBadge: any = statusBadge("Battery icon on");
            els.setBatteryStatusBadge = batteryStatusBadge;
            syncClockBarUi();
            batteryStatusCard = makeCollapsibleCard("Battery", batteryStatusBody, true, batteryStatusBadge);
            els.batteryStatusCard = batteryStatusCard;
        }
        var alarmDelayAudioCard: any = buildAlarmDelayAudioSettingsCard();
        var rotationCard: any = null;
        if (layout.config.features && layout.config.features.screenRotation) {
            var rotationBody: any = document.createElement("div");
            var rotField: any = document.createElement("div");
            rotField.className = "sp-field";
            rotField.appendChild(fieldLabel("Rotation", "sp-set-screen-rotation"));
            var rotSelect: any = document.createElement("select");
            rotSelect.className = "sp-select";
            rotSelect.id = "sp-set-screen-rotation";
            activeScreenRotationOptions().forEach(function (this: any, opt?: any) {
                appendScreenRotationOption(rotSelect, opt);
            });
            rotSelect.value = state.screenRotation;
            rotSelect.addEventListener("change", function (this: any) {
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
        var tempBody: any = document.createElement("div");
        var unitField: any = document.createElement("div");
        unitField.className = "sp-field";
        unitField.appendChild(fieldLabel("Temperature Unit", "sp-set-temperature-unit"));
        var unitSelect: any = document.createElement("select");
        unitSelect.className = "sp-select";
        unitSelect.id = "sp-set-temperature-unit";
        [
            ["Auto", "Auto (from timezone)"],
            ["\u00B0C", "Centigrade (\u00B0C)"],
            ["\u00B0F", "Fahrenheit (\u00B0F)"],
        ].forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt[0];
            o.textContent = opt[1];
            unitSelect.appendChild(o);
        });
        unitSelect.value = normalizeTemperatureUnit(state.temperatureUnit);
        unitSelect.addEventListener("change", function (this: any) {
            state.temperatureUnit = normalizeTemperatureUnit(this.value);
            postSelect(entityName("screen_temperature_unit"), state.temperatureUnit);
            updateTempPreview();
            renderPreview();
        });
        unitField.appendChild(unitSelect);
        tempBody.appendChild(unitField);
        els.setTemperatureUnit = unitSelect;
        syncTemperatureUi();
        var temperatureCard: any = makeCollapsibleCard("Temperature", tempBody, true);
        var ssBody: any = document.createElement("div");
        var ssMode: any = getActiveScreensaverMode();
        ssBody.appendChild(fieldLabel("Mode"));
        var ssModeSegment: any = segmentControl([
            ["disabled", "Disabled"],
            ["timer", "Timer"],
            ["sensor", "Sensor"],
        ], ssMode, function (this: any, mode?: any) {
            setSsMode(mode);
            state.screensaverMode = mode;
            postScreensaverMode(mode);
        }, "sp-segment sp-screensaver-mode");
        var disabledBtn: any = ssModeSegment.buttons.disabled;
        var timerBtn: any = ssModeSegment.buttons.timer;
        var sensorBtn: any = ssModeSegment.buttons.sensor;
        ssBody.appendChild(ssModeSegment.segment);
        var timerPanel: any = document.createElement("div");
        var timeoutControl: any = selectField("Timeout", "sp-set-ss-timeout", [], state.screensaverTimeout, function (this: any) {
            var n: any = parseFloat(this.value);
            if (isFinite(n))
                state.screensaverTimeout = n;
            postScreensaverTimeout(this.value);
        });
        var timeoutSelect: any = timeoutControl.select;
        timerPanel.appendChild(timeoutControl.field);
        var timerClockControls: any = createScreensaverThenControls("sp-set-clock-mode");
        timerPanel.appendChild(timerClockControls.clockField);
        timerPanel.appendChild(timerClockControls.dimBrightnessField);
        timerPanel.appendChild(timerClockControls.brightnessField);
        els.setClockSelect = timerClockControls.clockSelect;
        els.setClockField = timerClockControls.clockField;
        els.setDimBrightnessField = timerClockControls.dimBrightnessField;
        els.setManualDimBrightnessField = timerClockControls.manualDimBrightnessField;
        els.setAutomaticDimBrightnessField = timerClockControls.automaticDimBrightnessField;
        els.setDimBrightness = timerClockControls.dimBrightness;
        els.setDimBrightnessVal = timerClockControls.dimBrightnessVal;
        els.setDimBrightnessDay = timerClockControls.dimBrightnessDay;
        els.setDimBrightnessDayVal = timerClockControls.dimBrightnessDayVal;
        els.setDimBrightnessNight = timerClockControls.dimBrightnessNight;
        els.setDimBrightnessNightVal = timerClockControls.dimBrightnessNightVal;
        els.setClockBrightnessDay = timerClockControls.clockBrightnessDay;
        els.setClockBrightnessDayVal = timerClockControls.clockBrightnessDayVal;
        els.setClockBrightnessNight = timerClockControls.clockBrightnessNight;
        els.setClockBrightnessNightVal = timerClockControls.clockBrightnessNightVal;
        els.setClockBrightnessField = timerClockControls.brightnessField;
        var coverArtCard: any = buildCoverArtSettingsCard();
        ssBody.appendChild(timerPanel);
        els.setSSTimeout = timeoutSelect;
        syncScreensaverTimeoutUi();
        var sensorPanel: any = document.createElement("div");
        var presenceField: any = document.createElement("div");
        presenceField.className = "sp-field";
        presenceField.appendChild(fieldLabel("Presence Entity", "sp-set-presence"));
        var presInp: any = entityInput("sp-set-presence", state.presenceEntity, "Presence sensor entity", ["binary_sensor", "sensor"]);
        presenceField.appendChild(presInp);
        sensorPanel.appendChild(presenceField);
        bindTextPost(presInp, entityName("presence_sensor_entity"), {
            post: postPresenceSensorEntity,
        });
        var sensorClockControls: any = createScreensaverThenControls("sp-set-sensor-clock-mode");
        sensorPanel.appendChild(sensorClockControls.clockField);
        sensorPanel.appendChild(sensorClockControls.dimBrightnessField);
        sensorPanel.appendChild(sensorClockControls.brightnessField);
        ssBody.appendChild(sensorPanel);
        els.setPresence = presInp;
        els.setSensorClockSelect = sensorClockControls.clockSelect;
        els.setSensorClockField = sensorClockControls.clockField;
        els.setSensorDimBrightnessField = sensorClockControls.dimBrightnessField;
        els.setSensorManualDimBrightnessField = sensorClockControls.manualDimBrightnessField;
        els.setSensorAutomaticDimBrightnessField = sensorClockControls.automaticDimBrightnessField;
        els.setSensorDimBrightness = sensorClockControls.dimBrightness;
        els.setSensorDimBrightnessVal = sensorClockControls.dimBrightnessVal;
        els.setSensorDimBrightnessDay = sensorClockControls.dimBrightnessDay;
        els.setSensorDimBrightnessDayVal = sensorClockControls.dimBrightnessDayVal;
        els.setSensorDimBrightnessNight = sensorClockControls.dimBrightnessNight;
        els.setSensorDimBrightnessNightVal = sensorClockControls.dimBrightnessNightVal;
        els.setSensorClockBrightnessDay = sensorClockControls.clockBrightnessDay;
        els.setSensorClockBrightnessDayVal = sensorClockControls.clockBrightnessDayVal;
        els.setSensorClockBrightnessNight = sensorClockControls.clockBrightnessNight;
        els.setSensorClockBrightnessNightVal = sensorClockControls.clockBrightnessNightVal;
        els.setSensorClockBrightnessField = sensorClockControls.brightnessField;
        syncClockScreensaverControls();
        syncMediaPlayerSleepPreventionUi();
        syncCoverArtScreensaverUi();
        var ssBadge: any = statusBadge("Screensaver on");
        els.setScreensaverBadge = ssBadge;
        function setSsMode(this: any, mode?: any) {
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
        var screensaverCard: any = makeCollapsibleCard("Screensaver", ssBody, true, ssBadge);
        var idleBody: any = document.createElement("div");
        idleBody.appendChild(fieldLabel("Return Home After"));
        var hsSelect: any = document.createElement("select");
        hsSelect.className = "sp-select";
        hsSelect.id = "sp-set-hs-timeout";
        var hsOptions: any = [
            { label: "Disabled", value: 0 },
            { label: "10 seconds", value: 10 },
            { label: "20 seconds", value: 20 },
            { label: "30 seconds", value: 30 },
            { label: "1 minute", value: 60 },
            { label: "2 minutes", value: 120 },
            { label: "5 minutes", value: 300 },
        ];
        hsOptions.forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt.value;
            o.textContent = opt.label;
            if (opt.value === state.homeScreenTimeout)
                o.selected = true;
            hsSelect.appendChild(o);
        });
        hsSelect.addEventListener("change", function (this: any) {
            state.homeScreenTimeout = parseFloat(this.value) || 0;
            syncIdleUi(runtime);
            postHomeScreenTimeout(this.value);
        });
        idleBody.appendChild(hsSelect);
        els.setHSTimeout = hsSelect;
        var idleBadge: any = statusBadge("Idle on");
        els.setIdleBadge = idleBadge;
        syncIdleUi(runtime);
        var idleCard: any = makeCollapsibleCard("Idle", idleBody, true, idleBadge);
        var systemSettingsCards: any = buildSystemSettingsCards();
        appendSettingsSection(config, "Display", [
            appearanceCard,
            backlightCard,
            idleCard,
            clockBarCard,
            rotationCard,
        ]);
        appendSettingsSection(config, "Voice & Sounds", [
            voiceServicesCard,
            alarmDelayAudioCard,
        ]);
        appendSettingsSection(config, "Sleep & Schedule", [
            coverArtCard,
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
            systemSettingsCards.homeAssistantSettingsCard,
            batteryStatusCard,
        ]);
        page.appendChild(config);
        page.appendChild(buildApplyBar());
        parent.appendChild(page);
        els.settingsPage = page;
    }
    return {
        buildSettingsPage,
    };
}
