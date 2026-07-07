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
