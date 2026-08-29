"use strict";

const { describe, test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const ROOT = path.resolve(__dirname, "../../..");

function sourceFiles(directory) {
  return fs.readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
    const entryPath = path.join(directory, entry.name);
    return entry.isDirectory() ? sourceFiles(entryPath) : [entryPath];
  });
}

describe("browserless application contracts", () => {
  const { runClipboardFeatureTests } = loadTypescriptTest("tests/web/clipboard_feature.test.ts");
  const { runCardImagesFeatureTests } = loadTypescriptTest("tests/web/card_images_feature.test.ts");
  const { runApplicationContextTests } = loadTypescriptTest("tests/web/application_context.test.ts");
  const { runDeviceApiTests } = loadTypescriptTest("tests/web/device_api.test.ts");
  const { runSettingsFeatureTests } = loadTypescriptTest("tests/web/settings_feature.test.ts");
  const { runStateContractTests } = loadTypescriptTest("tests/web/state_contract.test.ts");
  const { createEntityStateFeature } = loadTypescriptTest("src/webserver/application/entity_state.ts");

  test("plans clipboard transfers", () => {
  runClipboardFeatureTests();
  runCardImagesFeatureTests();
  });

  test("owns browser composition and compatibility layout state", () => {
    runApplicationContextTests();
  });

  test("owns entity catalogue helpers as one explicit service", () => {
    const entities = createEntityStateFeature({
      actionCardStateEntity: () => "",
      totalSlots: () => 12,
      clockBarTemperatureEntities: () => [],
      textInput: () => ({}),
    });
    assert.equal(entities.entityName("button_order"), "Button Order");
    assert.equal(entities.entityNameForSlot("button_config", 3), "Button 3 Config");
    assert.deepEqual(Array.from(entities.entityLookupNames("firmware_version")), [
      "Firmware: Version", "firmware__version", "firmware_version", "firmware_version_sensor",
    ]);
    assert.deepEqual(
      { ...entities.parseHomeAssistantEntity("sensor.kitchen_temperature") },
      { id: "sensor.kitchen_temperature", domain: "sensor", objectId: "kitchen_temperature" },
    );
    assert.equal(entities.titleFromEntityId("sensor.kitchen_temperature"), "Kitchen Temperature");
  });

  test("composes entity state without a compatibility adapter", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const source = fs.readFileSync(path.join(ROOT, "src/webserver/application/entity_state.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(entry, /createEntityStateFeature\(/);
    assert.doesNotMatch(entry, /entityStateCompatibilityGlobals/);
    assert.doesNotMatch(source, /GlobalDescriptors|staticGlobal|entityStateCompatibilityGlobals/);
    assert.doesNotMatch(entry, /installEntityStateModule/);
    assert.match(source, /export type EntityStateFeature/);
    assert.doesNotMatch(globals, /\bvar (?:entityName|entityInput|refreshEntityDatalist|rememberEntityPostPath):/);
  });

  test("removes the empty controls bootstrap layer", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.equal(fs.existsSync(path.join(ROOT, "src/webserver/application/controls.ts")), false);
    assert.doesNotMatch(entry, /installControlsModule/);
  });

  test("owns the application API service and injects entity state into request modules", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const modules = [
      "api.ts",
      "firmware_update_post_api.ts",
      "artwork_post_api.ts",
      "screen_schedule_post_api.ts",
      "clock_bar_post_api.ts",
      "config_post_api.ts",
      "state_loader_api.ts",
    ];
    for (const fileName of modules) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", fileName), "utf8");
      assert.match(source, /Pick<EntityStateFeature,/i, `${fileName} should declare its entity dependency`);
    }
    assert.match(entry, /requestApi = createApplicationApiFeature\([\s\S]*entityState,[\s\S]*screensaverTimeout,[\s\S]*shell/);
    assert.doesNotMatch(entry, /installApiModule|applicationApiCompatibilityGlobals/);
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(globals, /\bvar (?:_postQueue|_postQueueHadError|post|postText|postSwitch|getJsonQuietly|entityDetailPath):/);
    const firmwarePosts = fs.readFileSync(path.join(ROOT, "src/webserver/application/firmware_update_post_api.ts"), "utf8");
    assert.match(entry, /firmwarePostApi = createFirmwareUpdatePostApiFeature\(entityState, requestApi\)/);
    assert.doesNotMatch(entry, /installFirmwareUpdatePostApiModule/);
    assert.doesNotMatch(firmwarePosts, /GlobalDescriptors|staticGlobal|installFirmwareUpdatePostApiModule/);
    assert.doesNotMatch(globals, /\bvar (?:postFirmwareUpdateInstall|postFirmwareUpdateCheck|postC6FirmwareUpdateInstall|postC6FirmwareUpdateCheck):/);
    const artworkPosts = fs.readFileSync(path.join(ROOT, "src/webserver/application/artwork_post_api.ts"), "utf8");
    assert.match(entry, /artworkPostApi = createArtworkPostApiFeature\(entityState, requestApi\)/);
    assert.doesNotMatch(entry, /installArtworkPostApiModule/);
    assert.doesNotMatch(artworkPosts, /GlobalDescriptors|staticGlobal|installArtworkPostApiModule/);
    assert.doesNotMatch(globals, /\bvar (?:postPresenceSensorEntity|postMediaPlayerSleepPrevention|postMediaPlayerSleepPreventionEntity|postCoverArtScreensaver|postCoverArtMediaPlayerEntity|postCoverArtSecondaryMediaPlayerEntity|postCoverArtConditions|coverArtHideExternalInputPostUrls|postCoverArtHideExternalInput|coverArtDelayPostUrls|postCoverArtDelay|coverArtTrackOverlayDurationPostUrls|postCoverArtTrackOverlayDuration|homeAssistantArtworkPortPostUrls|postHomeAssistantArtworkPort|postHomeAssistantArtworkProtocol):/);
    const schedulePosts = fs.readFileSync(path.join(ROOT, "src/webserver/application/screen_schedule_post_api.ts"), "utf8");
    assert.match(entry, /schedulePostApi = createScreenSchedulePostApiFeature\(entityState, requestApi\)/);
    assert.doesNotMatch(entry, /installScreenSchedulePostApiModule/);
    assert.doesNotMatch(schedulePosts, /GlobalDescriptors|staticGlobal|liveGlobal|installScreenSchedulePostApiModule/);
    assert.doesNotMatch(globals, /\bvar (?:BRIGHTNESS_TIME_UNAVAILABLE|SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE|SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE|SCREEN_SCHEDULE_MODE_UNAVAILABLE|SCREEN_SCHEDULE_SENSOR_ACTIVATION_UNAVAILABLE|SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE|SCREEN_SCHEDULE_UNAVAILABLE|SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE|SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE|postBrightnessMode|postDisplayBacklightBrightness|postBrightnessDawnTime|postBrightnessDuskTime|postScreenScheduleClockBrightness|postScreenScheduleDimmedBrightness|postScreenScheduleEnabled|postScreenScheduleMode|postScreenScheduleOffHour|postScreenScheduleOnHour|postScreenScheduleSensorActivation|postScreenScheduleSensorEntity|postScreenScheduleTrigger|postScreenScheduleWakeBrightness|postScreenScheduleWakeTimeout):/);
    const clockBarPosts = fs.readFileSync(path.join(ROOT, "src/webserver/application/clock_bar_post_api.ts"), "utf8");
    assert.match(entry, /clockBarPostApi = createClockBarPostApiFeature\(entityState, requestApi\)/);
    assert.doesNotMatch(entry, /installClockBarPostApiModule/);
    assert.doesNotMatch(clockBarPosts, /GlobalDescriptors|staticGlobal|liveGlobal|installClockBarPostApiModule/);
    assert.doesNotMatch(globals, /\bvar (?:CLOCK_BAR_NIGHT_MODE_UNAVAILABLE|CLOCK_BAR_TIME_UNAVAILABLE|CLOCK_BAR_UNAVAILABLE|NETWORK_STATUS_ICON_UNAVAILABLE|SUBPAGE_CHEVRON_UNAVAILABLE|TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE|VOICE_SERVICES_UNAVAILABLE|postClockBar|postClockBarNightMode|postClockBarTemperatureEntities|postClockBarTime|postClockBrightnessDay|postClockBrightnessNight|postClockScreensaver|postNetworkStatusIcon|postBatteryStatus|postSubpageChevron|postTemperatureDegreeSymbol|postVoiceServices|voiceServicesPostUrls|postAlarmDelayAudio|postAlarmDelayTts|postAlarmDelayEntryAnnouncement|postAlarmDelayExitAnnouncement|postAlarmDelayBeepVolume|postAlarmDelayFinalCountdown):/);
  });

  test("owns initial-state loading and reconnect through the application context", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const loader = fs.readFileSync(path.join(ROOT, "src/webserver/application/state_loader_api.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(loader, /export interface StateLoaderFeature/);
    assert.match(loader, /export function createStateLoaderFeature/);
    assert.match(entry, /stateLoader = createStateLoaderFeature\(/);
    assert.match(entry, /eventStreamEnabled: stateLoader\.eventStreamEnabled/);
    assert.match(entry, /stateLoader\.loadInitialState\(handleState, markConnected\)/);
    assert.doesNotMatch(entry, /installStateLoaderApiModule|stateLoaderCompatibilityGlobals/);
    assert.doesNotMatch(loader, /stateLoaderCompatibilityGlobals|GlobalDescriptors/);
    assert.doesNotMatch(globals, /\bvar (?:cardStateEntities|eventStreamEnabled|loadInitialState|loadStateItems|refreshFirmwareVersion|refreshScreensaverTimeout|settingsStateEntities|subpageStateEntities|waitForReboot):/);
  });

  test("owns legacy grid migration without application globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const migration = fs.readFileSync(path.join(ROOT, "src/webserver/application/grid_migration.ts"), "utf8");
    const statusPreview = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_status_preview.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(migration, /export interface GridMigrationFeature/);
    assert.match(migration, /export function createGridMigrationFeature/);
    assert.match(entry, /gridMigration = createGridMigrationFeature\(/);
    assert.match(entry, /createAppEventsFeature\([\s\S]*stateLoader,[\s\S]*gridMigration/);
    assert.match(entry, /requestApi,[\s\S]*gridMigration,[\s\S]*subpageEntityKeys:/);
    assert.doesNotMatch(statusPreview, /\b(?:gridHasAny|scheduleMigration)\b/);
    assert.doesNotMatch(globals, /\bvar (?:gridHasAny|scheduleMigration):/);
  });

  test("owns event-stream composition without application globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const events = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_events.ts"), "utf8");
    const configEvents = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_config_events.ts"), "utf8");
    const stateHandlers = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_state_event_handlers.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(events, /export interface AppEventsFeature/);
    assert.match(events, /export function createAppEventsFeature/);
    assert.match(configEvents, /export function createAppConfigEventsFeature/);
    assert.match(stateHandlers, /export function createAppStateEventHandlersFeature/);
    assert.match(entry, /requestApi\.connectReconnect\(appEvents\.connect\)/);
    assert.doesNotMatch(entry, /installApp(?:ConfigEvents|StateEventHandlers|Events)Module/);
    assert.doesNotMatch(globals, /\bvar (?:applyButtonConfigStateEvent|applySubpageConfigStateEvent|configEventPatterns|connectEvents|createSseHandlers|ensureSubpageRaw):/);
  });

  test("owns status and connectivity preview without compatibility globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const preview = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_status_preview.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(preview, /export interface AppStatusPreviewFeature/);
    assert.match(preview, /export function createAppStatusPreviewFeature/);
    assert.match(entry, /statusPreview = createAppStatusPreviewFeature\(runtime, core, layout, environment, clockBarState\)/);
    assert.doesNotMatch(entry, /installAppStatusPreviewModule|appStatusPreviewCompatibilityGlobals/);
    assert.doesNotMatch(preview, /GlobalDescriptors|staticGlobal|appStatusPreviewCompatibilityGlobals/);
    assert.doesNotMatch(globals, /\bvar (?:appendTimezoneOption|clockBarItemActive|clockBarItemLabel|clockBarItems|isClockBarTemperatureItem|networkPreviewIconSlug|normalizeNetworkTransport|syncInput|updateClock|updateClockBarItemUi|updateNetworkPreview|updateSunInfo|updateTempPreview|updateVoicePreview):/);
  });

  test("injects status preview into core startup and state consumers", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const app = fs.readFileSync(path.join(ROOT, "src/webserver/application/app.ts"), "utf8");
    const handlers = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_state_event_handlers.ts"), "utf8");
    assert.match(entry, /createAppStateEventHandlersFeature\([\s\S]*clockBarState,[\s\S]*statusPreview/);
    assert.match(entry, /app = createAppFeature\([\s\S]*appEvents, statusPreview/);
    assert.match(entry, /timezoneId: \(value\) => statusPreview\.getTzId\(value\)/);
    assert.match(app, /statusPreview\.updateClock\(\)/);
    assert.match(handlers, /statusPreview: Pick<AppStatusPreviewFeature,/);
    assert.match(handlers, /updateNetworkPreview,[\s\S]*updateSunInfo,[\s\S]*updateTempPreview,[\s\S]*\} = statusPreview/);
  });

  test("injects status preview into settings and backup consumers", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    for (const relativePath of ["settings_page.ts", "settings_page_helpers.ts", "settings_cover_art_section.ts", "app_backup.ts"]) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", relativePath), "utf8");
      assert.match(source, /AppStatusPreviewFeature/, `${relativePath} should declare its status-preview dependency`);
    }
    const ntp = fs.readFileSync(path.join(ROOT, "src/webserver/application/ntp_state.ts"), "utf8");
    assert.match(ntp, /syncNtpServerUi\(runtime: UiRuntimeState, syncInput:/);
    assert.match(entry, /settingsHelpers = createSettingsPageHelpersFeature\([\s\S]*statusPreview,/);
    assert.match(entry, /createAppBackupFeature\([\s\S]*requestApi,[\s\S]*statusPreview/);
  });

  test("imports the browser core service without ambient application names", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(entry, /(?:core|layout)CompatibilityDescriptors/);
    for (const name of ["activeLayout", "isPortraitRotation", "normalizeGridSpansForLayout", "syncPreviewGridTop", "syncPreviewOrientation", "subpageStateDisplayMode", "webserverNow"]) {
      assert.doesNotMatch(globals, new RegExp(`\\bvar ${name}:`));
    }
  });

  test("injects device configuration without the CFG application global", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(globals, /\bvar CFG:/);
    assert.match(entry, /settingsPage = createSettingsPageFeature\([\s\S]*configurationCodec, runtime, core, layout, environment/);
    assert.match(entry, /createConfigPersistenceFeature\(nativePanelConfig, runtime, layout, entityState, shell\)/);
  });

  test("injects the device ID without an application global", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const migration = fs.readFileSync(path.join(ROOT, "src/webserver/application/native_panel_config_migration.ts"), "utf8");
    assert.doesNotMatch(globals, /\bvar DEVICE_ID:/);
    assert.doesNotMatch(migration, /\bDEVICE_ID\b|\bNUM_SLOTS\b|dependencies\?/);
    assert.match(entry, /createFirmwareUpdateFeature\(runtime, layout\.deviceId, firmwareVersion/);
    const publicFirmware = fs.readFileSync(path.join(ROOT, "src/webserver/application/public_firmware_install.ts"), "utf8");
    assert.match(entry, /publicFirmwareInstall = createPublicFirmwareInstallFeature\([\s\S]*deviceApi,[\s\S]*layout\.deviceId,[\s\S]*firmwareUpdate,[\s\S]*shell,[\s\S]*requestApi,[\s\S]*appEvents/);
    assert.doesNotMatch(entry, /installPublicFirmwareInstallModule/);
    assert.doesNotMatch(publicFirmware, /GlobalDescriptors|staticGlobal|liveGlobal|installPublicFirmwareInstallModule/);
    assert.doesNotMatch(globals, /\bvar (?:ensurePublicFirmwareOtaUrl|publicFirmwareOtaFilename|installPublicFirmwareViaWebOta|waitForFirmwareRestart|failPublicFirmwareUpload):/);
  });

  test("owns all slot and grid geometry without layout globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const grid = fs.readFileSync(path.join(ROOT, "src/webserver/application/grid.ts"), "utf8");
    const buttonSettings = fs.readFileSync(path.join(ROOT, "src/webserver/application/button_settings.ts"), "utf8");
    const backup = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_backup.ts"), "utf8");
    const stateHandlers = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_state_event_handlers.ts"), "utf8");
    const previewHooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_preview.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.equal(fs.existsSync(path.join(ROOT, "src/webserver/runtime/layout_compatibility.ts")), false);
    assert.doesNotMatch(globals, /\bvar (?:NUM_SLOTS|TOTAL_SLOTS|GRID_COLS|GRID_ROWS):/);
    assert.match(entry, /grid = createGridFeature\(configurationCodec, runtime, layout, entityState, requestApi, renderQueue\)/);
    assert.doesNotMatch(entry, /gridCompatibilityGlobals/);
    assert.doesNotMatch(grid, /GlobalDescriptors|staticGlobal|gridCompatibilityGlobals/);
    assert.doesNotMatch(globals, /\bvar (?:ctx|scheduleMainGridSave|cancelMainGridSave|applyButtonOrderValue|applyImportedButtonOrder|parseOrder|resolveIcon|serializeGrid|sizeClass|btnDisplayName):/);
    assert.match(grid, /export interface GridFeature/);
    assert.match(grid, /export function createGridFeature/);
    assert.doesNotMatch(entry, /installGridModule/);
    assert.match(entry, /createAppConfigEventsFeature\(configurationPersistence, configurationCodec, layout, renderQueue\)/);
    assert.match(entry, /installAppTestHooksPreview\(context\.cards, context\.configuration\.codec, context\.runtime, context\.core, context\.layout, context\.controllers\.screenRotation, context\.controllers\.firmwareVersion, context\.controllers\.statusPreview, context\.controllers\.grid, register\)/);
    const previewGridConsumers = [
      "preview_render.ts",
      "preview_grid_placement.ts",
      "preview_context_menu.ts",
      "preview_clipboard.ts",
      "preview_interactions.ts",
      "button_settings_selection.ts",
    ].map((file) => fs.readFileSync(path.join(ROOT, "src/webserver/application", file), "utf8"));
    for (const source of [buttonSettings, backup, stateHandlers, previewHooks, ...previewGridConsumers]) {
      assert.match(source, /GridFeature/);
    }
    assert.match(entry, /renderSelectionBar: \(\) => selection\.renderSelectionBar\(grid\.ctx\(\)\)/);
    assert.match(entry, /createAppStateEventHandlersFeature\([\s\S]*statusPreview,[\s\S]*grid/);
    assert.match(entry, /createAppBackupFeature\(\{[\s\S]*statusPreview,[\s\S]*grid/);
  });

  test("imports shared settings state helpers without application globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const settings = fs.readFileSync(path.join(ROOT, "src/webserver/application/settings_page.ts"), "utf8");
    const handlers = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_state_event_handlers.ts"), "utf8");
    const backup = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_backup.ts"), "utf8");
    assert.doesNotMatch(entry, /install(?:Language|Ntp|Idle|Artwork|Screensaver)StateModule/);
    assert.equal(fs.existsSync(path.join(ROOT, "src/webserver/application/artwork_state.ts")), false);
    for (const source of [settings, handlers, backup]) {
      assert.match(source, /from "\.\/(?:language|ntp|idle|screensaver)_state"/);
    }
    assert.doesNotMatch(globals, /\bvar (?:appendLanguageOption|getActiveScreensaverMode|hasCustomNtpServers|languageLabel|languageOptionsWithFallback|resetNtpServersToDefaults|syncIdleUi|syncLanguageSelect|syncNtpServerUi):/);
  });

  test("owns environment and voice-services state without application globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const environment = fs.readFileSync(path.join(ROOT, "src/webserver/application/environment_state.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(entry, /createEnvironmentStateFeature\(/);
    assert.doesNotMatch(entry, /installEnvironmentStateModule/);
    assert.match(environment, /export interface EnvironmentStateFeature/);
    assert.doesNotMatch(environment, /GlobalDescriptors|liveGlobal|staticGlobal/);
    assert.doesNotMatch(globals, /\bvar (?:_voiceServicesController|voiceServicesSupported|voiceServicesState|applyVoiceServicesState|voiceServicesUiState|isHomeAssistantAutoTimezone|effectiveTimezoneOptionForWeb|timezoneOptionsWithFallback|monthNameForIndex):/);
  });

  test("owns screen-schedule state without application globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const schedule = fs.readFileSync(path.join(ROOT, "src/webserver/application/screen_schedule_state.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(entry, /createScreenScheduleStateFeature\(/);
    assert.doesNotMatch(entry, /installScreenScheduleStateModule/);
    assert.match(schedule, /export interface ScreenScheduleStateFeature/);
    assert.doesNotMatch(schedule, /GlobalDescriptors|liveGlobal|staticGlobal/);
    assert.doesNotMatch(globals, /\bvar (?:_screenScheduleController|screenScheduleControllerState|applyScreenScheduleControllerState|formatDuration|formatHour|syncScreenScheduleUi):/);
  });

  test("owns screensaver-timeout state without application globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const timeout = fs.readFileSync(path.join(ROOT, "src/webserver/application/screensaver_timeout.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(entry, /createScreensaverTimeoutFeature\(/);
    assert.doesNotMatch(entry, /installScreensaverTimeoutModule/);
    assert.match(timeout, /export interface ScreensaverTimeoutFeature/);
    assert.doesNotMatch(timeout, /GlobalDescriptors|liveGlobal|staticGlobal/);
    assert.doesNotMatch(globals, /\bvar (?:SCREENSAVER_TIMEOUT_OPTIONS|readNumberMeta|syncScreensaverTimeoutLimits|screensaverTimeoutSupported|syncScreensaverTimeoutUi|applyScreensaverTimeoutState):/);
  });

  test("owns screen-rotation startup and UI state without application globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const rotation = fs.readFileSync(path.join(ROOT, "src/webserver/application/screen_rotation_state.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(entry, /createScreenRotationFeature\(/);
    assert.doesNotMatch(entry, /installScreenRotationStateModule/);
    assert.match(rotation, /export interface ScreenRotationFeature/);
    assert.doesNotMatch(rotation, /GlobalDescriptors|liveGlobal|staticGlobal/);
    assert.doesNotMatch(globals, /\bvar (?:SCREEN_ROTATION_STARTUP_FALLBACK_MS|normalizeScreenRotation|activeScreenRotationOptions|allScreenRotationOptions|syncScreenRotationSelect|displayScreenRotation|screenRotationSortValue|sortScreenRotationOptions|appendScreenRotationOption|screenRotationStartupRequired|gridPreviewBlockedByRotationStartup|clearInitialScreenRotationTimer|startInitialScreenRotationCheck|applyDeferredButtonOrderValue|resolveInitialScreenRotationCheck):/);
  });

  test("registers migrated card families through the typed registry", () => {
    const migratedCards = [
      ["sensor", "registerSensorCardTypes"],
      ["switch", "registerSwitchCardTypes"],
      ["door_window", "registerDoorWindowCardTypes"],
      ["image", "registerImageCardTypes"],
      ["lawn_mower", "registerLawnMowerCardTypes"],
      ["presence", "registerPresenceCardTypes"],
      ["push", "registerPushCardTypes"],
      ["screen_lock", "registerScreenLockCardTypes"],
      ["action", "registerActionCardTypes"],
      ["alarm", "registerAlarmCardTypes"],
      ["climate", "registerClimateCardTypes"],
      ["fan", "registerFanCardTypes"],
      ["light_temperature", "registerLightTemperatureCardTypes"],
      ["media", "registerMediaCardTypes"],
      ["subpage", "registerSubpageCardTypes"],
      ["vacuum", "registerVacuumCardTypes"],
    ];
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    for (const [fileName, registrationFunction] of migratedCards) {
      const relativePath = `src/webserver/cards/${fileName}.ts`;
      const source = fs.readFileSync(path.join(ROOT, relativePath), "utf8");
      assert.match(source, /registry\.register\(/, `${relativePath} should use the typed card registry`);
      assert.doesNotMatch(source, /\bregisterButtonType\s*\(/, `${relativePath} should not read ambient registration state`);
      assert.match(entry, new RegExp(
        `${registrationFunction}\\(\\s*registry(?:,\\s*(?:context\\.(?:configuration\\.[A-Za-z]+|controllers\\.[A-Za-z]+|core|device\\.id)|lightCards|fields|cardUi))*[,]?\\s*\\)`,
      ));
    }
  });

  test("registers garage and gate through the explicit cover-card factory", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const coverFactory = fs.readFileSync(path.join(ROOT, "src/webserver/cards/cover_like_card.ts"), "utf8");
    assert.match(coverFactory, /registry\.register\(config\.type/);
    assert.doesNotMatch(coverFactory, /\bregisterButtonType\s*\(/);
    assert.doesNotMatch(coverFactory, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal|descriptors)\b/);
    assert.match(entry, /registerGarageCardTypes\(\s*coverLikeCards\.register,\s*context\.configuration\.accessClimateAlarm,\s*context\.configuration\.confirmationOptions,?\s*\)/);
    assert.match(entry, /registerGateCardTypes\(\s*coverLikeCards\.register,\s*context\.configuration\.accessClimateAlarm,?\s*\)/);
    assert.doesNotMatch(entry, /registerCoverLikeCardType/);
    assert.doesNotMatch(entry, /registerCompatibility\((?:coverLikeCards\.descriptors|registerGarageCardTypes|registerGateCardTypes)/);
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(globals, /\bvar (?:GARAGE_CARD_METADATA|GARAGE_MODE_OPTIONS|GATE_CARD_METADATA|GATE_MODE_OPTIONS|coverLikeModeValues|normalizeCoverLikeMode|renderCoverLikeConfirmationSettings|garageCommandMode|garageModeDefaultIcon|garageModeDefaultLabel|garageModeOptionValues|garageUsesDefaultIcon|gateCommandMode|gateModeDefaultIcon|gateModeDefaultLabel|gateModeOptionValues|gateUsesDefaultIcon|normalizeGarageMode|normalizeGateMode):/);
  });

  test("keeps the card registry independent from the compatibility bootstrap", () => {
    const registry = fs.readFileSync(path.join(ROOT, "src/webserver/application/card_registry.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.doesNotMatch(registry, /GlobalDescriptors|installGlobals|registerCompatibility|compatibilityDefinitionCount/);
    assert.match(registry, /export function createCardRegistry\(\)/);
    assert.match(entry, /const cards = createCardRegistry\(\);/);
  });

  test("registers static card families without compatibility descriptors", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const cards = ["push", "screen_lock"];
    for (const card of cards) {
      const source = fs.readFileSync(path.join(ROOT, `src/webserver/cards/${card}.ts`), "utf8");
      assert.doesNotMatch(source, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    }
    for (const registration of ["registerPushCardTypes", "registerScreenLockCardTypes"]) {
      assert.match(entry, new RegExp(`^  ${registration}\\(registry, fields\\);`, "m"));
      assert.doesNotMatch(entry, new RegExp(`registerCompatibility\\(${registration}`));
    }
    assert.doesNotMatch(globals, /\bvar (?:PUSH_CARD_METADATA|SCREEN_LOCK_CARD_METADATA|pushActionSpec|pushDefaultIcon|pushDefaultIconOn):/);
  });

  test("keeps card metadata private to registry definitions", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    for (const card of ["climate", "door_window", "presence"]) {
      const source = fs.readFileSync(path.join(ROOT, `src/webserver/cards/${card}.ts`), "utf8");
      assert.doesNotMatch(source, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    }
    for (const registration of ["registerClimateCardTypes", "registerDoorWindowCardTypes", "registerPresenceCardTypes"]) {
      assert.doesNotMatch(entry, new RegExp(`registerCompatibility\\(${registration}`));
    }
    assert.doesNotMatch(globals, /\bvar (?:CLIMATE_CARD_METADATA|DOOR_WINDOW_CARD_METADATA|PRESENCE_CARD_METADATA):/);
  });

  test("registers switch cards without compatibility metadata", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const source = fs.readFileSync(path.join(ROOT, "src/webserver/cards/switch.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(source, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /^  registerSwitchCardTypes\(registry, context\.configuration\.confirmationOptions, lightCards, fields\);/m);
    assert.doesNotMatch(entry, /registerCompatibility\(registerSwitchCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:SWITCH_CARD_METADATA|LIGHT_SWITCH_CARD_METADATA):/);
  });

  test("registers the image card without compatibility helpers", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const source = fs.readFileSync(path.join(ROOT, "src/webserver/cards/image.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(source, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerImageCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:IMAGE_CARD_METADATA|imageModalModeOptions|renderImageLabelSettings|renderImageModalSettings):/);
  });

  test("registers weather cards through explicit shared options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const weather = fs.readFileSync(path.join(ROOT, "src/webserver/cards/weather.ts"), "utf8");
    const forecast = fs.readFileSync(path.join(ROOT, "src/webserver/cards/weather_forecast.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(weather, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal|CFG)\b/);
    assert.doesNotMatch(forecast, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal|WEATHER_CARD_METADATA)\b/);
    assert.match(entry, /const weatherCards = registerWeatherCardTypes\(registry, context\.configuration\.weatherOptions, context\.controllers\.clockBarState, fields, cardUi\);/);
    assert.match(entry, /registerWeatherForecastCardTypes\(registry, weatherCards, context\.controllers\.clockBarState, fields\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerWeather/);
    assert.doesNotMatch(globals, /\bvar (?:WEATHER_CARD_METADATA|WEATHER_FORECAST_CARD_METADATA|normalizeWeatherCardMode|weatherCardDefaultForecastLabel|weatherCardIsForecastMode|weatherForecastCardsSupported|weatherModeOptionValues|weatherModeOptions):/);
  });

  test("registers the webhook card through explicit shared options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/webhook.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /registerWebhookCardTypes\(registry, context\.configuration\.webhookOptions, fields, cardUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerWebhookCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:WEBHOOK_CARD_METADATA|WEBHOOK_HEADERS_OPTION|WEBHOOK_METHODS|normalizeWebhookConfig|setWebhookHeaders|webhookHeaders|webhookMethod):/);
  });

  test("registers the internal relay card with profile-owned options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/internal.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal|CFG)\b/);
    assert.match(entry, /registerInternalCardTypes\(\s*registry,\s*context\.configuration\.internalRelayOptions,\s*context\.dom\.document,\s*fields,?\s*\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerInternalCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:INTERNAL_CARD_METADATA|internalRelayDefaultIcon|internalRelayDefaultOnIcon|internalRelayLabelFor|internalRelayMode|internalRelayModeOptionValues|internalRelayOptions|internalRelaySpec|internalRelayUsesDefaultIcon|internalRelayUsesDefaultOnIcon|normalizeInternalRelayMode):/);
  });

  test("imports entity-mode helpers without compatibility globals", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const helpers = fs.readFileSync(path.join(ROOT, "src/webserver/cards/entity_mode_card.ts"), "utf8");
    const mower = fs.readFileSync(path.join(ROOT, "src/webserver/cards/lawn_mower.ts"), "utf8");
    const vacuum = fs.readFileSync(path.join(ROOT, "src/webserver/cards/vacuum.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(helpers, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal|registerEntityModeCardHelpers)\b/);
    assert.match(mower, /from "\.\/entity_mode_card"/);
    assert.match(vacuum, /from "\.\/entity_mode_card"/);
    assert.doesNotMatch(entry, /registerEntityModeCardHelpers/);
    assert.doesNotMatch(globals, /\bvar (?:applyEntityModeCardModeChange|entityModeCardUsesDefaultIcon|entityModeValues|normalizeEntityMode|normalizeEntityModeCardConfig):/);
  });

  test("registers robot cards through explicit shared options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const mower = fs.readFileSync(path.join(ROOT, "src/webserver/cards/lawn_mower.ts"), "utf8");
    const vacuum = fs.readFileSync(path.join(ROOT, "src/webserver/cards/vacuum.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(mower, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.doesNotMatch(vacuum, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /registerLawnMowerCardTypes\(registry, context\.configuration\.robotOptions, fields, cardUi\);/);
    assert.match(entry, /registerVacuumCardTypes\(registry, context\.configuration\.robotOptions, fields, cardUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(register(?:LawnMower|Vacuum)CardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:LAWN_MOWER_CARD_METADATA|LAWN_MOWER_CARD_MODES|VACUUM_CARD_METADATA|VACUUM_CARD_MODES|lawnMowerModeBadgeIcon|lawnMowerModeDefaultIcon|lawnMowerModeValues|lawnMowerUsesDefaultIcon|normalizeLawnMowerConfig|normalizeLawnMowerMode|normalizeVacuumConfig|vacuumModeBadgeIcon|vacuumModeDefaultIcon|vacuumModeNeedsArea|vacuumModeValues):/);
  });

  test("registers the lock card through explicit shared options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/lock.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /registerLockCardTypes\(registry, context\.configuration\.lockOptions, fields, cardUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerLockCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:LOCK_CARD_METADATA|lockCommandMode|lockModeDefaultIcon|lockModeDefaultLabel|lockModeOptionValues|lockUsesDefaultIcon|normalizeLockMode):/);
  });

  test("registers date/time cards through one explicit service", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    for (const card of ["calendar", "clock", "timezone"]) {
      const source = fs.readFileSync(path.join(ROOT, `src/webserver/cards/${card}.ts`), "utf8");
      assert.doesNotMatch(source, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal|DATE_TIME_CARD_METADATA)\b/);
    }
    assert.match(entry, /registerCalendarCardTypes\(registry, context\.configuration\.dateTimeOptions, fields\);/);
    assert.match(entry, /registerClockCardTypes\(registry, context\.configuration\.dateTimeOptions, fields\);/);
    assert.match(entry, /registerTimezoneCardTypes\(registry, context\.configuration\.dateTimeOptions, context\.dom\.document, fields\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerCalendarCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:DATE_TIME_CARD_METADATA|dateTimeCardMode|dateTimeCardTimeParts|dateTimeLargeNumbersLabel|dateTimeModeOptionValues|defaultTimezoneCardEntity|normalizeDateTimeCardMode|setDateTimeCardMode):/);
  });

  test("registers slider card families without compatibility descriptors", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/slider.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /registerSliderCardTypes\(registry, context\.configuration\.modalTabs, lightCards, fields, context\.controllers\.settingsUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerSliderCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:renderCoverControlTabSettings|sliderCardMetadata|sliderTypeFactory):/);
  });

  test("registers fan card families without compatibility descriptors", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/fan.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /registerFanCardTypes\(registry, context\.configuration\.modalTabs, fields, cardUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerFanCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:FAN_CARD_METADATA|FAN_CONTROL_TYPE_OPTIONS|fanControlBadgeIcon|fanControlDefaultIcon|fanTypeFactory|normalizeFanControlType|renderFanControlTabSettings|renderFanControlTypeField|setFanControlType):/);
  });

  test("registers alarm card families without compatibility descriptors", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/alarm.ts"), "utf8");
    const hooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_config.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /registerAlarmCardTypes\(registry, context\.configuration\.accessClimateAlarm, context\.controllers\.renderQueue, fields, cardUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerAlarmCardTypes/);
    assert.match(hooks, /alarmBehaviorSpec,[\s\S]*alarmActionSpecs,/);
    assert.doesNotMatch(globals, /\bvar (?:ALARM_CARD_METADATA|ALARM_CONTROL_PANEL_VALUE|alarmCardTypeOptions|alarmCardTypeOptionsForSettings|alarmControlPanelValue|alarmIconIsGenerated|alarmLabelIsGenerated|alarmUsesDefaultIcon|renderAlarmCardTypeField|renderAlarmVisibleActionsField|setAlarmCardType):/);
  });

  test("registers sensor cards through context-owned source options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/sensor.ts"), "utf8");
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_sensor_options.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(options, /sensorCardLocalSource = LOCAL_SENSOR_SOURCE/);
    assert.match(entry, /registerSensorCardTypes\(registry, context\.configuration\.options, fields, cardUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerSensorCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:SENSOR_CARD_LOCAL_SENSOR|SENSOR_CARD_METADATA|renderSensorLocalSettings|sensorCardIsLocal|sensorLocalPreview):/);
  });

  test("registers media cards through context-owned media options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/media.ts"), "utf8");
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_media_options.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(options, /mediaPlaylistSourceDefinitions/);
    assert.match(entry, /registerMediaCardTypes\(registry, context\.configuration\.mediaOptions, context\.device\.id, fields, context\.controllers\.settingsUi, cardUi\);/);
    assert.match(card, /deviceId: string/);
    assert.doesNotMatch(card, /\bDEVICE_ID\b/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerMediaCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:MEDIA_CARD_METADATA|MEDIA_PLAYLIST_SOURCE_DEFINITIONS|mediaBehaviorSpec|mediaDefaultMode|mediaEditorMode|mediaEditorValidMode|mediaLabelIsGenerated|mediaModeOptionValues|mediaNowPlayingControlValues|mediaNowPlayingControls|mediaNowPlayingPlayPauseEnabled|mediaNowPlayingProgressEnabled|mediaPlaylistContentIdPlaceholder|mediaPlaylistContentTypeKnown|mediaPlaylistContentTypeOptions|mediaPlaylistSourceDefinition|mediaPlaylistSourceOptions|mediaStateDisplayModeSupported|parseMediaPlaylistContentId|buildMediaPlaylistContentId):/);
  });

  test("registers subpage cards without compatibility descriptors", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/subpage.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /registerSubpageCardTypes\(registry, context\.configuration\.codec, context\.core, context\.controllers\.selection, fields, cardUi\);/);
    assert.match(card, /core: Pick<CoreFeature, "subpageStateDisplayMode">/);
    assert.match(card, /const \{ subpageStateDisplayMode \} = core;/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerSubpageCardTypes/);
    assert.doesNotMatch(globals, /\bvar (?:SUBPAGE_CARD_METADATA|appendEditSubpageButton|subpageBadgeLabelHtml):/);
  });

  test("registers action cards through context-owned action options", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/action.ts"), "utf8");
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_confirmation_options.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(card, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(options, /const actionCardActions/);
    assert.match(entry, /registerActionCardTypes\(registry, context\.configuration\.confirmationOptions, context\.controllers\.entityState, fields, cardUi\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerActionCardTypes/);
    assert.match(entry, /actionCardStateEntity: \(button\) => confirmationOptions\.actionCardStateEntity\(button\)/);
    assert.doesNotMatch(globals, /\bvar (?:ACTION_CARD_ACTIONS|ACTION_CARD_METADATA|actionCardInfo|actionCardIsLocal|actionCardIsOptionSelect|actionCardNeedsExtraValue|actionCardStateDisplayMode|actionCardStateEntity|actionCardStatePrecision|actionCardStateUnit|normalizeActionCardConfig|normalizeSavedConfigActionFields|renderActionCardLocalSettings|setActionCardStateOptions):/);
  });

  test("registers light card families through an explicit shared interface", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const light = fs.readFileSync(path.join(ROOT, "src/webserver/cards/light_temperature.ts"), "utf8");
    const slider = fs.readFileSync(path.join(ROOT, "src/webserver/cards/slider.ts"), "utf8");
    const switchCard = fs.readFileSync(path.join(ROOT, "src/webserver/cards/switch.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(light, /\b(?:GlobalDescriptors|staticGlobal|liveGlobal)\b/);
    assert.match(entry, /const lightCards = registerLightTemperatureCardTypes\(registry, context\.configuration\.modalTabs, fields, cardUi\);/);
    assert.match(entry, /registerSliderCardTypes\(registry, context\.configuration\.modalTabs, lightCards, fields, context\.controllers\.settingsUi\);/);
    assert.match(entry, /registerSwitchCardTypes\(registry, context\.configuration\.confirmationOptions, lightCards, fields\);/);
    assert.doesNotMatch(entry, /registerCompatibility\(registerLightTemperatureCardTypes/);
    assert.match(slider, /lightCards: LightCardRegistration/);
    assert.match(switchCard, /lightCards: LightCardRegistration/);
    assert.doesNotMatch(globals, /\bvar (?:LIGHT_CONTROL_TYPE_METADATA|LIGHT_CONTROL_TYPE_OPTIONS|LIGHT_FULL_CONTROL_CARD_METADATA|LIGHT_TEMPERATURE_CARD_METADATA|lightTempClampMax|lightTempClampMin|lightTempDefaultRange|lightTempLegacySensorValues|lightTempMaxLimit|lightTempMinLimit|lightTempMinMaxLimit|lightTempParseRange|lightTempSensorNeedsCleanup|lightTempSpec|lightTempStep|normalizeLightControlType|renderLightControlTabSettings|renderLightControlTypeField|setLightControlType):/);
  });

  test("injects the card registry into editor and preview consumers", () => {
    const consumers = [
      "src/webserver/application/button_settings.ts",
      "src/webserver/application/config_codec.ts",
      "src/webserver/application/config_sensor_options.ts",
      "src/webserver/application/controls_fields.ts",
      "src/webserver/application/preview_clipboard.ts",
      "src/webserver/application/preview_context_menu.ts",
      "src/webserver/application/preview_render.ts",
      "src/webserver/testing/app_test_hooks_config.ts",
      "src/webserver/testing/app_test_hooks_preview.ts",
    ];
    for (const relativePath of consumers) {
      const source = fs.readFileSync(path.join(ROOT, relativePath), "utf8");
      assert.doesNotMatch(source, /\bBUTTON_TYPES\b/, `${relativePath} should use the injected registry`);
    }
    const core = fs.readFileSync(path.join(ROOT, "src/webserver/application/core.ts"), "utf8");
    assert.doesNotMatch(core, /["']BUTTON_TYPES["']/);
  });

  test("owns sensor and status-card options in the application context", () => {
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_sensor_options.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(options, /createConfigSensorOptionsFeature/);
    assert.doesNotMatch(options, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigSensorOptionsModule/);
    assert.match(entry, /configurationOptions = createConfigSensorOptionsFeature\(cards\)/);
  });

  test("imports shared option contracts without installing globals", () => {
    const core = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_option_core.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.doesNotMatch(core, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigOptionCoreModule/);
    assert.match(core, /from "\.\.\/model\/config_primitives"/);
    assert.match(core, /export \{/);
  });

  test("imports action-card option storage without mutable globals", () => {
    const contract = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_action_contract.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/action.ts"), "utf8");
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_confirmation_options.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(contract, /export const ACTION_CARD_LOCAL_ACTION/);
    assert.match(card, /from "\.\.\/application\/config_confirmation_options"/);
    assert.match(options, /from "\.\/config_action_contract"/);
    assert.doesNotMatch(card, /liveGlobal\(\(\) => ACTION_CARD_(?:LOCAL_ACTION|OPTION_SELECT_ACTION|STATE_)/);
    assert.doesNotMatch(globals, /var ACTION_CARD_(?:LOCAL_ACTION|OPTION_SELECT_ACTION|STATE_)/);
  });

  test("imports cover mode and position rules without card globals", () => {
    const contract = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_cover_contract.ts"), "utf8");
    const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards/slider.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(contract, /export function normalizeCoverMode/);
    assert.match(card, /from "\.\.\/application\/config_cover_contract"/);
    assert.doesNotMatch(card, /staticGlobal\((?:coverCommandMode|coverModeOptionValues|normalizeCoverMode|coverModeOptionsForSettings|normalizeCoverPosition)\)/);
    assert.doesNotMatch(globals, /var (?:coverCommandMode|coverModeOptionValues|normalizeCoverMode|coverModeOptionsForSettings|normalizeCoverPosition)/);
  });

  test("imports subpage option behavior without installing globals", () => {
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_subpage_options.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.doesNotMatch(options, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigSubpageOptionsModule/);
    assert.match(options, /from "\.\.\/model\/config_primitives"/);
  });

  test("owns media option behavior in the application context", () => {
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_media_options.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(options, /createConfigMediaOptionsFeature/);
    assert.doesNotMatch(options, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigMediaOptionsModule/);
    assert.match(entry, /mediaConfigurationOptions = createConfigMediaOptionsFeature\(layout\.config\)/);
    assert.match(options, /from "\.\.\/model\/config_primitives"/);
  });

  test("owns image capacity and option behavior in the application context", () => {
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_image_options.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(options, /createConfigImageOptionsFeature/);
    assert.doesNotMatch(options, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigImageOptionsModule/);
    assert.match(entry, /imageConfigurationOptions = createConfigImageOptionsFeature/);
    assert.match(options, /connectSubpageParser/);
  });

  test("owns modal-tab behavior in the application context", () => {
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_modal_tab_options.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(options, /createConfigModalTabOptionsFeature/);
    assert.doesNotMatch(options, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigModalTabOptionsModule/);
    assert.match(entry, /modalTabOptions = createConfigModalTabOptionsFeature/);
    assert.match(options, /from "\.\.\/model\/config_primitives"/);
  });

  test("owns access, climate, and alarm options in the application context", () => {
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_access_climate_alarm_options.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(options, /createConfigAccessClimateAlarmOptionsFeature/);
    assert.doesNotMatch(options, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigAccessClimateAlarmOptionsModule/);
    assert.match(entry, /accessClimateAlarmOptions = createConfigAccessClimateAlarmOptionsFeature/);
    assert.match(options, /connectGarageConfirmationNormalizer/);
  });

  test("owns confirmation options in the application context", () => {
    const options = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_confirmation_options.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(options, /createConfigConfirmationOptionsFeature/);
    assert.doesNotMatch(options, /\b(?:staticGlobal|liveGlobal|GlobalDescriptors)\b/);
    assert.doesNotMatch(entry, /installConfigConfirmationOptionsModule/);
    assert.match(entry, /confirmationOptions = createConfigConfirmationOptionsFeature/);
    assert.match(options, /from "\.\.\/model\/config_primitives"/);
  });

  test("owns one configuration codec instance in the application context", () => {
    const codec = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_codec.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(codec, /createConfigCodecFeature/);
    assert.doesNotMatch(entry, /installConfigCodecModule/);
    assert.match(entry, /configurationCodec = createConfigCodecFeature/);
    assert.doesNotMatch(entry, /configuration\.codec\.globals/);
    assert.match(entry, /configurationCodec\.normalizeButtonConfig/);
    assert.match(entry, /configurationCodec\.serializeSubpageConfig/);
  });

  test("injects the configuration codec into editor and preview consumers", () => {
    const consumers = [
      "src/webserver/application/button_settings.ts",
      "src/webserver/application/preview_render.ts",
      "src/webserver/application/preview_grid_placement.ts",
      "src/webserver/application/preview_context_menu.ts",
      "src/webserver/application/preview_clipboard.ts",
      "src/webserver/application/preview_interactions.ts",
      "src/webserver/cards/subpage.ts",
      "src/webserver/testing/app_test_hooks_config.ts",
      "src/webserver/testing/app_test_hooks_preview.ts",
    ];
    for (const relativePath of consumers) {
      const source = fs.readFileSync(path.join(ROOT, relativePath), "utf8");
      assert.match(source, /ConfigCodecFeature/, `${relativePath} should declare its codec dependency`);
    }
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(entry, /codec: configurationCodec/);
    assert.match(entry, /installAppTestHooksPreview\(context\.cards, context\.configuration\.codec, context\.runtime, context\.core, context\.layout, context\.controllers\.screenRotation, context\.controllers\.firmwareVersion, context\.controllers\.statusPreview, context\.controllers\.grid, register\)/);
  });

  test("injects the configuration codec into persistence and application services", () => {
    const consumers = [
      "src/webserver/application/config_post_api.ts",
      "src/webserver/application/app_config_events.ts",
      "src/webserver/application/backup_contract.ts",
      "src/webserver/application/app_backup.ts",
      "src/webserver/application/grid.ts",
      "src/webserver/application/settings_page.ts",
      "src/webserver/application/settings_page_helpers.ts",
      "src/webserver/application/settings_schedule_section.ts",
      "src/webserver/application/settings_cover_art_section.ts",
    ];
    for (const relativePath of consumers) {
      const source = fs.readFileSync(path.join(ROOT, relativePath), "utf8");
      assert.match(source, /ConfigCodecFeature/, `${relativePath} should declare its codec dependency`);
    }
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const core = fs.readFileSync(path.join(ROOT, "src/webserver/application/core.ts"), "utf8");
    assert.doesNotMatch(core, /ConfigCodecFeature|GlobalDescriptors|staticGlobal|liveGlobal/);
    assert.doesNotMatch(core, /from "\.\.\/state\/app_instance"|\bpostText\(|\bentityName\(|\bsaveSubpageEntity\(/);
    assert.match(core, /CoreFeatureDependencies/);
    assert.match(core, /serializeSubpageGrid: \(subpage: any\) => any/);
    assert.match(entry, /\(subpage\) => configurationCodec\.serializeSubpageGrid\(subpage\)/);
    assert.match(entry, /configurationPersistence\.connectCodec\(configurationCodec\)/);
    assert.match(entry, /createAppConfigEventsFeature\(configurationPersistence, configurationCodec, layout, renderQueue\)/);
    assert.doesNotMatch(entry, /configuration\.codec\.globals/);
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(globals, /\bvar (?:normalizeButtonConfig|serializeButtonConfig|parseSubpageConfig|serializeSubpageConfig|getSubpage|bindTextPost):/);
  });

  test("composes configuration persistence without compatibility globals", () => {
    const persistence = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_post_api.ts"), "utf8");
    const codec = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_codec.ts"), "utf8");
    const backup = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_backup.ts"), "utf8");
    const hooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_config.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(persistence, /GlobalDescriptors|staticGlobal|liveGlobal|readonly globals/);
    assert.doesNotMatch(entry, /configPersistence\.globals/);
    assert.match(codec, /ConfigPersistenceFeature/);
    assert.match(backup, /ConfigPersistenceFeature/);
    assert.match(hooks, /ConfigPersistenceFeature/);
    assert.match(entry, /createConfigCodecFeature\([\s\S]*configurationPersistence/);
    assert.match(entry, /configPersistence: configurationPersistence/);
    assert.doesNotMatch(globals, /\bvar (?:SUBPAGE_RAW_CHUNK_FIELDS|saveButtonConfig|saveSubpageEntity|saveSubpageEntityLegacy|scheduleSliderSubpageMigration|subpageChunkShouldPost|subpageEntityKeys):/);
  });

  test("composes the backup contract without compatibility globals", () => {
    const contract = fs.readFileSync(path.join(ROOT, "src/webserver/application/backup_contract.ts"), "utf8");
    const backup = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_backup.ts"), "utf8");
    const hooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_backup.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(contract, /export function createBackupContractFeature/);
    assert.match(contract, /buttonConfigDisabledForDevice/);
    assert.doesNotMatch(contract, /GlobalDescriptors|staticGlobal|liveGlobal|installBackupContractModule/);
    assert.doesNotMatch(entry, /installGlobals\(installBackupContractModule/);
    assert.match(entry, /createBackupContractFeature\(backupModel, configurationCodec, cards, layout\)/);
    assert.match(backup, /readonly backupContract:/);
    assert.match(hooks, /backup: BackupContractFeature/);
    assert.doesNotMatch(globals, /\bvar (?:_backupFeature|backupEmptyButtonConfig|backupNormalizeButtonConfig|createBackupConfig|normalizeBackupConfig|planBackupImport):/);
  });

  test("composes backup and restore UI without compatibility globals", () => {
    const backup = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_backup.ts"), "utf8");
    const hooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_backup.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(backup, /GlobalDescriptors|staticGlobal|readonly globals|installAppBackupModule/);
    assert.doesNotMatch(entry, /backupUiFeature\.globals/);
    assert.match(hooks, /application: AppBackupFeature/);
    assert.match(entry, /installAppTestHooksBackup\(context\.layout, context\.backup\.contract, context\.backup\.application, register\)/);
    assert.doesNotMatch(globals, /\bvar (?:addNativeConfigToBackup|backupExportFileDate|backupExportFileName|normalizeImportedPanelSettings|gridColsForImportedSettings|backupExportScreenSizeSlug|downloadBackupConfig|exportConfig|importConfig):/);
  });

  test("composes the card-editor icon picker without compatibility globals", () => {
    const picker = fs.readFileSync(path.join(ROOT, "src/webserver/application/button_settings_icon_picker.ts"), "utf8");
    const settings = fs.readFileSync(path.join(ROOT, "src/webserver/application/button_settings.ts"), "utf8");
    const context = fs.readFileSync(path.join(ROOT, "src/webserver/application/application_context.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(picker, /export function createButtonSettingsIconPickerFeature/);
    assert.doesNotMatch(picker, /GlobalDescriptors|staticGlobal|liveGlobal|installButtonSettingsIconPickerModule/);
    assert.match(settings, /iconPicker: ButtonSettingsIconPickerFeature/);
    assert.match(settings, /iconPicker\.init\(picker, currentVal, onSelect\)/);
    assert.match(context, /readonly iconPicker: ButtonSettingsIconPickerFeature/);
    assert.match(entry, /createButtonSettingsIconPickerFeature\(dom\.document, \(\) => preview\.render\(\)\)/);
    assert.doesNotMatch(entry, /installGlobals\(installButtonSettingsIconPickerModule/);
    assert.doesNotMatch(globals, /\bvar initIconPicker:/);
  });

  test("imports shared UI primitives without application globals", () => {
    const primitives = fs.readFileSync(path.join(ROOT, "src/webserver/application/ui_primitives.ts"), "utf8");
    const stateModule = fs.readFileSync(path.join(ROOT, "src/webserver/application/state.ts"), "utf8");
    const core = fs.readFileSync(path.join(ROOT, "src/webserver/application/core.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(primitives, /export function iconSlug/);
    assert.match(primitives, /export function mdiIcon/);
    assert.match(primitives, /export function escHtml/);
    assert.doesNotMatch(stateModule, /staticGlobal\((?:uniqueOptions|setSelectValue|escHtml|escAttr|mdiIcon|textSpan)\)/);
    assert.doesNotMatch(core, /staticGlobal\(iconSlug\)/);
    assert.doesNotMatch(entry, /\.\.\.Icons/);
    assert.doesNotMatch(globals, /\bvar (?:iconSlug|mdiIcon|textSpan|escHtml|escAttr|uniqueOptions|setSelectValue|ICON_OPTIONS|DOMAIN_ICONS):/);
  });

  test("owns mutable UI runtime state in the application context", () => {
    const runtime = fs.readFileSync(path.join(ROOT, "src/webserver/application/state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(runtime, /createUiRuntimeState/);
    assert.doesNotMatch(entry, /installStateModule/);
    assert.match(entry, /runtime = createUiRuntimeState\(layout, dom\.document\)/);
    assert.doesNotMatch(entry, /installGlobals\(context\.runtime\.globals\)/);
    assert.match(entry, /getActiveSource: \(\) => runtime\.eventSource/);
  });

  test("composes the UI shell as one context-owned service", () => {
    const shell = fs.readFileSync(path.join(ROOT, "src/webserver/application/controls_shell.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(shell, /export function createControlsShellFeature/);
    assert.match(entry, /shell = createControlsShellFeature\(runtime, \{/);
    assert.match(entry, /schedule: \(\(callback: TimerHandler, delay\?: number\) => window\.setTimeout\(callback, delay\)\)/);
    assert.match(entry, /cancelSchedule: \(handle\) => \{ dom\.window\.clearTimeout\(handle\); \}/);
    assert.match(entry, /showBanner: shell\.showBanner/);
    assert.match(entry, /createDisclosureChevron: shell\.createDisclosureChevron/);
    assert.doesNotMatch(entry, /controlsShellCompatibilityGlobals/);
    assert.doesNotMatch(shell, /GlobalDescriptors|staticGlobal|controlsShellCompatibilityGlobals/);
    assert.doesNotMatch(globals, /\bvar (?:createMdiIcon|createActionButton|createDisclosureChevron|showBanner|buildUI|buildHeader|buildScreenPage|buildApplyBar|switchTab|syncTabChrome|isConfigLocked|syncConfigLockUi|setConfigLocked):/);
  });

  test("injects the UI shell into API and reconnect modules", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    for (const file of ["api.ts", "app_events.ts", "state_loader_api.ts", "public_firmware_install.ts"]) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", file), "utf8");
      assert.match(source, /Pick<ControlsShellFeature, "setConfigLocked" \| "showBanner">/);
      assert.match(source, /const \{ setConfigLocked, showBanner \} = shell/);
    }
    assert.match(entry, /createApplicationApiFeature\([\s\S]*screensaverTimeout,[\s\S]*shell/);
    assert.doesNotMatch(entry, /applicationApiCompatibilityGlobals/);
    assert.match(entry, /createAppEventsFeature\([\s\S]*entityState,[\s\S]*shell/);
  });

  test("injects the UI shell into settings and card-editor modules", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    for (const file of [
      "controls_fields.ts",
      "settings_system_section.ts",
      "settings_page.ts",
      "settings_page_helpers.ts",
      "button_settings.ts",
      "button_settings_selection.ts",
    ]) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", file), "utf8");
      assert.match(source, /ControlsShellFeature/, `${file} should declare its shell dependency`);
    }
    assert.match(entry, /createControlsFieldsFeature\(cards, configurationOptions, shell, requestApi, cardImages,/);
    assert.match(entry, /createButtonSettingsSelectionFeature\(/);
    assert.doesNotMatch(entry, /installButtonSettingsSelectionModule/);
  });

  test("composes shared fields without compatibility globals", () => {
    const fields = fs.readFileSync(path.join(ROOT, "src/webserver/application/controls_fields.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(fields, /export interface ControlsFieldsFeature/);
    assert.match(fields, /export function createControlsFieldsFeature/);
    assert.doesNotMatch(fields, /GlobalDescriptors|staticGlobal|liveGlobal|readonly globals/);
    assert.match(entry, /fields = createControlsFieldsFeature\(cards, configurationOptions, shell, requestApi, cardImages,/);
    assert.doesNotMatch(entry, /fields\.globals|installControlsFieldsModule/);
    assert.doesNotMatch(globals, /\bvar (?:makeCollapsibleCard|fieldLabel|textInput|fieldWithControl|selectField|segmentControl|colorField|toggleRow|applyCardMetadataFields|renderBasicCardFields|cardSensorPreviewHtml|cardBadgeLabelHtml|cardBadgePreview|condField|createRangeSlider):/);
    for (const file of ["action.ts", "calendar.ts", "sensor.ts", "switch.ts", "weather.ts"]) {
      const card = fs.readFileSync(path.join(ROOT, "src/webserver/cards", file), "utf8");
      assert.match(card, /ControlsFieldsFeature/, `${file} should receive the shared-fields service`);
    }
  });

  test("composes card selection without compatibility globals", () => {
    const selection = fs.readFileSync(path.join(ROOT, "src/webserver/application/button_settings_selection.ts"), "utf8");
    const context = fs.readFileSync(path.join(ROOT, "src/webserver/application/application_context.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(selection, /export function createButtonSettingsSelectionFeature/);
    assert.match(selection, /fields: Pick<ControlsFieldsFeature/);
    assert.doesNotMatch(selection, /GlobalDescriptors|staticGlobal|liveGlobal|installButtonSettingsSelectionModule/);
    assert.match(context, /readonly selection: ButtonSettingsSelectionFeature/);
    assert.match(entry, /selection = createButtonSettingsSelectionFeature\(/);
    assert.doesNotMatch(globals, /\bvar (?:clearCardSelection|closeSettings|handleDocumentSelectionMouseDown|hideSettingsOverlay|isSelectionControlTarget|openClockBarTemperatureSettings|openSelectedCardSettings|renderClockBarSelectionBar|renderSelectionBar|selectClockBarItem|updatePreviewHint):/);
  });

  test("composes preview rendering without its temporary render adapter", () => {
    const preview = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_render.ts"), "utf8");
    const settings = fs.readFileSync(path.join(ROOT, "src/webserver/application/button_settings.ts"), "utf8");
    const clipboard = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_clipboard.ts"), "utf8");
    const hooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_config.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(preview, /export function createPreviewRenderFeature/);
    assert.doesNotMatch(preview, /GlobalDescriptors|staticGlobal|liveGlobal|installPreviewRenderModule|readonly globals/);
    assert.match(settings, /preview: Pick<PreviewRenderFeature/);
    assert.match(clipboard, /preview: Pick<PreviewRenderFeature/);
    assert.match(hooks, /preview: Pick<PreviewRenderFeature/);
    assert.match(entry, /preview = createPreviewRenderFeature\(\{/);
    assert.doesNotMatch(entry, /context\.controllers\.preview\.globals/);
    assert.doesNotMatch(globals, /\bvar renderPreview:/);
    assert.doesNotMatch(globals, /\bvar (?:buttonTypeDisabledForDevice|buttonConfigDisabledForDevice|buttonTypeInfoOnlyVisible|buttonTypePickerDetails|buttonTypePickerKeys|buttonTypePickerOptionList|buttonTypeRegistryValue|buttonTypeVisibleInPicker|defaultButtonTypeForPicker|previewHtmlValue):/);
  });

  test("composes preview grid placement without compatibility globals", () => {
    const placement = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_grid_placement.ts"), "utf8");
    const clipboard = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_clipboard.ts"), "utf8");
    const interactions = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_interactions.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(placement, /export function createPreviewGridPlacementFeature/);
    assert.doesNotMatch(placement, /GlobalDescriptors|staticGlobal|liveGlobal|installPreviewGridPlacementModule/);
    assert.match(clipboard, /placement: Pick<PreviewGridPlacementFeature/);
    assert.match(interactions, /placement: Pick<PreviewGridPlacementFeature/);
    assert.match(entry, /placement = createPreviewGridPlacementFeature\(\{/);
    assert.doesNotMatch(entry, /installPreviewGridPlacementModule/);
    assert.doesNotMatch(globals, /\bvar (?:canPlaceSlotAt|findDuplicatePlacement|findPlacementCell|getCellFromEvent|moveSelectedToCell|moveToCell|placeOrderedGridEntries|placeSlotAt|resolveSpanPos):/);
  });

  test("composes preview clipboard and card transfer without compatibility globals", () => {
    const clipboard = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_clipboard.ts"), "utf8");
    const menu = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_context_menu.ts"), "utf8");
    const hooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_config.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(clipboard, /export function createPreviewClipboardFeature/);
    assert.doesNotMatch(clipboard, /GlobalDescriptors|staticGlobal|liveGlobal|installPreviewClipboardModule/);
    assert.match(menu, /clipboard: Pick<PreviewClipboardFeature/);
    assert.match(hooks, /clipboard: Pick<PreviewClipboardFeature/);
    assert.match(entry, /clipboard = createPreviewClipboardFeature\(\{/);
    assert.doesNotMatch(entry, /installPreviewClipboardModule/);
    assert.doesNotMatch(globals, /\bvar (?:buildClipboardEntry|clipboardEntriesFromCardTransfer|copyButtons|copySlot|cutButtons|cutSlot|pasteButton|pasteSubpageButton|showCopyCardCode|showPasteCardCode):/);
  });

  test("composes preview menus and interactions without compatibility globals", () => {
    const menu = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_context_menu.ts"), "utf8");
    const interactions = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_interactions.ts"), "utf8");
    const app = fs.readFileSync(path.join(ROOT, "src/webserver/application/app.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(menu, /export function createPreviewContextMenuFeature/);
    assert.match(interactions, /export function createPreviewInteractionsFeature/);
    assert.doesNotMatch(menu, /GlobalDescriptors|staticGlobal|liveGlobal|installPreviewContextMenuModule/);
    assert.doesNotMatch(interactions, /GlobalDescriptors|staticGlobal|liveGlobal|installPreviewInteractionsModule/);
    assert.match(app, /interactions\.setup\(\)/);
    assert.match(app, /contextMenu\.hide/);
    assert.match(entry, /contextMenu = createPreviewContextMenuFeature\(\{/);
    assert.match(entry, /interactions = createPreviewInteractionsFeature\(\{/);
    assert.doesNotMatch(entry, /installPreviewContextMenuModule|installPreviewInteractionsModule/);
    assert.doesNotMatch(globals, /\bvar (?:addBackButtonMenuItems|addBulkCardMenuItems|addClockBarMenuItems|addCtxDivider|addCtxItem|addCtxSubmenu|addSingleCardMenuItems|addSlot|addSubItem|addSubpageSlot|beginNewCardDraft|cardSizeMenuOptions|clearPlaceholder|clearTextSelection|ctxMenu|deleteButtons|deleteSlot|duplicateButton|duplicateSubpageButton|emptyButtonConfig|firstFreeCell|firstFreeSlot|handleBtnClick|hideContextMenu|newCardDraftKey|positionMenu|resizeSlot|selectButton|setupPreviewEvents|showBackContextMenu|showClockBarContextMenu|showContextMenu|showEmptySlotMenu|showSelectionMenu):/);
  });

  test("injects the UI shell into preview, persistence, backup, and startup modules", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    for (const file of [
      "preview_context_menu.ts",
      "preview_interactions.ts",
      "preview_clipboard.ts",
      "preview_render.ts",
      "config_post_api.ts",
      "app_backup.ts",
      "app.ts",
    ]) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", file), "utf8");
      assert.match(source, /ControlsShellFeature/, `${file} should declare its shell dependency`);
    }
    assert.match(entry, /createConfigPersistenceFeature\(nativePanelConfig, runtime, layout, entityState, shell\)/);
    assert.match(entry, /app = createAppFeature\([\s\S]*clockBarState, shell/);
  });

  test("injects preview drag state without ambient globals", () => {
    const interactions = fs.readFileSync(path.join(ROOT, "src/webserver/application/preview_interactions.ts"), "utf8");
    const shell = fs.readFileSync(path.join(ROOT, "src/webserver/application/controls_shell.ts"), "utf8");
    const runtime = fs.readFileSync(path.join(ROOT, "src/webserver/application/state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(interactions, /readonly runtime: UiRuntimeState/);
    assert.match(shell, /createControlsShellFeature\([\s\S]*runtime: UiRuntimeState/);
    assert.doesNotMatch(entry, /controlsShellCompatibilityGlobals/);
    assert.match(entry, /runtime,/);
    assert.doesNotMatch(runtime, /"(?:dragSrcPos|didDrag|previewPlaceholder|previewDropIdx|dragRafPending|dragSrcEl|dragIsSubpage|dragEnterCount)"/);
    assert.doesNotMatch(globals, /\bvar (?:dragSrcPos|didDrag|previewPlaceholder|previewDropIdx|dragRafPending|dragSrcEl|dragIsSubpage|dragEnterCount):/);
  });

  test("injects migration state without ambient globals", () => {
    const runtime = fs.readFileSync(path.join(ROOT, "src/webserver/application/state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const persistence = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_post_api.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(entry, /createConfigPersistenceFeature\(nativePanelConfig, runtime, layout, entityState, shell\)/);
    assert.match(entry, /createAppEventsFeature\([\s\S]*reconnect,[\s\S]*stateEventHandlers,[\s\S]*configEvents,[\s\S]*runtime,[\s\S]*pageTitle/);
    assert.match(persistence, /runtime\.pendingSliderSubpageMigrations/);
    assert.doesNotMatch(runtime, /"(?:orderReceived|migrationTimer|sliderMigrationTimer|pendingSliderSubpageMigrations)"/);
    assert.doesNotMatch(globals, /\bvar (?:orderReceived|migrationTimer|sliderMigrationTimer|pendingSliderSubpageMigrations):/);
  });

  test("removes runtime helper globals", () => {
    const runtime = fs.readFileSync(path.join(ROOT, "src/webserver/application/state.ts"), "utf8");
    const loader = fs.readFileSync(path.join(ROOT, "src/webserver/application/state_loader_api.ts"), "utf8");
    const settings = fs.readFileSync(path.join(ROOT, "src/webserver/application/button_settings_render_queue.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(loader, /runtime\.eventSource\.close\(\)/);
    assert.match(settings, /runtime\.isSettingsOpen\(\)/);
    assert.match(settings, /export function createButtonSettingsRenderQueueFeature/);
    assert.doesNotMatch(settings, /GlobalDescriptors|staticGlobal|liveGlobal|installButtonSettingsRenderQueueModule/);
    assert.match(entry, /createButtonSettingsRenderQueueFeature\(runtime, \{/);
    assert.match(entry, /context\.controllers\.renderQueue/);
    assert.doesNotMatch(entry, /installButtonSettingsRenderQueueModule/);
    assert.doesNotMatch(runtime, /"(?:_eventSource|isSettingsFocused|isSettingsOpen)"/);
    assert.doesNotMatch(globals, /\bvar (?:_eventSource|isSettingsFocused|isSettingsOpen|_renderPending|_settingsDeferred|scheduleRender):/);
  });

  test("injects settings DOM references", () => {
    const modules = [
      "settings_page_helpers.ts",
      "settings_schedule_section.ts",
      "settings_cover_art_section.ts",
      "settings_page.ts",
      "settings_system_section.ts",
    ];
    for (const moduleName of modules) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", moduleName), "utf8");
      assert.match(source, /UiRuntimeState/, `${moduleName} should declare its runtime dependency`);
      assert.match(source, /const els = .*runtime\.els/, `${moduleName} should use context-owned DOM references`);
    }
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(entry, /settingsHelpers = createSettingsPageHelpersFeature\(/);
    assert.match(entry, /const scheduleSection = createSettingsScheduleSectionFeature\(/);
    assert.match(entry, /const coverArtSection = createSettingsCoverArtSectionFeature\(/);
    assert.match(entry, /const systemSection = createSettingsSystemSectionFeature\(/);
    assert.match(entry, /settingsPage = createSettingsPageFeature\(/);
    assert.doesNotMatch(entry, /installSettings(?:PageHelpers|ScheduleSection|CoverArtSection|SystemSection|Page)Module/);
    for (const moduleName of modules) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", moduleName), "utf8");
      assert.doesNotMatch(source, /GlobalDescriptors|staticGlobal|liveGlobal/);
    }
    assert.doesNotMatch(globals, /\bvar (?:appendSettingsSection|buildCoverArtSettingsCard|buildScreenScheduleSettingsCard|buildSettingsPage|buildSystemSettingsCards|openVoiceServicesSettings|syncAlarmDelayAudioUi|syncClockScreensaverControls|syncCoverArtScreensaverUi|syncMediaPlayerSleepPreventionUi):/);
  });

  test("injects display-state DOM references", () => {
    const modules = [
      "appearance_state.ts", "c6_firmware_ui.ts", "clock_bar_state.ts",
      "firmware_update_state.ts", "firmware_version_state.ts", "idle_state.ts",
      "language_state.ts", "ntp_state.ts", "screen_schedule_state.ts", "screensaver_timeout.ts",
    ];
    for (const moduleName of modules) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", moduleName), "utf8");
      assert.match(source, /UiRuntimeState/, `${moduleName} should declare its runtime dependency`);
      assert.match(source, /const els = runtime\.els/, `${moduleName} should use context-owned DOM references`);
    }
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    assert.match(entry, /createScreenScheduleStateFeature\(/);
    assert.match(entry, /createFirmwareUpdateFeature\(runtime, layout\.deviceId, firmwareVersion/);
  });

  test("owns appearance behavior without application globals", () => {
    const appearance = fs.readFileSync(path.join(ROOT, "src/webserver/application/appearance_state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(appearance, /export interface AppearanceFeature/);
    assert.match(appearance, /createAppearanceFeature/);
    assert.doesNotMatch(appearance, /GlobalDescriptors|staticGlobal/);
    assert.match(entry, /appearance = createAppearanceFeature/);
    assert.doesNotMatch(entry, /installAppearanceStateModule/);
    assert.doesNotMatch(globals, /\bvar (?:syncColorUi|resetAppearanceColors):/);
  });

  test("owns firmware version behavior without application globals", () => {
    const firmwareVersion = fs.readFileSync(path.join(ROOT, "src/webserver/application/firmware_version_state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(firmwareVersion, /export interface FirmwareVersionFeature/);
    assert.match(firmwareVersion, /createFirmwareVersionFeature/);
    assert.doesNotMatch(firmwareVersion, /GlobalDescriptors|(?:live|static)Global/);
    assert.match(entry, /firmwareVersion = createFirmwareVersionFeature/);
    assert.doesNotMatch(entry, /installFirmwareVersionStateModule/);
    assert.doesNotMatch(globals, /\bvar (?:renderFirmwareVersion|setFirmwareVersion|displayFirmwareVersion|firmwareVersionLabel):/);
  });

  test("owns firmware update behavior without application globals", () => {
    const firmwareUpdate = fs.readFileSync(path.join(ROOT, "src/webserver/application/firmware_update_state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(firmwareUpdate, /export interface FirmwareUpdateFeature/);
    assert.match(firmwareUpdate, /createFirmwareUpdateFeature/);
    assert.doesNotMatch(firmwareUpdate, /GlobalDescriptors|(?:live|static)Global/);
    assert.match(entry, /firmwareUpdate = createFirmwareUpdateFeature/);
    assert.doesNotMatch(entry, /installFirmwareUpdateStateModule/);
    assert.doesNotMatch(globals, /\bvar (?:firmwareUpdateAvailable|latestFirmwareInstallAction|setFirmwareUpdateInfo|renderFirmwareUpdateStatus|syncFirmwareUpdateUi|startFirmwareInstallRefresh):/);
  });

  test("owns C6 firmware behavior without application globals", () => {
    const c6Firmware = fs.readFileSync(path.join(ROOT, "src/webserver/application/c6_firmware_ui.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(c6Firmware, /export interface C6FirmwareFeature/);
    assert.match(c6Firmware, /createC6FirmwareFeature/);
    assert.doesNotMatch(c6Firmware, /GlobalDescriptors|(?:live|static)Global/);
    assert.match(entry, /c6Firmware = createC6FirmwareFeature/);
    assert.doesNotMatch(entry, /installC6FirmwareUiModule/);
    assert.doesNotMatch(globals, /\bvar (?:c6FirmwareUpdateKnownAvailable|syncC6FirmwareUi|setC6FirmwareCurrentVersion|setC6FirmwareLatestVersion|setC6FirmwareUpdateAvailable):/);
  });

  test("owns Clock Bar behavior without application globals", () => {
    const clockBar = fs.readFileSync(path.join(ROOT, "src/webserver/application/clock_bar_state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(clockBar, /export interface ClockBarFeature/);
    assert.match(clockBar, /createClockBarFeature/);
    assert.doesNotMatch(clockBar, /GlobalDescriptors|(?:live|static)Global/);
    assert.match(entry, /clockBarState = createClockBarFeature/);
    assert.doesNotMatch(entry, /installClockBarStateModule/);
    assert.doesNotMatch(globals, /\bvar (?:clockBarVisibleInPreview|temperatureUnitSymbol|applyClockBarTemperatureEntities|syncClockBarUi|setClockBarItemVisible):/);
  });

  test("removes the ambient DOM registry", () => {
    const runtime = fs.readFileSync(path.join(ROOT, "src/webserver/application/state.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const consumers = fs.readdirSync(path.join(ROOT, "src/webserver/application"))
      .filter((name) => name.endsWith(".ts"))
      .map((name) => [name, fs.readFileSync(path.join(ROOT, "src/webserver/application", name), "utf8")])
      .filter(([, source]) => /\bels\b/.test(source));
    for (const [name, source] of consumers) {
      if (name === "state.ts") continue;
      assert.match(source, /const els = .*runtime\.els/, `${name} should use the injected DOM registry`);
    }
    assert.doesNotMatch(runtime, /globals: GlobalDescriptors/);
    assert.doesNotMatch(entry, /context\.runtime\.globals/);
    assert.doesNotMatch(globals, /\bvar els:/);
  });

  test("owns page-title behavior without compatibility globals", () => {
    const title = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_title.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const events = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_events.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(title, /createAppTitleFeature/);
    assert.doesNotMatch(title, /GlobalDescriptors|staticGlobal/);
    assert.match(entry, /pageTitle = createAppTitleFeature/);
    assert.match(events, /pageTitle\.handleWebServerPingEvent/);
    assert.doesNotMatch(entry, /installAppTitleModule/);
    assert.doesNotMatch(globals, /\bvar (?:applyPageTitle|handleWebServerPingEvent|loadPageTitleFromEventStream):/);
  });

  test("imports web styles directly", () => {
    const styles = fs.readFileSync(path.join(ROOT, "src/webserver/application/styles.ts"), "utf8");
    const app = fs.readFileSync(path.join(ROOT, "src/webserver/application/app.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.match(styles, /export function createWebStyles\(dragAnimation: boolean\)/);
    assert.match(styles, /import \{ WEB_UI_COLORS \} from "\.\.\/state\/ui_tokens"/);
    assert.match(app, /style\.textContent = webStyles/);
    assert.match(entry, /createWebStyles\(layout\.config\.dragAnimation\)/);
    assert.doesNotMatch(styles, /\bCFG\b/);
    assert.doesNotMatch(entry, /installStylesModule/);
    assert.doesNotMatch(globals, /\bvar WEB_STYLES:/);
  });

  test("imports shared UI colour tokens directly", () => {
    const consumers = [
      "application/appearance_state.ts",
      "application/preview_render.ts",
      "application/settings_page.ts",
      "application/styles.ts",
      "cards/image.ts",
      "cards/media.ts",
      "state/app_state.ts",
    ];
    for (const consumer of consumers) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver", consumer), "utf8");
      assert.match(source, /import \{ WEB_UI_COLORS \} from /, `${consumer} should import the UI colours`);
    }
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    assert.doesNotMatch(entry, /UiTokens/);
    assert.doesNotMatch(globals, /\bvar WEB_UI_COLORS:/);
  });

  test("imports firmware metadata helpers directly", () => {
    const metadata = fs.readFileSync(path.join(ROOT, "src/webserver/application/firmware_metadata.ts"), "utf8");
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const consumers = [
      "application/firmware_update_state.ts",
      "application/firmware_version_state.ts",
      "application/public_firmware_install.ts",
      "application/settings_system_section.ts",
      "application/state_loader_api.ts",
      "testing/app_test_hooks_settings.ts",
    ];
    assert.match(metadata, /export function firmwareInfoFromPublicManifest/);
    assert.match(metadata, /import \{ deviceId \} from "\.\.\/device_config"/);
    assert.doesNotMatch(metadata, /GlobalDescriptors|liveGlobal|staticGlobal/);
    for (const consumer of consumers) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver", consumer), "utf8");
      assert.match(source, /from "(?:\.\.\/application\/|\.\/)firmware_metadata"/, `${consumer} should import firmware metadata`);
    }
    assert.doesNotMatch(entry, /installFirmwareMetadataModule/);
    assert.doesNotMatch(globals, /\bvar (?:FIRMWARE_VERSION_METADATA_PATH|FIRMWARE_PUBLIC_MANIFEST_BASE|isSpecificFirmwareVersion|firmwareVersionFromMetadata|firmwareVersionsSame|publicFirmwareManifestUrl|publicFirmwareVersionsUrl|publicFirmwareAssetUrl|firmwareInfoFromPublicManifest|firmwareInfoFromPublicVersionEntry|firmwareInfosFromPublicVersions):/);
  });

  test("imports request and event contracts directly", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const api = fs.readFileSync(path.join(ROOT, "src/webserver/application/api.ts"), "utf8");
    const events = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_events.ts"), "utf8");
    const handlers = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_state_event_handlers.ts"), "utf8");
    const entities = fs.readFileSync(path.join(ROOT, "src/webserver/application/entity_state.ts"), "utf8");
    assert.match(api, /import \{ requestFailureInfo \} from "\.\.\/api\/request_failure"/);
    assert.match(events, /from "\.\.\/state\/event_aliases"/);
    assert.match(events, /from "\.\.\/state\/event_state"/);
    assert.match(events, /from "\.\.\/state\/firmware_events"/);
    assert.match(handlers, /import \{ applyClockBarStateValue \} from "\.\.\/state\/event_state"/);
    assert.match(entities, /import \{ entityStateKeys \} from "\.\.\/state\/event_state"/);
    assert.doesNotMatch(entry, /\b(?:RequestFailure|EventAliases|EventState|FirmwareEvents)\b/);
    assert.doesNotMatch(globals, /\bvar (?:SSE_ALIAS_GROUPS|requestFailureInfo|applySseHandlerAliases|entityStateKeys|applyClockBarStateValue|isRemovedLegacyStateEvent|resetStateForConnection|parseEntityEventData|isFirmwareVersionEvent|isFirmwareUpdateEvent|isFirmwareCheckButtonEvent|isFirmwareInstallButtonEvent|isC6FirmwareCurrentEvent|isC6FirmwareLatestEvent|isC6FirmwareUpdateAvailableEvent|isC6FirmwareAutoUpdateEvent|isC6FirmwareCheckButtonEvent|isC6FirmwareInstallButtonEvent):/);
  });

  test("imports initial application-state contracts directly", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const consumers = [
      "application/app_state_event_handlers.ts",
      "application/environment_state.ts",
      "application/language_state.ts",
      "application/ntp_state.ts",
      "application/settings_page.ts",
    ];
    for (const consumer of consumers) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver", consumer), "utf8");
      assert.match(source, /from "\.\.\/state\/app_state"/, `${consumer} should import application-state contracts`);
    }
    assert.match(entry, /import \{ NTP_SERVER_DEFAULTS, defaultTimezoneOptionsForDevice \} from "\.\/state\/app_state"/);
    assert.doesNotMatch(entry, /\bAppState\b/);
    assert.doesNotMatch(globals, /\bvar (?:AUTO_TIMEZONE_OPTION|FALLBACK_TIMEZONE_OPTION|NTP_SERVER_DEFAULTS|LANGUAGE_LABELS|defaultTimezoneOptionsForDevice|createInitialState):/);
  });

  test("imports generated card contracts directly", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const roots = ["application", "cards", "testing"];
    for (const directory of roots) {
      const root = path.join(ROOT, "src/webserver", directory);
      for (const name of fs.readdirSync(root).filter((file) => file.endsWith(".ts"))) {
        const source = fs.readFileSync(path.join(root, name), "utf8");
        if (!/\b(?:cardContract[A-Z]|CARD_RUNTIME_SPECS)\b/.test(source)) continue;
        assert.match(source, /from "\.\.\/generated\/card_contract"/, `${directory}/${name} should import its generated card contract`);
      }
    }
    assert.doesNotMatch(entry, /\bCardContract\b/);
    assert.doesNotMatch(globals, /\bvar (?:CARD_CONFIG_FIELDS|CARD_CONTRACT_[A-Z_]+|cardContract[A-Z][A-Za-z]+):/);
  });

  test("keeps device configuration module-owned", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const metadata = fs.readFileSync(path.join(ROOT, "src/webserver/application/firmware_metadata.ts"), "utf8");
    const instance = fs.readFileSync(path.join(ROOT, "src/webserver/state/app_instance.ts"), "utf8");
    assert.doesNotMatch(entry, /\.\.\.DeviceConfig/);
    assert.match(metadata, /import \{ deviceId \} from "\.\.\/device_config"/);
    assert.match(instance, /import \{ deviceConfig \} from "\.\.\/device_config"/);
    assert.doesNotMatch(globals, /\bvar (?:deviceId|deviceConfig):/);
  });

  test("imports configuration primitives directly", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const roots = ["application", "cards"];
    for (const directory of roots) {
      const root = path.join(ROOT, "src/webserver", directory);
      for (const name of fs.readdirSync(root).filter((file) => file.endsWith(".ts"))) {
        const source = fs.readFileSync(path.join(root, name), "utf8");
        if (!/\b(?:configOptionEnabled|configOptionValue|setConfigOption|setConfigOptionValue)\b/.test(source)) continue;
        assert.match(source, /from "\.\.\/model\/config_primitives"/, `${directory}/${name} should import configuration primitives`);
      }
    }
    assert.doesNotMatch(entry, /\bConfigPrimitives\b/);
    assert.doesNotMatch(globals, /\bvar (?:configOptionEnabled|configOptionValue|setConfigOption|setConfigOptionValue):/);
  });

  test("imports the shared model namespace directly", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const roots = ["application", "testing"];
    for (const directory of roots) {
      const root = path.join(ROOT, "src/webserver", directory);
      for (const name of fs.readdirSync(root).filter((file) => file.endsWith(".ts"))) {
        const source = fs.readFileSync(path.join(root, name), "utf8");
        if (!/\bEspControlModel\b/.test(source)) continue;
        assert.match(source, /import \* as EspControlModel from "\.\.\/model"/, `${directory}/${name} should import the shared model`);
      }
    }
    assert.doesNotMatch(entry, /EspControlModel\s*:/);
    assert.doesNotMatch(globals, /\bvar EspControlModel:/);
  });

  test("imports static catalogues and injects timezone defaults", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    for (const name of ["config_post_api.ts", "entity_state.ts", "state_loader_api.ts"]) {
      const source = fs.readFileSync(path.join(ROOT, "src/webserver/application", name), "utf8");
      assert.match(source, /import \{ ENTITY_CATALOG \} from "\.\.\/generated\/entity_catalog"/);
    }
    const environment = fs.readFileSync(path.join(ROOT, "src/webserver/application/environment_state.ts"), "utf8");
    const settingsHooks = fs.readFileSync(path.join(ROOT, "src/webserver/testing/app_test_hooks_settings.ts"), "utf8");
    assert.match(environment, /defaultTimezoneOptions: \(\) => string\[\]/);
    assert.match(settingsHooks, /defaultTimezoneOptions: \(\) => string\[\]/);
    assert.doesNotMatch(entry, /import \{ ENTITY_CATALOG \}/);
    assert.doesNotMatch(globals, /\bvar (?:ENTITY_CATALOG|defaultTimezoneOptions):/);
  });

  test("removes the static Product Model bootstrap", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const runtime = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/globals.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const model = fs.readFileSync(path.join(ROOT, "src/webserver/model/index.ts"), "utf8");
    const exportedValues = [...model.matchAll(/export \{([\s\S]*?)\} from/g)]
      .flatMap((match) => match[1].split(","))
      .map((name) => name.trim())
      .filter((name) => /^[A-Za-z_$][\w$]*$/.test(name));
    assert.doesNotMatch(entry, /installStaticGlobals|\.\.\.Model/);
    assert.doesNotMatch(runtime, /function installStaticGlobals/);
    for (const name of exportedValues) {
      assert.doesNotMatch(globals, new RegExp(`\\bvar ${name}:`), `${name} should not be ambient`);
    }
  });

  test("removes the application compatibility bootstrap and enforces the global allowlist", () => {
    const entry = fs.readFileSync(path.join(ROOT, "src/webserver/entry.ts"), "utf8");
    const runtime = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/globals.ts"), "utf8");
    const globals = fs.readFileSync(path.join(ROOT, "src/webserver/runtime/application_globals.d.ts"), "utf8");
    const app = fs.readFileSync(path.join(ROOT, "src/webserver/application/app.ts"), "utf8");
    const start = fs.readFileSync(path.join(ROOT, "src/webserver/application/app_start.ts"), "utf8");
    const settings = fs.readFileSync(path.join(ROOT, "src/webserver/application/button_settings.ts"), "utf8");
    assert.doesNotMatch(entry, /installApplicationCompatibility|installGlobals|installAppModule|installButtonSettingsModule|installAppStartModule/);
    assert.doesNotMatch(runtime, /GlobalDescriptors|installGlobals|staticGlobal|liveGlobal/);
    assert.doesNotMatch(app, /GlobalDescriptors|installAppModule|staticGlobal|liveGlobal/);
    assert.doesNotMatch(settings, /GlobalDescriptors|installButtonSettingsModule|staticGlobal|liveGlobal/);
    assert.match(start, /startApp\(app:/);
    assert.match(entry, /startApp\(context\.controllers\.app\)/);
    const ambientNames = [...globals.matchAll(/\bvar\s+([A-Za-z_$][\w$]*):/g)].map((match) => match[1]);
    assert.deepEqual(ambientNames, ["__ESPCONTROL_TEST_HOOKS__"]);
    assert.match(entry, /if \(__ESPCONTROL_TEST_HOOKS_ENABLED__\) \{[\s\S]*installTestHooks\(context, lightCards\)/);

    const browserSources = [
      ...sourceFiles(path.join(ROOT, "src/webserver")).filter((file) => file.endsWith(".ts")),
      path.join(ROOT, "scripts/build_web_bundle.js"),
    ];
    const integrationNames = new Set();
    const globalAliasFiles = [];
    for (const file of browserSources) {
      const source = fs.readFileSync(file, "utf8");
      for (const match of source.matchAll(/__ESPCONTROL_[A-Z0-9_]+__/g)) {
        integrationNames.add(match[0]);
      }
      if (/globalThis\s+as\s+/.test(source)) {
        globalAliasFiles.push(path.relative(ROOT, file));
      }
    }
    assert.deepEqual([...integrationNames].sort(), [
      "__ESPCONTROL_DEFAULT_DEVICE_ID__",
      "__ESPCONTROL_DEVICE_PROFILES__",
      "__ESPCONTROL_DEVICE_PROFILE__",
      "__ESPCONTROL_EMBEDDED_MDI_STYLES__",
      "__ESPCONTROL_RELOAD_EMBEDDED__",
      "__ESPCONTROL_START_EMBEDDED__",
      "__ESPCONTROL_TEST_HOOKS_ENABLED__",
      "__ESPCONTROL_TEST_HOOKS__",
      "__ESPCONTROL_TIMEZONE_OPTIONS__",
      "__ESPCONTROL_UI_STARTED__",
      "__ESPCONTROL_UI_STARTING__",
      "__ESPCONTROL_USING_EMBEDDED__",
    ]);
    assert.deepEqual(globalAliasFiles.sort(), [
      "src/webserver/application/app_start.ts",
      "src/webserver/device_config.ts",
      "src/webserver/entry.ts",
    ]);
  });

  test("preserves settings normalization", () => {
    runSettingsFeatureTests();
  });

  test("preserves state and event aliases", () => {
    runStateContractTests();
  });

  test("preserves request fallback and ordering", async () => {
    await runDeviceApiTests();
  });
});
