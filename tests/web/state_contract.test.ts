import { createInitialState } from "../../src/webserver/state/app_state";
import { SSE_ALIAS_GROUPS, applySseHandlerAliases, type SseHandlers } from "../../src/webserver/state/event_aliases";
import {
  applyClockBarStateValue,
  entityStateKeys,
  isRemovedLegacyStateEvent,
  parseEntityEventData,
  resetStateForConnection,
} from "../../src/webserver/state/event_state";
import {
  isC6FirmwareInstallButtonEvent,
  isFirmwareUpdateEvent,
  isFirmwareVersionEvent,
} from "../../src/webserver/state/firmware_events";
import type { DeviceConfig } from "../../src/webserver/state/types";

function assert(condition: unknown, message: string): asserts condition {
  if (!condition) throw new Error(message);
}

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deviceConfig(overrides: Partial<DeviceConfig> = {}): DeviceConfig {
  return {
    slots: 4,
    cols: 2,
    rows: 2,
    screenSize: "test",
    dragMode: "swap",
    dragAnimation: true,
    imageSlotCapacity: 4,
    screen: { width: "100%", aspect: "1/1" },
    ...overrides,
  };
}

export function runStateContractTests(): void {
  const first = createInitialState(deviceConfig({
    timezoneOptions: ["UTC (GMT+0)", "Auto (Home Assistant)"],
    features: { screenRotation: true, screenRotationDefault: "180", screenRotationOptions: ["0", "180"] },
  }));
  const second = createInitialState(deviceConfig());
  equal(first.grid.length, 4, "startup creates one grid slot per device slot");
  equal(first.buttons.length, 4, "startup creates one card per device slot");
  equal(first.buttons[0]?.icon, "Auto", "startup card defaults remain compatible");
  equal(first.screenRotation, "180", "startup uses the device rotation default");
  equal(first.screenRotationInitialReady, false, "rotation-capable devices wait for the initial event");
  equal(first.timezoneOptions.length, 2, "startup copies embedded timezone options");
  first.grid[0] = 9;
  first.buttons[0]!.label = "Changed";
  equal(second.grid[0], 0, "state factories do not share grid arrays");
  equal(second.buttons[0]?.label, "", "state factories do not share card objects");

  equal(SSE_ALIAS_GROUPS.clockBar.join("|"), [
    "switch-screen__clock_bar", "switch-screen_clock_bar", "switch-clock_bar_enabled",
  ].join("|"), "clock-bar aliases retain their compatibility order");
  equal(SSE_ALIAS_GROUPS.coverArtHideExternalInput.join("|"), [
    "switch-screen_saver__hide_cover_art_on_external_input",
    "switch-screen_saver_hide_cover_art_on_external_input",
    "switch-hide_cover_art_on_external_input",
    "switch-cover_art_hide_external_input",
    "switch-screen_saver__hide_for_external_sources",
  ].join("|"), "cover-art external-input aliases remain exact");
  equal(SSE_ALIAS_GROUPS.trackOverlayDuration.join("|"), [
    "number-screen_saver__track_overlay_duration",
    "number-screen_saver_track_overlay_duration",
    "number-track_overlay_duration",
    "number-screen_saver__show_track_overlay",
  ].join("|"), "track-overlay aliases remain exact");

  first.selectedSlots = [1, 2];
  first.lastClickedSlot = 2;
  first.editingSubpage = 3;
  first.subpageSelectedSlots = [4];
  first.subpageLastClicked = 4;
  first.onColor = "ABCDEF";
  resetStateForConnection(first);
  equal(first.selectedSlots.length, 0, "reconnect clears selected main-grid slots");
  equal(first.lastClickedSlot, -1, "reconnect clears the last main-grid slot");
  equal(first.editingSubpage, null, "reconnect exits subpage editing");
  equal(first.subpageSelectedSlots.length, 0, "reconnect clears selected subpage slots");
  equal(first.subpageLastClicked, -1, "reconnect clears the last subpage slot");
  equal(first.onColor, "ABCDEF", "reconnect preserves loaded configuration values");

  const calls: string[] = [];
  const handlers: SseHandlers = {};
  const canonicals: Readonly<Record<keyof typeof SSE_ALIAS_GROUPS, string>> = {
    clockBar: "switch-screen__clock_bar",
    clockBarTime: "switch-screen__clock_bar_time",
    clockBarTemperatureEntities: "text-clock_bar_temperature_entities",
    networkStatus: "switch-screen__network_status_icon",
    voiceServices: "switch-voice_services",
    temperatureDegreeSymbol: "switch-screen__temperature_degree_symbol",
    subpageChevron: "switch-screen__subpage_chevron",
    screensaverTimeout: "number-screensaver_timeout",
    clockScreensaver: "switch-screen_saver__clock",
    mediaPlayerSleepPrevention: "switch-screen_saver__media_player_sleep_prevention",
    mediaPlayerSleepPreventionEntity: "text-media_player_sleep_prevention_entity",
    coverArt: "switch-screen_saver__cover_art",
    coverArtEntity: "text-screen_saver__cover_art_entity",
    coverArtConditions: "text-screen_saver__cover_art_conditions",
    coverArtDelay: "number-screen_saver__cover_art_delay",
    trackOverlayDuration: "number-screen_saver__track_overlay_duration",
    coverArtHideExternalInput: "switch-screen_saver__hide_cover_art_on_external_input",
    homeAssistantArtworkProtocol: "select-home_assistant_artwork_protocol",
    homeAssistantArtworkPort: "number-home_assistant_artwork_port",
    scheduleTrigger: "text-screen__schedule_trigger",
    scheduleSensorActivation: "select-screen__schedule_sensor_activation",
    scheduleWakeTimeout: "number-screen__schedule_wake_timeout",
    scheduleWakeBrightness: "number-screen__schedule_wake_brightness",
    scheduleDimmedBrightness: "number-screen__schedule_dimmed_brightness",
    scheduleClockBrightness: "number-screen__schedule_clock_brightness",
    scheduleClockTextColor: "text-screen__schedule_clock_text_color",
    screenActiveTimezone: "text_sensor-screen__active_timezone",
    screenLanguage: "select-screen__language",
    ntpServer1: "text-screen__ntp_server_1",
    ntpServer2: "text-screen__ntp_server_2",
    ntpServer3: "text-screen__ntp_server_3",
    firmwareAutoUpdate: "switch-firmware__auto_update",
    firmwareUpdateFrequency: "select-firmware__update_frequency",
  };
  for (const [group, canonical] of Object.entries(canonicals)) {
    handlers[canonical] = () => calls.push(group);
  }
  applySseHandlerAliases(handlers);
  handlers["switch-clock_bar_enabled"]?.("ON", {}, "switch-clock_bar_enabled");
  handlers["text-ntp_server_1"]?.("time.example", {}, "text-ntp_server_1");
  equal(calls.join(","), "clockBar,ntpServer1", "legacy aliases dispatch to their canonical handlers");

  const clockState = createInitialState(deviceConfig());
  applyClockBarStateValue(clockState, "ON", { id: "switch-screen__clock_bar", value: true }, "switch-screen__clock_bar");
  applyClockBarStateValue(clockState, "OFF", { id: "switch-clock_bar_enabled", value: false }, "switch-clock_bar_enabled");
  equal(clockState.clockBarOn, true, "a stale alias cannot turn off an active canonical clock-bar source");
  applyClockBarStateValue(clockState, "OFF", { id: "switch-screen__clock_bar", value: false }, "switch-screen__clock_bar");
  equal(clockState.clockBarOn, false, "the active clock-bar source can turn itself off");

  const nameKeys = entityStateKeys({ name_id: "text_sensor/Screen: Active Timezone" });
  assert(nameKeys.includes("text_sensor-screen__active_timezone"), "slash-form entity events include their object-id alias");
  assert(nameKeys.includes("text_sensor:Screen: Active Timezone"), "slash-form entity events retain their name alias");
  assert(isRemovedLegacyStateEvent("", { id: "text-cover_art_fallback_server" }), "removed fallback-server events remain ignored");
  assert(!isRemovedLegacyStateEvent("", { id: "text-screen_saver__cover_art_entity" }), "current cover-art events remain active");

  equal(parseEntityEventData("not json"), null, "malformed event payloads are ignored");
  equal(parseEntityEventData("[]"), null, "array event payloads are ignored");
  equal(parseEntityEventData("null"), null, "null event payloads are ignored");
  const missing = parseEntityEventData("{}");
  assert(missing !== null, "missing optional event fields remain safe to process");
  equal(missing.state, undefined, "missing state remains undefined");

  assert(isFirmwareVersionEvent("", { name_id: "text_sensor/Firmware: Version" }), "firmware version name aliases are recognized");
  assert(isFirmwareUpdateEvent("update-firmware__update", {}), "firmware update object ids are recognized");
  assert(isC6FirmwareInstallButtonEvent("button-firmware_esp32_c6__install_update", {}), "C6 install event aliases are recognized");
}
