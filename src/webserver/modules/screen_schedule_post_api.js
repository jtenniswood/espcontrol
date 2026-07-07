// ── Screen Schedule Post API ──────────────────────────────────────────
// @web-module-requires: state, screen_schedule_state, entity_state, api

var SCREEN_SCHEDULE_UNAVAILABLE =
  "Screen schedule is not available on this firmware. Update the device firmware, then reload this page.";
var SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE =
  "The schedule trigger setting is not available on this firmware. Update the device firmware, then reload this page.";
var SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE =
  "The schedule wake timeout setting is not available on this firmware. Update the device firmware, then reload this page.";
var SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE =
  "The schedule wake brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
var SCREEN_SCHEDULE_MODE_UNAVAILABLE =
  "The schedule mode setting is not available on this firmware. Update the device firmware, then reload this page.";
var SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE =
  "The schedule dimmed brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
var SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE =
  "The schedule clock brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
var AUTOMATIC_BRIGHTNESS_UNAVAILABLE =
  "Automatic brightness control is not available on this firmware. Update the device firmware, then reload this page.";
var BRIGHTNESS_TIME_UNAVAILABLE =
  "Manual brightness times are not available on this firmware. Update the device firmware, then reload this page.";

function postAutomaticBrightnessEnabled(on) {
  postSwitchWithObjectIds(
    entityName("screen_automatic_brightness"),
    entityObjectIds("screen_automatic_brightness"),
    on,
    AUTOMATIC_BRIGHTNESS_UNAVAILABLE
  );
}

function postBrightnessDawnTime(value) {
  postTextWithObjectIds(
    entityName("screen_brightness_dawn_time"),
    entityObjectIds("screen_brightness_dawn_time"),
    normalizeTimeOfDay(value, state.brightnessDawnTime || "06:00"),
    BRIGHTNESS_TIME_UNAVAILABLE
  );
}

function postBrightnessDuskTime(value) {
  postTextWithObjectIds(
    entityName("screen_brightness_dusk_time"),
    entityObjectIds("screen_brightness_dusk_time"),
    normalizeTimeOfDay(value, state.brightnessDuskTime || "18:00"),
    BRIGHTNESS_TIME_UNAVAILABLE
  );
}

function postScreenScheduleEnabled(on) {
  postSwitchWithObjectIds(
    entityName("screen_schedule_enabled"),
    entityObjectIds("screen_schedule_enabled"),
    on,
    SCREEN_SCHEDULE_UNAVAILABLE
  );
}

function postScreenScheduleTrigger(value) {
  postTextWithObjectIds(
    entityName("screen_schedule_trigger"),
    entityObjectIds("screen_schedule_trigger"),
    normalizeScheduleTrigger(value, state.scheduleEnabled),
    SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE
  );
}

function postScreenScheduleOnHour(value) {
  postNumberWithObjectIds(
    entityName("screen_schedule_on_hour"),
    entityObjectIds("screen_schedule_on_hour"),
    value,
    SCREEN_SCHEDULE_UNAVAILABLE
  );
}

function postScreenScheduleOffHour(value) {
  postNumberWithObjectIds(
    entityName("screen_schedule_off_hour"),
    entityObjectIds("screen_schedule_off_hour"),
    value,
    SCREEN_SCHEDULE_UNAVAILABLE
  );
}

function postScreenScheduleMode(value) {
  postSelectWithObjectIds(
    entityName("screen_schedule_mode"),
    entityObjectIds("screen_schedule_mode"),
    scheduleModeOption(value),
    SCREEN_SCHEDULE_MODE_UNAVAILABLE
  );
}

function postScreenScheduleWakeTimeout(value) {
  postNumberWithObjectIds(
    entityName("screen_schedule_wake_timeout"),
    entityObjectIds("screen_schedule_wake_timeout"),
    value,
    SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE
  );
}

function postScreenScheduleWakeBrightness(value) {
  postNumberWithObjectIds(
    entityName("screen_schedule_wake_brightness"),
    entityObjectIds("screen_schedule_wake_brightness"),
    value,
    SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE
  );
}

function postScreenScheduleDimmedBrightness(value) {
  postNumberWithObjectIds(
    entityName("screen_schedule_dimmed_brightness"),
    entityObjectIds("screen_schedule_dimmed_brightness"),
    value,
    SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE
  );
}

function postScreenScheduleClockBrightness(value) {
  postNumberWithObjectIds(
    entityName("screen_schedule_clock_brightness"),
    entityObjectIds("screen_schedule_clock_brightness"),
    value,
    SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE
  );
}
