import * as DeviceConfig from "./device_config";
import * as Model from "./model";
import { createDeviceApi } from "./api/device_api";
import { NTP_SERVER_DEFAULTS, defaultTimezoneOptionsForDevice } from "./state/app_state";
import * as AppInstance from "./state/app_instance";
import { state } from "./state/app_instance";
import { textSpan } from "./application/ui_primitives";
import { createCoreFeature } from "./application/core";
import {
  createApplicationContext,
  createApplicationLayoutState,
  type ApplicationContext,
  type ApplicationDomServices,
} from "./application/application_context";
import { createCardRegistry } from "./application/card_registry";
import { createWebStyles } from "./application/styles";
import { createUiRuntimeState } from "./application/state";
import { createEnvironmentStateFeature } from "./application/environment_state";
import { createScreenRotationFeature } from "./application/screen_rotation_state";
import { createScreenScheduleStateFeature } from "./application/screen_schedule_state";
import { createAppearanceFeature } from "./application/appearance_state";
import { createFirmwareVersionFeature } from "./application/firmware_version_state";
import { createEntityStateFeature } from "./application/entity_state";
import { createClockBarFeature, type ClockBarFeature } from "./application/clock_bar_state";
import { createFirmwareUpdateFeature, type FirmwareUpdateFeature } from "./application/firmware_update_state";
import { createScreensaverTimeoutFeature } from "./application/screensaver_timeout";
import { createC6FirmwareFeature, type C6FirmwareFeature } from "./application/c6_firmware_ui";
import { createGridFeature } from "./application/grid";
import {
  createApplicationApiFeature,
  type ApplicationApiFeature,
} from "./application/api";
import { createFirmwareUpdatePostApiFeature, type FirmwareUpdatePostApiFeature } from "./application/firmware_update_post_api";
import { createPublicFirmwareInstallFeature, type PublicFirmwareInstallFeature } from "./application/public_firmware_install";
import { createConfigMediaOptionsFeature } from "./application/config_media_options";
import { createConfigImageOptionsFeature } from "./application/config_image_options";
import { createConfigWeatherOptionsFeature } from "./application/config_weather_options";
import { createConfigWebhookOptionsFeature } from "./application/config_webhook_options";
import { createConfigInternalRelayOptionsFeature } from "./application/config_internal_relay_options";
import { createConfigRobotCardOptionsFeature } from "./application/config_robot_card_options";
import { createConfigLockOptionsFeature } from "./application/config_lock_options";
import { createConfigDateTimeOptionsFeature } from "./application/config_date_time_options";
import { createConfigModalTabOptionsFeature } from "./application/config_modal_tab_options";
import { createConfigSensorOptionsFeature } from "./application/config_sensor_options";
import { createConfigConfirmationOptionsFeature } from "./application/config_confirmation_options";
import { createConfigAccessClimateAlarmOptionsFeature } from "./application/config_access_climate_alarm_options";
import { createConfigCodecFeature } from "./application/config_codec";
import { createNativePanelConfigMigrationController } from "./application/native_panel_config_migration";
import { createConfigPersistenceFeature } from "./application/config_post_api";
import { createStateLoaderFeature, type StateLoaderFeature } from "./application/state_loader_api";
import { createGridMigrationFeature } from "./application/grid_migration";
import { createArtworkPostApiFeature } from "./application/artwork_post_api";
import { createScreenSchedulePostApiFeature } from "./application/screen_schedule_post_api";
import { createClockBarPostApiFeature } from "./application/clock_bar_post_api";
import { createControlsShellFeature } from "./application/controls_shell";
import { createSettingsPageHelpersFeature, type SettingsPageHelpersFeature } from "./application/settings_page_helpers";
import { createSettingsScheduleSectionFeature } from "./application/settings_schedule_section";
import { createSettingsCoverArtSectionFeature } from "./application/settings_cover_art_section";
import { createSettingsSystemSectionFeature } from "./application/settings_system_section";
import { createSettingsCompanionSectionFeature } from "./application/settings_companion_section";
import { createConnectorsPageFeature, type ConnectorsPageFeature } from "./application/connectors_page";
import { createSettingsPageFeature, type SettingsPageFeature } from "./application/settings_page";
import { createControlsFieldsFeature, type ControlsFieldsFeature } from "./application/controls_fields";
import { createPreviewRenderFeature, type PreviewRenderFeature } from "./application/preview_render";
import { createButtonSettingsSelectionFeature, type ButtonSettingsSelectionFeature } from "./application/button_settings_selection";
import { createButtonSettingsRenderQueueFeature } from "./application/button_settings_render_queue";
import { createButtonSettingsIconPickerFeature } from "./application/button_settings_icon_picker";
import { createButtonSettingsFeature, type ButtonSettingsFeature } from "./application/button_settings";
import { createPreviewGridPlacementFeature } from "./application/preview_grid_placement";
import { createPreviewContextMenuFeature, type PreviewContextMenuFeature } from "./application/preview_context_menu";
import { createPreviewClipboardFeature } from "./application/preview_clipboard";
import { createPreviewInteractionsFeature, type PreviewInteractionsFeature } from "./application/preview_interactions";
import { createCardEditorDraftController } from "./features/card_editor_draft_controller";
import { createCardEditorValidationController } from "./features/card_editor_validation_controller";
import { createCardEditorSaveController } from "./features/card_editor_save_controller";
import { createPreviewPlacementController } from "./features/preview_placement_controller";
import { createClockBarController } from "./features/clock_bar_controller";
import { createVoiceServicesController } from "./features/voice_services_controller";
import { createScreenScheduleController } from "./features/screen_schedule_controller";
import { createSettingsUiFeature } from "./features/settings";
import { createAlarmDelayAudioController } from "./features/alarm_delay_audio_controller";
import { createScreensaverController } from "./features/screensaver_controller";
import { createCoverArtScreensaverController } from "./features/cover_art_screensaver_controller";
import { createMediaPlaybackController } from "./features/media_playback_controller";
import { createBackupImportController } from "./features/backup_import_controller";
import { createBackupExportController } from "./features/backup_export_controller";
import { createBackupFileController } from "./features/backup_file_controller";
import { createBackupRestoreController } from "./features/backup_restore_controller";
import { createBackupFeature } from "./features/backup";
import { createBackupContractFeature } from "./application/backup_contract";
import { createAppBackupFeature } from "./application/app_backup";
import { createAppStatusPreviewFeature, type AppStatusPreviewFeature } from "./application/app_status_preview";
import { createAppTitleFeature } from "./application/app_title";
import { createAppConfigEventsFeature } from "./application/app_config_events";
import { createAppStateEventHandlersFeature } from "./application/app_state_event_handlers";
import { createAppEventsFeature, type AppEventsFeature } from "./application/app_events";
import { createAppFeature, type AppFeature } from "./application/app";
import { startApp } from "./application/app_start";
import { createReconnectController } from "./features/reconnect";
import { registerActionCardTypes } from "./cards/action";
import { registerAlarmCardTypes } from "./cards/alarm";
import { registerCalendarCardTypes } from "./cards/calendar";
import { registerCompanionCardTypes } from "./cards/companion";
import { registerClimateCardTypes } from "./cards/climate";
import { registerClockCardTypes } from "./cards/clock";
import { createCoverLikeCardRegistration } from "./cards/cover_like_card";
import { registerDoorWindowCardTypes } from "./cards/door_window";
import { registerFanCardTypes } from "./cards/fan";
import { registerGarageCardTypes } from "./cards/garage";
import { registerGateCardTypes } from "./cards/gate";
import { registerImageCardTypes } from "./cards/image";
import { registerWifiQrCardTypes } from "./cards/wifi_qr";
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
import { createAppTestHookRegistrar } from "./testing/app_test_hooks";
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

function registerCards(context: ApplicationContext) {
  const registry = context.cards;
  const fields = context.controllers.fields;
  const cardUi = {
    renderPreview: () => context.controllers.preview.render(),
    renderButtonSettings: (force?: boolean) => context.controllers.buttonSettings.render(force),
  };
  const coverLikeCards = createCoverLikeCardRegistration(registry, context.controllers.renderQueue, fields, cardUi);
  registerActionCardTypes(registry, context.configuration.confirmationOptions, context.controllers.entityState, fields, cardUi);
  registerAlarmCardTypes(registry, context.configuration.accessClimateAlarm, context.controllers.renderQueue, fields, cardUi);
  registerCalendarCardTypes(registry, context.configuration.dateTimeOptions, fields);
  registerCompanionCardTypes(
    registry,
    !!context.device.profile.features?.companion,
    context.dom.document,
    context.dom.fetch,
    fields,
    cardUi,
    context.configuration.codec,
    context.controllers.selection,
  );
  registerClimateCardTypes(
    registry,
    context.configuration.modalTabs,
    context.configuration.accessClimateAlarm,
    context.controllers.clockBarState,
    context.controllers.renderQueue,
    fields,
  );
  registerClockCardTypes(registry, context.configuration.dateTimeOptions, fields);
  registerDoorWindowCardTypes(registry, context.configuration.options, fields);
  registerFanCardTypes(registry, context.configuration.modalTabs, fields, cardUi);
  registerGarageCardTypes(
    coverLikeCards.register,
    context.configuration.accessClimateAlarm,
    context.configuration.confirmationOptions,
  );
  registerGateCardTypes(
    coverLikeCards.register,
    context.configuration.accessClimateAlarm,
  );
  registerImageCardTypes(
    registry,
    context.configuration.imageOptions,
    fields,
    cardUi,
  );
  registerWifiQrCardTypes(registry, context.configuration.modalTabs, fields, cardUi, context.configuration.native);
  registerInternalCardTypes(
    registry,
    context.configuration.internalRelayOptions,
    context.dom.document,
    fields,
  );
  registerLawnMowerCardTypes(registry, context.configuration.robotOptions, fields, cardUi);
  const lightCards = registerLightTemperatureCardTypes(registry, context.configuration.modalTabs, fields, cardUi);
  registerLockCardTypes(registry, context.configuration.lockOptions, fields, cardUi);
  registerMediaCardTypes(registry, context.configuration.mediaOptions, context.device.id, fields, context.controllers.settingsUi, cardUi);
  registerPresenceCardTypes(registry, context.configuration.options, fields);
  registerPushCardTypes(registry, fields);
  registerScreenLockCardTypes(registry, fields);
  registerSensorCardTypes(registry, context.configuration.options, fields, cardUi);
  registerSliderCardTypes(
    registry,
    context.configuration.modalTabs,
    lightCards,
    fields,
    context.controllers.settingsUi,
    !!context.device.profile.features?.companion,
    cardUi,
  );
  registerSubpageCardTypes(registry, context.configuration.codec, context.core, context.controllers.selection, fields, cardUi);
  registerSwitchCardTypes(registry, context.configuration.confirmationOptions, lightCards, fields);
  registerTimezoneCardTypes(registry, context.configuration.dateTimeOptions, context.dom.document, fields);
  registerVacuumCardTypes(registry, context.configuration.robotOptions, fields, cardUi);
  const weatherCards = registerWeatherCardTypes(registry, context.configuration.weatherOptions, context.controllers.clockBarState, fields, cardUi);
  registerWeatherForecastCardTypes(registry, weatherCards, context.controllers.clockBarState, fields);
  registerWebhookCardTypes(registry, context.configuration.webhookOptions, fields, cardUi);
  return lightCards;
}

function installTestHooks(context: ApplicationContext, lightCards: ReturnType<typeof registerLightTemperatureCardTypes>): void {
  const register = createAppTestHookRegistrar();
  installAppTestHooksConfig(
    context.cards,
    context.configuration.options,
    context.configuration.mediaOptions,
    context.configuration.imageOptions,
    context.configuration.weatherOptions,
    context.configuration.webhookOptions,
    context.configuration.internalRelayOptions,
    context.configuration.lockOptions,
    context.configuration.dateTimeOptions,
    context.configuration.modalTabs,
    context.configuration.accessClimateAlarm,
    context.configuration.confirmationOptions,
    context.configuration.codec,
    lightCards,
    context.core,
    context.layout,
    context.configuration.persistence,
    context.controllers.preview,
    context.controllers.clipboard,
    context.controllers.contextMenu,
    context.controllers.fields,
    register,
  );
  installAppTestHooksPreview(context.cards, context.configuration.codec, context.runtime, context.core, context.layout, context.controllers.screenRotation, context.controllers.firmwareVersion, context.controllers.statusPreview, context.controllers.grid, register);
  installAppTestHooksBackup(context.layout, context.backup.contract, context.backup.application, register);
  installAppTestHooksSettings(
    () => defaultTimezoneOptionsForDevice(context.device.profile),
    context.controllers.environment,
    context.controllers.screensaverTimeout,
    context.controllers.firmwareVersion,
    context.controllers.firmwareUpdate,
    context.controllers.clockBarState,
    context.controllers.entityState,
    context.controllers.requestApi,
    context.controllers.statusPreview,
    context.controllers.artworkPostApi,
    context.controllers.clockBarPostApi,
    context.controllers.publicFirmwareInstall,
    register,
  );
}

function composeApplicationContext(): ApplicationContext {
  const fetchService: typeof fetch = typeof fetch === "function"
    ? fetch.bind(globalThis)
    : (() => Promise.reject(new Error("Fetch is not available"))) as typeof fetch;
  const dom: ApplicationDomServices = {
    document,
    window,
    fetch: fetchService,
    createEventSource: () => new EventSource("/events"),
    schedule: ((callback: TimerHandler, delay?: number) => window.setTimeout(callback, delay)) as typeof setTimeout,
  };
  const deviceApi = createDeviceApi((url, init) =>
    dom.fetch(url, init as RequestInit));
  const pageTitle = createAppTitleFeature({
    document: dom.document,
    eventStreamEnabled: () => {
      try { return new URLSearchParams(dom.window.location.search).get("events") === "1"; }
      catch (_) { return false; }
    },
    eventSourceAvailable: () => typeof EventSource === "function",
    createEventSource: dom.createEventSource,
  });
  const layout = createApplicationLayoutState(
    DeviceConfig.deviceId,
    DeviceConfig.deviceConfig,
  );
  const runtime = createUiRuntimeState(layout, dom.document);
  let requestApi: ApplicationApiFeature;
  let stateLoader: StateLoaderFeature;
  let appEvents: AppEventsFeature;
  let selection: ButtonSettingsSelectionFeature;
  let preview: PreviewRenderFeature;
  let contextMenu: PreviewContextMenuFeature;
  let interactions: PreviewInteractionsFeature;
  let fields: ControlsFieldsFeature;
  let settingsHelpers: SettingsPageHelpersFeature;
  let settingsPage: SettingsPageFeature;
  let connectorsPage: ConnectorsPageFeature;
  let buttonSettings: ButtonSettingsFeature;
  let app: AppFeature;
  const shell = createControlsShellFeature(runtime, {
    document: dom.document,
    state: AppInstance.state,
    schedule: dom.schedule,
    cancelSchedule: (handle) => { dom.window.clearTimeout(handle); },
    buildSettingsPage: (parent) => { settingsPage.buildSettingsPage(parent); },
    buildConnectorsPage: (parent) => { connectorsPage.buildPage(parent); },
    closeSettings: () => { selection.closeSettings(); },
    postButtonPress: (name) => requestApi.postButtonPress(name),
    waitForReboot: () => { stateLoader.waitForReboot(); },
    hideContextMenu: () => { contextMenu.hide(); },
    hideSettingsOverlay: () => { selection.hideSettingsOverlay(); },
    clearPlaceholder: () => { interactions.clearPlaceholder(); },
    updatePreviewHint: () => { selection.updatePreviewHint(); },
    renderPreview: () => { preview.render(); },
  });
  let confirmationOptions: ReturnType<typeof createConfigConfirmationOptionsFeature>;
  let clockBarState: ClockBarFeature;
  let statusPreview: AppStatusPreviewFeature;
  const entityState = createEntityStateFeature({
    actionCardStateEntity: (button) => confirmationOptions.actionCardStateEntity(button),
    totalSlots: () => layout.totalSlots,
    clockBarTemperatureEntities: () => clockBarState.temperatureEntities(),
    textInput: (id, value, placeholder) => fields.textInput(id, value, placeholder),
  });
  const screenRotation = createScreenRotationFeature(runtime, layout, {
    applyButtonOrder: (value, skipSpanNormalization) => grid.applyButtonOrderValue(value, skipSpanNormalization),
    postNormalizedOrder: (value) => requestApi.postText(entityState.entityName("button_order"), value),
    renderPreview: () => preview.render(),
  });
  const appearance = createAppearanceFeature(runtime, {
    renderPreview: () => preview.render(),
    postOnColor: (value) => requestApi.postText(entityState.entityName("button_on_color"), value),
  });
  let firmwareUpdate: FirmwareUpdateFeature;
  let firmwarePostApi: FirmwareUpdatePostApiFeature;
  let publicFirmwareInstall: PublicFirmwareInstallFeature;
  let c6Firmware: C6FirmwareFeature;
  const firmwareVersion = createFirmwareVersionFeature(runtime, {
    syncVersionSelect: () => firmwareUpdate.syncVersionSelect(),
    renderUpdateStatus: () => firmwareUpdate.renderStatus(),
    stopInstallRefreshIfComplete: () => firmwareUpdate.stopInstallRefreshIfComplete(),
  });
  firmwareUpdate = createFirmwareUpdateFeature(runtime, layout.deviceId, firmwareVersion, {
    postInstall: () => firmwarePostApi.postFirmwareUpdateInstall(),
    refreshVersion: () => stateLoader.refreshFirmwareVersion(),
    installViaWebOta: (info) => { void publicFirmwareInstall.installPublicFirmwareViaWebOta(info); },
    c6UpdateKnownAvailable: () => c6Firmware.updateKnownAvailable(),
  });
  c6Firmware = createC6FirmwareFeature(runtime, firmwareUpdate);
  const voiceServices = createVoiceServicesController();
  const environment = createEnvironmentStateFeature(
    voiceServices,
    () => defaultTimezoneOptionsForDevice(layout.config),
    layout,
  );
  const nativePanelConfig = createNativePanelConfigMigrationController({
    deviceProfile: () => layout.deviceId,
    slotCount: () => layout.numSlots,
    entityName: (name) => entityState.entityName(name),
    entityNameForSlot: (name, slot) => entityState.entityNameForSlot(name, slot),
    normalizeHexColor: (value, fallback) => Model.normalizeHexColor(value, fallback),
    showBanner: shell.showBanner,
    delay: (callback, milliseconds) => dom.schedule(callback, milliseconds),
  });
  const configurationPersistence = createConfigPersistenceFeature(nativePanelConfig, runtime, layout, entityState, shell);
  const cards = createCardRegistry();
  const iconPicker = createButtonSettingsIconPickerFeature(dom.document, () => preview.render());
  const renderQueue = createButtonSettingsRenderQueueFeature(runtime, {
    document: dom.document,
    requestFrame: (callback) => requestAnimationFrame(callback),
    renderPreview: () => preview.render(),
    renderButtonSettings: () => buttonSettings.render(),
    closeSettings: () => selection.closeSettings(),
  });
  const configurationOptions = createConfigSensorOptionsFeature(cards);
  const mediaConfigurationOptions = createConfigMediaOptionsFeature(layout.config);
  const imageConfigurationOptions = createConfigImageOptionsFeature({
    layout,
    mediaOptions: mediaConfigurationOptions,
    showBanner: shell.showBanner,
  });
  const weatherConfigurationOptions = createConfigWeatherOptionsFeature(layout.config);
  const webhookConfigurationOptions = createConfigWebhookOptionsFeature();
  const internalRelayConfigurationOptions = createConfigInternalRelayOptionsFeature(layout.config);
  const robotConfigurationOptions = createConfigRobotCardOptionsFeature();
  const lockConfigurationOptions = createConfigLockOptionsFeature();
  let configurationCodec: ReturnType<typeof createConfigCodecFeature>;
  const core = createCoreFeature(
    layout,
    (subpage) => configurationCodec.serializeSubpageGrid(subpage),
    runtime,
    {
      state: AppInstance.state,
      document: dom.document,
      clockBarVisibleInPreview: () => clockBarState.visibleInPreview(),
      postButtonOrder: (value) => requestApi.postText(entityState.entityName("button_order"), value),
      saveSubpage: (homeSlot) => configurationPersistence.saveSubpageEntity(Number(homeSlot)),
    },
  );
  const dateTimeConfigurationOptions = createConfigDateTimeOptionsFeature({
    state: AppInstance.state,
    now: core.now,
    renderButtonSettings: () => buttonSettings.render(),
    effectiveTimezoneOption: (value) => environment.effectiveTimezoneOptionForWeb(value),
    timezoneId: (value) => statusPreview.getTzId(value),
    timezoneOptionsWithFallback: (options, selected) => environment.timezoneOptionsWithFallback(options, selected),
    appendTimezoneOption: (select, option) => statusPreview.appendTimezoneOption(select, option),
    monthNameForIndex: (index) => environment.monthNameForIndex(index),
  });
  const modalTabOptions = createConfigModalTabOptionsFeature({
    document: dom.document,
    renderButtonSettings: () => buttonSettings.render(),
  });
  const accessClimateAlarmOptions = createConfigAccessClimateAlarmOptionsFeature(modalTabOptions);
  confirmationOptions = createConfigConfirmationOptionsFeature(accessClimateAlarmOptions);
  configurationCodec = createConfigCodecFeature(
    cards,
    configurationOptions,
    mediaConfigurationOptions,
    imageConfigurationOptions,
    weatherConfigurationOptions,
    webhookConfigurationOptions,
    robotConfigurationOptions,
    lockConfigurationOptions,
    dateTimeConfigurationOptions,
    modalTabOptions,
    accessClimateAlarmOptions,
    confirmationOptions,
    layout,
    configurationPersistence,
    renderQueue,
    {
      renderPreview: () => preview.render(),
      renderButtonSettings: (force) => buttonSettings.render(force),
    },
  );
  configurationPersistence.connectCodec(configurationCodec);
  const cardEditorDraft = createCardEditorDraftController({
    cloneCard: (button) => Model.cloneCardConfig(button),
    emptyCard: () => Model.emptyCardConfig(),
  });
  const cardEditorSave = createCardEditorSaveController({
    emptyCard: () => Model.emptyCardConfig(),
    copyCard: (target, source) => {
      Model.copyCardConfig(target, source);
      configurationCodec.normalizeButtonConfig(target);
    },
  });
  const cardEditorValidation = createCardEditorValidationController();
  const previewPlacement = createPreviewPlacementController();
  const screenSchedule = createScreenScheduleController({
    trigger: (value, enabled) => Model.normalizeScheduleTrigger(value, enabled),
    sensorActivation: (value) => Model.normalizeScheduleSensorActivation(value),
    hour: (value, fallback) => Model.normalizeHour(value, fallback),
    mode: (value) => Model.normalizeScheduleMode(value),
    wakeTimeout: (value) => Model.normalizeScheduleWakeTimeout(value),
    wakeBrightness: (value) => Model.normalizeScheduleWakeBrightness(value),
    dimmedBrightness: (value) => Model.normalizeScheduleDimmedBrightness(value),
    clockBrightness: (value) => Model.normalizeScheduleClockBrightness(value),
  });
  const screenScheduleState = createScreenScheduleStateFeature(
    screenSchedule,
    runtime,
    {
      syncClockScreensaverControls: () => settingsHelpers.syncClockScreensaverControls(),
      updateSunInfo: () => statusPreview.updateSunInfo(),
    },
  );
  const screensaverTimeout = createScreensaverTimeoutFeature(runtime, screenScheduleState);
  requestApi = createApplicationApiFeature(
    nativePanelConfig,
    deviceApi,
    entityState,
    screensaverTimeout,
    shell,
  );
  firmwarePostApi = createFirmwareUpdatePostApiFeature(entityState, requestApi);
  const artworkPostApi = createArtworkPostApiFeature(entityState, requestApi);
  const schedulePostApi = createScreenSchedulePostApiFeature(entityState, requestApi);
  const clockBarPostApi = createClockBarPostApiFeature(entityState, requestApi);
  configurationPersistence.connectRequestApi(requestApi);
  configurationCodec.connectRequestApi(requestApi);
  const grid = createGridFeature(configurationCodec, runtime, layout, entityState, requestApi, renderQueue);
  fields = createControlsFieldsFeature(cards, configurationOptions, shell, requestApi);
  const placement = createPreviewGridPlacementFeature({
    controller: previewPlacement,
    layout,
    codec: configurationCodec,
    grid,
  });
  const gridMigration = createGridMigrationFeature(runtime, layout, {
    renderPreview: () => preview.render(),
    renderButtonSettings: () => buttonSettings.render(),
    postOrder: (value) => { void requestApi.postText(entityState.entityName("button_order"), value); },
  });
  stateLoader = createStateLoaderFeature(
    runtime,
    layout,
    screensaverTimeout,
    firmwareVersion,
    firmwareUpdate,
    c6Firmware,
    entityState,
    shell,
    requestApi,
    gridMigration,
    {
      subpageEntityKeys: configurationPersistence.subpageEntityKeys,
      connectEvents: () => appEvents.connect(),
    },
  );
  const clockBar = createClockBarController();
  clockBarState = createClockBarFeature(clockBar, runtime, core, environment, {
    hideSettingsOverlay: () => selection.hideSettingsOverlay(),
    timezoneId: (value) => statusPreview.getTzId(value),
    postTemperatureEntities: (value) => clockBarPostApi.postClockBarTemperatureEntities(value),
    postSwitch: (name, value) => requestApi.postSwitch(name, value),
    entityName: (key) => entityState.entityName(key),
    postText: (name, value) => requestApi.postText(name, value),
    updateTemperaturePreview: () => statusPreview.updateTempPreview(),
    updateItemUi: () => statusPreview.updateClockBarItemUi(),
    postTemperatureDegreeSymbol: (value) => clockBarPostApi.postTemperatureDegreeSymbol(value),
    isTemperatureItem: (item) => statusPreview.isClockBarTemperatureItem(item),
    postTime: (value) => clockBarPostApi.postClockBarTime(value),
    postVoiceServices: (value) => clockBarPostApi.postVoiceServices(value),
    postNetworkStatus: (value) => clockBarPostApi.postNetworkStatusIcon(value),
    renderSelectionBar: () => selection.renderSelectionBar(grid.ctx()),
    updateNetworkPreview: () => statusPreview.updateNetworkPreview(),
    updateVoicePreview: () => statusPreview.updateVoicePreview(),
  });
  statusPreview = createAppStatusPreviewFeature(runtime, core, layout, environment, clockBarState);
  const settingsUi = createSettingsUiFeature({
    document: dom.document,
    textSpan: (text, className) => textSpan(text, className),
    createDisclosureChevron: shell.createDisclosureChevron,
  });
  const alarmDelayAudio = createAlarmDelayAudioController({
    announcement: (value, fallback) => Model.normalizeAlarmDelayAnnouncement(value, fallback),
    beepVolume: (value) => Model.normalizeAlarmDelayBeepVolume(value),
    finalCountdown: (value) => Model.normalizeAlarmDelayFinalCountdown(value),
  });
  const screensaver = createScreensaverController({
    action: (value) => Model.normalizeScreensaverAction(value),
    dimBrightness: (value) => Model.normalizeScreensaverDimmedBrightness(value),
    clockBrightness: (value, fallback) => Model.normalizeClockBrightness(value, fallback),
  });
  const coverArtScreensaver = createCoverArtScreensaverController({
    delay: (value) => Model.normalizeCoverArtDelay(value),
    trackOverlayDuration: (value) => parseFloat(String(value)) || 0,
  });
  const mediaPlayback = createMediaPlaybackController();
  settingsHelpers = createSettingsPageHelpersFeature({
    settingsUiFeature: settingsUi,
    alarmDelayAudio,
    screensaver,
    coverArtScreensaver,
    mediaPlayback,
    codec: configurationCodec,
    runtime,
    layout,
    screenScheduleState,
    clockBar: clockBarState,
    entityState,
    shell,
    requestApi,
    statusPreview,
    clockBarPostApi,
    fields,
  });
  selection = createButtonSettingsSelectionFeature(
    runtime,
    clockBarState,
    entityState,
    shell,
    statusPreview,
    grid,
    renderQueue,
    {
      document: dom.document,
      fields,
      renderPreview: () => preview.render(),
      renderButtonSettings: (force) => buttonSettings.render(force),
      showSelectionMenu: (event) => contextMenu.showSelection(event),
      contextMenuContains: (target) => contextMenu.contains(target),
      openVoiceServicesSettings: () => settingsHelpers.openVoiceServicesSettings(),
    },
  );
  preview = createPreviewRenderFeature({
    document: dom.document,
    layout,
    cards,
    confirmationOptions,
    codec: configurationCodec,
    runtime,
    screenRotation,
    shell,
    grid,
    selection,
  });
  const clipboard = createPreviewClipboardFeature({
    configPersistence: configurationPersistence,
    document: dom.document,
    layout,
    cards,
    imageOptions: imageConfigurationOptions,
    sensorOptions: configurationOptions,
    codec: configurationCodec,
    entityState,
    shell,
    requestApi,
    grid,
    preview,
    placement,
    deleteSlot: (slot) => interactions.deleteSlot(slot),
    deleteButtons: (slots) => interactions.deleteButtons(slots),
    emptyButtonConfig: () => interactions.emptyButtonConfig(),
  });
  contextMenu = createPreviewContextMenuFeature({
    document: dom.document,
    window: dom.window,
    layout,
    cards,
    codec: configurationCodec,
    clockBar: clockBarState,
    shell,
    statusPreview,
    grid,
    selection,
    preview,
    clipboard,
    renderPreview: () => preview.render(),
    renderButtonSettings: () => buttonSettings.render(),
    openCardSettings: (slot) => buttonSettings.openCardSettings(slot),
    openVoiceServicesSettings: () => settingsHelpers.openVoiceServicesSettings(),
    addSlot: (position) => interactions.addSlot(position),
    addSubpageSlot: (position) => interactions.addSubpageSlot(position),
    duplicateButton: (slot) => interactions.duplicateButton(slot),
    duplicateSubpageButton: (slot) => interactions.duplicateSubpageButton(slot),
    deleteSlot: (slot) => interactions.deleteSlot(slot),
    deleteButtons: (slots) => interactions.deleteButtons(slots),
  });
  interactions = createPreviewInteractionsFeature({
    cardEditorDraft,
    configPersistence: configurationPersistence,
    layout,
    window: dom.window,
    imageOptions: imageConfigurationOptions,
    codec: configurationCodec,
    runtime,
    entityState,
    shell,
    requestApi,
    grid,
    selection,
    placement,
    contextMenu,
    renderPreview: () => preview.render(),
    renderButtonSettings: (force) => buttonSettings.render(force),
  });
  buttonSettings = createButtonSettingsFeature(
    cardEditorDraft, cardEditorValidation, cardEditorSave,
    configurationPersistence, cards, imageConfigurationOptions,
    confirmationOptions, configurationCodec, layout, runtime, entityState,
    shell, requestApi, grid, iconPicker, selection, preview, interactions, fields,
  );
  const configEvents = createAppConfigEventsFeature(configurationPersistence, configurationCodec, layout, renderQueue);
  const stateEventHandlers = createAppStateEventHandlersFeature(
    runtime,
    core,
    environment,
    screenScheduleState,
    screensaverTimeout,
    screenRotation,
    appearance,
    firmwareVersion,
    firmwareUpdate,
    c6Firmware,
    clockBarState,
    statusPreview,
    grid,
    settingsHelpers,
    preview,
  );
  const backupModel = createBackupFeature({
    deviceId: layout.deviceId,
    gridCols: layout.gridCols,
    numSlots: layout.numSlots,
    normalizeButtonConfig: (button) => configurationCodec.normalizeButtonConfig(button),
    parseSubpageConfig: (value) => configurationCodec.parseSubpageConfig(value),
    serializeSubpageConfig: (subpage) => configurationCodec.serializeSubpageConfig(subpage),
    buildSubpageGrid: (subpage) => {
      configurationCodec.buildSubpageGridAndNormalizeOrder(subpage);
      return subpage.grid || [];
    },
  });
  const backupContract = createBackupContractFeature(backupModel, configurationCodec, cards, layout);
  const normalizeImportedPanelSettings = (settings: any) => {
    if (!settings) return null;
    return Model.normalizeBackupPanelSettings(settings, {
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
      coverArtHomeAssistantEndpointMode: state.homeAssistantArtworkEndpointMode,
      autoUpdate: state.autoUpdate,
      updateFrequency: state.updateFrequency,
      updateFrequencyOptions: state.updateFreqOptions,
      screenRotationOptions: screenRotation.allOptions(),
    });
  };
  const gridColsForImportedSettings = (importedSettings: any): number => {
    const rotation = importedSettings ? importedSettings.screenRotation : state.screenRotation;
    const profile = core.isPortraitRotation(rotation) && layout.config.portrait
      ? layout.config.portrait
      : layout.config;
    return profile.cols || layout.config.cols;
  };
  const backupExport = createBackupExportController({
    serializeButtonConfig: (button) => configurationCodec.serializeButtonConfig(button),
    serializeSubpageConfig: (subpage) => configurationCodec.serializeSubpageConfig(subpage),
  });
  const backupImport = createBackupImportController<any, any, any, any>({
    normalizeBackup: (data) => backupContract.normalizeBackupConfig(data),
    normalizeSettings: normalizeImportedPanelSettings,
    gridColsForSettings: gridColsForImportedSettings,
    getGridCols: () => layout.gridCols,
    setGridCols: (gridCols) => { layout.gridCols = gridCols; },
    planBackupImport: (data, target) => backupContract.planBackupImport(data, target),
  });
  const backupRestore = createBackupRestoreController<any, any>({
    plan: backupImport.plan,
    warnings: (plannedImport) => plannedImport.backupPlan.warnings,
    showBanner: shell.showBanner,
    setPostThrottle: (milliseconds) => requestApi.setPostThrottle(milliseconds),
    resetPostQueueError: () => requestApi.resetPostQueueError(),
    postQueueIdle: () => requestApi.postQueueIdle(),
    postQueueHadError: () => requestApi.postQueueHadError(),
  });
  const backupFile = createBackupFileController({
    transport: {
      download(content, filename) {
        const blob = new Blob([content], { type: "application/json" });
        const url = URL.createObjectURL(blob);
        const link = dom.document.createElement("a");
        link.href = url;
        link.download = filename;
        dom.document.body.appendChild(link);
        link.click();
        dom.document.body.removeChild(link);
        URL.revokeObjectURL(url);
      },
      chooseJsonFile(onText, onError) {
        const input = dom.document.createElement("input");
        input.type = "file";
        input.accept = ".json";
        input.style.display = "none";
        const cleanupInput = () => {
          if (input.parentNode) input.parentNode.removeChild(input);
        };
        input.addEventListener("cancel", cleanupInput);
        input.addEventListener("change", () => {
          if (!input.files || !input.files[0]) {
            cleanupInput();
            return;
          }
          const reader = new FileReader();
          reader.onerror = () => {
            cleanupInput();
            onError();
          };
          reader.onload = () => {
            cleanupInput();
            onText(String(reader.result || ""));
          };
          reader.readAsText(input.files[0]);
        });
        dom.document.body.appendChild(input);
        input.click();
      },
    },
    showBanner: shell.showBanner,
  });
  const backupApplication = createAppBackupFeature({
    layout,
    backupExport,
    backupImport,
    backupRestore,
    backupFile,
    normalizeImportedPanelSettings,
    gridColsForImportedSettings,
    nativePanelConfig,
    codec: configurationCodec,
    configPersistence: configurationPersistence,
    backupContract,
    runtime,
    core,
    screenScheduleState,
    screensaverTimeout,
    firmwareUpdate,
    clockBar: clockBarState,
    entityState,
    shell,
    requestApi,
    statusPreview,
    grid,
    artworkPostApi,
    schedulePostApi,
    clockBarPostApi,
    settingsHelpers,
    preview,
    buttonSettings,
  });
  const reconnect = createReconnectController<unknown>({
    eventStreamEnabled: stateLoader.eventStreamEnabled,
    loadInitialState: (handleState, markConnected) =>
      stateLoader.loadInitialState(handleState, markConnected),
    createEventSource: dom.createEventSource,
    getActiveSource: () => runtime.eventSource,
    setActiveSource: (source) => { runtime.eventSource = source; },
    schedule: (callback, delayMs) => dom.schedule(callback, delayMs),
  });
  appEvents = createAppEventsFeature(
    reconnect,
    stateEventHandlers,
    configEvents,
    runtime,
    pageTitle,
    firmwareVersion,
    firmwareUpdate,
    c6Firmware,
    entityState,
    shell,
    stateLoader,
    gridMigration,
  );
  publicFirmwareInstall = createPublicFirmwareInstallFeature(
    deviceApi,
    layout.deviceId,
    firmwareUpdate,
    shell,
    requestApi,
    appEvents,
  );
  const scheduleSection = createSettingsScheduleSectionFeature(
    configurationCodec, runtime, screenScheduleState, entityState, requestApi,
    schedulePostApi, fields, settingsHelpers,
  );
  const coverArtSection = createSettingsCoverArtSectionFeature(
    configurationCodec, runtime, entityState, statusPreview, artworkPostApi,
    fields, settingsHelpers, coverArtScreensaver, mediaPlayback,
    !!layout.config.features?.companion,
  );
  const systemSection = createSettingsSystemSectionFeature({
    exportBackup: backupApplication.exportConfig,
    importBackup: backupApplication.importConfig,
  }, runtime, firmwareVersion, firmwareUpdate, c6Firmware, shell, requestApi,
  stateLoader, firmwarePostApi, artworkPostApi, publicFirmwareInstall, fields,
  settingsHelpers);
  const companionSection = createSettingsCompanionSectionFeature(dom, shell, fields);
  connectorsPage = createConnectorsPageFeature(
    dom, shell, fields, companionSection, !!layout.config.features?.companion,
  );
  settingsPage = createSettingsPageFeature(
    configurationCodec, runtime, core, layout, environment, screenScheduleState,
    screensaverTimeout, screenRotation, appearance, clockBarState, entityState,
    shell, requestApi, statusPreview, artworkPostApi, schedulePostApi,
    clockBarPostApi, fields, settingsHelpers, scheduleSection, coverArtSection,
    systemSection, preview,
  );
  app = createAppFeature(
    pageTitle, createWebStyles(layout.config.dragAnimation), core, screenRotation,
    clockBarState, shell, appEvents, statusPreview, selection, contextMenu,
    interactions, preview, buttonSettings, connectorsPage,
  );
  app = createAppFeature(
    pageTitle, createWebStyles(layout.config.dragAnimation), core, screenRotation,
    clockBarState, shell, appEvents, statusPreview, selection, contextMenu,
    interactions, preview, buttonSettings, connectorsPage,
  );
  requestApi.connectReconnect(appEvents.connect);
  return createApplicationContext({
    layout,
    model: Model,
    state: AppInstance.state,
    runtime,
    core,
    api: deviceApi,
    nativeConfiguration: nativePanelConfig,
    configurationPersistence,
    configurationOptions,
    mediaConfigurationOptions,
    imageConfigurationOptions,
    weatherConfigurationOptions,
    webhookConfigurationOptions,
    internalRelayConfigurationOptions,
    robotConfigurationOptions,
    lockConfigurationOptions,
    dateTimeConfigurationOptions,
    modalTabOptions,
    accessClimateAlarmOptions,
    confirmationOptions,
    configurationCodec,
    backupContract,
    backupExport,
    backupFile,
    backupImport,
    backupRestore,
    backupApplication,
    appearance,
    firmwareVersion,
    firmwareUpdate,
    firmwarePostApi,
    artworkPostApi,
    schedulePostApi,
    clockBarPostApi,
    publicFirmwareInstall,
    c6Firmware,
    clockBarState,
    entityState,
    shell,
    requestApi,
    stateLoader,
    gridMigration,
    configEvents,
    stateEventHandlers,
    appEvents,
    statusPreview,
    grid,
    alarmDelayAudio,
    cardEditorDraft,
    cardEditorSave,
    cardEditorValidation,
    clockBar,
    coverArtScreensaver,
    mediaPlayback,
    pageTitle,
    previewPlacement,
    reconnect,
    screenSchedule,
    screenScheduleState,
    screenRotation,
    screensaverTimeout,
    screensaver,
    settingsUi,
    voiceServices,
    environment,
    iconPicker,
    renderQueue,
    fields,
    selection,
    preview,
    placement,
    clipboard,
    contextMenu,
    interactions,
    buttonSettings,
    app,
    dom,
    cards,
  });
}

function startEspControl(): void {
  if (startupState.__ESPCONTROL_UI_STARTED__ || startupState.__ESPCONTROL_UI_STARTING__) return;
  AppInstance.initializeAppState();

  const context = composeApplicationContext();

  const lightCards = registerCards(context);
  if (__ESPCONTROL_TEST_HOOKS_ENABLED__) {
    installTestHooks(context, lightCards);
  }
  startApp(context.controllers.app);

  // Native configuration is initialized after the display on P4 devices, so
  // the first capabilities request can legitimately receive a temporary 503.
  // Retry after the UI has started and redraw an open card picker once support
  // is confirmed. Otherwise Wifi Sharing remains hidden for the whole page
  // session even though the API becomes available a few seconds later.
  const discoverNativeCards = () => {
    void context.configuration.native.waitForDiscovery().then((result) => {
      if (result === true) context.controllers.buttonSettings.render();
    });
  };
  if (context.dom.document.readyState === "loading") {
    context.dom.document.addEventListener("DOMContentLoaded", discoverNativeCards, { once: true });
  } else {
    discoverNativeCards();
  }
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
