// ── Clock Bar Post API ────────────────────────────────────────────────
// @web-module-requires: environment_state, clock_bar_state, entity_state, api

function postClockBrightnessDay(value) {
  postNumberWithObjectIds(
    entityName("screen_saver_daytime_clock_brightness"),
    entityObjectIds("screen_saver_daytime_clock_brightness"),
    value
  );
}

function postClockBrightnessNight(value) {
  postNumberWithObjectIds(
    entityName("screen_saver_nighttime_clock_brightness"),
    entityObjectIds("screen_saver_nighttime_clock_brightness"),
    value
  );
}

function postClockScreensaver(on) {
  return postSwitchWithObjectIds(
    entityName("screen_saver_clock"),
    entityObjectIds("screen_saver_clock"),
    on
  );
}

var CLOCK_BAR_UNAVAILABLE =
  "Clock bar setting is not available on this firmware. Update the device firmware, then reload this page.";

function postClockBar(on) {
  postSwitchWithObjectIds(entityName("screen_clock_bar"), entityObjectIds("screen_clock_bar"), on, CLOCK_BAR_UNAVAILABLE);
}

function postClockBarTemperatureEntities(value) {
  var name = entityName("clock_bar_temperature_entities");
  var objectIds = entityObjectIds("clock_bar_temperature_entities");
  return postOptional(entityPostUrls("text", name, objectIds, "set?value=" + encodeURIComponent(value)));
}

var CLOCK_BAR_TIME_UNAVAILABLE =
  "Clock bar time setting is not available on this firmware. Update the device firmware, then reload this page.";

function postClockBarTime(on) {
  postSwitchWithObjectIds(
    entityName("screen_clock_bar_time"),
    entityObjectIds("screen_clock_bar_time"),
    on,
    CLOCK_BAR_TIME_UNAVAILABLE
  );
}

var NETWORK_STATUS_ICON_UNAVAILABLE =
  "Network status icon setting is not available on this firmware. Update the device firmware, then reload this page.";

function postNetworkStatusIcon(on) {
  postSwitchWithObjectIds(
    entityName("screen_network_status_icon"),
    entityObjectIds("screen_network_status_icon"),
    on,
    NETWORK_STATUS_ICON_UNAVAILABLE
  );
}

var VOICE_SERVICES_UNAVAILABLE =
  "Voice services setting is not available on this firmware. Update the device firmware, then reload this page.";

function voiceServicesPostUrls(on) {
  return entityPostUrls(
    "switch",
    entityName("voice_services"),
    entityObjectIds("voice_services"),
    on ? "turn_on" : "turn_off"
  );
}

function postVoiceServices(on) {
  post(voiceServicesPostUrls(on), null, VOICE_SERVICES_UNAVAILABLE);
}

var TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE =
  "Temperature degree symbol setting is not available on this firmware. Update the device firmware, then reload this page.";

function postTemperatureDegreeSymbol(on) {
  postSwitchWithObjectIds(
    entityName("screen_temperature_degree_symbol"),
    entityObjectIds("screen_temperature_degree_symbol"),
    on,
    TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE
  );
}

var SUBPAGE_CHEVRON_UNAVAILABLE =
  "Subpage chevron setting is not available on this firmware. Update the device firmware, then reload this page.";

function postSubpageChevron(on) {
  postSwitchWithObjectIds(
    entityName("screen_subpage_chevron"),
    entityObjectIds("screen_subpage_chevron"),
    on,
    SUBPAGE_CHEVRON_UNAVAILABLE
  );
}
