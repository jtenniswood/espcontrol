import {
  createApplicationContext,
  createApplicationLayoutState,
} from "../../src/webserver/application/application_context";
import { createCardRegistry } from "../../src/webserver/application/card_registry";
import { createConfigWeatherOptionsFeature } from "../../src/webserver/application/config_weather_options";
import { createConfigWebhookOptionsFeature } from "../../src/webserver/application/config_webhook_options";
import { createConfigInternalRelayOptionsFeature } from "../../src/webserver/application/config_internal_relay_options";
import { createConfigRobotCardOptionsFeature } from "../../src/webserver/application/config_robot_card_options";
import { createConfigLockOptionsFeature } from "../../src/webserver/application/config_lock_options";
import { createConfigDateTimeOptionsFeature } from "../../src/webserver/application/config_date_time_options";
import type { DeviceConfig } from "../../src/webserver/state/types";

const profile: DeviceConfig = {
  slots: 12,
  cols: 4,
  rows: 3,
  screenSize: "10-inch",
  dragMode: "swap",
  dragAnimation: true,
  imageSlotCapacity: 12,
  screen: { width: "100%", aspect: "16 / 10" },
  grid: { fr: "1fr" },
};

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) {
    throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
  }
}

export function runApplicationContextTests(): void {
  const cards = createCardRegistry();
  const api = { request() {} } as any;
  const nativeConfiguration = { begin() {} } as any;
  const configurationPersistence = { globals: {}, saveButtonConfig() {}, saveSubpageEntity() {} } as any;
  const configurationOptions = {} as any;
  const mediaConfigurationOptions = {} as any;
  const imageConfigurationOptions = {} as any;
  const weatherConfigurationOptions = {} as any;
  const webhookConfigurationOptions = {} as any;
  const internalRelayConfigurationOptions = {} as any;
  const robotConfigurationOptions = {} as any;
  const lockConfigurationOptions = {} as any;
  const dateTimeConfigurationOptions = {} as any;
  const modalTabOptions = {} as any;
  const accessClimateAlarmOptions = {} as any;
  const confirmationOptions = {} as any;
  const configurationCodec = {} as any;
  const backupContract = {} as any;
  const backupExport = {} as any;
  const backupFile = {} as any;
  const backupImport = {} as any;
  const backupRestore = {} as any;
  const backupApplication = {} as any;
  const appearance = {} as any;
  const firmwareVersion = {} as any;
  const firmwareUpdate = {} as any;
  const c6Firmware = {} as any;
  const reconnect = { connect() {} } as any;
  const alarmDelayAudio = {} as any;
  const cardEditorDraft = {} as any;
  const cardEditorSave = {} as any;
  const cardEditorValidation = {} as any;
  const previewPlacement = {} as any;
  const clockBar = {} as any;
  const clockBarState = {} as any;
  const entityState = {} as any;
  const shell = {} as any;
  const requestApi = {} as any;
  const stateLoader = {} as any;
  const gridMigration = {} as any;
  const configEvents = {} as any;
  const stateEventHandlers = {} as any;
  const appEvents = {} as any;
  const statusPreview = {} as any;
  const grid = {} as any;
  const firmwarePostApi = {} as any;
  const artworkPostApi = {} as any;
  const schedulePostApi = {} as any;
  const clockBarPostApi = {} as any;
  const publicFirmwareInstall = {} as any;
  const coverArtScreensaver = {} as any;
  const mediaPlayback = {} as any;
  const pageTitle = {} as any;
  const screenSchedule = {} as any;
  const screenScheduleState = {} as any;
  const screenRotation = {} as any;
  const screensaver = {} as any;
  const screensaverTimeout = {} as any;
  const settingsUi = {} as any;
  const voiceServices = {} as any;
  const environment = {} as any;
  const iconPicker = {} as any;
  const renderQueue = {} as any;
  const fields = {} as any;
  const selection = {} as any;
  const preview = {} as any;
  const placement = {} as any;
  const clipboard = {} as any;
  const contextMenu = {} as any;
  const interactions = {} as any;
  const buttonSettings = {} as any;
  const app = {} as any;
  const state = { grid: [] } as any;
  const runtime = {} as any;
  const core = {} as any;
  const model = {} as any;
  const dom = {} as any;
  const context = createApplicationContext({
    layout: createApplicationLayoutState("guition-esp32-p4-jc8012p4a1", profile),
    model,
    state,
    runtime,
    core,
    api,
    nativeConfiguration,
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
    alarmDelayAudio,
    cardEditorDraft,
    cardEditorSave,
    cardEditorValidation,
    previewPlacement,
    clockBar,
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
    coverArtScreensaver,
    mediaPlayback,
    pageTitle,
    reconnect,
    screenSchedule,
    screenScheduleState,
    screenRotation,
    screensaver,
    screensaverTimeout,
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

  equal(context.device.id, "guition-esp32-p4-jc8012p4a1", "context owns the selected device");
  equal(context.controllers.appearance, appearance, "context owns appearance behavior");
  equal(context.controllers.firmwareVersion, firmwareVersion, "context owns firmware version behavior");
  equal(context.controllers.firmwareUpdate, firmwareUpdate, "context owns firmware update behavior");
  equal(context.controllers.firmwarePostApi, firmwarePostApi, "context owns firmware update posting behavior");
  equal(context.controllers.artworkPostApi, artworkPostApi, "context owns artwork posting behavior");
  equal(context.controllers.schedulePostApi, schedulePostApi, "context owns screen schedule posting behavior");
  equal(context.controllers.clockBarPostApi, clockBarPostApi, "context owns Clock Bar posting behavior");
  equal(context.controllers.publicFirmwareInstall, publicFirmwareInstall, "context owns public firmware upload behavior");
  equal(context.controllers.c6Firmware, c6Firmware, "context owns C6 firmware behavior");
  equal(context.controllers.clockBarState, clockBarState, "context owns Clock Bar behavior");
  equal(context.controllers.entityState, entityState, "context owns entity lookup and post-path behavior");
  equal(context.controllers.shell, shell, "context owns UI shell behavior");
  equal(context.controllers.requestApi, requestApi, "context owns application request behavior");
  equal(context.controllers.stateLoader, stateLoader, "context owns initial-state loading");
  equal(context.controllers.gridMigration, gridMigration, "context owns legacy grid migration");
  equal(context.controllers.configEvents, configEvents, "context owns configuration event matching");
  equal(context.controllers.stateEventHandlers, stateEventHandlers, "context owns state event handling");
  equal(context.controllers.appEvents, appEvents, "context owns event-stream reconnect behavior");
  equal(context.controllers.statusPreview, statusPreview, "context owns status and connectivity preview behavior");
  equal(context.controllers.grid, grid, "context owns grid and card-order behavior");
  equal(context.layout.numSlots, 12, "context initializes slot count");
  equal(context.layout.totalSlots, 12, "context initializes total slot count");
  equal(context.layout.gridCols, 4, "context initializes grid columns");
  equal(context.layout.gridRows, 3, "context initializes grid rows");
  equal(context.api, api, "context retains the API instance");
  equal(context.runtime, runtime, "context retains mutable UI runtime state");
  equal(context.core, core, "context retains the typed core service");
  equal(context.configuration.native, nativeConfiguration, "context retains native persistence");
  equal(context.configuration.persistence, configurationPersistence, "context retains save persistence");
  equal(context.configuration.options, configurationOptions, "context retains typed configuration options");
  equal(context.configuration.mediaOptions, mediaConfigurationOptions, "context retains typed media options");
  equal(context.configuration.imageOptions, imageConfigurationOptions, "context retains typed image options");
  equal(context.configuration.weatherOptions, weatherConfigurationOptions, "context retains typed weather options");
  equal(context.configuration.webhookOptions, webhookConfigurationOptions, "context retains typed webhook options");
  equal(context.configuration.internalRelayOptions, internalRelayConfigurationOptions, "context retains typed internal-relay options");
  equal(context.configuration.robotOptions, robotConfigurationOptions, "context retains typed robot-card options");
  equal(context.configuration.lockOptions, lockConfigurationOptions, "context retains typed lock options");
  equal(context.configuration.dateTimeOptions, dateTimeConfigurationOptions, "context retains typed date/time options");
  equal(context.configuration.modalTabs, modalTabOptions, "context retains typed modal-tab options");
  equal(context.configuration.accessClimateAlarm, accessClimateAlarmOptions, "context retains typed access/climate/alarm options");
  equal(context.configuration.confirmationOptions, confirmationOptions, "context retains typed confirmation options");
  equal(context.configuration.codec, configurationCodec, "context retains the typed configuration codec");
  equal(context.backup.contract, backupContract, "context retains the backup contract");
  equal(context.backup.export, backupExport, "context retains backup export ownership");
  equal(context.backup.file, backupFile, "context retains backup file ownership");
  equal(context.backup.import, backupImport, "context retains backup import ownership");
  equal(context.backup.restore, backupRestore, "context retains backup restore ownership");
  equal(context.backup.application, backupApplication, "context retains the backup journey");
  equal(context.controllers.cardEditorDraft, cardEditorDraft, "context retains editor draft ownership");
  equal(context.controllers.environment, environment, "context retains environment state ownership");
  equal(context.controllers.iconPicker, iconPicker, "context retains icon picker ownership");
  equal(context.controllers.renderQueue, renderQueue, "context retains render queue ownership");
  equal(context.controllers.fields, fields, "context retains shared field ownership");
  equal(context.controllers.selection, selection, "context retains card selection ownership");
  equal(context.controllers.preview, preview, "context retains preview rendering ownership");
  equal(context.controllers.placement, placement, "context retains preview placement ownership");
  equal(context.controllers.clipboard, clipboard, "context retains preview clipboard ownership");
  equal(context.controllers.contextMenu, contextMenu, "context retains preview context-menu ownership");
  equal(context.controllers.interactions, interactions, "context retains preview interaction ownership");
  equal(context.controllers.screenScheduleState, screenScheduleState, "context retains screen schedule state ownership");
  equal(context.controllers.screenRotation, screenRotation, "context retains screen rotation ownership");
  equal(context.controllers.screensaverTimeout, screensaverTimeout, "context retains screensaver timeout ownership");
  equal(context.controllers.alarmDelayAudio, alarmDelayAudio, "context retains alarm settings ownership");
  equal(context.controllers.cardEditorSave, cardEditorSave, "context retains editor save ownership");
  equal(context.controllers.cardEditorValidation, cardEditorValidation, "context retains editor validation ownership");
  equal(context.controllers.previewPlacement, previewPlacement, "context retains preview placement ownership");
  equal(context.controllers.clockBar, clockBar, "context retains clock bar ownership");
  equal(context.controllers.coverArtScreensaver, coverArtScreensaver, "context retains cover art settings ownership");
  equal(context.controllers.mediaPlayback, mediaPlayback, "context retains media settings ownership");
  equal(context.controllers.pageTitle, pageTitle, "context retains page title ownership");
  equal(context.controllers.screenSchedule, screenSchedule, "context retains schedule settings ownership");
  equal(context.controllers.screensaver, screensaver, "context retains screensaver settings ownership");
  equal(context.controllers.settingsUi, settingsUi, "context retains settings DOM ownership");
  equal(context.controllers.voiceServices, voiceServices, "context retains voice settings ownership");
  equal(context.controllers.reconnect, reconnect, "context retains reconnect ownership");

  const weatherOptions = createConfigWeatherOptionsFeature(profile);
  equal(weatherOptions.normalizeWeatherCardMode("today"), "today", "weather options preserve supported forecast modes");
  equal(weatherOptions.normalizeWeatherCardMode("invalid"), "", "weather options reject unknown modes");
  equal(weatherOptions.weatherCardDefaultForecastLabel({ precision: "today" }), "Today", "weather options label today's forecast");
  equal(weatherOptions.weatherCardDefaultForecastLabel({ precision: "tomorrow" }), "Tomorrow", "weather options label tomorrow's forecast");
  const currentOnlyWeather = createConfigWeatherOptionsFeature({ ...profile, disabledCardTypes: ["weather_forecast"] });
  equal(currentOnlyWeather.normalizeWeatherCardMode("tomorrow"), "", "disabled forecast support normalizes to current conditions");
  equal(currentOnlyWeather.weatherCardIsForecastMode({ precision: "tomorrow" }), false, "disabled forecast support hides forecast controls");

  const webhookOptions = createConfigWebhookOptionsFeature();
  equal(webhookOptions.webhookMethod("post"), "POST", "webhook options normalize supported methods");
  equal(webhookOptions.webhookMethod("unknown"), "GET", "webhook options fall back to GET");
  const webhook = { sensor: "POST", unit: "{}", icon: "", icon_on: "Flash", precision: "1", options: "webhook_headers=Content-Type%3A%20application/json,unused=value" } as any;
  webhookOptions.normalizeWebhookConfig(webhook);
  equal(webhook.icon, "Auto", "webhook normalization restores the default icon");
  equal(webhook.icon_on, "Auto", "webhook normalization removes the active icon");
  equal(webhook.precision, "", "webhook normalization clears precision");
  equal(webhookOptions.webhookHeaders(webhook), "Content-Type: application/json", "webhook normalization preserves encoded headers");

  const internalRelayOptions = createConfigInternalRelayOptionsFeature({
    ...profile,
    features: { internalRelays: [{ key: "relay_1", label: "Relay One" }] },
  });
  equal(internalRelayOptions.normalizeInternalRelayMode("push"), "push", "internal relay options preserve push mode");
  equal(internalRelayOptions.normalizeInternalRelayMode("invalid"), "switch", "internal relay options reject unknown modes");
  equal(internalRelayOptions.internalRelayLabelFor("relay_1"), "Relay One", "internal relay options use profile labels");
  equal(internalRelayOptions.internalRelayLabelFor("porch_light"), "Porch Light", "internal relay options format unknown relay keys");

  const robotOptions = createConfigRobotCardOptionsFeature();
  equal(robotOptions.normalizeLawnMowerMode("dock"), "dock", "robot options preserve lawn-mower modes");
  equal(robotOptions.normalizeLawnMowerMode("invalid"), "start_mowing", "robot options normalize invalid lawn-mower modes");
  equal(robotOptions.vacuumModeNeedsArea("clean_area"), true, "robot options retain area identifiers for clean-area mode");
  equal(robotOptions.vacuumModeDefaultIcon("dock"), "Robot Vacuum Variant", "robot options provide vacuum mode icons");

  const lockOptions = createConfigLockOptionsFeature();
  equal(lockOptions.normalizeLockMode("unlock"), "unlock", "lock options preserve command modes");
  equal(lockOptions.normalizeLockMode("invalid"), "", "lock options normalize invalid modes to toggle");
  equal(lockOptions.lockModeDefaultIcon("unlock"), "Lock Open", "lock options provide command icons");
  equal(lockOptions.lockUsesDefaultIcon("Lock"), true, "lock options recognize historical default icons");

  const dateTimeOptions = createConfigDateTimeOptionsFeature({
    state: { timezone: "Europe/London (GMT+0)", timezoneOptions: ["Europe/London (GMT+0)"], clockFormat: "24h" } as any,
    now: () => new Date("2026-01-01T09:05:00Z"),
    renderButtonSettings() {},
    effectiveTimezoneOption: (value) => value,
    timezoneId: () => "Europe/London",
    timezoneOptionsWithFallback: (options) => options,
    appendTimezoneOption() {},
    monthNameForIndex: () => "January",
  });
  equal(dateTimeOptions.normalizeDateTimeCardMode("clock"), "clock", "date/time options preserve clock mode");
  equal(dateTimeOptions.normalizeDateTimeCardMode("invalid"), "", "date/time options reject unknown modes");
  equal(dateTimeOptions.dateTimeCardTimeParts().value, "09:05", "date/time options format the shared current time");
  equal(dateTimeOptions.defaultTimezoneCardEntity(), "Europe/London (GMT+0)", "date/time options use the active timezone");

  const sensor = cards.register("sensor", { label: "Sensor", allowInSubpage: true });
  equal(cards.typedDefinitionCount, 1, "registry counts typed card definitions");
  equal(cards.definitions.sensor, sensor, "registry owns typed card definitions");
  equal(sensor.key, "sensor", "registry assigns the card key");
  equal(sensor.label, "Sensor", "registry preserves typed card metadata");
  equal(sensor.runtimeSpec != null, true, "registry attaches the generated runtime contract");

}
