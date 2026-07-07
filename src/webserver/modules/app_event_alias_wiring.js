// ── Event Alias Wiring ────────────────────────────────────────────────
// @web-module-requires: app_event_aliases

function addSseAliases(handlers, names, fn) {
  for (var i = 0; i < names.length; i++) handlers[names[i]] = fn;
}

function applySseHandlerAliases(sseHandlers) {
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockBar, sseHandlers["switch-screen__clock_bar"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockBarTime, sseHandlers["switch-screen__clock_bar_time"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockBarTemperatureEntities, sseHandlers["text-clock_bar_temperature_entities"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.networkStatus, sseHandlers["switch-screen__network_status_icon"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.voiceServices, sseHandlers["switch-voice_services"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.temperatureDegreeSymbol, sseHandlers["switch-screen__temperature_degree_symbol"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.subpageChevron, sseHandlers["switch-screen__subpage_chevron"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screensaverTimeout, sseHandlers["number-screensaver_timeout"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.clockScreensaver, sseHandlers["switch-screen_saver__clock"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.mediaPlayerSleepPrevention, sseHandlers["switch-screen_saver__media_player_sleep_prevention"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.mediaPlayerSleepPreventionEntity, sseHandlers["text-media_player_sleep_prevention_entity"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArt, sseHandlers["switch-screen_saver__cover_art"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtEntity, sseHandlers["text-screen_saver__cover_art_entity"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtConditions, sseHandlers["text-screen_saver__cover_art_conditions"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtDelay, sseHandlers["number-screen_saver__cover_art_delay"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtTouchPause, sseHandlers["number-screen_saver__cover_art_touch_pause"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.trackOverlayDuration, sseHandlers["number-screen_saver__track_overlay_duration"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.coverArtHideExternalInput, sseHandlers["switch-screen_saver__hide_cover_art_on_external_input"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.homeAssistantArtworkProtocol, sseHandlers["select-home_assistant_artwork_protocol"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.homeAssistantArtworkPort, sseHandlers["number-home_assistant_artwork_port"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleTrigger, sseHandlers["text-screen__schedule_trigger"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.schedulePresenceDetectedMode, sseHandlers["select-screen__schedule_presence_detected_mode"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.schedulePresenceNotDetectedMode, sseHandlers["select-screen__schedule_presence_not_detected_mode"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleWakeTimeout, sseHandlers["number-screen__schedule_wake_timeout"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleWakeBrightness, sseHandlers["number-screen__schedule_wake_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleDimmedBrightness, sseHandlers["number-screen__schedule_dimmed_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleClockBrightness, sseHandlers["number-screen__schedule_clock_brightness"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.scheduleClockTextColor, sseHandlers["text-screen__schedule_clock_text_color"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screenActiveTimezone, sseHandlers["text_sensor-screen__active_timezone"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.screenLanguage, sseHandlers["select-screen__language"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer1, sseHandlers["text-screen__ntp_server_1"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer2, sseHandlers["text-screen__ntp_server_2"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.ntpServer3, sseHandlers["text-screen__ntp_server_3"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.firmwareAutoUpdate, sseHandlers["switch-firmware__auto_update"]);
  addSseAliases(sseHandlers, SSE_ALIAS_GROUPS.firmwareUpdateFrequency, sseHandlers["select-firmware__update_frequency"]);
}
