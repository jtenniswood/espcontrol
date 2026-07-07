// ── Artwork Post API ──────────────────────────────────────────────────
// @web-module-requires: artwork_state, entity_state, api

function postPresenceSensorEntity(value) {
  return postTextWithObjectIds(
    entityName("presence_sensor_entity"),
    entityObjectIds("presence_sensor_entity"),
    value
  );
}

function postMediaPlayerSleepPrevention(on) {
  return postSwitchWithObjectIds(
    entityName("screen_saver_media_player_sleep_prevention"),
    entityObjectIds("screen_saver_media_player_sleep_prevention"),
    on
  );
}

function postMediaPlayerSleepPreventionEntity(value) {
  return postTextWithObjectIds(
    entityName("media_player_sleep_prevention_entity"),
    entityObjectIds("media_player_sleep_prevention_entity"),
    value
  );
}

function postCoverArtScreensaver(on) {
  return postSwitchWithObjectIds(
    entityName("screen_saver_cover_art"),
    entityObjectIds("screen_saver_cover_art"),
    on
  );
}

function postCoverArtMediaPlayerEntity(value) {
  return postTextWithObjectIds(
    entityName("screen_saver_cover_art_entity"),
    entityObjectIds("screen_saver_cover_art_entity"),
    value
  );
}

function postCoverArtConditions(value) {
  return postTextWithObjectIds(
    entityName("screen_saver_cover_art_conditions"),
    entityObjectIds("screen_saver_cover_art_conditions"),
    value
  );
}

function coverArtHideExternalInputPostUrls(on) {
  return entityPostUrls(
    "switch",
    entityName("screen_saver_hide_cover_art_external_input"),
    entityObjectIds("screen_saver_hide_cover_art_external_input"),
    on ? "turn_on" : "turn_off"
  );
}

function postCoverArtHideExternalInput(on) {
  return post(coverArtHideExternalInputPostUrls(on));
}

function coverArtDelayPostUrls(value) {
  return entityPostUrls(
    "number",
    entityName("screen_saver_cover_art_delay"),
    entityObjectIds("screen_saver_cover_art_delay"),
    "set?value=" + encodeURIComponent(value)
  );
}

function postCoverArtDelay(value) {
  return post(coverArtDelayPostUrls(value));
}

function coverArtTouchPausePostUrls(value) {
  return entityPostUrls(
    "number",
    entityName("screen_saver_cover_art_touch_pause"),
    entityObjectIds("screen_saver_cover_art_touch_pause"),
    "set?value=" + encodeURIComponent(value)
  );
}

function postCoverArtTouchPause(value) {
  return post(coverArtTouchPausePostUrls(value));
}

function coverArtTrackOverlayDurationPostUrls(value) {
  return entityPostUrls(
    "number",
    entityName("screen_saver_track_overlay_duration"),
    entityObjectIds("screen_saver_track_overlay_duration"),
    "set?value=" + encodeURIComponent(value)
  );
}

function postCoverArtTrackOverlayDuration(value) {
  return post(coverArtTrackOverlayDurationPostUrls(value));
}

function homeAssistantArtworkPortPostUrls(value) {
  return entityPostUrls(
    "number",
    entityName("home_assistant_artwork_port"),
    entityObjectIds("home_assistant_artwork_port"),
    "set?value=" + encodeURIComponent(value)
  );
}

function postHomeAssistantArtworkPort(value) {
  return post(homeAssistantArtworkPortPostUrls(value));
}

function postHomeAssistantArtworkProtocol(value) {
  return postSelectWithObjectIds(
    entityName("home_assistant_artwork_protocol"),
    entityObjectIds("home_assistant_artwork_protocol"),
    normalizeHomeAssistantArtworkProtocol(value)
  );
}
