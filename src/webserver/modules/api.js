// ── POST queue ─────────────────────────────────────────────────────────

var _postQueue = Promise.resolve();

function uniquePush(list, value) {
  if (value && list.indexOf(value) === -1) list.push(value);
}

function esphomeObjectId(value) {
  return String(value || "").replace(/./g, function (ch) {
    if (ch === " ") return "_";
    var lower = ch.toLowerCase();
    if ((lower >= "a" && lower <= "z") || (ch >= "0" && ch <= "9") || ch === "-" || ch === "_") return lower;
    return "_";
  });
}

function parseEntityId(value) {
  var id = String(value || "");
  if (!id) return null;
  if (id.indexOf("/") !== -1) {
    var parts = id.split("/");
    if (parts.length < 2 || !parts[0] || !parts[parts.length - 1]) return null;
    return {
      raw: id,
      domain: parts[0],
      name: parts[parts.length - 1],
      objectId: esphomeObjectId(parts[parts.length - 1]),
      path: "/" + parts.map(encodeURIComponent).join("/"),
    };
  }
  var dash = id.indexOf("-");
  if (dash <= 0) return null;
  return {
    raw: id,
    domain: id.substring(0, dash),
    objectId: id.substring(dash + 1),
    path: "/" + encodeURIComponent(id.substring(0, dash)) + "/" + encodeURIComponent(id.substring(dash + 1)),
  };
}

function parseHomeAssistantEntity(value) {
  var text = String(value || "").trim();
  var dot = text.indexOf(".");
  if (dot <= 0 || dot >= text.length - 1) return null;
  return {
    id: text,
    domain: text.substring(0, dot),
    objectId: text.substring(dot + 1),
  };
}

function titleFromEntityId(entityId) {
  var parsed = parseHomeAssistantEntity(entityId);
  if (!parsed) return entityId;
  return parsed.objectId.replace(/_/g, " ").replace(/\b\w/g, function (ch) {
    return ch.toUpperCase();
  });
}

function rememberEntityName(entityId, name) {
  var parsed = parseHomeAssistantEntity(entityId);
  if (!parsed || !name) return;
  if (!state.entityNames[parsed.id]) state.entityNames[parsed.id] = [];
  uniquePush(state.entityNames[parsed.id], String(name));
}

function rememberConfiguredButtonEntities(button) {
  if (!button) return;
  var label = button.label || "";
  if (button.entity) rememberEntityName(button.entity, label || titleFromEntityId(button.entity));
  if (button.sensor && parseHomeAssistantEntity(button.sensor)) {
    rememberEntityName(button.sensor, label || titleFromEntityId(button.sensor));
  }
}

function rememberConfiguredEntities() {
  for (var i = 0; i < state.buttons.length; i++) rememberConfiguredButtonEntities(state.buttons[i]);
  for (var slot in state.subpages) {
    var sp = state.subpages[slot];
    if (!sp || !sp.buttons) continue;
    for (var bi = 0; bi < sp.buttons.length; bi++) rememberConfiguredButtonEntities(sp.buttons[bi]);
  }
  rememberEntityName(state.indoorEntity, "Indoor Temperature");
  rememberEntityName(state.outdoorEntity, "Outdoor Temperature");
  rememberEntityName(state.presenceEntity, "Presence Sensor");
  rememberEntityName(state.mediaPlayerSleepPreventionEntity, "Media Player");
}

function optionLabelForEntity(entityId) {
  var names = state.entityNames[entityId] || [];
  if (!names.length) return titleFromEntityId(entityId);
  return names.join(" / ");
}

function entitySuggestions(domains) {
  rememberConfiguredEntities();
  var allowed = {};
  (domains || []).forEach(function (domain) { allowed[domain] = true; });
  var ids = [];
  for (var id in state.entityNames) {
    var parsed = parseHomeAssistantEntity(id);
    if (!parsed) continue;
    if (domains && domains.length && !allowed[parsed.domain]) continue;
    ids.push(id);
  }
  ids.sort(function (a, b) {
    var al = optionLabelForEntity(a).toLowerCase();
    var bl = optionLabelForEntity(b).toLowerCase();
    if (al === bl) return a.localeCompare(b);
    return al.localeCompare(bl);
  });
  return ids.map(function (id) {
    return { value: id, label: optionLabelForEntity(id) };
  });
}

function ensureEntityDropdown(input) {
  if (!input || input._entityDropdown || !input.parentNode) return;
  var wrap = document.createElement("div");
  wrap.className = "sp-entity-input-wrap";
  input.parentNode.insertBefore(wrap, input);
  wrap.appendChild(input);
  var dropdown = document.createElement("div");
  dropdown.className = "sp-entity-dropdown";
  wrap.appendChild(dropdown);
  input._entityDropdown = dropdown;
}

function closeEntityDropdown(input) {
  if (input && input._entityDropdown) input._entityDropdown.classList.remove("sp-open");
}

function refreshEntityDatalist(input) {
  if (!input) return;
  ensureEntityDropdown(input);
  var dropdown = input._entityDropdown;
  if (!dropdown) return;
  if (input._entitySuppressDropdown) {
    closeEntityDropdown(input);
    return;
  }
  dropdown.innerHTML = "";
  var query = String(input.value || "").trim().toLowerCase();
  var items = entitySuggestions(input._entityDomains || []).filter(function (item) {
    if (!query) return true;
    return item.value.toLowerCase().indexOf(query) !== -1 ||
      item.label.toLowerCase().indexOf(query) !== -1;
  }).slice(0, 12);
  items.forEach(function (item) {
    var option = document.createElement("button");
    option.type = "button";
    option.className = "sp-entity-option";
    option.textContent = item.value;
    option.addEventListener("mousedown", function (e) {
      e.preventDefault();
      input._entitySuppressDropdown = true;
      input.value = item.value;
      rememberEntityName(item.value, item.label || titleFromEntityId(item.value));
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
      closeEntityDropdown(input);
      input._entitySuppressDropdown = false;
    });
    dropdown.appendChild(option);
  });
  dropdown.classList.toggle("sp-open", document.activeElement === input && items.length > 0);
}

function attachEntitySuggestions(input, domains) {
  if (!input || input._entitySuggestionsAttached) return input;
  input._entityDomains = domains || [];
  input._entitySuggestionsAttached = true;
  input.addEventListener("focus", function () { refreshEntityDatalist(input); });
  input.addEventListener("input", function () {
    rememberEntityName(input.value, optionLabelForEntity(input.value));
    refreshEntityDatalist(input);
  });
  input.addEventListener("blur", function () {
    setTimeout(function () { closeEntityDropdown(input); }, 120);
  });
  input.addEventListener("keydown", function (e) {
    if (e.key === "Escape") closeEntityDropdown(input);
  });
  refreshEntityDatalist(input);
  return input;
}

function entityInput(id, value, placeholder, domains) {
  var el = textInput(id, value, placeholder);
  return attachEntitySuggestions(el, domains);
}

function entityStateKeys(data) {
  var keys = [];
  [data && data.id, data && data.name_id].forEach(function (id) {
    var parsed = parseEntityId(id);
    uniquePush(keys, id);
    if (parsed && parsed.domain && parsed.objectId) uniquePush(keys, parsed.domain + "-" + parsed.objectId);
    if (parsed && parsed.domain && parsed.name) uniquePush(keys, parsed.domain + ":" + parsed.name);
  });
  return keys;
}

function rememberEntityPostPath(data) {
  var preferred = parseEntityId(data && data.name_id) || parseEntityId(data && data.id);
  if (data && data.domain && data.name) rememberEntityName(data.domain + "." + esphomeObjectId(data.name), data.name);
  if (!preferred || !preferred.path) return;
  entityStateKeys(data).forEach(function (key) {
    state.entityPostPaths[key] = preferred.path;
  });
  if (preferred.domain && preferred.name) state.entityPostPaths[preferred.domain + ":" + preferred.name] = preferred.path;
  if (preferred.domain && preferred.objectId) state.entityPostPaths[preferred.domain + ":" + preferred.objectId] = preferred.path;
}

function rememberedPostUrls(domain, name, objectIds, action) {
  var urls = [];
  var keys = [domain + ":" + name, domain + "-" + esphomeObjectId(name)];
  objectIds.forEach(function (objectId) {
    keys.push(domain + ":" + objectId);
    keys.push(domain + "-" + objectId);
  });
  keys.forEach(function (key) {
    if (state.entityPostPaths[key]) uniquePush(urls, state.entityPostPaths[key] + "/" + action);
  });
  return urls;
}

function post(url, fallbackUrl, errorMessage) {
  var urls = Array.isArray(url) ? url.slice() : [url];
  if (fallbackUrl) urls.push(fallbackUrl);
  _postQueue = _postQueue.then(function () {
    var index = 0;
    function tryNext() {
      return fetch(urls[index], { method: "POST" }).then(function (r) {
        if (r.ok || index >= urls.length - 1) {
          if (!r.ok) showBanner(errorMessage || ("Request failed: " + r.status), "error");
          return r;
        }
        index++;
        return tryNext();
      });
    }
    return tryNext().catch(function () {
      setConfigLocked(true, "Reconnecting to device\u2026");
      showBanner("Cannot reach device \u2014 is it connected?", "error");
      setTimeout(connectEvents, 5000);
    });
  });
  return _postQueue;
}

function postText(name, value) {
  post("/text/" + encodeURIComponent(name) + "/set?value=" + encodeURIComponent(value));
}

function saveButtonConfig(slot) {
  var b = state.buttons[slot - 1];
  postText("Button " + slot + " Config", serializeButtonConfig(b));
}

function saveSubpageEntity(slot) {
  var sp = state.subpages[slot];
  var full = sp ? serializeSubpageConfig(sp) : "";
  var chunks = ["", "", "", ""];
  var rest = full;
  for (var ci = 0; ci < chunks.length && rest; ci++) {
    if (rest.length <= 255) {
      chunks[ci] = rest;
      rest = "";
      break;
    }
    var splitAt = rest.lastIndexOf("|", 255);
    if (splitAt <= 0) splitAt = 255;
    chunks[ci] = rest.substring(0, splitAt);
    rest = rest.substring(splitAt);
  }
  if (rest) {
    showBanner("Subpage is too large to save. Shorten labels or entity IDs.", "error");
    return;
  }
  state.subpageSavePending[slot] = full;
  postText("Subpage " + slot + " Config", chunks[0]);
  postText("Subpage " + slot + " Config Ext", chunks[1]);
  postText("Subpage " + slot + " Config Ext 2", chunks[2]);
  postText("Subpage " + slot + " Config Ext 3", chunks[3]);
}

function scheduleSliderSubpageMigration(slot) {
  pendingSliderSubpageMigrations[slot] = true;
  clearTimeout(sliderMigrationTimer);
  sliderMigrationTimer = setTimeout(function () {
    var pending = pendingSliderSubpageMigrations;
    pendingSliderSubpageMigrations = {};
    for (var key in pending) {
      if (state.subpages[key]) saveSubpageEntity(key);
    }
  }, 5000);
}

function postSelect(name, option) {
  post("/select/" + encodeURIComponent(name) + "/set?option=" + encodeURIComponent(option));
}

function postButtonPress(name) {
  post("/button/" + encodeURIComponent(name) + "/press");
}

function postUpdateInstall(name) {
  post("/update/" + encodeURIComponent(name) + "/install");
}

function postSwitch(name, on) {
  post("/switch/" + encodeURIComponent(name) + "/" + (on ? "turn_on" : "turn_off"));
}

function postNumber(name, value) {
  post("/number/" + encodeURIComponent(name) + "/set?value=" + encodeURIComponent(value));
}

function postWithObjectId(domain, name, objectId, action, errorMessage) {
  postWithObjectIds(domain, name, [objectId], action, errorMessage);
}

function postWithObjectIds(domain, name, objectIds, action, errorMessage) {
  var urls = rememberedPostUrls(domain, name, objectIds, action);
  uniquePush(urls, "/" + domain + "/" + encodeURIComponent(name) + "/" + action);
  objectIds.forEach(function (objectId) {
    uniquePush(urls, "/" + domain + "/" + encodeURIComponent(objectId) + "/" + action);
  });
  post(urls, null, errorMessage);
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
  postNumberWithObjectIds("Screensaver Timeout", ["screensaver_timeout"], value);
}

var SCREENSAVER_ACTION_UNAVAILABLE =
  "Screen dimmed screensaver is not available on this firmware. Update the device firmware, then reload this page.";

function postScreensaverAction(value) {
  postSelectWithObjectIds("Screen Saver: Action", [
    "screen_saver__action",
    "screen_saver_action",
    "screensaver_action",
  ], screensaverActionOption(value), SCREENSAVER_ACTION_UNAVAILABLE);
}

function postScreensaverDimmedBrightness(value) {
  postNumberWithObjectIds("Screen Saver: Dimmed Brightness", [
    "screen_saver__dimmed_brightness",
    "screen_saver_dimmed_brightness",
    "screensaver_dimmed_brightness",
  ], value, SCREENSAVER_ACTION_UNAVAILABLE);
}

function postClockBrightnessDay(value) {
  postNumberWithObjectIds("Screen Saver: Daytime Clock Brightness", [
    "screen_saver__daytime_clock_brightness",
    "screen_saver__clock_brightness",
  ], value);
}

function postClockBrightnessNight(value) {
  postNumberWithObjectIds("Screen Saver: Nighttime Clock Brightness", [
    "screen_saver__nighttime_clock_brightness",
    "screen_saver__clock_brightness",
  ], value);
}

function postSwitchWithObjectId(name, objectId, on, errorMessage) {
  postWithObjectId("switch", name, objectId, on ? "turn_on" : "turn_off", errorMessage);
}

function postSwitchWithObjectIds(name, objectIds, on, errorMessage) {
  postWithObjectIds("switch", name, objectIds, on ? "turn_on" : "turn_off", errorMessage);
}

var CLOCK_BAR_UNAVAILABLE =
  "Clock bar setting is not available on this firmware. Update the device firmware, then reload this page.";

function postClockBar(on) {
  postSwitchWithObjectIds("Screen: Clock Bar", [
    "screen__clock_bar",
    "screen_clock_bar",
    "clock_bar_enabled",
  ], on, CLOCK_BAR_UNAVAILABLE);
}

var NETWORK_STATUS_ICON_UNAVAILABLE =
  "Network status icon setting is not available on this firmware. Update the device firmware, then reload this page.";

function postNetworkStatusIcon(on) {
  postSwitchWithObjectIds("Screen: Network Status Icon", [
    "screen__network_status_icon",
    "screen_network_status_icon",
    "network_status_enabled",
  ], on, NETWORK_STATUS_ICON_UNAVAILABLE);
}

var TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE =
  "Temperature degree symbol setting is not available on this firmware. Update the device firmware, then reload this page.";

function postTemperatureDegreeSymbol(on) {
  postSwitchWithObjectIds("Screen: Temperature Degree Symbol", [
    "screen__temperature_degree_symbol",
    "screen_temperature_degree_symbol",
    "temperature_degree_symbol_enabled",
  ], on, TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE);
}

var SCREEN_SCHEDULE_UNAVAILABLE =
  "Screen schedule is not available on this firmware. Update the device firmware, then reload this page.";
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

function postScreenScheduleEnabled(on) {
  postSwitchWithObjectId("Screen: Schedule Enabled", "screen__schedule_enabled", on, SCREEN_SCHEDULE_UNAVAILABLE);
}

function postScreenScheduleOnHour(value) {
  postNumberWithObjectId("Screen: Schedule On Hour", "screen__schedule_on_hour", value, SCREEN_SCHEDULE_UNAVAILABLE);
}

function postScreenScheduleOffHour(value) {
  postNumberWithObjectId("Screen: Schedule Off Hour", "screen__schedule_off_hour", value, SCREEN_SCHEDULE_UNAVAILABLE);
}

function postScreenScheduleMode(value) {
  postSelectWithObjectId(
    "Screen: Schedule Mode",
    "screen__schedule_mode",
    scheduleModeOption(value),
    SCREEN_SCHEDULE_MODE_UNAVAILABLE
  );
}

function postScreenScheduleWakeTimeout(value) {
  postNumberWithObjectIds("Screen: Schedule Wake Timeout", [
    "screen__schedule_wake_timeout",
    "screen_schedule_wake_timeout",
    "schedule_wake_timeout",
  ], value, SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE);
}

function postScreenScheduleWakeBrightness(value) {
  postNumberWithObjectIds("Screen: Schedule Wake Brightness", [
    "screen__schedule_wake_brightness",
    "screen_schedule_wake_brightness",
    "schedule_wake_brightness",
  ], value, SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE);
}

function postScreenScheduleDimmedBrightness(value) {
  postNumberWithObjectIds("Screen: Schedule Dimmed Brightness", [
    "screen__schedule_dimmed_brightness",
    "screen_schedule_dimmed_brightness",
    "schedule_dimmed_brightness",
  ], value, SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE);
}

function postScreenScheduleClockBrightness(value) {
  postNumberWithObjectIds("Screen: Schedule Clock Brightness", [
    "screen__schedule_clock_brightness",
    "screen_schedule_clock_brightness",
    "schedule_clock_brightness",
  ], value, SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE);
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

function entityDetailPath(domain, name) {
  return "/" + encodeURIComponent(domain) + "/" + encodeURIComponent(name) + "?detail=all";
}

function eventStreamEnabled() {
  try {
    return new URLSearchParams(window.location.search).get("events") === "1";
  } catch (_) {
    return false;
  }
}

function cardStateEntities() {
  var items = [
    ["text", "Button Order"],
    ["text", "Button On Color"],
    ["text", "Button Off Color"],
    ["text", "Sensor Card Color"],
  ];

  for (var i = 1; i <= NUM_SLOTS; i++) {
    items.push(["text", "Button " + i + " Config"]);
  }

  return items;
}

function settingsStateEntities() {
  var items = [
    ["switch", "Indoor Temp Enable"],
    ["switch", "Outdoor Temp Enable"],
    ["switch", "Screen: Clock Bar"],
    ["switch", "Screen: Network Status Icon"],
    ["switch", "Screen: Temperature Degree Symbol"],
    ["select", "Screen: Temperature Unit"],
    ["text", "Indoor Temp Entity"],
    ["text", "Outdoor Temp Entity"],
    ["text", "Screensaver Mode"],
    ["select", "Screen Saver: Action"],
    ["text", "Presence Sensor Entity"],
    ["switch", "Screen Saver: Media Player Sleep Prevention"],
    ["text", "Media Player Sleep Prevention Entity"],
    ["number", "Screen Saver: Daytime Clock Brightness"],
    ["number", "Screen Saver: Nighttime Clock Brightness"],
    ["number", "Screen Saver: Clock Brightness"],
    ["number", "Screen Saver: Dimmed Brightness"],
    ["number", "Screensaver Timeout"],
    ["number", "Home Screen Timeout"],
    ["switch", "Screen Saver: Clock"],
    ["select", "Screen: Timezone"],
    ["select", "Screen: Clock Format"],
    ["text", "Screen: NTP Server 1"],
    ["text", "Screen: NTP Server 2"],
    ["text", "Screen: NTP Server 3"],
    ["text_sensor", "Screen: Sunrise"],
    ["text_sensor", "Screen: Sunset"],
    ["text_sensor", "Network Transport"],
    ["sensor", "Wifi Strength"],
    ["switch", "Screen: Schedule Enabled"],
    ["select", "Screen: Schedule Mode"],
    ["number", "Screen: Schedule On Hour"],
    ["number", "Screen: Schedule Off Hour"],
    ["number", "Screen: Schedule Wake Timeout"],
    ["number", "Screen: Schedule Wake Brightness"],
    ["number", "Screen: Schedule Dimmed Brightness"],
    ["number", "Screen: Schedule Clock Brightness"],
    ["number", "Screen: Daytime Brightness"],
    ["number", "Screen: Nighttime Brightness"],
    ["text_sensor", "Firmware: Version"],
    ["update", "Firmware: Update"],
    ["switch", "Firmware: Auto Update"],
    ["select", "Firmware: Update Frequency"],
    ["switch", "Developer: Experimental Features"],
  ];

  if (CFG.features && CFG.features.screenRotation) {
    items.push(["select", "Screen: Rotation"]);
  }

  return items;
}

function subpageStateEntities() {
  var items = [];
  for (var i = 1; i <= NUM_SLOTS; i++) {
    items.push(["text", "Subpage " + i + " Config"]);
    items.push(["text", "Subpage " + i + " Config Ext"]);
    items.push(["text", "Subpage " + i + " Config Ext 2"]);
    items.push(["text", "Subpage " + i + " Config Ext 3"]);
  }

  return items;
}

function loadStateItems(items, handleState, concurrency) {
  var index = 0;
  var active = 0;
  var loadedCount = 0;
  var limit = Math.max(1, concurrency || 1);

  return new Promise(function (resolve) {
    function done() {
      active--;
      run();
    }

    function run() {
      if (index >= items.length && active === 0) {
        resolve(loadedCount);
        return;
      }

      while (active < limit && index < items.length) {
        var item = items[index++];
        active++;
        getJsonQuietly(entityDetailPath(item[0], item[1])).then(function (data) {
          if (data) {
            loadedCount++;
            handleState(data);
          }
        }).then(done, done);
      }
    }

    run();
  });
}

function loadInitialState(handleState, onLoaded) {
  loadStateItems(cardStateEntities(), handleState, 4).then(function (loadedCount) {
    if (loadedCount === 0) {
      setConfigLocked(true, "Reconnecting to device\u2026");
      showBanner("Reconnecting to device\u2026", "offline");
      setTimeout(connectEvents, 5000);
      return;
    }
    if (onLoaded) onLoaded();
    clearTimeout(migrationTimer);
    migrationTimer = setTimeout(scheduleMigration, 5000);
    clearTimeout(sliderMigrationTimer);
    pendingSliderSubpageMigrations = {};

    loadStateItems(settingsStateEntities(), handleState, 2).then(function () {
      loadStateItems(subpageStateEntities(), handleState, 2);
    });
  });
}

function refreshFirmwareVersion() {
  getJsonQuietly("/text_sensor/" + encodeURIComponent("Firmware: Version") + "?detail=all", function (d) {
    setFirmwareVersion(d.state || d.value);
  });
  getJsonQuietly("/update/" + encodeURIComponent("Firmware: Update") + "?detail=all", function (d) {
    setFirmwareUpdateInfo(d);
  });
}

function refreshScreensaverTimeout() {
  getJsonQuietly("/number/" + encodeURIComponent("Screensaver Timeout") + "?detail=all", applyScreensaverTimeoutState)
    .then(function (data) {
      if (!data) {
        getJsonQuietly("/number/" + encodeURIComponent("screensaver_timeout") + "?detail=all", applyScreensaverTimeoutState);
      }
    });
}

function waitForReboot() {
  if (_eventSource) { _eventSource.close(); _eventSource = null; }
  setConfigLocked(true, "Restarting device\u2026");
  showBanner("Restarting device\u2026", "offline");
  setTimeout(function () {
    connectEvents();
  }, 15000);
}
