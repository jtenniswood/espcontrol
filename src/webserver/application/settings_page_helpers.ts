import { state } from "../state/app_instance";
import {
    DEFAULT_ALARM_DELAY_ENTRY_ANNOUNCEMENT,
    DEFAULT_ALARM_DELAY_EXIT_ANNOUNCEMENT,
    normalizeBrightnessMode,
    normalizeHomeAssistantArtworkPort,
    normalizeHomeAssistantArtworkProtocol,
    normalizeHour,
    normalizeTimeOfDay,
} from "../model/settings";
import { setSelectValue } from "./ui_primitives";
import { timedSettingLabel, type SettingsUiFeature } from "../features/settings";
import type { AlarmDelayAudioController } from "../features/alarm_delay_audio_controller";
import type { ScreensaverController } from "../features/screensaver_controller";
import type { CoverArtScreensaverController } from "../features/cover_art_screensaver_controller";
import type { MediaPlaybackController } from "../features/media_playback_controller";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { ApplicationLayoutState } from "./application_context";
import type { ScreenScheduleStateFeature } from "./screen_schedule_state";
import type { ClockBarFeature } from "./clock_bar_state";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { ApplicationApiFeature } from "./api";
import type { AppStatusPreviewFeature } from "./app_status_preview";
import type { ClockBarPostApiFeature } from "./clock_bar_post_api";
import type { ControlsFieldsFeature } from "./controls_fields";

export interface SettingsPageHelpersControllers {
    readonly settingsUiFeature: SettingsUiFeature;
    readonly alarmDelayAudio: AlarmDelayAudioController;
    readonly screensaver: ScreensaverController;
    readonly coverArtScreensaver: CoverArtScreensaverController;
    readonly mediaPlayback: MediaPlaybackController;
    readonly codec: Pick<ConfigCodecFeature, "bindTextPost">;
    readonly runtime: UiRuntimeState;
    readonly layout: ApplicationLayoutState;
    readonly screenScheduleState: ScreenScheduleStateFeature;
    readonly clockBar: Pick<ClockBarFeature, "syncUi">;
    readonly entityState: Pick<EntityStateFeature, "entityInput">;
    readonly shell: Pick<ControlsShellFeature, "isConfigLocked" | "switchTab">;
    readonly requestApi: Pick<ApplicationApiFeature, "postScreensaverAction" | "postScreensaverDimmedBrightness" | "postScreensaverDimmedBrightnessDay" | "postScreensaverDimmedBrightnessNight" | "postSwitch">;
    readonly statusPreview: Pick<AppStatusPreviewFeature, "syncInput">;
    readonly clockBarPostApi: Pick<ClockBarPostApiFeature, "postClockBrightnessDay" | "postClockBrightnessNight" | "postClockScreensaver" | "postAlarmDelayAudio" | "postAlarmDelayTts" | "postAlarmDelayEntryAnnouncement" | "postAlarmDelayExitAnnouncement" | "postAlarmDelayBeepVolume" | "postAlarmDelayFinalCountdown">;
    readonly fields: Pick<ControlsFieldsFeature, "condField" | "createRangeSlider" | "fieldLabel" | "makeCollapsibleCard" | "toggleRow">;
}

export interface SettingsPageHelpersFeature {
    coverArtScreensaverState(...args: any[]): any;
    applyCoverArtScreensaverState(...args: any[]): any;
    mediaPlaybackState(...args: any[]): any;
    applyMediaPlaybackState(...args: any[]): any;
    settingsStatusHeader(...args: any[]): any;
    appendSettingsSection(...args: any[]): any;
    openVoiceServicesSettings(...args: any[]): any;
    syncAlarmDelayAudioUi(...args: any[]): any;
    buildAlarmDelayAudioSettingsCard(...args: any[]): any;
    coverArtTrackOverlayDurationSupported(...args: any[]): any;
    infoPanel(...args: any[]): any;
    statusBadge(...args: any[]): any;
    disclosureBadge(...args: any[]): any;
    inlineDisclosure(...args: any[]): any;
    syncClockScreensaverControls(...args: any[]): any;
    syncMediaPlayerSleepPreventionUi(...args: any[]): any;
    syncCoverArtScreensaverUi(...args: any[]): any;
    syncOptionalClockBrightness(...args: any[]): any;
    createScreensaverThenControls(...args: any[]): any;
    createHourSelect(...args: any[]): any;
    createTimeInput(...args: any[]): any;
    createEntityToggleSection(...args: any[]): any;
}

export function createSettingsPageHelpersFeature(
    controllers: SettingsPageHelpersControllers,
): SettingsPageHelpersFeature {
    const { entityInput } = controllers.entityState;
    const { isConfigLocked, switchTab } = controllers.shell;
    const { bindTextPost } = controllers.codec;
    const { syncInput } = controllers.statusPreview;
    const { condField, createRangeSlider, fieldLabel, makeCollapsibleCard, toggleRow } = controllers.fields;
    const {
        postClockBrightnessDay,
        postClockBrightnessNight,
        postClockScreensaver,
        postAlarmDelayAudio,
        postAlarmDelayTts,
        postAlarmDelayEntryAnnouncement,
        postAlarmDelayExitAnnouncement,
        postAlarmDelayBeepVolume,
        postAlarmDelayFinalCountdown,
    } = controllers.clockBarPostApi;
    const {
        postScreensaverAction,
        postScreensaverDimmedBrightness,
        postScreensaverDimmedBrightnessDay,
        postScreensaverDimmedBrightnessNight,
        postSwitch,
    } = controllers.requestApi;
    const els = controllers.runtime.els;
    const { formatDuration, formatHour } = controllers.screenScheduleState;
    const { syncUi: syncClockBarUi } = controllers.clockBar;
    // ── Settings Page Helpers ──────────────────────────────────────────
    // ── Settings UI helpers ─────────────────────────────────────────────
    const _settingsUiFeature: SettingsUiFeature = controllers.settingsUiFeature;
    const _alarmDelayAudioController: AlarmDelayAudioController = controllers.alarmDelayAudio;
    const _screensaverController: ScreensaverController = controllers.screensaver;
    const _coverArtScreensaverController: CoverArtScreensaverController = controllers.coverArtScreensaver;
    const _mediaPlaybackController: MediaPlaybackController = controllers.mediaPlayback;
    function alarmDelayAudioState(this: any) {
        return {
            audioEnabled: !!state.alarmDelayAudioOn,
            ttsEnabled: !!state.alarmDelayTtsOn,
            entryAnnouncement: state.alarmDelayEntryAnnouncement,
            exitAnnouncement: state.alarmDelayExitAnnouncement,
            beepVolume: state.alarmDelayBeepVolume,
            finalCountdown: state.alarmDelayFinalCountdown,
        };
    }
    function applyAlarmDelayAudioState(this: any, next?: any) {
        state.alarmDelayAudioOn = next.audioEnabled;
        state.alarmDelayTtsOn = next.ttsEnabled;
        state.alarmDelayEntryAnnouncement = next.entryAnnouncement;
        state.alarmDelayExitAnnouncement = next.exitAnnouncement;
        state.alarmDelayBeepVolume = next.beepVolume;
        state.alarmDelayFinalCountdown = next.finalCountdown;
    }
    function screensaverState(this: any) {
        return {
            action: state.screensaverAction,
            clockBrightnessDay: state.clockBrightnessDay,
            clockBrightnessNight: state.clockBrightnessNight,
            dimBrightness: state.screensaverDimmedBrightness,
            dimBrightnessDay: state.screensaverDimmedBrightnessDay,
            dimBrightnessNight: state.screensaverDimmedBrightnessNight,
        };
    }
    function applyScreensaverState(this: any, next?: any) {
        state.screensaverAction = next.action;
        state.clockBrightnessDay = next.clockBrightnessDay;
        state.clockBrightnessNight = next.clockBrightnessNight;
        state.screensaverDimmedBrightness = next.dimBrightness;
        state.screensaverDimmedBrightnessDay = next.dimBrightnessDay;
        state.screensaverDimmedBrightnessNight = next.dimBrightnessNight;
        state.clockScreensaverOn = next.action === "clock";
    }
    function coverArtScreensaverState(this: any) {
        return {
            enabled: !!state.coverArtScreensaverOn,
            delay: state.coverArtDelay,
            trackOverlayDuration: state.coverArtTrackOverlayDuration,
            hideExternalInput: !!state.coverArtHideExternalInputOn,
            filteringEnabled: !!state.coverArtFilteringEnabled,
            attributeConditions: state.coverArtAttributeConditions || "",
        };
    }
    function applyCoverArtScreensaverState(this: any, next?: any) {
        state.coverArtScreensaverOn = next.enabled;
        state.coverArtDelay = next.delay;
        state.coverArtTrackOverlayDuration = next.trackOverlayDuration;
        state.coverArtHideExternalInputOn = next.hideExternalInput;
        state.coverArtFilteringEnabled = next.filteringEnabled;
        state.coverArtAttributeConditions = next.attributeConditions;
    }
    function mediaPlaybackState(this: any) {
        return {
            sleepPreventionEnabled: !!state.mediaPlayerSleepPreventionOn,
            sleepPreventionEntity: state.mediaPlayerSleepPreventionEntity || "",
            coverArtEntity: state.coverArtMediaPlayerEntity || "",
        };
    }
    function applyMediaPlaybackState(this: any, next?: any) {
        state.mediaPlayerSleepPreventionOn = next.sleepPreventionEnabled;
        state.mediaPlayerSleepPreventionEntity = next.sleepPreventionEntity;
        state.coverArtMediaPlayerEntity = next.coverArtEntity;
    }
    function settingsStatusHeader(this: any, title?: any) {
        return _settingsUiFeature.settingsStatusHeader(title);
    }
    function appendSettingsSection(this: any, parent?: any, title?: any, cards?: any) {
        _settingsUiFeature.appendSettingsSection(parent, title, cards);
    }
    function openVoiceServicesSettings(this: any) {
        if (isConfigLocked() || !els.voiceServicesCard)
            return;
        switchTab("settings");
        els.voiceServicesCard.classList.remove("collapsed");
        els.voiceServicesCard.scrollIntoView({ block: "center", behavior: "smooth" });
        if (els.setVoiceServicesToggle) {
            window.setTimeout(function (this: any) { els.setVoiceServicesToggle.focus(); }, 150);
        }
    }
    function syncAlarmDelayAudioUi(this: any) {
        var audioState: any = alarmDelayAudioState();
        var uiState: any = _alarmDelayAudioController.uiState(audioState);
        if (els.setAlarmDelayAudioToggle)
            els.setAlarmDelayAudioToggle.checked = !!state.alarmDelayAudioOn;
        if (els.setAlarmDelayTtsToggle)
            els.setAlarmDelayTtsToggle.checked = !!state.alarmDelayTtsOn;
        if (els.alarmDelayAudioOptions)
            els.alarmDelayAudioOptions.style.display = uiState.audioOptionsVisible ? "" : "none";
        if (els.alarmDelayTtsOptions)
            els.alarmDelayTtsOptions.style.display = uiState.ttsOptionsVisible ? "" : "none";
        syncInput(els.setAlarmDelayEntryAnnouncement, state.alarmDelayEntryAnnouncement);
        syncInput(els.setAlarmDelayExitAnnouncement, state.alarmDelayExitAnnouncement);
        if (els.setAlarmDelayBeepVolume)
            els.setAlarmDelayBeepVolume.value = String(uiState.beepVolumePercent);
        if (els.setAlarmDelayBeepVolumeVal)
            els.setAlarmDelayBeepVolumeVal.textContent = uiState.beepVolumePercent + "%";
        if (els.setAlarmDelayFinalCountdown)
            els.setAlarmDelayFinalCountdown.value = String(uiState.finalCountdown);
    }
    function buildAlarmDelayAudioSettingsCard(this: any) {
        if (!(controllers.layout.config.features && controllers.layout.config.features.alarmDelayAudio))
            return null;
        var body: any = document.createElement("div");
        var master: any = toggleRow("Alarm Delay Audio", "sp-set-alarm-delay-audio", state.alarmDelayAudioOn);
        body.appendChild(master.row);
        els.setAlarmDelayAudioToggle = master.input;
        master.input.addEventListener("change", function (this: any) {
            applyAlarmDelayAudioState(_alarmDelayAudioController.setAudioEnabled(alarmDelayAudioState(), this.checked));
            postAlarmDelayAudio(state.alarmDelayAudioOn);
            syncAlarmDelayAudioUi();
        });

        var options: any = condField();
        els.alarmDelayAudioOptions = options;
        var tts: any = toggleRow("TTS Announcements", "sp-set-alarm-delay-tts", state.alarmDelayTtsOn);
        options.appendChild(tts.row);
        els.setAlarmDelayTtsToggle = tts.input;
        tts.input.addEventListener("change", function (this: any) {
            applyAlarmDelayAudioState(_alarmDelayAudioController.setTtsEnabled(alarmDelayAudioState(), this.checked));
            postAlarmDelayTts(state.alarmDelayTtsOn);
            syncAlarmDelayAudioUi();
        });

        var ttsOptions: any = condField();
        els.alarmDelayTtsOptions = ttsOptions;
        function announcementInput(
            label: any,
            id: any,
            value: any,
            fallback: any,
            stateKey: "alarmDelayEntryAnnouncement" | "alarmDelayExitAnnouncement",
            controllerField: "entryAnnouncement" | "exitAnnouncement",
            postValue: any,
        ) {
            var field: any = document.createElement("div");
            field.className = "sp-field";
            field.appendChild(fieldLabel(label, id));
            var input: any = document.createElement("input");
            input.type = "text";
            input.className = "sp-input";
            input.id = id;
            input.maxLength = 120;
            input.value = value;
            input.addEventListener("change", function (this: any) {
                var normalized: any = _alarmDelayAudioController.setAnnouncement(
                    alarmDelayAudioState(), controllerField, this.value, fallback,
                )[controllerField];
                this.value = normalized;
                state[stateKey] = normalized;
                postValue(normalized);
            });
            field.appendChild(input);
            ttsOptions.appendChild(field);
            return input;
        }
        els.setAlarmDelayEntryAnnouncement = announcementInput(
            "Entry Announcement", "sp-set-alarm-delay-entry-announcement",
            state.alarmDelayEntryAnnouncement, DEFAULT_ALARM_DELAY_ENTRY_ANNOUNCEMENT,
            "alarmDelayEntryAnnouncement",
            "entryAnnouncement",
            postAlarmDelayEntryAnnouncement);
        els.setAlarmDelayExitAnnouncement = announcementInput(
            "Exit Announcement", "sp-set-alarm-delay-exit-announcement",
            state.alarmDelayExitAnnouncement, DEFAULT_ALARM_DELAY_EXIT_ANNOUNCEMENT,
            "alarmDelayExitAnnouncement",
            "exitAnnouncement",
            postAlarmDelayExitAnnouncement);
        options.appendChild(ttsOptions);

        var volume: any = createRangeSlider("Beep Volume", state.alarmDelayBeepVolume * 100, null);
        volume.range.id = "sp-set-alarm-delay-beep-volume";
        volume.range.min = "5";
        volume.range.max = "100";
        volume.range.step = "5";
        volume.range.addEventListener("input", function (this: any) {
            applyAlarmDelayAudioState(_alarmDelayAudioController.setBeepVolume(alarmDelayAudioState(), parseFloat(this.value) / 100));
            volume.val.textContent = Math.round(state.alarmDelayBeepVolume * 100) + "%";
        });
        volume.range.addEventListener("change", function (this: any) {
            applyAlarmDelayAudioState(_alarmDelayAudioController.setBeepVolume(alarmDelayAudioState(), parseFloat(this.value) / 100));
            postAlarmDelayBeepVolume(state.alarmDelayBeepVolume);
        });
        options.appendChild(volume.wrap);
        els.setAlarmDelayBeepVolume = volume.range;
        els.setAlarmDelayBeepVolumeVal = volume.val;

        var countdownField: any = document.createElement("div");
        countdownField.className = "sp-field";
        countdownField.appendChild(fieldLabel("Faster Beeps During Final Seconds", "sp-set-alarm-delay-final-countdown"));
        var countdown: any = document.createElement("input");
        countdown.type = "number";
        countdown.className = "sp-input";
        countdown.id = "sp-set-alarm-delay-final-countdown";
        countdown.min = "0";
        countdown.max = "60";
        countdown.step = "1";
        countdown.value = String(state.alarmDelayFinalCountdown);
        countdown.addEventListener("change", function (this: any) {
            applyAlarmDelayAudioState(_alarmDelayAudioController.setFinalCountdown(alarmDelayAudioState(), this.value));
            this.value = String(state.alarmDelayFinalCountdown);
            postAlarmDelayFinalCountdown(state.alarmDelayFinalCountdown);
        });
        countdownField.appendChild(countdown);
        options.appendChild(countdownField);
        els.setAlarmDelayFinalCountdown = countdown;
        body.appendChild(options);
        body.appendChild(infoPanel(
            "sp-alarm-delay-audio-info",
            "Entry and exit beeps use the panel speaker. TTS is sent as a Home Assistant announcement event only while Voice Services are enabled."));
        syncAlarmDelayAudioUi();
        return makeCollapsibleCard("Alarm Audio", body, true);
    }
    function coverArtTrackOverlayDurationSupported(this: any) {
        return !!controllers.layout.config.coverArtSquareOverlay;
    }
    function infoPanel(this: any, id?: any, text?: any) {
        return _settingsUiFeature.infoPanel(id, text);
    }
    function statusBadge(this: any, label?: any, text?: any) {
        return _settingsUiFeature.statusBadge(label, text);
    }
    function disclosureBadge(this: any, text?: any, label?: any) {
        return _settingsUiFeature.disclosureBadge(text, label);
    }
    function inlineDisclosure(this: any, title?: any, bodyElement?: any, defaultOpen?: any, badgeElement?: any) {
        return _settingsUiFeature.inlineDisclosure(title, bodyElement, defaultOpen, badgeElement);
    }
    // ── Settings sync helpers ───────────────────────────────────────────
    function syncClockScreensaverControls(this: any) {
        var controlState: any = _screensaverController.uiState(screensaverState());
        var mode: any = controlState.mode;
        var clockDisplay: any = controlState.clockVisible ? "" : "none";
        var dimDisplay: any = controlState.dimVisible ? "" : "none";
        var automaticBrightness: any = normalizeBrightnessMode(state.brightnessMode) !== "manual";
        state.clockScreensaverOn = mode === "clock";
        syncClockBarUi();
        if (els.setClockSelect)
            els.setClockSelect.value = mode;
        if (els.setSensorClockSelect)
            els.setSensorClockSelect.value = mode;
        syncOptionalClockBrightness(els.setClockBrightnessField, els.setDimBrightnessField || els.setClockField, clockDisplay);
        syncOptionalClockBrightness(els.setSensorClockBrightnessField, els.setSensorDimBrightnessField || els.setSensorClockField, clockDisplay);
        syncOptionalClockBrightness(els.setDimBrightnessField, els.setClockField, dimDisplay);
        syncOptionalClockBrightness(els.setSensorDimBrightnessField, els.setSensorClockField, dimDisplay);
        if (els.setManualDimBrightnessField)
            els.setManualDimBrightnessField.style.display = automaticBrightness ? "none" : "";
        if (els.setAutomaticDimBrightnessField)
            els.setAutomaticDimBrightnessField.style.display = automaticBrightness ? "" : "none";
        if (els.setSensorManualDimBrightnessField)
            els.setSensorManualDimBrightnessField.style.display = automaticBrightness ? "none" : "";
        if (els.setSensorAutomaticDimBrightnessField)
            els.setSensorAutomaticDimBrightnessField.style.display = automaticBrightness ? "" : "none";
        if (els.setDimBrightness) {
            els.setDimBrightness.value = state.screensaverDimmedBrightness;
            els.setDimBrightnessVal.textContent = controlState.dimBrightnessLabel;
        }
        if (els.setSensorDimBrightness) {
            els.setSensorDimBrightness.value = state.screensaverDimmedBrightness;
            els.setSensorDimBrightnessVal.textContent = controlState.dimBrightnessLabel;
        }
        if (els.setDimBrightnessDay) {
            els.setDimBrightnessDay.value = state.screensaverDimmedBrightnessDay;
            els.setDimBrightnessDayVal.textContent = controlState.dimBrightnessDayLabel;
        }
        if (els.setDimBrightnessNight) {
            els.setDimBrightnessNight.value = state.screensaverDimmedBrightnessNight;
            els.setDimBrightnessNightVal.textContent = controlState.dimBrightnessNightLabel;
        }
        if (els.setSensorDimBrightnessDay) {
            els.setSensorDimBrightnessDay.value = state.screensaverDimmedBrightnessDay;
            els.setSensorDimBrightnessDayVal.textContent = controlState.dimBrightnessDayLabel;
        }
        if (els.setSensorDimBrightnessNight) {
            els.setSensorDimBrightnessNight.value = state.screensaverDimmedBrightnessNight;
            els.setSensorDimBrightnessNightVal.textContent = controlState.dimBrightnessNightLabel;
        }
        if (els.setClockBrightnessDay) {
            els.setClockBrightnessDay.value = state.clockBrightnessDay;
            els.setClockBrightnessDayVal.textContent = controlState.dayBrightnessLabel;
        }
        if (els.setClockBrightnessNight) {
            els.setClockBrightnessNight.value = state.clockBrightnessNight;
            els.setClockBrightnessNightVal.textContent = controlState.nightBrightnessLabel;
        }
        if (els.setSensorClockBrightnessDay) {
            els.setSensorClockBrightnessDay.value = state.clockBrightnessDay;
            els.setSensorClockBrightnessDayVal.textContent = controlState.dayBrightnessLabel;
        }
        if (els.setSensorClockBrightnessNight) {
            els.setSensorClockBrightnessNight.value = state.clockBrightnessNight;
            els.setSensorClockBrightnessNightVal.textContent = controlState.nightBrightnessLabel;
        }
    }
    function syncMediaPlayerSleepPreventionUi(this: any) {
        var uiState: any = _mediaPlaybackController.uiState(mediaPlaybackState());
        if (els.setMediaPlayerSleepPreventionToggle) {
            els.setMediaPlayerSleepPreventionToggle.checked = uiState.sleepPreventionEnabled;
        }
        if (els.setSensorMediaPlayerSleepPreventionToggle) {
            els.setSensorMediaPlayerSleepPreventionToggle.checked = uiState.sleepPreventionEnabled;
        }
    }
    function syncCoverArtScreensaverUi(this: any) {
        applyCoverArtScreensaverState(_coverArtScreensaverController.normalize(coverArtScreensaverState()));
        var uiState: any = _coverArtScreensaverController.uiState(coverArtScreensaverState());
        if (els.setCoverArtToggle) {
            els.setCoverArtToggle.checked = !!state.coverArtScreensaverOn;
        }
        if (els.setCoverArtOptions) {
            els.setCoverArtOptions.classList.toggle("sp-visible", uiState.contentVisible);
        }
        if (els.setCoverArtOnlyOptions) {
            els.setCoverArtOnlyOptions.classList.toggle("sp-visible", uiState.contentVisible);
        }
        if (els.setCoverArtBadge) {
            els.setCoverArtBadge.className = "sp-card-badge" + (uiState.badgeVisible ? "" : " sp-hidden");
        }
        if (els.setCoverArtDelay) {
            setSelectValue(els.setCoverArtDelay, state.coverArtDelay, formatDuration(state.coverArtDelay));
        }
        if (els.setCoverArtTrackOverlayDuration) {
            var value: any = state.coverArtTrackOverlayDuration;
            setSelectValue(els.setCoverArtTrackOverlayDuration, value, timedSettingLabel(value, formatDuration));
        }
        if (els.setCoverArtHideExternalInputToggle) {
            els.setCoverArtHideExternalInputToggle.checked = !state.coverArtHideExternalInputOn;
        }
        if (els.setCoverArtSecondaryMediaPlayerOptions) {
            els.setCoverArtSecondaryMediaPlayerOptions.classList.toggle(
                "sp-visible", uiState.externalSourcesVisible);
        }
        if (els.setHomeAssistantArtworkProtocol) {
            els.setHomeAssistantArtworkProtocol.value =
                normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
        }
        if (els.setCoverArtHomeAssistantPort) {
            els.setCoverArtHomeAssistantPort.value = String(normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort));
        }
        if (els.setHomeAssistantArtworkEndpointMode) {
            els.setHomeAssistantArtworkEndpointMode.value = state.homeAssistantArtworkEndpointMode;
        }
        var manualEndpoint: any = state.homeAssistantArtworkEndpointMode === "Manual";
        if (els.setHomeAssistantArtworkProtocol) els.setHomeAssistantArtworkProtocol.disabled = !manualEndpoint;
        if (els.setCoverArtHomeAssistantPort) els.setCoverArtHomeAssistantPort.disabled = !manualEndpoint;
        if (els.homeAssistantArtworkEndpointStatus) {
            els.homeAssistantArtworkEndpointStatus.textContent = state.homeAssistantArtworkEndpointStatus || "Discovering";
        }
        if (els.setCoverArtFilterToggle) {
            els.setCoverArtFilterToggle.checked = !!state.coverArtFilteringEnabled;
        }
        if (els.setCoverArtFilterOptions) {
            els.setCoverArtFilterOptions.classList.toggle("sp-visible", uiState.filterOptionsVisible);
        }
        syncInput(els.setCoverArtConditions, state.coverArtAttributeConditions || "");
    }
    function syncOptionalClockBrightness(this: any, field?: any, previousField?: any, display?: any) {
        if (field)
            field.style.display = display;
        if (previousField)
            previousField.style.marginBottom = display === "none" ? "20px" : "";
    }
    function createScreensaverThenControls(this: any, selectId?: any) {
        var clockField: any = document.createElement("div");
        clockField.className = "sp-field";
        clockField.appendChild(fieldLabel("Then", selectId));
        var clockSelect: any = document.createElement("select");
        clockSelect.className = "sp-select";
        clockSelect.id = selectId;
        [
            { value: "off", label: "Display Off" },
            { value: "dim", label: "Screen Dimmed" },
            { value: "clock", label: "Clock" },
        ].forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt.value;
            o.textContent = opt.label;
            clockSelect.appendChild(o);
        });
        clockSelect.value = _screensaverController.uiState(screensaverState()).mode;
        clockSelect.addEventListener("change", function (this: any) {
            applyScreensaverState(_screensaverController.setAction(screensaverState(), this.value));
            syncClockScreensaverControls();
            postScreensaverAction(state.screensaverAction);
            postClockScreensaver(state.clockScreensaverOn);
        });
        clockField.appendChild(clockSelect);
        var dimBrightnessField: any = document.createElement("div");
        dimBrightnessField.style.display = _screensaverController.uiState(screensaverState()).dimVisible ? "" : "none";
        var manualDimBrightnessField: any = document.createElement("div");
        var dimSlider: any = createRangeSlider("Dimmed Screen Brightness", state.screensaverDimmedBrightness, postScreensaverDimmedBrightness);
        dimSlider.range.id = selectId === "sp-set-sensor-clock-mode"
            ? "sp-set-sensor-dimmed-brightness"
            : "sp-set-dimmed-brightness";
        dimSlider.range.min = "1";
        dimSlider.range.step = "1";
        dimSlider.range.addEventListener("input", function (this: any) {
            applyScreensaverState(_screensaverController.setDimBrightness(screensaverState(), this.value));
            syncClockScreensaverControls();
        });
        manualDimBrightnessField.appendChild(dimSlider.wrap);
        dimBrightnessField.appendChild(manualDimBrightnessField);
        var automaticDimBrightnessField: any = document.createElement("div");
        var dimDaySlider: any = createRangeSlider("Daytime Dimmed Screen Brightness", state.screensaverDimmedBrightnessDay, postScreensaverDimmedBrightnessDay);
        dimDaySlider.range.id = selectId === "sp-set-sensor-clock-mode"
            ? "sp-set-sensor-daytime-dimmed-brightness"
            : "sp-set-daytime-dimmed-brightness";
        dimDaySlider.range.min = "1";
        dimDaySlider.range.step = "1";
        dimDaySlider.range.addEventListener("input", function (this: any) {
            applyScreensaverState(_screensaverController.setDimBrightnessByPeriod(screensaverState(), "dimBrightnessDay", this.value));
            syncClockScreensaverControls();
        });
        automaticDimBrightnessField.appendChild(dimDaySlider.wrap);
        var dimNightSlider: any = createRangeSlider("Nighttime Dimmed Screen Brightness", state.screensaverDimmedBrightnessNight, postScreensaverDimmedBrightnessNight);
        dimNightSlider.range.id = selectId === "sp-set-sensor-clock-mode"
            ? "sp-set-sensor-nighttime-dimmed-brightness"
            : "sp-set-nighttime-dimmed-brightness";
        dimNightSlider.range.min = "1";
        dimNightSlider.range.step = "1";
        dimNightSlider.range.addEventListener("input", function (this: any) {
            applyScreensaverState(_screensaverController.setDimBrightnessByPeriod(screensaverState(), "dimBrightnessNight", this.value));
            syncClockScreensaverControls();
        });
        automaticDimBrightnessField.appendChild(dimNightSlider.wrap);
        dimBrightnessField.appendChild(automaticDimBrightnessField);
        var clockBrightnessField: any = document.createElement("div");
        clockBrightnessField.className = "sp-clock-brightness-field";
        clockBrightnessField.style.display = _screensaverController.uiState(screensaverState()).clockVisible ? "" : "none";
        var daySlider: any = createRangeSlider("Daytime Clock Brightness", state.clockBrightnessDay, postClockBrightnessDay);
        daySlider.range.min = "1";
        daySlider.range.step = "1";
        daySlider.range.addEventListener("input", function (this: any) {
            applyScreensaverState(_screensaverController.setClockBrightness(screensaverState(), "clockBrightnessDay", this.value));
            syncClockScreensaverControls();
        });
        clockBrightnessField.appendChild(daySlider.wrap);
        var nightSlider: any = createRangeSlider("Nighttime Clock Brightness", state.clockBrightnessNight, postClockBrightnessNight);
        nightSlider.range.min = "1";
        nightSlider.range.step = "1";
        nightSlider.range.addEventListener("input", function (this: any) {
            applyScreensaverState(_screensaverController.setClockBrightness(screensaverState(), "clockBrightnessNight", this.value));
            syncClockScreensaverControls();
        });
        clockBrightnessField.appendChild(nightSlider.wrap);
        return {
            clockField: clockField,
            clockSelect: clockSelect,
            dimBrightnessField: dimBrightnessField,
            manualDimBrightnessField: manualDimBrightnessField,
            automaticDimBrightnessField: automaticDimBrightnessField,
            dimBrightness: dimSlider.range,
            dimBrightnessVal: dimSlider.val,
            dimBrightnessDay: dimDaySlider.range,
            dimBrightnessDayVal: dimDaySlider.val,
            dimBrightnessNight: dimNightSlider.range,
            dimBrightnessNightVal: dimNightSlider.val,
            brightnessField: clockBrightnessField,
            clockBrightnessDay: daySlider.range,
            clockBrightnessDayVal: daySlider.val,
            clockBrightnessNight: nightSlider.range,
            clockBrightnessNightVal: nightSlider.val,
        };
    }
    function createHourSelect(this: any, label?: any, id?: any, initial?: any, onChange?: any) {
        var wrap: any = document.createElement("div");
        wrap.className = "sp-field";
        wrap.appendChild(fieldLabel(label, id));
        var select: any = document.createElement("select");
        select.className = "sp-select";
        select.id = id;
        for (var h: any = 0; h < 24; h++) {
            var o: any = document.createElement("option");
            o.value = String(h);
            o.textContent = formatHour(h);
            select.appendChild(o);
        }
        select.value = String(normalizeHour(initial, 0));
        select.addEventListener("change", function (this: any) {
            onChange(normalizeHour(this.value, 0));
        });
        wrap.appendChild(select);
        return { wrap: wrap, select: select };
    }
    function createTimeInput(this: any, label?: any, id?: any, initial?: any, fallback?: any, onChange?: any) {
        var wrap: any = document.createElement("div");
        wrap.className = "sp-field";
        wrap.appendChild(fieldLabel(label, id));
        var input: any = document.createElement("input");
        input.type = "time";
        input.className = "sp-input";
        input.id = id;
        input.step = "60";
        input.value = normalizeTimeOfDay(initial, fallback);
        input.addEventListener("change", function (this: any) {
            var value: any = normalizeTimeOfDay(this.value, fallback);
            this.value = value;
            onChange(value);
        });
        wrap.appendChild(input);
        return { wrap: wrap, input: input };
    }
    function createEntityToggleSection(this: any, label?: any, id?: any, checked?: any, switchName?: any, entityLabel?: any, entityPostName?: any, placeholder?: any) {
        var toggle: any = toggleRow(label, id, checked);
        var field: any = condField();
        var inp: any = entityInput("", "", placeholder, ["sensor"]);
        field.appendChild(inp);
        toggle.input.addEventListener("change", function (this: any) { postSwitch(switchName, this.checked); });
        bindTextPost(inp, entityPostName, {});
        return { toggle: toggle, field: field, input: inp };
    }
    return {
        coverArtScreensaverState, applyCoverArtScreensaverState,
        mediaPlaybackState, applyMediaPlaybackState, settingsStatusHeader,
        appendSettingsSection, openVoiceServicesSettings, syncAlarmDelayAudioUi,
        buildAlarmDelayAudioSettingsCard, coverArtTrackOverlayDurationSupported,
        infoPanel, statusBadge, disclosureBadge, inlineDisclosure,
        syncClockScreensaverControls, syncMediaPlayerSleepPreventionUi,
        syncCoverArtScreensaverUi, syncOptionalClockBrightness,
        createScreensaverThenControls, createHourSelect, createTimeInput,
        createEntityToggleSection,
    };
}
