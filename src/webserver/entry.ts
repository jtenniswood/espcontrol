import * as DeviceConfig from "./device_config";
import * as Model from "./model";
import * as DeviceApi from "./api/device_api";
import * as RequestFailure from "./api/request_failure";
import * as PreviewGridFeature from "./features/preview_grid";
import * as PreviewFeature from "./features/preview";
import * as BackupFeature from "./features/backup";
import * as BackupExportController from "./features/backup_export_controller";
import * as SettingsFeature from "./features/settings";
import * as AlarmDelayAudioController from "./features/alarm_delay_audio_controller";
import * as ScreensaverController from "./features/screensaver_controller";
import * as CoverArtScreensaverController from "./features/cover_art_screensaver_controller";
import * as MediaPlaybackController from "./features/media_playback_controller";
import * as VoiceServicesController from "./features/voice_services_controller";
import * as ClockBarController from "./features/clock_bar_controller";
import * as ScreenScheduleController from "./features/screen_schedule_controller";
import * as ClipboardFeature from "./features/clipboard";
import * as UiTokens from "./state/ui_tokens";
import * as AppState from "./state/app_state";
import * as AppInstance from "./state/app_instance";
import * as EventAliases from "./state/event_aliases";
import * as EventState from "./state/event_state";
import * as FirmwareEvents from "./state/firmware_events";
import * as ConfigPrimitives from "./model/config_primitives";
import * as CardContract from "./generated/card_contract";
import * as Icons from "./generated/icons";
import { ENTITY_CATALOG } from "./generated/entity_catalog";
import { installStaticGlobals } from "./runtime/globals";
import { installEditorBootstrap, type EditorBootstrapModule } from "./runtime/editor_bootstrap";
import { installCore } from "./application/core";
import { installFirmwareMetadataModule } from "./application/firmware_metadata";
import { installStylesModule } from "./application/styles";
import { installStateModule } from "./application/state";
import { installLanguageStateModule } from "./application/language_state";
import { installEnvironmentStateModule } from "./application/environment_state";
import { installScreenRotationStateModule } from "./application/screen_rotation_state";
import { installScreenScheduleStateModule } from "./application/screen_schedule_state";
import { installNtpStateModule } from "./application/ntp_state";
import { installAppearanceStateModule } from "./application/appearance_state";
import { installIdleStateModule } from "./application/idle_state";
import { installArtworkStateModule } from "./application/artwork_state";
import { installScreensaverStateModule } from "./application/screensaver_state";
import { installFirmwareVersionStateModule } from "./application/firmware_version_state";
import { installEntityStateModule } from "./application/entity_state";
import { installClockBarStateModule } from "./application/clock_bar_state";
import { installFirmwareUpdateStateModule } from "./application/firmware_update_state";
import { installScreensaverTimeoutModule } from "./application/screensaver_timeout";
import { installC6FirmwareUiModule } from "./application/c6_firmware_ui";
import { installGridModule } from "./application/grid";
import { installApiModule } from "./application/api";
import { installFirmwareUpdatePostApiModule } from "./application/firmware_update_post_api";
import { installPublicFirmwareInstallModule } from "./application/public_firmware_install";
import { installConfigOptionCoreModule } from "./application/config_option_core";
import { installConfigMediaOptionsModule } from "./application/config_media_options";
import { installConfigImageOptionsModule } from "./application/config_image_options";
import { installConfigModalTabOptionsModule } from "./application/config_modal_tab_options";
import { installConfigSubpageOptionsModule } from "./application/config_subpage_options";
import { installConfigSensorOptionsModule } from "./application/config_sensor_options";
import { installConfigConfirmationOptionsModule } from "./application/config_confirmation_options";
import { installConfigAccessClimateAlarmOptionsModule } from "./application/config_access_climate_alarm_options";
import { installConfigCodecModule } from "./application/config_codec";
import { installNativePanelConfigMigrationModule } from "./application/native_panel_config_migration";
import { installConfigPostApiModule } from "./application/config_post_api";
import { installStateLoaderApiModule } from "./application/state_loader_api";
import { installArtworkPostApiModule } from "./application/artwork_post_api";
import { installScreenSchedulePostApiModule } from "./application/screen_schedule_post_api";
import { installClockBarPostApiModule } from "./application/clock_bar_post_api";
import { installControlsModule } from "./application/controls";
import { installControlsShellModule } from "./application/controls_shell";
import { installSettingsPageHelpersModule } from "./application/settings_page_helpers";
import { installSettingsScheduleSectionModule } from "./application/settings_schedule_section";
import { installSettingsCoverArtSectionModule } from "./application/settings_cover_art_section";
import { installSettingsSystemSectionModule } from "./application/settings_system_section";
import { installSettingsPageModule } from "./application/settings_page";
import { installControlsFieldsModule } from "./application/controls_fields";
import { installPreviewRenderModule } from "./application/preview_render";
import { installButtonSettingsSelectionModule } from "./application/button_settings_selection";
import { installButtonSettingsRenderQueueModule } from "./application/button_settings_render_queue";
import { installButtonSettingsIconPickerModule } from "./application/button_settings_icon_picker";
import { installButtonSettingsModule } from "./application/button_settings";
import { installPreviewGridPlacementModule } from "./application/preview_grid_placement";
import { installPreviewContextMenuModule } from "./application/preview_context_menu";
import { installPreviewClipboardModule } from "./application/preview_clipboard";
import { installPreviewInteractionsModule } from "./application/preview_interactions";
import { installBackupContractModule } from "./application/backup_contract";
import { installAppBackupModule } from "./application/app_backup";
import { installAppStatusPreviewModule } from "./application/app_status_preview";
import { installAppTitleModule } from "./application/app_title";
import { installAppConfigEventsModule } from "./application/app_config_events";
import { installAppStateEventHandlersModule } from "./application/app_state_event_handlers";
import { installAppEventsModule } from "./application/app_events";
import { installAppModule } from "./application/app";
import { installAppStartModule } from "./application/app_start";
import { registerActionCardTypes } from "./cards/action";
import { registerAlarmCardTypes } from "./cards/alarm";
import { registerCalendarCardTypes } from "./cards/calendar";
import { registerCompanionCardTypes } from "./cards/companion";
import { registerClimateCardTypes } from "./cards/climate";
import { registerClockCardTypes } from "./cards/clock";
import { registerCoverLikeCardHelpers } from "./cards/cover_like_card";
import { registerDoorWindowCardTypes } from "./cards/door_window";
import { registerEntityModeCardHelpers } from "./cards/entity_mode_card";
import { registerFanCardTypes } from "./cards/fan";
import { registerGarageCardTypes } from "./cards/garage";
import { registerGateCardTypes } from "./cards/gate";
import { registerImageCardTypes } from "./cards/image";
import { registerInternalCardTypes } from "./cards/internal";
import { registerLawnMowerCardTypes } from "./cards/lawn_mower";
import { registerLightTemperatureCardTypes } from "./cards/light_temperature";
import { registerLockCardTypes } from "./cards/lock";
import { registerMediaCardTypes } from "./cards/media";
import { registerPresenceCardTypes } from "./cards/presence";
import { registerPushCardTypes } from "./cards/push";
import { registerScreenLockCardTypes } from "./cards/screen_lock";
import { registerSensorCardTypes } from "./cards/sensor";
import { registerSliderCardTypes } from "./cards/slider";
import { registerSubpageCardTypes } from "./cards/subpage";
import { registerSwitchCardTypes } from "./cards/switch";
import { registerTimezoneCardTypes } from "./cards/timezone";
import { registerVacuumCardTypes } from "./cards/vacuum";
import { registerWeatherCardTypes } from "./cards/weather";
import { registerWeatherForecastCardTypes } from "./cards/weather_forecast";
import { registerWebhookCardTypes } from "./cards/webhook";
import { installAppTestHooks } from "./testing/app_test_hooks";
import { installAppTestHooksConfig } from "./testing/app_test_hooks_config";
import { installAppTestHooksPreview } from "./testing/app_test_hooks_preview";
import { installAppTestHooksBackup } from "./testing/app_test_hooks_backup";
import { installAppTestHooksSettings } from "./testing/app_test_hooks_settings";

declare const __ESPCONTROL_TEST_HOOKS_ENABLED__: boolean;

const startupState = globalThis as typeof globalThis & {
  __ESPCONTROL_START_EMBEDDED__?: () => void;
  __ESPCONTROL_RELOAD_EMBEDDED__?: () => void;
  __ESPCONTROL_UI_STARTED__?: boolean;
  __ESPCONTROL_UI_STARTING__?: boolean;
};

const applicationBootstrapModules: readonly EditorBootstrapModule[] = [
  { name: "core", install: installCore },
  { name: "firmware-metadata", install: installFirmwareMetadataModule },
  { name: "styles", install: installStylesModule },
  { name: "state", install: installStateModule },
  { name: "language-state", install: installLanguageStateModule },
  { name: "environment-state", install: installEnvironmentStateModule },
  { name: "screen-rotation-state", install: installScreenRotationStateModule },
  { name: "screen-schedule-state", install: installScreenScheduleStateModule },
  { name: "ntp-state", install: installNtpStateModule },
  { name: "appearance-state", install: installAppearanceStateModule },
  { name: "idle-state", install: installIdleStateModule },
  { name: "artwork-state", install: installArtworkStateModule },
  { name: "screensaver-state", install: installScreensaverStateModule },
  { name: "firmware-version-state", install: installFirmwareVersionStateModule },
  { name: "entity-state", install: installEntityStateModule },
  { name: "clock-bar-state", install: installClockBarStateModule },
  { name: "firmware-update-state", install: installFirmwareUpdateStateModule },
  { name: "screensaver-timeout", install: installScreensaverTimeoutModule },
  { name: "c6-firmware-ui", install: installC6FirmwareUiModule },
  { name: "grid", install: installGridModule },
  { name: "native-panel-config-migration", install: installNativePanelConfigMigrationModule },
  { name: "api", install: installApiModule },
  { name: "firmware-update-post-api", install: installFirmwareUpdatePostApiModule },
  { name: "public-firmware-install", install: installPublicFirmwareInstallModule },
  { name: "config-option-core", install: installConfigOptionCoreModule },
  { name: "config-media-options", install: installConfigMediaOptionsModule },
  { name: "config-image-options", install: installConfigImageOptionsModule },
  { name: "config-modal-tab-options", install: installConfigModalTabOptionsModule },
  { name: "config-subpage-options", install: installConfigSubpageOptionsModule },
  { name: "config-sensor-options", install: installConfigSensorOptionsModule },
  { name: "config-confirmation-options", install: installConfigConfirmationOptionsModule },
  { name: "config-access-climate-alarm-options", install: installConfigAccessClimateAlarmOptionsModule },
  { name: "config-codec", install: installConfigCodecModule },
  { name: "config-post-api", install: installConfigPostApiModule },
  { name: "state-loader-api", install: installStateLoaderApiModule },
  { name: "artwork-post-api", install: installArtworkPostApiModule },
  { name: "screen-schedule-post-api", install: installScreenSchedulePostApiModule },
  { name: "clock-bar-post-api", install: installClockBarPostApiModule },
  { name: "controls", install: installControlsModule },
  { name: "controls-shell", install: installControlsShellModule },
  { name: "settings-page-helpers", install: installSettingsPageHelpersModule },
  { name: "settings-schedule-section", install: installSettingsScheduleSectionModule },
  { name: "settings-cover-art-section", install: installSettingsCoverArtSectionModule },
  { name: "settings-system-section", install: installSettingsSystemSectionModule },
  { name: "settings-page", install: installSettingsPageModule },
  { name: "controls-fields", install: installControlsFieldsModule },
  { name: "preview-render", install: installPreviewRenderModule },
  { name: "button-settings-selection", install: installButtonSettingsSelectionModule },
  { name: "button-settings-render-queue", install: installButtonSettingsRenderQueueModule },
  { name: "button-settings-icon-picker", install: installButtonSettingsIconPickerModule },
  { name: "button-settings", install: installButtonSettingsModule },
  { name: "preview-grid-placement", install: installPreviewGridPlacementModule },
  { name: "preview-context-menu", install: installPreviewContextMenuModule },
  { name: "preview-clipboard", install: installPreviewClipboardModule },
  { name: "preview-interactions", install: installPreviewInteractionsModule },
  { name: "backup-contract", install: installBackupContractModule },
  { name: "app-backup", install: installAppBackupModule },
  { name: "app-status-preview", install: installAppStatusPreviewModule },
  { name: "app-title", install: installAppTitleModule },
  { name: "app-config-events", install: installAppConfigEventsModule },
  { name: "app-state-event-handlers", install: installAppStateEventHandlersModule },
  { name: "app-events", install: installAppEventsModule },
  { name: "app", install: installAppModule },
];

const cardBootstrapModules: readonly EditorBootstrapModule[] = [
  { name: "card-action", install: registerActionCardTypes },
  { name: "card-alarm", install: registerAlarmCardTypes },
  { name: "card-calendar", install: registerCalendarCardTypes },
  { name: "card-companion", install: registerCompanionCardTypes },
  { name: "card-climate", install: registerClimateCardTypes },
  { name: "card-clock", install: registerClockCardTypes },
  { name: "card-cover-like", install: registerCoverLikeCardHelpers },
  { name: "card-door-window", install: registerDoorWindowCardTypes },
  { name: "card-entity-mode", install: registerEntityModeCardHelpers },
  { name: "card-fan", install: registerFanCardTypes },
  { name: "card-garage", install: registerGarageCardTypes },
  { name: "card-gate", install: registerGateCardTypes },
  { name: "card-image", install: registerImageCardTypes },
  { name: "card-internal", install: registerInternalCardTypes },
  { name: "card-lawn-mower", install: registerLawnMowerCardTypes },
  { name: "card-light-temperature", install: registerLightTemperatureCardTypes },
  { name: "card-lock", install: registerLockCardTypes },
  { name: "card-media", install: registerMediaCardTypes },
  { name: "card-presence", install: registerPresenceCardTypes },
  { name: "card-push", install: registerPushCardTypes },
  { name: "card-screen-lock", install: registerScreenLockCardTypes },
  { name: "card-sensor", install: registerSensorCardTypes },
  { name: "card-slider", install: registerSliderCardTypes },
  { name: "card-subpage", install: registerSubpageCardTypes },
  { name: "card-switch", install: registerSwitchCardTypes },
  { name: "card-timezone", install: registerTimezoneCardTypes },
  { name: "card-vacuum", install: registerVacuumCardTypes },
  { name: "card-weather", install: registerWeatherCardTypes },
  { name: "card-weather-forecast", install: registerWeatherForecastCardTypes },
  { name: "card-webhook", install: registerWebhookCardTypes },
];

const testHookBootstrapModules: readonly EditorBootstrapModule[] = [
  { name: "test-hooks", install: installAppTestHooks },
  { name: "test-hooks-config", install: installAppTestHooksConfig },
  { name: "test-hooks-preview", install: installAppTestHooksPreview },
  { name: "test-hooks-backup", install: installAppTestHooksBackup },
  { name: "test-hooks-settings", install: installAppTestHooksSettings },
];

function startEspControl(): void {
  if (startupState.__ESPCONTROL_UI_STARTED__ || startupState.__ESPCONTROL_UI_STARTING__) return;
  AppInstance.initializeAppState();
  installStaticGlobals({
    ...DeviceConfig,
    EspControlModel: Model,
    ...Model,
    ...DeviceApi,
    ...RequestFailure,
    PreviewGridFeature,
    PreviewFeature,
    ClipboardFeature,
    createBackupFeature: BackupFeature.createBackupFeature,
    createBackupExportController: BackupExportController.createBackupExportController,
    createSettingsUiFeature: SettingsFeature.createSettingsUiFeature,
    createAlarmDelayAudioController: AlarmDelayAudioController.createAlarmDelayAudioController,
    createScreensaverController: ScreensaverController.createScreensaverController,
    createCoverArtScreensaverController: CoverArtScreensaverController.createCoverArtScreensaverController,
    createMediaPlaybackController: MediaPlaybackController.createMediaPlaybackController,
    createVoiceServicesController: VoiceServicesController.createVoiceServicesController,
    createClockBarController: ClockBarController.createClockBarController,
    createScreenScheduleController: ScreenScheduleController.createScreenScheduleController,
    screensaverControlState: SettingsFeature.screensaverControlState,
    timedSettingLabel: SettingsFeature.timedSettingLabel,
    ...UiTokens,
    ...AppState,
    ...EventAliases,
    ...EventState,
    ...FirmwareEvents,
    ...ConfigPrimitives,
    ...CardContract,
    ...Icons,
    ENTITY_CATALOG,
    defaultTimezoneOptions: () =>
      AppState.defaultTimezoneOptionsForDevice(DeviceConfig.deviceConfig),
  });

  const installedModules = new Set<string>();
  installEditorBootstrap(applicationBootstrapModules, undefined, installedModules);
  installEditorBootstrap(cardBootstrapModules, undefined, installedModules);
  if (__ESPCONTROL_TEST_HOOKS_ENABLED__) {
    installEditorBootstrap(testHookBootstrapModules, undefined, installedModules);
  }
  installEditorBootstrap([{ name: "app-start", install: installAppStartModule }], undefined, installedModules);
}

function startEmbeddedFallback(error: unknown): void {
  console.error("Unable to start EspControl", error);
  startupState.__ESPCONTROL_UI_STARTED__ = false;
  startupState.__ESPCONTROL_UI_STARTING__ = false;
  const reload = startupState.__ESPCONTROL_RELOAD_EMBEDDED__;
  if (typeof reload === "function") {
    reload();
    return;
  }
  const start = startupState.__ESPCONTROL_START_EMBEDDED__;
  if (typeof start === "function") start();
}

const deviceConfigReady = DeviceConfig.initializeDeviceConfig();
if (deviceConfigReady) {
  void deviceConfigReady.then(startEspControl).catch(startEmbeddedFallback);
} else {
  try {
    startEspControl();
  } catch (error) {
    startEmbeddedFallback(error);
  }
}
