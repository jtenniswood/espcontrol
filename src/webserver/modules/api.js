// ── POST queue ─────────────────────────────────────────────────────────

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

function uniquePush(list, value) {
  if (value && list.indexOf(value) === -1) list.push(value);
}

function postQuiet(url) {
  return fetch(url, { method: "POST", keepalive: true }).catch(function () {
    return null;
  });
}

function entityDef(key) {
  return ENTITY_CATALOG.entities[key] || {};
}

function entityName(key) {
  return entityDef(key).name || "";
}

function entityNameForSlot(key, slot) {
  return String(entityDef(key).template || "").replace("{slot}", String(slot));
}

function entityObjectIds(key) {
  return (entityDef(key).objectIds || []).slice();
}

function entityLookupNames(key) {
  var names = [];
  uniquePush(names, entityName(key));
  entityObjectIds(key).forEach(function (objectId) { uniquePush(names, objectId); });
  return names;
}

function entityStateItem(key) {
  var def = entityDef(key);
  return [def.domain, def.name];
}

function entityStateItems(keys) {
  return keys.map(entityStateItem);
}

function entityStateItemsForSlots(keys) {
  var items = [];
  for (var i = 1; i <= TOTAL_SLOTS; i++) {
    keys.forEach(function (key) {
      items.push([entityDef(key).domain, entityNameForSlot(key, i)]);
    });
  }
  return items;
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
  if (button.type === "action") {
    var stateEntity = actionCardStateEntity(button);
    if (stateEntity) rememberEntityName(stateEntity, titleFromEntityId(stateEntity));
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
  clockBarTemperatureEntities().forEach(function (entityId, index) {
    rememberEntityName(entityId, "Clock Bar Temperature " + (index + 1));
  });
  rememberEntityName(state.presenceEntity, "Presence Sensor");
  rememberEntityName(state.coverArtMediaPlayerEntity, "Media Player");
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

function hasRememberedPostPath(domain, name, objectIds) {
  var keys = [domain + ":" + name, domain + "-" + esphomeObjectId(name)];
  (objectIds || []).forEach(function (objectId) {
    keys.push(domain + ":" + objectId);
    keys.push(domain + "-" + objectId);
  });
  return keys.some(function (key) {
    return !!state.entityPostPaths[key];
  });
}

function entityPostUrls(domain, name, objectIds, action) {
  var urls = [];
  rememberedPostUrls(domain, name, objectIds || [], action).forEach(function (url) {
    uniquePush(urls, url);
  });
  (objectIds || []).forEach(function (objectId) {
    uniquePush(urls, "/" + domain + "/" + encodeURIComponent(objectId) + "/" + action);
  });
  uniquePush(urls, "/" + domain + "/" + encodeURIComponent(name) + "/" + action);
  uniquePush(urls, "/" + domain + "/" + encodeURIComponent(esphomeObjectId(name)) + "/" + action);
  return urls;
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

function saveButtonConfig(slot) {
  var b = state.buttons[slot - 1];
  postText(entityNameForSlot("button_config", slot), serializeButtonConfig(b));
}

function subpageEntityKeys() {
  var keys = ENTITY_CATALOG.groups.subpage_slot || [];
  var count = (CFG.features && CFG.features.subpageConfigChunks) || keys.length;
  count = Math.max(1, Math.min(keys.length, parseInt(count, 10) || keys.length));
  return keys.slice(0, count);
}

var SUBPAGE_RAW_CHUNK_FIELDS = ["main", "ext", "ext2", "ext3", "ext4", "ext5", "ext6", "ext7"];

function subpageChunkShouldPost(slot, keys, chunks, index, previousPendingChunks) {
  if (chunks[index] || index === 0) return true;
  var chunkName = entityNameForSlot(keys[index], slot);
  if (hasRememberedPostPath("text", chunkName, [])) return true;
  var raw = state.subpageRaw[slot];
  var rawField = SUBPAGE_RAW_CHUNK_FIELDS[index];
  return !!(
    (raw && rawField && raw[rawField]) ||
    (previousPendingChunks && previousPendingChunks[index])
  );
}

function saveSubpageEntity(slot) {
  var sp = state.subpages[slot];
  var full = sp ? serializeSubpageConfig(sp) : "";
  var keys = subpageEntityKeys();
  var chunks = EspControlModel.splitSubpageConfigChunks(full, keys.length, 255);
  if (!chunks) {
    showBanner("Subpage is too large to save. Shorten labels or entity IDs.", "error");
    return;
  }
  var previousPendingChunks = EspControlModel.splitSubpageConfigChunks(
    state.subpageSavePending[slot] || "", keys.length, 255) || [];
  state.subpageSavePending[slot] = full;
  for (var ki = 0; ki < keys.length; ki++) {
    var chunkName = entityNameForSlot(keys[ki], slot);
    var chunk = chunks[ki] || "";
    if (!subpageChunkShouldPost(slot, keys, chunks, ki, previousPendingChunks)) continue;
    postText(chunkName, chunk);
  }
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
  return post(entityPostUrls("select", name, [], "set?option=" + encodeURIComponent(option)));
}

function postButtonPress(name) {
  return post(entityPostUrls("button", name, [], "press"));
}

function postFirmwareUpdateInstall() {
  var urls = [];
  rememberedPostUrls("button", entityName("firmware_install_update"), entityObjectIds("firmware_install_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("firmware_install_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  rememberedPostUrls("update", entityName("firmware_update"), entityObjectIds("firmware_update"), "install")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("firmware_update").forEach(function (name) {
    uniquePush(urls, "/update/" + encodeURIComponent(name) + "/install");
  });

  post(urls, null, "Could not start firmware update.");
}

function postFirmwareUpdateCheck() {
  var urls = [];
  rememberedPostUrls("button", entityName("firmware_check_for_update"), entityObjectIds("firmware_check_for_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("firmware_check_for_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  post(urls, null, "Could not check for firmware update.");
}

function postC6FirmwareUpdateInstall() {
  var urls = [];
  rememberedPostUrls("button", entityName("esp32_c6_install_update"), entityObjectIds("esp32_c6_install_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("esp32_c6_install_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  post(urls, null, "Could not start WiFi firmware update.");
}

function postC6FirmwareUpdateCheck() {
  var urls = [];
  rememberedPostUrls("button", entityName("esp32_c6_check_for_update"), entityObjectIds("esp32_c6_check_for_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("esp32_c6_check_for_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  post(urls, null, "Could not check WiFi firmware update.");
}

function ensurePublicFirmwareOtaUrl(info) {
  info = info || selectedFirmwareInfo();
  if (info && info.ota_url) return Promise.resolve(info.ota_url);
  if (state.firmwareOtaUrl) return Promise.resolve(state.firmwareOtaUrl);
  return getJsonQuietly(publicFirmwareVersionsUrl(), function (d) {
    setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
  }).then(function () {
    info = selectedFirmwareInfo();
    if (info && info.ota_url) return info.ota_url;
    return getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
      setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
    }).then(function () {
      return state.firmwareOtaUrl || "";
    });
  });
}

function publicFirmwareOtaFilename(info) {
  return info && info.ota_filename ? info.ota_filename :
    (state.firmwareOtaFilename || (DEVICE_ID + ".ota.bin"));
}

function installPublicFirmwareViaWebOta(info) {
  info = info || selectedFirmwareInfo();
  return getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
    if (!info || selectedFirmwareIsLatest()) setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
  }).then(function () {
    info = info || selectedFirmwareInfo();
    var targetVersion = info && info.latest_version ? info.latest_version : state.firmwareLatestVersion;
    if (isSpecificFirmwareVersion(targetVersion)) {
      state.firmwareInstallTargetVersion = targetVersion;
    }
  }).then(function () {
    clearFirmwareWebOtaFallback();
    state.firmwareInstallPostPending = false;
    state.firmwareChecking = false;
    state.firmwareUpdateState = "INSTALLING";
    state.firmwareInstallError = "";
    state.firmwareInstallStatus = state.firmwareInstallTargetVersion ?
      "Uploading firmware " + state.firmwareInstallTargetVersion + "\u2026" :
      "Uploading firmware update\u2026";
    renderFirmwareUpdateStatus();
    startFirmwareInstallRefresh();

    var uploadStarted = false;
    var uploadResponseReceived = false;
    return ensurePublicFirmwareOtaUrl(info).then(function (otaUrl) {
      if (!otaUrl) throw new Error("Firmware file is not available yet.");
      return fetch(otaUrl, { cache: "no-store" });
    }).then(function (response) {
      if (!response.ok) throw new Error("Could not download firmware file (" + response.status + ").");
      return response.blob();
    }).then(function (blob) {
      var filename = publicFirmwareOtaFilename(info);
      var form = new FormData();
      form.append("file", blob, filename);
      uploadStarted = true;
      return fetch("/update", { method: "POST", body: form });
    }).then(function (response) {
      uploadResponseReceived = true;
      return response.text().catch(function () {
        return "";
      }).then(function (text) {
        if (!response.ok) {
          throw new Error("Device rejected firmware upload (" + response.status + ").");
        }
        if (/update failed/i.test(text)) {
          throw new Error("Device reported that the firmware upload failed.");
        }
        waitForFirmwareRestart();
        return true;
      });
    }).catch(function (err) {
      if (uploadStarted && !uploadResponseReceived) {
        waitForFirmwareRestart();
        return true;
      }
      failPublicFirmwareUpload(err && err.message);
      return false;
    });
  });
}

function waitForFirmwareRestart() {
  state.firmwareInstallError = "";
  state.firmwareInstallStatus = "Waiting for device to restart\u2026";
  renderFirmwareUpdateStatus();
  setConfigLocked(true, "Waiting for device to restart\u2026");
  showBanner("Firmware uploaded. Waiting for device to restart\u2026", "offline");
  setTimeout(connectEvents, 5000);
}

function failPublicFirmwareUpload(message) {
  var reason = message || "Could not upload firmware update.";
  stopFirmwareInstallRefresh();
  state.firmwareUpdateState = "";
  state.firmwareInstallError = "Firmware update failed: " + reason;
  renderFirmwareUpdateStatus();
  showBanner(reason, "error");
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

function eventStreamEnabled() {
  try {
    return new URLSearchParams(window.location.search).get("events") === "1";
  } catch (_) {
    return false;
  }
}

function cardStateEntities() {
  return entityStateItems(ENTITY_CATALOG.groups.card)
    .concat(entityStateItemsForSlots(ENTITY_CATALOG.groups.card_slot));
}

function settingsStateEntities() {
  var items = entityStateItems(ENTITY_CATALOG.groups.settings);

  if (CFG.features && CFG.features.screenRotation) {
    items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_optional));
  }
  if (CFG.features && CFG.features.voiceServices) {
    items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_voice));
  }

  return items;
}

function subpageStateEntities() {
  return entityStateItemsForSlots(subpageEntityKeys());
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
        getJsonQuietly(entityDetailPath(item[0], item[1], entityInitialDetail(item[0]))).then(function (data) {
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
  var pending = 12;
  if (!state.firmwareVersion) {
    state.firmwareVersionRefreshPending = true;
    renderFirmwareVersion();
  }
  function finishFirmwareVersionRefresh() {
    pending--;
    if (pending > 0) return;
    state.firmwareVersionRefreshPending = false;
    renderFirmwareVersion();
  }

  getJsonQuietly(FIRMWARE_VERSION_METADATA_PATH, function (d) {
    setFirmwareVersion(firmwareVersionFromMetadata(d));
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
    setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonQuietly(publicFirmwareVersionsUrl(), function (d) {
    setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("firmware_version")), function (d) {
    setFirmwareVersion(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("update", entityLookupNames("firmware_update")), function (d) {
    rememberEntityPostPath(d);
    setFirmwareUpdateInfo(d);
  }).then(function (data) {
    if (!data && state.firmwareUpdateControlsSupported !== true) {
      state.firmwareUpdateControlsSupported = false;
      syncFirmwareUpdateUi();
    }
    finishFirmwareVersionRefresh();
  }, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("firmware_install_update")), function (d) {
    rememberEntityPostPath(d);
    state.firmwareUpdateControlsSupported = true;
    state.firmwareInstallControlsSupported = true;
    renderFirmwareUpdateStatus();
    syncFirmwareUpdateUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("firmware_check_for_update")), function (d) {
    rememberEntityPostPath(d);
    state.firmwareUpdateControlsSupported = true;
    renderFirmwareUpdateStatus();
    syncFirmwareUpdateUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_current_firmware")), function (d) {
    rememberEntityPostPath(d);
    setC6FirmwareCurrentVersion(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_latest_firmware")), function (d) {
    rememberEntityPostPath(d);
    setC6FirmwareLatestVersion(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_update_available")), function (d) {
    rememberEntityPostPath(d);
    setC6FirmwareUpdateAvailable(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("esp32_c6_install_update")), function (d) {
    rememberEntityPostPath(d);
    state.c6FirmwareUpdateControlsSupported = true;
    state.c6FirmwareInstallControlsSupported = true;
    syncC6FirmwareUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("esp32_c6_check_for_update")), function (d) {
    rememberEntityPostPath(d);
    state.c6FirmwareUpdateControlsSupported = true;
    syncC6FirmwareUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
}

function refreshScreensaverTimeout() {
  getJsonQuietly("/number/" + encodeURIComponent(entityName("screensaver_timeout")) + "?detail=all", applyScreensaverTimeoutState)
    .then(function (data) {
      if (!data) {
        getJsonQuietly("/number/" + encodeURIComponent(entityObjectIds("screensaver_timeout")[0]) + "?detail=all", applyScreensaverTimeoutState);
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
