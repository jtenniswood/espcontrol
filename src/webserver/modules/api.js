// ── POST queue ─────────────────────────────────────────────────────────
// @web-module-requires: state, screen_schedule_state, artwork_state, screensaver_state, firmware_version_state, clock_bar_state, firmware_update_state, screensaver_timeout, c6_firmware_ui, entity_state

var _deviceApi = createDeviceApi(function (url, init) { return fetch(url, init); });
var _postQueue = Promise.resolve(null);
var _postQueueHadError = false;

function setPostThrottle(ms) {
  _deviceApi.setPostThrottle(ms);
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
  return _deviceApi.postQuiet(url).then(function (result) {
    return result.ok || result.kind === "http-error" ? result.value : null;
  });
}

function post(url, fallbackUrl, errorMessage) {
  var urls = Array.isArray(url) ? url.slice() : [url];
  if (fallbackUrl) urls.push(fallbackUrl);
  _postQueue = _deviceApi.enqueuePost(urls).then(function (result) {
    var failure = requestFailureInfo(result, errorMessage);
    if (failure && failure.reconnect) {
      _postQueueHadError = true;
      setConfigLocked(true, "Reconnecting to device\u2026");
      showBanner(failure.message, "error");
      setTimeout(connectEvents, 5000);
      return null;
    }
    if (failure) {
      _postQueueHadError = true;
      showBanner(failure.message, "error");
    }
    return result.value;
  });
  return _postQueue;
}

function postOptional(url) {
  var urls = Array.isArray(url) ? url.slice() : [url];
  _postQueue = _deviceApi.enqueuePost(urls).then(function (result) {
    var failure = requestFailureInfo(result);
    if (failure && failure.reconnect) {
      _postQueueHadError = true;
      setConfigLocked(true, "Reconnecting to device\u2026");
      showBanner(failure.message, "error");
      setTimeout(connectEvents, 5000);
      return null;
    }
    return result.value;
  });
  return _postQueue;
}

function postFirstAvailable(urls) {
  return _deviceApi.postFirstAvailable(urls).then(function (result) {
    if (result.kind === "network-error") throw result.error;
    return result.value;
  });
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
  return _deviceApi.getJson(path).then(function (result) {
    var data = result.ok ? result.value : null;
    if (data && callback) callback(data);
    return data;
  });
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
