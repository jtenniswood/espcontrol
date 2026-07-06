// ── POST queue ─────────────────────────────────────────────────────────
// @web-module-requires: state, screen_schedule_state, artwork_state, screensaver_state, firmware_version_state, clock_bar_state, firmware_update_state, screensaver_timeout, c6_firmware_ui, entity_state

var _postQueue = Promise.resolve();
var _postThrottleMs = 0;
var _postQueueHadError = false;

function postDelay(ms) {
  ms = parseInt(ms, 10) || 0;
  if (ms <= 0) return Promise.resolve();
  return new Promise(function (resolve) { setTimeout(resolve, ms); });
}

function setPostThrottle(ms) {
  _postThrottleMs = Math.max(0, parseInt(ms, 10) || 0);
}

function postQueueIdle() {
  return _postQueue;
}

function resetPostQueueError() {
  _postQueueHadError = false;
}

function postQueueHadError() {
  return _postQueueHadError;
}

function postQuiet(url) {
  return fetch(url, { method: "POST", keepalive: true }).catch(function () {
    return null;
  });
}

function post(url, fallbackUrl, errorMessage) {
  var urls = Array.isArray(url) ? url.slice() : [url];
  if (fallbackUrl) urls.push(fallbackUrl);
  var throttleMs = _postThrottleMs;
  _postQueue = _postQueue.then(function () {
    return postFirstAvailable(urls).then(function (r) {
      if (r && !r.ok) {
        _postQueueHadError = true;
        showBanner(errorMessage || ("Request failed: " + r.status), "error");
      }
      return postDelay(throttleMs).then(function () { return r; });
    }).catch(function () {
      _postQueueHadError = true;
      setConfigLocked(true, "Reconnecting to device\u2026");
      showBanner("Cannot reach device \u2014 is it connected?", "error");
      setTimeout(connectEvents, 5000);
    });
  });
  return _postQueue;
}

function postOptional(url) {
  var urls = Array.isArray(url) ? url.slice() : [url];
  var throttleMs = _postThrottleMs;
  _postQueue = _postQueue.then(function () {
    return postFirstAvailable(urls).then(function (r) {
      return postDelay(throttleMs).then(function () { return r; });
    }).catch(function () {
      _postQueueHadError = true;
      setConfigLocked(true, "Reconnecting to device\u2026");
      showBanner("Cannot reach device \u2014 is it connected?", "error");
      setTimeout(connectEvents, 5000);
    });
  });
  return _postQueue;
}

function postFirstAvailable(urls) {
  var index = 0;
  function tryNext() {
    return fetch(urls[index], { method: "POST" }).then(function (r) {
      if (r.ok || index >= urls.length - 1) return r;
      index++;
      return tryNext();
    });
  }
  return tryNext();
}

function postText(name, value) {
  var encodedValue = encodeURIComponent(value);
  return post(entityPostUrls("text", name, [], "set?value=" + encodedValue));
}

function postTextWithObjectIds(name, objectIds, value, errorMessage) {
  return postWithObjectIds("text", name, objectIds, "set?value=" + encodeURIComponent(value), errorMessage);
}

function postSelect(name, option) {
  return post(entityPostUrls("select", name, [], "set?option=" + encodeURIComponent(option)));
}

function postButtonPress(name) {
  return post(entityPostUrls("button", name, [], "press"));
}

function postSwitch(name, on) {
  return post(entityPostUrls("switch", name, [], on ? "turn_on" : "turn_off"));
}

function postScreensaverMode(value) {
  return postTextWithObjectIds(
    entityName("screensaver_mode"),
    entityObjectIds("screensaver_mode"),
    value
  );
}

function postFirmwareAutoUpdate(on) {
  return postSwitchWithObjectIds(
    entityName("firmware_auto_update"),
    entityObjectIds("firmware_auto_update"),
    on
  );
}

function postFirmwareUpdateFrequency(value) {
  return postSelectWithObjectIds(
    entityName("firmware_update_frequency"),
    entityObjectIds("firmware_update_frequency"),
    value
  );
}

function postNumber(name, value) {
  return post(entityPostUrls("number", name, [], "set?value=" + encodeURIComponent(value)));
}

function postWithObjectId(domain, name, objectId, action, errorMessage) {
  postWithObjectIds(domain, name, [objectId], action, errorMessage);
}

function postWithObjectIds(domain, name, objectIds, action, errorMessage) {
  return post(entityPostUrls(domain, name, objectIds, action), null, errorMessage);
}

function postNumberWithObjectId(name, objectId, value, errorMessage) {
  postWithObjectId("number", name, objectId, "set?value=" + encodeURIComponent(value), errorMessage);
}

function postNumberWithObjectIds(name, objectIds, value, errorMessage) {
  postWithObjectIds("number", name, objectIds, "set?value=" + encodeURIComponent(value), errorMessage);
}

function postSelectWithObjectId(name, objectId, option, errorMessage) {
  postWithObjectId("select", name, objectId, "set?option=" + encodeURIComponent(option), errorMessage);
}

function postSelectWithObjectIds(name, objectIds, option, errorMessage) {
  postWithObjectIds("select", name, objectIds, "set?option=" + encodeURIComponent(option), errorMessage);
}

function postScreensaverTimeout(value) {
  if (!screensaverTimeoutSupported(value)) {
    showBanner("Update the device firmware before using shorter screensaver timers.", "error");
    syncScreensaverTimeoutUi();
    return;
  }
  postNumberWithObjectIds(entityName("screensaver_timeout"), entityObjectIds("screensaver_timeout"), value);
}

var SCREENSAVER_ACTION_UNAVAILABLE =
  "Screen dimmed screensaver is not available on this firmware. Update the device firmware, then reload this page.";

function postScreensaverAction(value) {
  postSelectWithObjectIds(
    entityName("screen_saver_action"),
    entityObjectIds("screen_saver_action"),
    screensaverActionOption(value),
    SCREENSAVER_ACTION_UNAVAILABLE
  );
}

function postScreensaverDimmedBrightness(value) {
  postNumberWithObjectIds(
    entityName("screen_saver_dimmed_brightness"),
    entityObjectIds("screen_saver_dimmed_brightness"),
    value,
    SCREENSAVER_ACTION_UNAVAILABLE
  );
}

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

function postHomeScreenTimeout(value) {
  postNumberWithObjectIds(
    entityName("home_screen_timeout"),
    entityObjectIds("home_screen_timeout"),
    value
  );
}

function postSwitchWithObjectId(name, objectId, on, errorMessage) {
  postWithObjectId("switch", name, objectId, on ? "turn_on" : "turn_off", errorMessage);
}

function postSwitchWithObjectIds(name, objectIds, on, errorMessage) {
  postWithObjectIds("switch", name, objectIds, on ? "turn_on" : "turn_off", errorMessage);
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

function getJsonQuietly(path, callback) {
  return fetch(path, { cache: "no-store" }).then(function (r) {
    if (!r.ok) return null;
    return r.json();
  }).then(function (data) {
    if (data && callback) callback(data);
    return data;
  }).catch(function () {});
}

function getJsonFirst(paths, callback) {
  var index = 0;
  function tryNext() {
    if (index >= paths.length) return Promise.resolve(null);
    return getJsonQuietly(paths[index++]).then(function (data) {
      if (data) {
        if (callback) callback(data);
        return data;
      }
      return tryNext();
    });
  }
  return tryNext();
}

function entityDetailPath(domain, name, detail) {
  var query = detail === "state" ? "" : "?detail=all";
  return "/" + encodeURIComponent(domain) + "/" + encodeURIComponent(name) + query;
}

function entityDetailPaths(domain, names, detail) {
  return names.map(function (name) { return entityDetailPath(domain, name, detail); });
}

function entityInitialDetail(domain) {
  return domain === "select" ? "state" : "all";
}
