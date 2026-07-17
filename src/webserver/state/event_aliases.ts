import type { EntityEvent } from "./types";

export type SseHandler = (value: string, event: EntityEvent, matchedKey: string) => void;
export type SseHandlers = Record<string, SseHandler>;

export const SSE_ALIAS_GROUPS = {
  clockBar: ["switch-screen__clock_bar", "switch-screen_clock_bar", "switch-clock_bar_enabled"],
  clockBarTime: ["switch-screen__clock_bar_time", "switch-screen_clock_bar_time", "switch-clock_bar_time_enabled"],
  clockBarTemperatureEntities: ["text-clock_bar_temperature_entities", "text-clock_bar__temperature_entities"],
  networkStatus: ["switch-screen__network_status_icon", "switch-screen_network_status_icon", "switch-network_status_enabled"],
  voiceServices: ["switch-voice_services", "switch-voice_services_enabled"],
  temperatureDegreeSymbol: ["switch-screen__temperature_degree_symbol", "switch-screen_temperature_degree_symbol", "switch-temperature_degree_symbol_enabled"],
  subpageChevron: ["switch-screen__subpage_chevron", "switch-screen_subpage_chevron", "switch-subpage_chevrons_enabled"],
  screensaverTimeout: ["number-screensaver_timeout", "number-screen_saver__timeout", "number-screen_saver_timeout"],
  screensaverPinRequired: ["switch-screensaver__require_pin_after_wake", "switch-screensaver__pin_required", "switch-screensaver_pin_required"],
  screensaverPin: ["text-screensaver__pin", "text-screensaver_pin"],
  clockScreensaver: ["switch-screen_saver__clock", "switch-screen_saver_clock", "switch-clock_screensaver_enabled"],
  mediaPlayerSleepPrevention: ["switch-screen_saver__media_player_sleep_prevention", "switch-screen_saver_media_player_sleep_prevention", "switch-media_player_sleep_prevention_enabled"],
  mediaPlayerSleepPreventionEntity: ["text-media_player_sleep_prevention_entity"],
  coverArt: ["switch-screen_saver__cover_art", "switch-screen_saver_cover_art", "switch-screensaver_cover_art"],
  coverArtEntity: ["text-screen_saver__cover_art_entity", "text-screen_saver_cover_art_entity", "text-cover_art_media_player_entity"],
  coverArtConditions: ["text-screen_saver__cover_art_conditions", "text-screen_saver_cover_art_conditions", "text-cover_art_attribute_conditions"],
  coverArtDelay: ["number-screen_saver__cover_art_delay", "number-screen_saver_cover_art_delay", "number-cover_art_delay"],
  trackOverlayDuration: ["number-screen_saver__track_overlay_duration", "number-screen_saver_track_overlay_duration", "number-track_overlay_duration", "number-screen_saver__show_track_overlay"],
  coverArtHideExternalInput: ["switch-screen_saver__hide_cover_art_on_external_input", "switch-screen_saver_hide_cover_art_on_external_input", "switch-hide_cover_art_on_external_input", "switch-cover_art_hide_external_input", "switch-screen_saver__hide_for_external_sources"],
  homeAssistantArtworkProtocol: ["select-home_assistant_artwork_protocol", "select-cover_art_home_assistant_artwork_protocol"],
  homeAssistantArtworkPort: ["number-home_assistant_artwork_port"],
  scheduleTrigger: ["text-screen__schedule_trigger", "text-screen_schedule_trigger", "text-schedule_trigger"],
  scheduleSensorActivation: ["select-screen__schedule_sensor_activation", "select-screen_schedule_sensor_activation", "select-schedule_sensor_activation"],
  scheduleWakeTimeout: ["number-screen__schedule_wake_timeout", "number-screen_schedule_wake_timeout", "number-schedule_wake_timeout"],
  scheduleWakeBrightness: ["number-screen__schedule_wake_brightness", "number-screen_schedule_wake_brightness", "number-schedule_wake_brightness"],
  scheduleDimmedBrightness: ["number-screen__schedule_dimmed_brightness", "number-screen_schedule_dimmed_brightness", "number-schedule_dimmed_brightness"],
  scheduleClockBrightness: ["number-screen__schedule_clock_brightness", "number-screen_schedule_clock_brightness", "number-schedule_clock_brightness"],
  scheduleClockTextColor: ["text-screen__schedule_clock_text_color", "text-screen_schedule_clock_text_color", "text-schedule_clock_text_color"],
  screenActiveTimezone: ["text_sensor-screen__active_timezone", "text_sensor-screen_active_timezone", "text_sensor:Screen: Active Timezone"],
  screenLanguage: ["select-screen__language", "select-screen_language"],
  ntpServer1: ["text-screen__ntp_server_1", "text-ntp_server_1"],
  ntpServer2: ["text-screen__ntp_server_2", "text-ntp_server_2"],
  ntpServer3: ["text-screen__ntp_server_3", "text-ntp_server_3"],
  firmwareAutoUpdate: ["switch-firmware__auto_update", "switch-firmware_auto_update", "switch-auto_update_switch"],
  firmwareUpdateFrequency: ["select-firmware__update_frequency", "select-firmware_update_frequency", "select-update_frequency_select"],
} as const;

function addSseAliases(handlers: SseHandlers, names: readonly string[], canonical: string): void {
  const handler = handlers[canonical];
  if (!handler) throw new Error(`Missing canonical SSE handler: ${canonical}`);
  for (const name of names) handlers[name] = handler;
}

export function applySseHandlerAliases(handlers: SseHandlers): void {
  addSseAliases(handlers, SSE_ALIAS_GROUPS.clockBar, "switch-screen__clock_bar");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.clockBarTime, "switch-screen__clock_bar_time");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.clockBarTemperatureEntities, "text-clock_bar_temperature_entities");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.networkStatus, "switch-screen__network_status_icon");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.voiceServices, "switch-voice_services");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.temperatureDegreeSymbol, "switch-screen__temperature_degree_symbol");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.subpageChevron, "switch-screen__subpage_chevron");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.screensaverTimeout, "number-screensaver_timeout");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.screensaverPinRequired, "switch-screensaver__require_pin_after_wake");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.screensaverPin, "text-screensaver__pin");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.clockScreensaver, "switch-screen_saver__clock");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.mediaPlayerSleepPrevention, "switch-screen_saver__media_player_sleep_prevention");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.mediaPlayerSleepPreventionEntity, "text-media_player_sleep_prevention_entity");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.coverArt, "switch-screen_saver__cover_art");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.coverArtEntity, "text-screen_saver__cover_art_entity");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.coverArtConditions, "text-screen_saver__cover_art_conditions");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.coverArtDelay, "number-screen_saver__cover_art_delay");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.trackOverlayDuration, "number-screen_saver__track_overlay_duration");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.coverArtHideExternalInput, "switch-screen_saver__hide_cover_art_on_external_input");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.homeAssistantArtworkProtocol, "select-home_assistant_artwork_protocol");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.homeAssistantArtworkPort, "number-home_assistant_artwork_port");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.scheduleTrigger, "text-screen__schedule_trigger");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.scheduleSensorActivation, "select-screen__schedule_sensor_activation");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.scheduleWakeTimeout, "number-screen__schedule_wake_timeout");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.scheduleWakeBrightness, "number-screen__schedule_wake_brightness");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.scheduleDimmedBrightness, "number-screen__schedule_dimmed_brightness");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.scheduleClockBrightness, "number-screen__schedule_clock_brightness");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.scheduleClockTextColor, "text-screen__schedule_clock_text_color");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.screenActiveTimezone, "text_sensor-screen__active_timezone");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.screenLanguage, "select-screen__language");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.ntpServer1, "text-screen__ntp_server_1");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.ntpServer2, "text-screen__ntp_server_2");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.ntpServer3, "text-screen__ntp_server_3");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.firmwareAutoUpdate, "switch-firmware__auto_update");
  addSseAliases(handlers, SSE_ALIAS_GROUPS.firmwareUpdateFrequency, "select-firmware__update_frequency");
}
