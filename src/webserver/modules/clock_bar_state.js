// ── Clock Bar State ───────────────────────────────────────────────────
// @web-module-requires: state, environment_state, entity_state

function clockBarVisibleInPreview() {
  return !!state.clockBarOn;
}

function timezonePrefersFahrenheit(timezone) {
  var tz = getTzId(effectiveTimezoneOptionForWeb(timezone || state.timezone));
  var fahrenheitZones = {
    "America/Adak": true,
    "America/Anchorage": true,
    "America/Boise": true,
    "America/Chicago": true,
    "America/Denver": true,
    "America/Detroit": true,
    "America/Juneau": true,
    "America/Los_Angeles": true,
    "America/New_York": true,
    "America/Phoenix": true,
    "America/Puerto_Rico": true,
    "Pacific/Guam": true,
    "Pacific/Honolulu": true,
    "Pacific/Pago_Pago": true,
  };
  return !!fahrenheitZones[tz];
}

function temperatureUnitSymbol() {
  var unit = normalizeTemperatureUnit(state.temperatureUnit);
  if (unit === "\u00B0F") return "\u00B0F";
  if (unit === "\u00B0C") return "\u00B0C";
  return timezonePrefersFahrenheit(state.timezone) ? "\u00B0F" : "\u00B0C";
}

function clockBarTemperatureUnitSymbol() {
  return state.temperatureDegreeSymbolOn ? "\u00B0" : "";
}

var MAX_CLOCK_BAR_TEMPERATURES = 1;

function defaultClockBarTemperatureEntity(index) {
  if (index === 0) return "sensor.outdoor_temperature";
  return "";
}

function normalizeClockBarTemperatureEntries(value) {
  var input = Array.isArray(value) ? value : String(value || "").split(/[|,\n]/);
  return input.map(function (entry) {
    return String(entry || "").trim();
  }).slice(0, MAX_CLOCK_BAR_TEMPERATURES);
}

function normalizeClockBarTemperatureEntities(value) {
  var input = normalizeClockBarTemperatureEntries(value);
  var out = [];
  input.forEach(function (entry) {
    if (entry && out.indexOf(entry) === -1) out.push(entry);
  });
  return out.slice(0, MAX_CLOCK_BAR_TEMPERATURES);
}

function serializeClockBarTemperatureEntities(list) {
  return normalizeClockBarTemperatureEntities(list).join("|");
}

function legacyClockBarTemperatureEntities() {
  var list = [];
  if (state._outdoorOn && state.outdoorEntity) list.push(state.outdoorEntity);
  if (state._indoorOn && state.indoorEntity) list.push(state.indoorEntity);
  return normalizeClockBarTemperatureEntities(list);
}

function clockBarTemperatureEntries() {
  var list = normalizeClockBarTemperatureEntries(state.clockBarTemperatureEntities);
  if (!list.length && !state._clockBarTemperatureEntitiesReceived) return legacyClockBarTemperatureEntities();
  return list;
}

function clockBarTemperatureEntities() {
  return normalizeClockBarTemperatureEntities(clockBarTemperatureEntries());
}

function primaryClockBarTemperatureEntity() {
  return clockBarTemperatureEntities()[0] || state.outdoorEntity || "";
}

function clockBarTemperatureVisible() {
  return !!(state._outdoorOn && primaryClockBarTemperatureEntity());
}

function applyClockBarTemperatureEntities(list, postDevice) {
  state.clockBarTemperatureEntities = normalizeClockBarTemperatureEntries(list);
  state._clockBarTemperatureEntitiesReceived = true;
  var configured = clockBarTemperatureEntities();
  if (!state._clockBarTemperatureVisibilityReceived) {
    state._outdoorOn = configured.length > 0;
  }
  state._indoorOn = false;
  state.outdoorEntity = configured[0] || "";
  state.indoorEntity = "";
  if (postDevice) {
    postClockBarTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
    postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
    postSwitch(entityName("indoor_temp_enable"), state._indoorOn);
    postText(entityName("outdoor_temp_entity"), state.outdoorEntity);
    postText(entityName("indoor_temp_entity"), state.indoorEntity);
  }
  syncTemperatureUi();
  updateTempPreview();
  updateClockBarItemUi();
}

function saveClockBarTemperatureSettings(entity, degreeSymbolOn) {
  entity = String(entity || "").trim();
  state.clockBarTemperatureEntities = entity ? [entity] : [];
  state._clockBarTemperatureEntitiesReceived = true;
  state._clockBarTemperatureVisibilityReceived = true;
  state._outdoorOn = !!entity;
  state._indoorOn = false;
  state.outdoorEntity = entity;
  state.indoorEntity = "";
  state.temperatureDegreeSymbolOn = !!degreeSymbolOn;
  postClockBarTemperatureEntities(serializeClockBarTemperatureEntities(state.clockBarTemperatureEntities));
  postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
  postSwitch(entityName("indoor_temp_enable"), false);
  postText(entityName("outdoor_temp_entity"), state.outdoorEntity);
  postText(entityName("indoor_temp_entity"), "");
  postTemperatureDegreeSymbol(state.temperatureDegreeSymbolOn);
  syncTemperatureUi();
  syncClockBarUi();
}

function setClockBarItemVisible(item, visible) {
  visible = !!visible;
  if (isClockBarTemperatureItem(item)) {
    var entity = primaryClockBarTemperatureEntity();
    if (visible && !entity) {
      entity = defaultClockBarTemperatureEntity(0);
      state.clockBarTemperatureEntities = [entity];
      state._clockBarTemperatureEntitiesReceived = true;
      state.outdoorEntity = entity;
      postClockBarTemperatureEntities(entity);
      postText(entityName("outdoor_temp_entity"), entity);
    }
    state._clockBarTemperatureVisibilityReceived = true;
    state._outdoorOn = visible && !!entity;
    state._indoorOn = false;
    postSwitch(entityName("outdoor_temp_enable"), state._outdoorOn);
    postSwitch(entityName("indoor_temp_enable"), false);
    postText(entityName("indoor_temp_entity"), "");
  } else if (item === "time") {
    state.clockBarTimeOn = visible;
    postClockBarTime(state.clockBarTimeOn);
  } else if (item === "voice" && voiceServicesSupported()) {
    state.voiceServicesOn = visible;
    postVoiceServices(state.voiceServicesOn);
  } else if (item === "network") {
    state.networkStatusOn = visible;
    postNetworkStatusIcon(state.networkStatusOn);
  }
  syncClockBarUi();
  syncTemperatureUi();
}

function syncTemperatureUi() {
  if (els.setIndoorToggle) els.setIndoorToggle.checked = !!state._indoorOn;
  if (els.setIndoorField) {
    els.setIndoorField.className = "sp-cond-field" + (state._indoorOn ? " sp-visible" : "");
  }
  if (els.setOutdoorToggle) els.setOutdoorToggle.checked = !!state._outdoorOn;
  if (els.setOutdoorField) {
    els.setOutdoorField.className = "sp-cond-field" + (state._outdoorOn ? " sp-visible" : "");
  }
}

function syncClockBarUi() {
  var visible = clockBarVisibleInPreview();
  if (!visible && state.clockBarSelectedItem) {
    state.clockBarSelectedItem = "";
    hideSettingsOverlay();
  }
  syncPreviewGridTop();
  if (els.topbar) els.topbar.className = "sp-topbar" + (visible ? "" : " sp-hidden");
  if (els.setClockBarToggle) els.setClockBarToggle.checked = !!state.clockBarOn;
  if (els.setClockBarTimeToggle) els.setClockBarTimeToggle.checked = !!state.clockBarTimeOn;
  if (els.setNetworkStatusToggle) {
    els.setNetworkStatusToggle.checked = !!state.networkStatusOn;
  }
  if (els.setVoiceServicesToggle) {
    els.setVoiceServicesToggle.checked = !!state.voiceServicesOn;
  }
  if (els.setClockBarBadge) {
    els.setClockBarBadge.className = "sp-card-badge" + (state.clockBarOn ? "" : " sp-hidden");
  }
  if (els.setTemperatureDegreeSymbolToggle) {
    els.setTemperatureDegreeSymbolToggle.checked = !!state.temperatureDegreeSymbolOn;
  }
  if (els.setSubpageChevronToggle) {
    els.setSubpageChevronToggle.checked = !!state.subpageChevronsOn;
  }
  updateClockBarItemUi();
  renderSelectionBar(ctx());
  updateNetworkPreview();
  updateVoicePreview();
  updateTempPreview();
}
