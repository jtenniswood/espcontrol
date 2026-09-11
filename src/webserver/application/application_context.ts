import type { DeviceApi } from "../api/device_api";
import type { NativePanelConfigController } from "../controllers/native_panel_config_controller";
import type { ReconnectController } from "../features/reconnect";
import type { CardEditorDraftController } from "../features/card_editor_draft_controller";
import type { CardEditorSaveController } from "../features/card_editor_save_controller";
import type { CardEditorValidationController } from "../features/card_editor_validation_controller";
import type { PreviewPlacementController } from "../features/preview_placement_controller";
import type { AlarmDelayAudioController } from "../features/alarm_delay_audio_controller";
import type { ClockBarController } from "../features/clock_bar_controller";
import type { CoverArtScreensaverController } from "../features/cover_art_screensaver_controller";
import type { MediaPlaybackController } from "../features/media_playback_controller";
import type { ScreenScheduleController } from "../features/screen_schedule_controller";
import type { ScreensaverController } from "../features/screensaver_controller";
import type { SettingsUiFeature } from "../features/settings";
import type { VoiceServicesController } from "../features/voice_services_controller";
import type { BackupFeature } from "../features/backup";
import type { BackupExportController } from "../features/backup_export_controller";
import type { BackupFileController } from "../features/backup_file_controller";
import type { BackupImportController } from "../features/backup_import_controller";
import type { BackupRestoreController } from "../features/backup_restore_controller";
import type { AppBackupFeature } from "./app_backup";
import type { DeviceConfig, AppState } from "../state/types";
import type { ConfigPersistenceFeature } from "./config_post_api";
import type { CardRegistry } from "./card_registry";
import type { ConfigSensorOptionsFeature } from "./config_sensor_options";
import type { ConfigMediaOptionsFeature } from "./config_media_options";
import type { ConfigImageOptionsFeature } from "./config_image_options";
import type { ConfigWeatherOptionsFeature } from "./config_weather_options";
import type { ConfigWebhookOptionsFeature } from "./config_webhook_options";
import type { ConfigInternalRelayOptionsFeature } from "./config_internal_relay_options";
import type { ConfigRobotCardOptionsFeature } from "./config_robot_card_options";
import type { ConfigLockOptionsFeature } from "./config_lock_options";
import type { ConfigDateTimeOptionsFeature } from "./config_date_time_options";
import type { ConfigModalTabOptionsFeature } from "./config_modal_tab_options";
import type { ConfigAccessClimateAlarmOptionsFeature } from "./config_access_climate_alarm_options";
import type { ConfigConfirmationOptionsFeature } from "./config_confirmation_options";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { AppTitleFeature } from "./app_title";
import type { CoreFeature } from "./core";
import type { EnvironmentStateFeature } from "./environment_state";
import type { ScreenScheduleStateFeature } from "./screen_schedule_state";
import type { ScreensaverTimeoutFeature } from "./screensaver_timeout";
import type { ScreenRotationFeature } from "./screen_rotation_state";
import type { AppearanceFeature } from "./appearance_state";
import type { FirmwareVersionFeature } from "./firmware_version_state";
import type { FirmwareUpdateFeature } from "./firmware_update_state";
import type { C6FirmwareFeature } from "./c6_firmware_ui";
import type { ClockBarFeature } from "./clock_bar_state";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { ApplicationApiFeature } from "./api";
import type { StateLoaderFeature } from "./state_loader_api";
import type { GridMigrationFeature } from "./grid_migration";
import type { AppConfigEventsFeature } from "./app_config_events";
import type { AppStateEventHandlersFeature } from "./app_state_event_handlers";
import type { AppEventsFeature } from "./app_events";
import type { AppStatusPreviewFeature } from "./app_status_preview";
import type { GridFeature } from "./grid";
import type { FirmwareUpdatePostApiFeature } from "./firmware_update_post_api";
import type { ArtworkPostApiFeature } from "./artwork_post_api";
import type { ScreenSchedulePostApiFeature } from "./screen_schedule_post_api";
import type { ClockBarPostApiFeature } from "./clock_bar_post_api";
import type { PublicFirmwareInstallFeature } from "./public_firmware_install";
import type { ButtonSettingsIconPickerFeature } from "./button_settings_icon_picker";
import type { ButtonSettingsRenderQueueFeature } from "./button_settings_render_queue";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { ButtonSettingsSelectionFeature } from "./button_settings_selection";
import type { PreviewRenderFeature } from "./preview_render";
import type { PreviewGridPlacementFeature } from "./preview_grid_placement";
import type { PreviewClipboardFeature } from "./preview_clipboard";
import type { PreviewContextMenuFeature } from "./preview_context_menu";
import type { PreviewInteractionsFeature } from "./preview_interactions";
import type { ButtonSettingsFeature } from "./button_settings";
import type { AppFeature } from "./app";

export type { CardRegistry } from "./card_registry";

export interface ApplicationLayoutState {
  deviceId: string;
  config: DeviceConfig;
  numSlots: number;
  totalSlots: number;
  gridCols: number;
  gridRows: number;
}

export interface ApplicationDomServices {
  readonly document: Document;
  readonly window: Window;
  readonly fetch: typeof fetch;
  readonly createEventSource: () => EventSource;
  readonly schedule: typeof setTimeout;
}

export interface ApplicationContext {
  readonly device: {
    readonly id: string;
    readonly profile: DeviceConfig;
  };
  readonly model: typeof import("../model");
  readonly state: AppState;
  readonly runtime: UiRuntimeState;
  readonly core: CoreFeature;
  readonly layout: ApplicationLayoutState;
  readonly api: DeviceApi;
  readonly configuration: {
    readonly native: NativePanelConfigController;
    readonly persistence: ConfigPersistenceFeature;
    readonly options: ConfigSensorOptionsFeature;
    readonly mediaOptions: ConfigMediaOptionsFeature;
    readonly imageOptions: ConfigImageOptionsFeature;
    readonly weatherOptions: ConfigWeatherOptionsFeature;
    readonly webhookOptions: ConfigWebhookOptionsFeature;
    readonly internalRelayOptions: ConfigInternalRelayOptionsFeature;
    readonly robotOptions: ConfigRobotCardOptionsFeature;
    readonly lockOptions: ConfigLockOptionsFeature;
    readonly dateTimeOptions: ConfigDateTimeOptionsFeature;
    readonly modalTabs: ConfigModalTabOptionsFeature;
    readonly accessClimateAlarm: ConfigAccessClimateAlarmOptionsFeature;
    readonly confirmationOptions: ConfigConfirmationOptionsFeature;
    readonly codec: ConfigCodecFeature;
  };
  readonly backup: {
    readonly contract: BackupFeature;
    readonly export: BackupExportController;
    readonly file: BackupFileController;
    readonly import: BackupImportController<any, any, any>;
    readonly restore: BackupRestoreController<any, any>;
    readonly application: AppBackupFeature;
  };
  readonly controllers: {
    readonly appearance: AppearanceFeature;
    readonly firmwareVersion: FirmwareVersionFeature;
    readonly firmwareUpdate: FirmwareUpdateFeature;
    readonly firmwarePostApi: FirmwareUpdatePostApiFeature;
    readonly artworkPostApi: ArtworkPostApiFeature;
    readonly schedulePostApi: ScreenSchedulePostApiFeature;
    readonly clockBarPostApi: ClockBarPostApiFeature;
    readonly publicFirmwareInstall: PublicFirmwareInstallFeature;
    readonly c6Firmware: C6FirmwareFeature;
    readonly clockBarState: ClockBarFeature;
    readonly entityState: EntityStateFeature;
    readonly shell: ControlsShellFeature;
    readonly requestApi: ApplicationApiFeature;
    readonly stateLoader: StateLoaderFeature;
    readonly gridMigration: GridMigrationFeature;
    readonly configEvents: AppConfigEventsFeature;
    readonly stateEventHandlers: AppStateEventHandlersFeature;
    readonly appEvents: AppEventsFeature;
    readonly statusPreview: AppStatusPreviewFeature;
    readonly grid: GridFeature;
    readonly alarmDelayAudio: AlarmDelayAudioController;
    readonly cardEditorDraft: CardEditorDraftController;
    readonly cardEditorSave: CardEditorSaveController;
    readonly cardEditorValidation: CardEditorValidationController;
    readonly clockBar: ClockBarController;
    readonly coverArtScreensaver: CoverArtScreensaverController;
    readonly mediaPlayback: MediaPlaybackController;
    readonly pageTitle: AppTitleFeature;
    readonly previewPlacement: PreviewPlacementController;
    readonly reconnect: ReconnectController<unknown>;
    readonly screenSchedule: ScreenScheduleController;
    readonly screenScheduleState: ScreenScheduleStateFeature;
    readonly screenRotation: ScreenRotationFeature;
    readonly screensaver: ScreensaverController;
    readonly screensaverTimeout: ScreensaverTimeoutFeature;
    readonly settingsUi: SettingsUiFeature;
    readonly voiceServices: VoiceServicesController;
    readonly environment: EnvironmentStateFeature;
    readonly iconPicker: ButtonSettingsIconPickerFeature;
    readonly renderQueue: ButtonSettingsRenderQueueFeature;
    readonly fields: ControlsFieldsFeature;
    readonly selection: ButtonSettingsSelectionFeature;
    readonly preview: PreviewRenderFeature;
    readonly placement: PreviewGridPlacementFeature;
    readonly clipboard: PreviewClipboardFeature;
    readonly contextMenu: PreviewContextMenuFeature;
    readonly interactions: PreviewInteractionsFeature;
    readonly buttonSettings: ButtonSettingsFeature;
    readonly app: AppFeature;
  };
  readonly dom: ApplicationDomServices;
  readonly cards: CardRegistry;
}

export interface ApplicationContextOptions {
  readonly layout: ApplicationLayoutState;
  readonly model: typeof import("../model");
  readonly state: AppState;
  readonly runtime: UiRuntimeState;
  readonly core: CoreFeature;
  readonly api: DeviceApi;
  readonly nativeConfiguration: NativePanelConfigController;
  readonly configurationPersistence: ConfigPersistenceFeature;
  readonly configurationOptions: ConfigSensorOptionsFeature;
  readonly mediaConfigurationOptions: ConfigMediaOptionsFeature;
  readonly imageConfigurationOptions: ConfigImageOptionsFeature;
  readonly weatherConfigurationOptions: ConfigWeatherOptionsFeature;
  readonly webhookConfigurationOptions: ConfigWebhookOptionsFeature;
  readonly internalRelayConfigurationOptions: ConfigInternalRelayOptionsFeature;
  readonly robotConfigurationOptions: ConfigRobotCardOptionsFeature;
  readonly lockConfigurationOptions: ConfigLockOptionsFeature;
  readonly dateTimeConfigurationOptions: ConfigDateTimeOptionsFeature;
  readonly modalTabOptions: ConfigModalTabOptionsFeature;
  readonly accessClimateAlarmOptions: ConfigAccessClimateAlarmOptionsFeature;
  readonly confirmationOptions: ConfigConfirmationOptionsFeature;
  readonly configurationCodec: ConfigCodecFeature;
  readonly backupContract: BackupFeature;
  readonly backupExport: BackupExportController;
  readonly backupFile: BackupFileController;
  readonly backupImport: BackupImportController<any, any, any>;
  readonly backupRestore: BackupRestoreController<any, any>;
  readonly backupApplication: AppBackupFeature;
  readonly appearance: AppearanceFeature;
  readonly firmwareVersion: FirmwareVersionFeature;
  readonly firmwareUpdate: FirmwareUpdateFeature;
  readonly firmwarePostApi: FirmwareUpdatePostApiFeature;
  readonly artworkPostApi: ArtworkPostApiFeature;
  readonly schedulePostApi: ScreenSchedulePostApiFeature;
  readonly clockBarPostApi: ClockBarPostApiFeature;
  readonly publicFirmwareInstall: PublicFirmwareInstallFeature;
  readonly c6Firmware: C6FirmwareFeature;
  readonly clockBarState: ClockBarFeature;
  readonly entityState: EntityStateFeature;
  readonly shell: ControlsShellFeature;
  readonly requestApi: ApplicationApiFeature;
  readonly stateLoader: StateLoaderFeature;
  readonly gridMigration: GridMigrationFeature;
  readonly configEvents: AppConfigEventsFeature;
  readonly stateEventHandlers: AppStateEventHandlersFeature;
  readonly appEvents: AppEventsFeature;
  readonly statusPreview: AppStatusPreviewFeature;
  readonly grid: GridFeature;
  readonly alarmDelayAudio: AlarmDelayAudioController;
  readonly cardEditorDraft: CardEditorDraftController;
  readonly cardEditorSave: CardEditorSaveController;
  readonly cardEditorValidation: CardEditorValidationController;
  readonly clockBar: ClockBarController;
  readonly coverArtScreensaver: CoverArtScreensaverController;
  readonly mediaPlayback: MediaPlaybackController;
  readonly pageTitle: AppTitleFeature;
  readonly previewPlacement: PreviewPlacementController;
  readonly reconnect: ReconnectController<unknown>;
  readonly screenSchedule: ScreenScheduleController;
  readonly screenScheduleState: ScreenScheduleStateFeature;
  readonly screenRotation: ScreenRotationFeature;
  readonly screensaver: ScreensaverController;
  readonly screensaverTimeout: ScreensaverTimeoutFeature;
  readonly settingsUi: SettingsUiFeature;
  readonly voiceServices: VoiceServicesController;
  readonly environment: EnvironmentStateFeature;
  readonly iconPicker: ButtonSettingsIconPickerFeature;
  readonly renderQueue: ButtonSettingsRenderQueueFeature;
  readonly fields: ControlsFieldsFeature;
  readonly selection: ButtonSettingsSelectionFeature;
  readonly preview: PreviewRenderFeature;
  readonly placement: PreviewGridPlacementFeature;
  readonly clipboard: PreviewClipboardFeature;
  readonly contextMenu: PreviewContextMenuFeature;
  readonly interactions: PreviewInteractionsFeature;
  readonly buttonSettings: ButtonSettingsFeature;
  readonly app: AppFeature;
  readonly dom: ApplicationDomServices;
  readonly cards: CardRegistry;
}

export function createApplicationLayoutState(
  deviceId: string,
  config: DeviceConfig,
): ApplicationLayoutState {
  return {
    deviceId,
    config,
    numSlots: config.slots,
    totalSlots: config.slots,
    gridCols: config.cols,
    gridRows: config.rows,
  };
}

export function createApplicationContext(options: ApplicationContextOptions): ApplicationContext {
  return {
    device: { id: options.layout.deviceId, profile: options.layout.config },
    model: options.model,
    state: options.state,
    runtime: options.runtime,
    core: options.core,
    layout: options.layout,
    api: options.api,
    configuration: {
      native: options.nativeConfiguration,
      persistence: options.configurationPersistence,
      options: options.configurationOptions,
      mediaOptions: options.mediaConfigurationOptions,
      imageOptions: options.imageConfigurationOptions,
      weatherOptions: options.weatherConfigurationOptions,
      webhookOptions: options.webhookConfigurationOptions,
      internalRelayOptions: options.internalRelayConfigurationOptions,
      robotOptions: options.robotConfigurationOptions,
      lockOptions: options.lockConfigurationOptions,
      dateTimeOptions: options.dateTimeConfigurationOptions,
      modalTabs: options.modalTabOptions,
      accessClimateAlarm: options.accessClimateAlarmOptions,
      confirmationOptions: options.confirmationOptions,
      codec: options.configurationCodec,
    },
    backup: {
      contract: options.backupContract,
      export: options.backupExport,
      file: options.backupFile,
      import: options.backupImport,
      restore: options.backupRestore,
      application: options.backupApplication,
    },
    controllers: {
      appearance: options.appearance,
      firmwareVersion: options.firmwareVersion,
      firmwareUpdate: options.firmwareUpdate,
      firmwarePostApi: options.firmwarePostApi,
      artworkPostApi: options.artworkPostApi,
      schedulePostApi: options.schedulePostApi,
      clockBarPostApi: options.clockBarPostApi,
      publicFirmwareInstall: options.publicFirmwareInstall,
      c6Firmware: options.c6Firmware,
      clockBarState: options.clockBarState,
      entityState: options.entityState,
      shell: options.shell,
      requestApi: options.requestApi,
      stateLoader: options.stateLoader,
      gridMigration: options.gridMigration,
      configEvents: options.configEvents,
      stateEventHandlers: options.stateEventHandlers,
      appEvents: options.appEvents,
      statusPreview: options.statusPreview,
      grid: options.grid,
      alarmDelayAudio: options.alarmDelayAudio,
      cardEditorDraft: options.cardEditorDraft,
      cardEditorSave: options.cardEditorSave,
      cardEditorValidation: options.cardEditorValidation,
      clockBar: options.clockBar,
      coverArtScreensaver: options.coverArtScreensaver,
      mediaPlayback: options.mediaPlayback,
      pageTitle: options.pageTitle,
      previewPlacement: options.previewPlacement,
      reconnect: options.reconnect,
      screenSchedule: options.screenSchedule,
      screenScheduleState: options.screenScheduleState,
      screenRotation: options.screenRotation,
      screensaver: options.screensaver,
      screensaverTimeout: options.screensaverTimeout,
      settingsUi: options.settingsUi,
      voiceServices: options.voiceServices,
      environment: options.environment,
      iconPicker: options.iconPicker,
      renderQueue: options.renderQueue,
      fields: options.fields,
      selection: options.selection,
      preview: options.preview,
      placement: options.placement,
      clipboard: options.clipboard,
      contextMenu: options.contextMenu,
      interactions: options.interactions,
      buttonSettings: options.buttonSettings,
      app: options.app,
    },
    dom: options.dom,
    cards: options.cards,
  };
}
