import { state } from "../state/app_instance";
import * as EspControlModel from "../model";
import {
    normalizeBrightnessMode,
    normalizeHexColor,
    normalizeHomeAssistantArtworkPort,
    normalizeHomeAssistantArtworkProtocol,
    normalizeHomeAssistantArtworkEndpointMode,
    normalizeHour,
    normalizeLanguage,
    normalizeScheduleClockBrightness,
    normalizeScheduleDimmedBrightness,
    normalizeScheduleMode,
    normalizeScheduleSensorActivation,
    normalizeScheduleTrigger,
    normalizeScheduleWakeBrightness,
    normalizeScheduleWakeTimeout,
    normalizeScreensaverAction,
    normalizeScreensaverDimmedBrightness,
    normalizeTemperatureUnit,
    normalizeTimeOfDay,
} from "../model/settings";
import type { BackupImportController } from "../features/backup_import_controller";
import type { BackupExportController } from "../features/backup_export_controller";
import type { BackupFileController } from "../features/backup_file_controller";
import type { BackupRestoreController } from "../features/backup_restore_controller";
import type { ApplicationLayoutState } from "./application_context";
import type { NativePanelConfigController } from "../controllers/native_panel_config_controller";
import type { PanelConfigDocument } from "../model";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { CoreFeature } from "./core";
import { syncLanguageSelect } from "./language_state";
import { hasCustomNtpServers, syncNtpServerUi } from "./ntp_state";
import { syncIdleUi } from "./idle_state";
import { getActiveScreensaverMode } from "./screensaver_state";
import type { ScreenScheduleStateFeature } from "./screen_schedule_state";
import type { ScreensaverTimeoutFeature } from "./screensaver_timeout";
import type { FirmwareUpdateFeature } from "./firmware_update_state";
import type { ClockBarFeature } from "./clock_bar_state";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { ApplicationApiFeature } from "./api";
import type { AppStatusPreviewFeature } from "./app_status_preview";
import type { GridFeature } from "./grid";
import type { ArtworkPostApiFeature } from "./artwork_post_api";
import type { ScreenSchedulePostApiFeature } from "./screen_schedule_post_api";
import type { ClockBarPostApiFeature } from "./clock_bar_post_api";
import type { ConfigPersistenceFeature } from "./config_post_api";
import type { BackupContractFeature } from "./backup_contract";
import type { SettingsPageHelpersFeature } from "./settings_page_helpers";
import type { PreviewRenderFeature } from "./preview_render";
import type { ButtonSettingsFeature } from "./button_settings";
import { legacyRestoreFailureMessage, restoreLegacyLayoutDocument } from "../features/legacy_layout_restore";

export interface AppBackupControllers {
    readonly layout: ApplicationLayoutState;
    readonly backupExport: BackupExportController;
    readonly backupImport: BackupImportController<any, any, any>;
    readonly backupRestore: BackupRestoreController<any, any>;
    readonly backupFile: BackupFileController;
    readonly normalizeImportedPanelSettings: (settings: any) => any;
    readonly gridColsForImportedSettings: (settings: any) => number;
    readonly nativePanelConfig?: NativePanelConfigController;
    readonly codec: ConfigCodecFeature;
    readonly configPersistence: Pick<ConfigPersistenceFeature, "subpageEntityKeys">;
    readonly backupContract: Pick<BackupContractFeature, "createBackupConfig" | "normalizeButtonConfig">;
    readonly runtime: UiRuntimeState;
    readonly core: Pick<CoreFeature, "syncPreviewOrientation">;
    readonly screenScheduleState: ScreenScheduleStateFeature;
    readonly screensaverTimeout: ScreensaverTimeoutFeature;
    readonly firmwareUpdate: FirmwareUpdateFeature;
    readonly clockBar: ClockBarFeature;
    readonly entityState: Pick<EntityStateFeature, "entityName" | "entityNameForSlot">;
    readonly shell: Pick<ControlsShellFeature, "switchTab">;
    readonly requestApi: ApplicationApiFeature;
    readonly statusPreview: Pick<AppStatusPreviewFeature, "syncInput" | "updateTempPreview">;
    readonly grid: Pick<GridFeature, "applyImportedButtonOrder" | "cancelMainGridSave" | "serializeGrid">;
    readonly artworkPostApi: ArtworkPostApiFeature;
    readonly schedulePostApi: ScreenSchedulePostApiFeature;
    readonly clockBarPostApi: ClockBarPostApiFeature;
    readonly settingsHelpers: Pick<SettingsPageHelpersFeature, "syncAlarmDelayAudioUi" | "syncClockScreensaverControls" | "syncCoverArtScreensaverUi" | "syncMediaPlayerSleepPreventionUi">;
    readonly preview: Pick<PreviewRenderFeature, "render">;
    readonly buttonSettings: Pick<ButtonSettingsFeature, "render">;
}

export interface AppBackupFeature {
    backupExportFileName(value?: unknown): string;
    normalizeImportedPanelSettings(settings?: unknown): unknown;
    gridColsForImportedSettings(settings?: unknown): number;
    exportConfig(): void;
    importConfig(): void;
}

export function createAppBackupFeature(controllers: AppBackupControllers): AppBackupFeature {
    const { syncAlarmDelayAudioUi, syncClockScreensaverControls, syncCoverArtScreensaverUi, syncMediaPlayerSleepPreventionUi } = controllers.settingsHelpers;
    const { render: renderPreview } = controllers.preview;
    const { render: renderButtonSettings } = controllers.buttonSettings;
    const { subpageEntityKeys } = controllers.configPersistence;
    const { createBackupConfig, normalizeButtonConfig: backupNormalizeButtonConfig } = controllers.backupContract;
    const { entityName, entityNameForSlot } = controllers.entityState;
    const { switchTab } = controllers.shell;
    const requestApi = controllers.requestApi;
    const { syncInput, updateTempPreview } = controllers.statusPreview;
    const { applyImportedButtonOrder, cancelMainGridSave, serializeGrid } = controllers.grid;
    const {
        postPresenceSensorEntity,
        postMediaPlayerSleepPrevention,
        postMediaPlayerSleepPreventionEntity,
        postCoverArtScreensaver,
        postCoverArtMediaPlayerEntity,
        postCoverArtSecondaryMediaPlayerEntity,
        postCoverArtConditions,
        postCoverArtDelay,
        postCoverArtTrackOverlayDuration,
        postCoverArtHideExternalInput,
        postHomeAssistantArtworkProtocol,
        postHomeAssistantArtworkPort,
        postHomeAssistantArtworkEndpointMode,
    } = controllers.artworkPostApi;
    const {
        postBrightnessMode,
        postDisplayBacklightBrightness,
        postBrightnessDawnTime,
        postBrightnessDuskTime,
        postScreenScheduleEnabled,
        postScreenScheduleTrigger,
        postScreenScheduleSensorActivation,
        postScreenScheduleSensorEntity,
        postScreenScheduleOnHour,
        postScreenScheduleOffHour,
        postScreenScheduleMode,
        postScreenScheduleWakeTimeout,
        postScreenScheduleWakeBrightness,
        postScreenScheduleDimmedBrightness,
        postScreenScheduleClockBrightness,
    } = controllers.schedulePostApi;
    const {
        postClockBrightnessDay,
        postClockBrightnessNight,
        postClockScreensaver,
        postClockBar,
        postClockBarTemperatureEntities,
        postClockBarTime,
        postClockBarNightMode,
        postNetworkStatusIcon,
        postVoiceServices,
        postAlarmDelayAudio,
        postAlarmDelayTts,
        postAlarmDelayEntryAnnouncement,
        postAlarmDelayExitAnnouncement,
        postAlarmDelayBeepVolume,
        postAlarmDelayFinalCountdown,
        postTemperatureDegreeSymbol,
        postSubpageChevron,
    } = controllers.clockBarPostApi;
    const {
        postText,
        postSwitch,
        postSelect,
        postScreensaverMode,
        postFirmwareAutoUpdate,
        postFirmwareUpdateFrequency,
        postScreensaverAction,
        postScreensaverDimmedBrightness,
        postScreensaverDimmedBrightnessDay,
        postScreensaverDimmedBrightnessNight,
        postScreensaverTimeout,
        postHomeScreenTimeout,
        postNumber,
    } = requestApi;
    const { syncPreviewOrientation } = controllers.core;
    const {
        buildSubpageGrid,
        parseButtonConfig,
        parseSubpageConfig,
        serializeButtonConfig,
        serializeSubpageConfig,
    } = controllers.codec;
    const els = controllers.runtime.els;
    const { syncUi: syncScreenScheduleUi } = controllers.screenScheduleState;
    const { syncUi: syncScreensaverTimeoutUi } = controllers.screensaverTimeout;
    const { controlsVisible: firmwareUpdateControlsVisible, syncUi: syncFirmwareUpdateUi } = controllers.firmwareUpdate;
    const {
        applyTemperatureEntities: applyClockBarTemperatureEntities,
        temperatureEntities: clockBarTemperatureEntities,
        serializeTemperatureEntities: serializeClockBarTemperatureEntities,
        syncUi: syncClockBarUi,
        syncTemperatureUi,
    } = controllers.clockBar;
    // ── Export / Import ────────────────────────────────────────────────────
    var backupExportController: BackupExportController = controllers.backupExport;
    function backupExportScreenSizeSlug(this: any, value?: any) {
        return backupExportController.screenSizeSlug(value);
    }
    function backupExportFileDate(this: any, value?: any) {
        return backupExportController.fileDate(value);
    }
    function backupExportFileName(this: any, value?: any) {
        return backupExportController.fileName(controllers.layout.config.screenSize, value);
    }
    function normalizeImportedPanelSettings(this: any, settings?: any) {
        return controllers.normalizeImportedPanelSettings(settings);
    }
    function gridColsForImportedSettings(this: any, importedSettings?: any) {
        return controllers.gridColsForImportedSettings(importedSettings);
    }
    var backupImportController: BackupImportController<any, any, any> = controllers.backupImport;
    var backupRestoreController: BackupRestoreController<any, any> = controllers.backupRestore;
    var backupFileController: BackupFileController = controllers.backupFile;
    function downloadBackupConfig(this: any, data?: any) {
        backupFileController.download(data, backupExportFileName());
    }
    function addNativeConfigToBackup(this: any, data?: any) {
        return backupExportController.addNativeConfig(data, {
            "deviceProfile": controllers.layout.deviceId,
            "buttons": state.buttons,
            "subpages": state.subpages,
            "buttonOrder": data.button_order,
            "buttonOnColor": data.button_on_color,
        });
    }
    function exportConfig(this: any) {
        var data: any = createBackupConfig({
            device: controllers.layout.deviceId,
            slots: controllers.layout.numSlots,
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
                screensaver_camera_entity: state.screensaverCameraEntity,
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
                home_assistant_artwork_endpoint_mode: normalizeHomeAssistantArtworkEndpointMode(
                    state.homeAssistantArtworkEndpointMode,
                    state.homeAssistantArtworkProtocol,
                    state.coverArtHomeAssistantPort),
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
        } as any);
        downloadBackupConfig(addNativeConfigToBackup(data));
    }
    function importConfig(this: any) {
        backupFileController.import(function (data: any) {
                function applyBackupRestorePlan(this: any, plannedImport: any) {
                var importedSettings: any = plannedImport.importedSettings;
                var importedGridCols: any = plannedImport.importedGridCols;
                var backupPlan: any = plannedImport.backupPlan;
                cancelMainGridSave();
                var activeGridCols: any = controllers.layout.gridCols;
                controllers.layout.gridCols = importedGridCols;
                var nativeDocument: PanelConfigDocument = {
                    deviceProfile: controllers.layout.deviceId,
                    buttons: {},
                    subpages: {},
                    settings: {
                        button_order: normalizedButtonOrder,
                        button_on_color: backupPlan.config.button_on_color,
                    },
                };
                for (var nativeButtonIndex: any = 0; nativeButtonIndex < controllers.layout.numSlots; nativeButtonIndex++) {
                    var nativeButtonValue: any = serializeButtonConfig(backupNormalizeButtonConfig(backupPlan.buttons[nativeButtonIndex]));
                    if (nativeButtonValue)
                        nativeDocument.buttons[nativeButtonIndex + 1] = nativeButtonValue;
                }
                for (var nativeSubpageKey in backupPlan.subpages) {
                    var nativeSubpageValue: any = serializeSubpageConfig(backupPlan.subpages[nativeSubpageKey]);
                    if (nativeSubpageValue)
                        nativeDocument.subpages[Number(nativeSubpageKey)] = nativeSubpageValue;
                }
                var backedUpNativeConfig: any = backupPlan.config.native_config;
                if (backedUpNativeConfig &&
                    backedUpNativeConfig.device_profile === controllers.layout.deviceId) {
                    nativeDocument = EspControlModel.decodePanelConfig(
                        EspControlModel.decodePanelConfigBackupPayload(backedUpNativeConfig));
                }
                nativeDocument.settings.button_order = String(nativeDocument.settings.button_order || backupPlan.button_order || "");
                nativeDocument.settings.button_on_color = String(nativeDocument.settings.button_on_color || backupPlan.config.button_on_color || "");

                state.buttons = [];
                for (var canonicalButtonIndex: any = 0; canonicalButtonIndex < controllers.layout.numSlots; canonicalButtonIndex++)
                    state.buttons[canonicalButtonIndex] = parseButtonConfig(nativeDocument.buttons[canonicalButtonIndex + 1] || "");
                state.subpages = {};
                state.subpageRaw = {};
                for (var canonicalSubpageKey in nativeDocument.subpages) {
                    var canonicalSubpage: any = parseSubpageConfig(nativeDocument.subpages[Number(canonicalSubpageKey)] || "");
                    buildSubpageGrid(canonicalSubpage);
                    state.subpages[canonicalSubpageKey] = canonicalSubpage;
                }
                var normalizedButtonOrder: any;
                try {
                    normalizedButtonOrder = applyImportedButtonOrder(nativeDocument.settings.button_order, {});
                    nativeDocument.settings.button_order = normalizedButtonOrder;
                }
                finally {
                    controllers.layout.gridCols = activeGridCols;
                }

                function readLegacyText(name: string) {
                    return requestApi.getJsonFirst(requestApi.entityDetailPaths("text", [name], "state"));
                }
                function queueLegacyLayoutRestore() {
                    return restoreLegacyLayoutDocument(nativeDocument, {
                        slotCount: controllers.layout.numSlots,
                        subpageEntityKeys: subpageEntityKeys(),
                        entityName: entityName,
                        entityNameForSlot: entityNameForSlot,
                        splitSubpageConfigChunks: EspControlModel.splitSubpageConfigChunks,
                        postText: requestApi.postTextLegacy,
                        readText: readLegacyText,
                    }).then(function (result: any) {
                        if (!result.ok) {
                            requestApi.postQueueError = true;
                            throw Object.assign(new Error(legacyRestoreFailureMessage(result)), {
                                backupMessage: legacyRestoreFailureMessage(result),
                            });
                        }
                        return "legacy-fallback";
                    });
                }
                var nativeRestore: any = controllers.nativePanelConfig
                    ? controllers.nativePanelConfig.writeDocument(nativeDocument)
                    : null;
                var nativeRestoreCompletion: any = null;
                if (nativeRestore) {
                    nativeRestoreCompletion = requestApi.postQueue.then(function () { return nativeRestore; }).then(function (result: any) {
                        if (result === "legacy-fallback") {
                            return queueLegacyLayoutRestore();
                        }
                        else if (result !== "saved") {
                            requestApi.postQueueError = true;
                        }
                        return result;
                    });
                    requestApi.postQueue = nativeRestoreCompletion;
                }
                else {
                    nativeRestoreCompletion = queueLegacyLayoutRestore();
                    requestApi.postQueue = nativeRestoreCompletion;
                }
                state.onColor = nativeDocument.settings.button_on_color;
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
                    if (controllers.layout.config.features && controllers.layout.config.features.voiceServices)
                        postVoiceServices(importedSettings.voiceServices);
                    if (controllers.layout.config.features && controllers.layout.config.features.alarmDelayAudio) {
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
                    if (CFG.features && CFG.features.cameraScreensaver)
                        postText(entityName("screen_saver_camera_entity"), importedSettings.screensaverCameraEntity);
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
                    postHomeAssistantArtworkEndpointMode(importedSettings.coverArtHomeAssistantEndpointMode);
                    if (firmwareUpdateControlsVisible()) {
                        postFirmwareAutoUpdate(importedSettings.autoUpdate);
                        postFirmwareUpdateFrequency(importedSettings.updateFrequency);
                    }
                    var importedScreensaverAction: any = importedSettings.screensaverAction;
                    if (importedScreensaverAction === "camera" &&
                        !(CFG.features && CFG.features.cameraScreensaver))
                        importedScreensaverAction = "off";
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
                    if (controllers.layout.config.features && controllers.layout.config.features.screenRotation)
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
                    state.screensaverCameraEntity = importedSettings.screensaverCameraEntity;
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
                    state.homeAssistantArtworkEndpointMode = importedSettings.coverArtHomeAssistantEndpointMode;
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
                    syncInput(els.setScreensaverCamera, state.screensaverCameraEntity);
                    syncInput(els.setSchedulePresence, state.scheduleSensorEntity);
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
                    syncLanguageSelect(controllers.runtime);
                    if (els.setClockFormat)
                        els.setClockFormat.value = state.clockFormat;
                    syncNtpServerUi(controllers.runtime, syncInput);
                    syncClockScreensaverControls();
                    syncScreensaverTimeoutUi();
                    syncIdleUi(controllers.runtime);
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
                return nativeRestoreCompletion;
                }
                backupRestoreController.restore(data, {
                    device: controllers.layout.deviceId,
                    slots: controllers.layout.numSlots,
                }, applyBackupRestorePlan);
        });
    }
    return {
        backupExportFileName: (value) => backupExportFileName(value),
        normalizeImportedPanelSettings: (settings) => normalizeImportedPanelSettings(settings),
        gridColsForImportedSettings: (settings) => gridColsForImportedSettings(settings),
        exportConfig: () => exportConfig(),
        importConfig: () => importConfig(),
    };
}
