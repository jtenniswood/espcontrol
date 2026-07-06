// ── Subpage helpers ────────────────────────────────────────────────────

var SENSOR_STATE_LABELS_OPTION = cardContractOptionName("state_labels");
var SENSOR_STATE_INPUT_OPTION = cardContractOptionName("state_input");
var SENSOR_STATE_OUTPUT_OPTION = cardContractOptionName("state_output");
var SENSOR_STATE_INPUT_2_OPTION = cardContractOptionName("state_input_2");
var SENSOR_STATE_OUTPUT_2_OPTION = cardContractOptionName("state_output_2");
var SENSOR_STATE_LOW_LABEL_OPTION = cardContractOptionName("state_low_label");
var SENSOR_STATE_HIGH_LABEL_OPTION = cardContractOptionName("state_high_label");
var CARD_ON_PATTERN_OPTION = cardContractOptionName("on_pattern");

function normalizeWithRegisteredCardType(b) {
  if (!b || typeof BUTTON_TYPES === "undefined") return false;
  var typeDef = BUTTON_TYPES[b.type || ""];
  if (!typeDef || typeof typeDef.normalizeConfig !== "function") return false;
  typeDef.normalizeConfig(b);
  return true;
}

function normalizeButtonConfig(b) {
  if (b) b.options = b.options || "";
  if (b && b.type === "local") {
    b.type = "action";
    b.sensor = ACTION_CARD_LOCAL_ACTION;
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto" || b.icon === "Flash") b.icon = "Gesture Tap";
  }
  if (b && b.type === "local_sensor") {
    b.type = "sensor";
    b.sensor = SENSOR_CARD_LOCAL_SENSOR;
    b.icon_on = "Auto";
    b.options = "";
    if (b.precision !== "text" && b.precision !== "1" && b.precision !== "2") b.precision = "";
    if (b.precision !== "text" && (!b.icon || b.icon === "Auto")) b.icon = "Auto";
  }
  if (b && b.type === "action" && b.sensor === "vacuum.start") {
    b.type = "vacuum";
    b.sensor = "start_stop";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Robot Vacuum";
  }
  if (b && b.type === "action" && b.sensor === "vacuum.return_to_base") {
    b.type = "vacuum";
    b.sensor = "dock";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Robot Vacuum Variant";
  }
  if (b && isBrightnessSliderType(b.type) && b.sensor) {
    b.sensor = "";
  }
  if (b && isFanCardType(b.type)) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = b.type === "fan_control" ? normalizeFanControlOptions(b.options) : "";
    if (!b.icon || b.icon === "Auto") b.icon = fanCardDefaultIcon(b.type);
    if (b.type === "fan_switch") {
      if (!b.icon_on || b.icon_on === "Auto") b.icon_on = "Fan";
    } else {
      b.icon_on = "Auto";
    }
  }
  if (b && b.type === "weather_forecast") {
    var weatherAlias = cardContractMigrationAlias(b.type);
    b.type = weatherAlias && weatherAlias.type || "weather";
    b.precision = weatherAlias && weatherAlias.precision || "tomorrow";
    if (b.label === "Weather") b.label = "";
  }
  if (b && b.type === "weather") {
    b.sensor = "";
    b.precision = normalizeWeatherCardMode(b.precision);
    b.options = cardLargeNumbersSupported(b) ? copyLargeNumbersOption("", b.options) : "";
  }
  if (b && b.type === "text_sensor") {
    var textSensorAlias = cardContractMigrationAlias(b.type);
    b.type = textSensorAlias && textSensorAlias.type || "sensor";
    b.precision = textSensorAlias && textSensorAlias.precision || "text";
    b.entity = "";
    b.label = "";
    b.unit = "";
    b.icon_on = "Auto";
    if (!b.icon) b.icon = "Auto";
  }
  if (b && b.type === "media") {
    var rawMediaMode = b.sensor;
    if (rawMediaMode === "controls") {
      if (!b.icon || b.icon === "Speaker") b.icon = "Auto";
    }
    b.sensor = mediaEditorMode(b.sensor);
    if (b.sensor === "previous" && b.label === "Skip Previous") b.label = "Previous";
    if (b.sensor === "next" && b.label === "Skip Next") b.label = "Next";
    if (b.sensor === "volume") {
      if (!b.label || b.label === "Media") b.label = "Volume";
      b.icon = "Auto";
    }
    if (b.sensor === "playlist") {
      if (!b.label || b.label === "Media") b.label = "Playlist";
      if (!b.icon || b.icon === "Auto") b.icon = "Music";
    }
    if (b.sensor === "position" && (!b.label || b.label === "Track")) b.label = "Position";
    if (b.sensor === "now_playing") {
      b.precision = mediaNowPlayingControls(b);
    } else if (mediaStateDisplayModeSupported(b.sensor) && b.precision === "state") {
      b.precision = "state";
    } else {
      b.precision = "";
    }
    b.options = normalizeMediaOptions(b.options, b.sensor);
  }
  if (b && isClimateCardType(b.type)) {
    b.sensor = "";
    b.unit = "";
    if (!b.icon) b.icon = "Thermostat";
    if (!b.icon_on) b.icon_on = "Auto";
    b.precision = normalizeClimatePrecisionConfig(b.precision);
    b.options = normalizeClimateOptions(b.options, b.type === "climate_control");
  }
  if (b && b.type === "garage") {
    if (b.sensor !== "open" && b.sensor !== "close") b.sensor = "";
    b.unit = "";
    b.precision = "";
    if (b.sensor === "open" || b.sensor === "close") b.icon_on = "Auto";
    b.options = normalizeGarageOptions(b.options, b.sensor);
  }
  if (b && b.type === "gate") {
    if (b.sensor !== "open" && b.sensor !== "close" && b.sensor !== "stop") b.sensor = "";
    b.unit = "";
    b.precision = "";
    if (b.sensor === "open" || b.sensor === "close" || b.sensor === "stop") b.icon_on = "Auto";
    b.options = normalizeGateOptions(b.options, b.sensor);
  }
  if (b && b.type === "cover") {
    b.sensor = normalizeCoverMode(b.sensor, true);
    b.options = normalizeCoverOptionsForMode(b.options, b.sensor);
  }
  if (b && b.type === "lock") {
    b.sensor = (b.sensor === "lock" || b.sensor === "unlock") ? b.sensor : "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = b.sensor ? "Auto" : ((!b.icon_on || b.icon_on === "Auto") ? "Lock Open" : b.icon_on);
  }
  if (b && b.type === "alarm") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Security";
    b.options = normalizeAlarmOptions(b.options);
  }
  if (b && b.type === "alarm_action") {
    b.sensor = alarmActionInfo(b.sensor) ? b.sensor : "away";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.label) b.label = alarmActionInfo(b.sensor).label;
    if (!b.icon || b.icon === "Auto" || b.icon === alarmActionLegacyIcon(b.sensor)) {
      b.icon = alarmActionInfo(b.sensor).icon;
    }
    b.options = normalizeAlarmOptions(b.options);
  }
  if (b && b.type === "webhook") {
    if (typeof normalizeWebhookConfig === "function") normalizeWebhookConfig(b);
  }
  normalizeWithRegisteredCardType(b);
  if (b && b.type === "screen_lock") {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon = "Lock";
    b.icon_on = "Lock Open";
  }
  if (b && b.type === "calendar") {
    if (!b.entity) b.entity = cardContractDefaultConfig("calendar").entity;
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = b.precision === "datetime" ? "datetime" : "";
    b.options = normalizeDateTimeOptions("calendar", b.options, b.precision);
  }
  if (b && b.type === "clock") {
    b.entity = "";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeDateTimeOptions("clock", b.options, b.precision);
  }
  if (b && b.type === "timezone") {
    if (!b.entity) b.entity = cardContractDefaultConfig("timezone").entity;
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeDateTimeOptions("timezone", b.options, b.precision);
  }
  if (b && b.type === "todo") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Check";
    b.options = normalizeTodoOptions(b.options);
  }
  if (b && b.type === "image") {
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeImageOptions(b.options);
    b.icon = imageIconEnabled(b) ? (b.icon && b.icon !== "Auto" ? b.icon : "Camera") : "Auto";
    if (!imageLabelEnabled(b)) b.label = "";
  }
  if (b && b.type === "light_switch") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
  }
  if (b && b.type === "light_control") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeLightControlOptions(b.options);
  }
  if (b && b.type === "fan_control") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeFanControlOptions(b.options);
  }
  if (b && b.type === "subpage") {
    applySubpagePresetConfig(b);
    b.options = normalizeSubpageOptions(b.options, b.sensor, b.precision);
  }
  if (b && b.type === "option_select") {
    b.type = "action";
    b.sensor = ACTION_CARD_OPTION_SELECT_ACTION;
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = "";
    if (!b.icon || b.icon === "Auto" || b.icon === "Chevron Down") b.icon = "Flash";
  }
  if (b && actionCardIsOptionSelect(b)) {
    b.sensor = ACTION_CARD_OPTION_SELECT_ACTION;
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = "";
    if (!b.icon || b.icon === "Auto" || b.icon === "Chevron Down") b.icon = "Flash";
  } else if (b && actionCardIsLocal(b)) {
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = "";
    if (!b.icon || b.icon === "Auto" || b.icon === "Flash") b.icon = "Gesture Tap";
  } else if (b && b.type === "action") {
    b.options = normalizeActionOptions(b.options, b.sensor);
  }
  if (b && sensorCardIsLocal(b)) {
    b.type = "sensor";
    b.sensor = SENSOR_CARD_LOCAL_SENSOR;
    b.icon_on = "Auto";
    b.options = "";
    if (b.precision !== "text" && b.precision !== "1" && b.precision !== "2") b.precision = "";
    if (b.precision !== "text" && (!b.icon || b.icon === "Auto")) b.icon = "Auto";
  } else if (b && !b.type) {
    b.options = normalizeSwitchConfirmationOptions(b.options);
  } else if (b && b.type === "sensor") {
    b.options = normalizeSensorOptions(b.options, b.precision);
  } else if (b && b.type === "door_window") {
    b.entity = "";
    b.unit = "";
    b.precision = normalizeDoorWindowSubtype(b.precision);
    if (!b.icon || b.icon === "Auto") b.icon = doorWindowClosedIcon(b.precision);
    if (!b.icon_on || b.icon_on === "Auto") b.icon_on = doorWindowOpenIcon(b.precision);
    b.options = normalizeDoorWindowOptions(b.options);
  } else if (b && b.type === "presence") {
    b.entity = "";
    b.unit = "";
    b.precision = "";
    if (!b.icon || b.icon === "Auto") b.icon = "Motion Sensor Off";
    if (!b.icon_on || b.icon_on === "Auto") b.icon_on = "Motion Sensor";
    b.options = normalizePresenceOptions(b.options);
  } else if (b && b.type !== "action" && b.type !== "alarm" && b.type !== "alarm_action" && !isClimateCardType(b.type) && b.type !== "cover" && b.type !== "garage" && b.type !== "gate" && b.type !== "webhook" && b.type !== "screen_lock" && b.type !== "todo" && b.type !== "media" && b.type !== "presence" && b.type !== "subpage" && b.type !== "image" && b.type !== "light_control" && b.type !== "vacuum" && b.type !== "lawn_mower" && !isFanCardType(b.type) && !cardLargeNumbersSupported(b)) {
    b.options = "";
  }
  return b;
}

function isBrightnessSliderType(type) {
  return cardContractIsBrightnessSliderType(type);
}

function isFanCardType(type) {
  return cardContractIsFanCardType(type);
}

function isClimateCardType(type) {
  return type === "climate" || type === "climate_control";
}

function isOptionSelectType(type) {
  return cardContractIsOptionSelectType(type);
}

function fanCardDefaultIcon(type) {
  return cardContractFanDefaultIcon(type);
}

var SENSOR_LARGE_NUMBERS_OPTION = cardContractOptionName("large_numbers");
var SENSOR_LARGE_NUMBERS_OFF_VALUE = "off";
var SENSOR_ACTIVE_COLOR_OPTION = cardContractOptionName("active_color");
var SWITCH_CONFIRM_OFF_OPTION = cardContractOptionName("confirm_off");
var SWITCH_CONFIRM_ON_OPTION = cardContractOptionName("confirm_on");
var SWITCH_CONFIRM_MESSAGE_OPTION = cardContractOptionName("confirm_message");
var SWITCH_CONFIRM_YES_OPTION = cardContractOptionName("confirm_yes");
var SWITCH_CONFIRM_NO_OPTION = cardContractOptionName("confirm_no");
var SWITCH_CONFIRM_DEFAULT_MESSAGE = "Turn off this device?";
var SWITCH_CONFIRM_ON_DEFAULT_MESSAGE = "Turn on this device?";
var SWITCH_CONFIRM_BOTH_DEFAULT_MESSAGE = "Toggle this device?";
var SWITCH_CONFIRM_DEFAULT_YES = "Yes";
var SWITCH_CONFIRM_DEFAULT_NO = "No";
var ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE = "Run this script?";
var ACTION_SCRIPT_FIELDS_OPTION = "script_fields";
var ALARM_PIN_ARM_OPTION = cardContractOptionName("pin_arm");
var ALARM_PIN_DISARM_OPTION = cardContractOptionName("pin_disarm");
var ALARM_ACTIONS_OPTION = cardContractOptionName("actions");
var ALARM_ICON_DISPLAY_OPTION = cardContractOptionName("icon_display");
var ALARM_LABEL_DISPLAY_OPTION = cardContractOptionName("label_display");
var GARAGE_LABEL_DISPLAY_OPTION = cardContractOptionName("label_display");
var GATE_LABEL_DISPLAY_OPTION = cardContractOptionName("label_display");
var CLIMATE_LABEL_DISPLAY_OPTION = cardContractOptionName("label_display");
var CLIMATE_NUMBER_DISPLAY_OPTION = cardContractOptionName("number_display");
var CLIMATE_TEMPERATURE_STEP_OPTION = cardContractOptionName("temperature_step");
var MEDIA_VOLUME_MAX_OPTION = cardContractOptionName("volume_max");
var MEDIA_LABEL_DISPLAY_OPTION = cardContractOptionName("label_display");
var MEDIA_NUMBER_DISPLAY_OPTION = cardContractOptionName("number_display");
var MEDIA_PLAYLIST_CONTENT_ID_OPTION = cardContractOptionName("playlist_content_id");
var MEDIA_PLAYLIST_CONTENT_TYPE_OPTION = cardContractOptionName("playlist_content_type");
var MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION = cardContractOptionName("playlist_player_source");
var SUBPAGE_KIND_OPTION = cardContractOptionName("subpage_kind");
var IMAGE_LABEL_OPTION = cardContractOptionName("image_label");
var IMAGE_ICON_OPTION = cardContractOptionName("image_icon");
var IMAGE_MODAL_MODE_OPTION = cardContractOptionName("image_modal_mode");
var IMAGE_REFRESH_OPTION = cardContractOptionName("image_refresh");
var IMAGE_REFRESH_MODE_OPTION = cardContractOptionName("image_refresh_mode");
var LIGHT_CONTROL_TABS_OPTION = cardContractOptionName("light_tabs");
var COVER_CONTROL_TABS_OPTION = cardContractOptionName("cover_tabs");
var CLIMATE_CONTROL_TABS_OPTION = cardContractOptionName("climate_tabs");
var FAN_CONTROL_TABS_OPTION = cardContractOptionName("fan_tabs");
var IMAGE_CARD_LIMIT = Math.max(0, parseInt(CFG && CFG.imageCardLimit != null ? CFG.imageCardLimit : 4, 10) || 0);
function alarmBehaviorSpec() {
  var card = cardContractCard("alarm");
  return card && card.behavior && card.behavior.alarm || {};
}

function alarmActionSpecs() {
  var actions = alarmBehaviorSpec().actions;
  return actions && actions.length ? actions : [];
}

function alarmDefaultActions() {
  var actions = alarmBehaviorSpec().defaultActions;
  return actions && actions.length ? actions.slice() : [];
}

function alarmMaxVisibleActions() {
  var max = parseInt(alarmBehaviorSpec().maxVisibleActions, 10);
  return isFinite(max) && max > 0 ? max : alarmDefaultActions().length;
}

function alarmActionLegacyIcon(value) {
  var info = alarmActionInfo(value);
  if (info && info.legacyIcon) return info.legacyIcon;
  return "";
}

function alarmActionIconIsGenerated(value, icon) {
  var info = alarmActionInfo(value);
  return !!info && (icon === info.icon || icon === alarmActionLegacyIcon(value));
}

function configOptionEnabled(options, name) {
  var parts = String(options || "").split(",");
  for (var i = 0; i < parts.length; i++) {
    if (parts[i] === name) return true;
  }
  return false;
}

function setConfigOption(options, name, enabled) {
  var parts = String(options || "").split(",");
  var out = [];
  var found = false;
  for (var i = 0; i < parts.length; i++) {
    var part = parts[i];
    if (!part) continue;
    if (part === name) {
      found = true;
      if (enabled) out.push(part);
    } else if (out.indexOf(part) < 0) {
      out.push(part);
    }
  }
  if (enabled && !found) out.push(name);
  return out.join(",");
}

function configOptionValue(options, name) {
  var prefix = name + "=";
  var parts = String(options || "").split(",");
  for (var i = 0; i < parts.length; i++) {
    var part = parts[i];
    if (part.indexOf(prefix) === 0) return decodeConfigField(part.substring(prefix.length));
  }
  return "";
}

function setConfigOptionValue(options, name, value) {
  var prefix = name + "=";
  var parts = String(options || "").split(",");
  var out = [];
  for (var i = 0; i < parts.length; i++) {
    var part = parts[i];
    if (!part || part.indexOf(prefix) === 0) continue;
    if (out.indexOf(part) < 0) out.push(part);
  }
  value = String(value || "").trim();
  if (value) out.push(prefix + encodeConfigField(value));
  return out.join(",");
}

function largeNumbersExplicitlyDisabled(options) {
  return configOptionValue(options, SENSOR_LARGE_NUMBERS_OPTION) === SENSOR_LARGE_NUMBERS_OFF_VALUE;
}

function copyLargeNumbersOption(out, options) {
  if (largeNumbersExplicitlyDisabled(options)) {
    return setConfigOptionValue(out, SENSOR_LARGE_NUMBERS_OPTION, SENSOR_LARGE_NUMBERS_OFF_VALUE);
  }
  if (configOptionEnabled(options, SENSOR_LARGE_NUMBERS_OPTION)) {
    return setConfigOption(out, SENSOR_LARGE_NUMBERS_OPTION, true);
  }
  return out;
}

function normalizeMediaVolumeMax(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("media", MEDIA_VOLUME_MAX_OPTION) || {};
  var fallback = cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100");
  if (!value) return fallback;
  var parsed = parseInt(value, 10);
  if (!isFinite(parsed)) return fallback;
  if (typeof spec.min === "number" && parsed < spec.min) parsed = spec.min;
  if (typeof spec.max === "number" && parsed > spec.max) parsed = spec.max;
  return String(parsed);
}

function normalizeMediaOptions(options, mode) {
  mode = mediaEditorMode(mode);
  if (mode === "control_modal") {
    var controlOut = "";
    var labelMode = normalizeMediaLabelDisplayMode(
      configOptionValue(options, MEDIA_LABEL_DISPLAY_OPTION));
    var numberMode = normalizeMediaNumberDisplayMode(
      configOptionValue(options, MEDIA_NUMBER_DISPLAY_OPTION));
    if (labelMode !== "status") {
      controlOut = setConfigOptionValue(controlOut, MEDIA_LABEL_DISPLAY_OPTION, labelMode);
    }
    if (numberMode !== "icon") {
      controlOut = setConfigOptionValue(controlOut, MEDIA_NUMBER_DISPLAY_OPTION, numberMode);
    }
    var controlMaxVolume = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
    if (controlMaxVolume !== cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100")) {
      controlOut = setConfigOptionValue(controlOut, MEDIA_VOLUME_MAX_OPTION, controlMaxVolume);
    }
    return controlOut;
  }
  if (mode === "playlist") {
    var playlistOut = "";
    var contentId = configOptionValue(options, MEDIA_PLAYLIST_CONTENT_ID_OPTION);
    if (contentId) playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_CONTENT_ID_OPTION, contentId);
    var defaultType = cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
    var contentType = configOptionValue(options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION) || defaultType;
    if (contentType !== defaultType) {
      playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, contentType);
    }
    var playerSource = configOptionValue(options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION);
    if (playerSource) playlistOut = setConfigOptionValue(playlistOut, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION, playerSource);
    return playlistOut;
  }
  if (mode !== "volume" && mode !== "position") return "";
  var out = "";
  var maxVolume = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
  if (mode === "volume" && maxVolume !== cardContractOptionDefaultValue("media", MEDIA_VOLUME_MAX_OPTION, "100")) {
    out = setConfigOptionValue(out, MEDIA_VOLUME_MAX_OPTION, maxVolume);
  }
  out = copyLargeNumbersOption(out, options);
  return out;
}

function normalizeMediaLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("media", MEDIA_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["label", "status"];
  var fallback = cardContractOptionDefaultValue("media", MEDIA_LABEL_DISPLAY_OPTION, "status");
  return values.indexOf(value) >= 0 ? value : fallback;
}

function normalizeMediaNumberDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("media", MEDIA_NUMBER_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["icon", "volume"];
  return values.indexOf(value) >= 0 ? value : "icon";
}

function imageRefreshIntervalValues() {
  var spec = cardContractOptionSpec("image", IMAGE_REFRESH_OPTION);
  return spec && spec.values ? spec.values.slice() : ["off", "10", "30", "60", "300"];
}

function imageRefreshModeValues() {
  var spec = cardContractOptionSpec("image", IMAGE_REFRESH_MODE_OPTION);
  return spec && spec.values ? spec.values.slice() : ["changes_timer", "timer"];
}

function imageModalModeValues() {
  var spec = cardContractOptionSpec("image", IMAGE_MODAL_MODE_OPTION);
  return spec && spec.values ? spec.values.slice() : [];
}

function normalizeImageRefreshInterval(value) {
  value = String(value || "").trim();
  return imageRefreshIntervalValues().indexOf(value) >= 0 ? value : "off";
}

function normalizeImageRefreshMode(value) {
  value = String(value || "").trim();
  return imageRefreshModeValues().indexOf(value) >= 0 ? value : "changes_timer";
}

function normalizeImageModalMode(value) {
  value = String(value || "").trim();
  var fallback = cardContractOptionDefaultValue("image", IMAGE_MODAL_MODE_OPTION, "fill");
  return imageModalModeValues().indexOf(value) >= 0 ? value : fallback;
}

function imageRefreshInterval(b) {
  return normalizeImageRefreshInterval(configOptionValue(b && b.options, IMAGE_REFRESH_OPTION));
}

function imageRefreshMode(b) {
  return normalizeImageRefreshMode(configOptionValue(b && b.options, IMAGE_REFRESH_MODE_OPTION));
}

function imageCardLimit() {
  return IMAGE_CARD_LIMIT;
}

function imageCardLimitMessage() {
  if (IMAGE_CARD_LIMIT <= 0) return "Image cards are not available on this display.";
  return "Image cards use shared firmware download slots. You can save up to " +
    IMAGE_CARD_LIMIT + " image cards total across the main page and subpages.";
}

function isImageCard(button) {
  return !!button && button.type === "image";
}

function activeGridSlots(grid) {
  var slots = [];
  var seen = {};
  (grid || []).forEach(function (slot) {
    if (slot <= 0 || seen[slot]) return;
    seen[slot] = true;
    slots.push(slot);
  });
  return slots;
}

function imageCardCountInButtons(buttons, grid) {
  var count = 0;
  var slots = activeGridSlots(grid);
  if (!slots.length && buttons && buttons.length) {
    for (var fallbackSlot = 1; fallbackSlot <= buttons.length; fallbackSlot++) {
      slots.push(fallbackSlot);
    }
  }
  slots.forEach(function (slot) {
    if (isImageCard(buttons && buttons[slot - 1])) count++;
  });
  return count;
}

function imageCardCountInSubpage(sp) {
  return imageCardCountInButtons(sp && sp.buttons, sp && sp.grid);
}

function imageCardCountInClipboardEntry(entry) {
  var count = isImageCard(entry) ? 1 : 0;
  if (entry && entry.subpageConfig) {
    count += imageCardCountInSubpage(parseSubpageConfig(entry.subpageConfig));
  }
  return count;
}

function imageCardCountInClipboardEntries(entries) {
  var count = 0;
  (entries || []).forEach(function (entry) {
    count += imageCardCountInClipboardEntry(entry);
  });
  return count;
}

function imageCardCountWithCandidate(candidate) {
  var count = 0;
  var matchedCandidate = false;

  activeGridSlots(state.grid).forEach(function (slot) {
    var button = state.buttons[slot - 1];
    if (candidate && !candidate.isSub && candidate.slot === slot) {
      button = candidate.button;
      matchedCandidate = true;
    }
    if (isImageCard(button)) count++;
  });

  for (var homeSlot in state.subpages) {
    var sp = state.subpages[homeSlot];
    activeGridSlots(sp && sp.grid).forEach(function (slot) {
      var button = sp && sp.buttons && sp.buttons[slot - 1];
      if (candidate && candidate.isSub &&
          String(candidate.homeSlot) === String(homeSlot) &&
          candidate.slot === slot) {
        button = candidate.button;
        matchedCandidate = true;
      }
      if (isImageCard(button)) count++;
    });
  }

  if (candidate && !matchedCandidate && isImageCard(candidate.button)) count++;
  return count;
}

function canAddImageCards(extraCount) {
  extraCount = parseInt(extraCount || 0, 10);
  if (!isFinite(extraCount) || extraCount <= 0) return true;
  return imageCardCountWithCandidate() + extraCount <= IMAGE_CARD_LIMIT;
}

function showImageCardLimitBanner() {
  showBanner(imageCardLimitMessage(), "error");
}

function imageModalMode(b) {
  return normalizeImageModalMode(configOptionValue(b && b.options, IMAGE_MODAL_MODE_OPTION));
}

function imageLabelEnabled(b) {
  return !!(b && configOptionEnabled(b.options, IMAGE_LABEL_OPTION));
}

function imageIconEnabled(b) {
  return !!(b && configOptionEnabled(b.options, IMAGE_ICON_OPTION));
}

function normalizeImageOptions(options) {
  var out = "";
  if (configOptionEnabled(options, IMAGE_LABEL_OPTION)) {
    out = setConfigOption(out, IMAGE_LABEL_OPTION, true);
  }
  if (configOptionEnabled(options, IMAGE_ICON_OPTION)) {
    out = setConfigOption(out, IMAGE_ICON_OPTION, true);
  }
  var modalMode = normalizeImageModalMode(configOptionValue(options, IMAGE_MODAL_MODE_OPTION));
  if (modalMode !== cardContractOptionDefaultValue("image", IMAGE_MODAL_MODE_OPTION, "fill")) {
    out = setConfigOptionValue(out, IMAGE_MODAL_MODE_OPTION, modalMode);
  }
  return out;
}

function setImageLabelEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, IMAGE_LABEL_OPTION, !!enabled);
  if (!enabled) b.label = "";
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageIconEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, IMAGE_ICON_OPTION, !!enabled);
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageModalMode(b, value) {
  if (!b) return "";
  var mode = normalizeImageModalMode(value);
  b.options = setConfigOptionValue(b.options, IMAGE_MODAL_MODE_OPTION, mode === "fill" ? "" : mode);
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageRefreshInterval(b, value) {
  if (!b) return "";
  var interval = normalizeImageRefreshInterval(value);
  b.options = setConfigOptionValue(b.options, IMAGE_REFRESH_OPTION, interval === "off" ? "" : interval);
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageRefreshMode(b, value) {
  if (!b) return "";
  var mode = normalizeImageRefreshMode(value);
  b.options = setConfigOptionValue(
    b.options,
    IMAGE_REFRESH_MODE_OPTION,
    mode === "changes_timer" ? "" : mode
  );
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function lightControlTabDefinitions() {
  var labels = {
    power: "Power",
    brightness: "Brightness",
    temperature: "Colour Temperature",
    color: "Colour Presets",
  };
  var spec = cardContractOptionSpec("light_control", LIGHT_CONTROL_TABS_OPTION);
  var values = spec && spec.values ? spec.values : [];
  return values.map(function (value) {
    return { value: value, label: labels[value] || value };
  });
}

function lightControlDefaultTabs() {
  return cardContractOptionDefaultValue(
    "light_control",
    LIGHT_CONTROL_TABS_OPTION,
    "power|brightness|temperature|color"
  ).split("|");
}

function normalizeLightControlTabs(value) {
  var raw = String(value || "").trim();
  var parts = raw ? raw.split("|") : lightControlDefaultTabs();
  var definitions = lightControlTabDefinitions();
  var valid = {};
  definitions.forEach(function (tab) { valid[tab.value] = true; });
  var out = [];
  parts.forEach(function (part) {
    part = String(part || "").trim();
    if (valid[part] && out.indexOf(part) < 0) out.push(part);
  });
  return out.length ? out : ["power"];
}

function lightControlTabs(b) {
  return normalizeLightControlTabs(configOptionValue(b && b.options, LIGHT_CONTROL_TABS_OPTION));
}

function lightControlTabsAreDefault(tabs) {
  tabs = normalizeLightControlTabs((tabs || []).join("|"));
  var defaults = lightControlDefaultTabs();
  if (tabs.length !== defaults.length) return false;
  for (var i = 0; i < defaults.length; i++) {
    if (tabs[i] !== defaults[i]) return false;
  }
  return true;
}

function normalizeLightControlOptions(options) {
  var tabs = normalizeLightControlTabs(configOptionValue(options, LIGHT_CONTROL_TABS_OPTION));
  return lightControlTabsAreDefault(tabs)
    ? ""
    : setConfigOptionValue("", LIGHT_CONTROL_TABS_OPTION, tabs.join("|"));
}

function setLightControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeLightControlTabs((tabs || []).join("|"));
  b.options = lightControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, LIGHT_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, LIGHT_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeLightControlOptions(b.options);
  return b.options;
}

function coverControlTabDefinitions() {
  var labels = {
    position: "Position",
    controls: "Controls",
    tilt: "Tilt",
  };
  var spec = cardContractOptionSpec("cover", COVER_CONTROL_TABS_OPTION);
  var values = spec && spec.values ? spec.values : [];
  return values.map(function (value) {
    return { value: value, label: labels[value] || value };
  });
}

function coverControlDefaultTabs() {
  return cardContractOptionDefaultValue(
    "cover",
    COVER_CONTROL_TABS_OPTION,
    "position|controls|tilt"
  ).split("|");
}

function normalizeTabList(value, definitions, defaults, fallback) {
  var raw = String(value || "").trim();
  var parts = raw ? raw.split("|") : defaults;
  var valid = {};
  definitions.forEach(function (tab) { valid[tab.value] = true; });
  var out = [];
  parts.forEach(function (part) {
    part = String(part || "").trim();
    if (valid[part] && out.indexOf(part) < 0) out.push(part);
  });
  return out.length ? out : [fallback];
}

function tabListIsDefault(tabs, defaults) {
  tabs = tabs || [];
  if (tabs.length !== defaults.length) return false;
  for (var i = 0; i < defaults.length; i++) {
    if (tabs[i] !== defaults[i]) return false;
  }
  return true;
}

function normalizeCoverControlTabs(value) {
  return normalizeTabList(
    value,
    coverControlTabDefinitions(),
    coverControlDefaultTabs(),
    "position"
  );
}

function coverControlTabs(b) {
  return normalizeCoverControlTabs(configOptionValue(b && b.options, COVER_CONTROL_TABS_OPTION));
}

function coverControlTabsAreDefault(tabs) {
  return tabListIsDefault(
    normalizeCoverControlTabs((tabs || []).join("|")),
    coverControlDefaultTabs()
  );
}

function normalizeCoverOptions(options) {
  var tabs = normalizeCoverControlTabs(configOptionValue(options, COVER_CONTROL_TABS_OPTION));
  return coverControlTabsAreDefault(tabs)
    ? ""
    : setConfigOptionValue("", COVER_CONTROL_TABS_OPTION, tabs.join("|"));
}

function normalizeCoverOptionsForMode(options, mode) {
  return normalizeCoverMode(mode, true) === "modal" ? normalizeCoverOptions(options) : "";
}

function setCoverControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeCoverControlTabs((tabs || []).join("|"));
  b.options = coverControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, COVER_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, COVER_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeCoverOptions(b.options);
  return b.options;
}

function climateControlTabDefinitions() {
  return [
    { value: "temperature", label: "Temperature" },
    { value: "mode", label: "Mode" },
    { value: "preset", label: "Preset" },
    { value: "fan", label: "Fan" },
    { value: "swing", label: "Swing" },
  ];
}

function climateControlDefaultTabs() {
  return climateControlTabDefinitions().map(function (tab) { return tab.value; });
}

function normalizeClimateControlTabs(value) {
  return normalizeTabList(
    value,
    climateControlTabDefinitions(),
    climateControlDefaultTabs(),
    "temperature"
  );
}

function climateControlTabs(b) {
  return normalizeClimateControlTabs(configOptionValue(b && b.options, CLIMATE_CONTROL_TABS_OPTION));
}

function climateControlTabsAreDefault(tabs) {
  return tabListIsDefault(
    normalizeClimateControlTabs((tabs || []).join("|")),
    climateControlDefaultTabs()
  );
}

function setClimateControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeClimateControlTabs((tabs || []).join("|"));
  b.options = climateControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, CLIMATE_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, CLIMATE_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeClimateOptions(b.options, true);
  return b.options;
}

function fanControlTabDefinitions() {
  return [
    { value: "power", label: "Power" },
    { value: "speed", label: "Speed" },
    { value: "preset", label: "Preset" },
    { value: "oscillation", label: "Oscillation" },
    { value: "direction", label: "Direction" },
  ];
}

function fanControlDefaultTabs() {
  return fanControlTabDefinitions().map(function (tab) { return tab.value; });
}

function normalizeFanControlTabs(value) {
  return normalizeTabList(
    value,
    fanControlTabDefinitions(),
    fanControlDefaultTabs(),
    "power"
  );
}

function fanControlTabs(b) {
  return normalizeFanControlTabs(configOptionValue(b && b.options, FAN_CONTROL_TABS_OPTION));
}

function fanControlTabsAreDefault(tabs) {
  return tabListIsDefault(
    normalizeFanControlTabs((tabs || []).join("|")),
    fanControlDefaultTabs()
  );
}

function normalizeFanControlOptions(options) {
  var tabs = normalizeFanControlTabs(configOptionValue(options, FAN_CONTROL_TABS_OPTION));
  return fanControlTabsAreDefault(tabs)
    ? ""
    : setConfigOptionValue("", FAN_CONTROL_TABS_OPTION, tabs.join("|"));
}

function setFanControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeFanControlTabs((tabs || []).join("|"));
  b.options = fanControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, FAN_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, FAN_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeFanControlOptions(b.options);
  return b.options;
}

function renderModalTabSettings(panel, b, helpers, config) {
  var section = document.createElement("div");
  panel.appendChild(section);

  b.options = config.normalizeOptions(b.options);
  var tabs = config.tabs(b);
  var definitions = config.definitions();
  var definitionByValue = {};
  definitions.forEach(function (definition) {
    definitionByValue[definition.value] = definition;
  });
  var orderedDefinitions = [];
  tabs.forEach(function (tab) {
    if (definitionByValue[tab]) orderedDefinitions.push(definitionByValue[tab]);
  });
  definitions.forEach(function (definition) {
    if (tabs.indexOf(definition.value) < 0) orderedDefinitions.push(definition);
  });

  if (!config.hideHeading) {
    var heading = document.createElement("div");
    heading.className = "sp-field";
    heading.appendChild(helpers.fieldLabel("Modal Tabs"));
    section.appendChild(heading);
  }

  var list = document.createElement("div");
  list.className = "sp-light-tab-list";
  section.appendChild(list);

  function listRows() {
    return Array.prototype.slice.call(list.querySelectorAll(".sp-light-tab-row"));
  }

  function saveTabsFromRows() {
    var nextTabs = [];
    listRows().forEach(function (row) {
      var input = row.querySelector("input[type=checkbox]");
      if (input && input.checked) nextTabs.push(row.getAttribute("data-tab"));
    });
    if (!nextTabs.length) return false;
    saveTabs(nextTabs);
    return true;
  }

  function saveTabs(nextTabs) {
    config.setTabs(b, nextTabs);
    b._modalSettingsOpen = true;
    helpers.saveField("options", b.options);
    renderButtonSettings();
  }

  function moveRow(row, direction) {
    var sibling = direction < 0 ? row.previousElementSibling : row.nextElementSibling;
    if (!sibling) return;
    if (direction < 0) {
      list.insertBefore(row, sibling);
    } else {
      list.insertBefore(sibling, row);
    }
    saveTabsFromRows();
  }

  orderedDefinitions.forEach(function (definition) {
    var tabIndex = tabs.indexOf(definition.value);
    var visible = tabIndex >= 0;

    var row = document.createElement("div");
    row.className = "sp-light-tab-row";
    row.setAttribute("data-tab", definition.value);
    row.draggable = true;

    var controls = document.createElement("div");
    controls.className = "sp-light-tab-controls";

    var drag = document.createElement("button");
    drag.type = "button";
    drag.className = "sp-light-tab-drag mdi mdi-drag";
    drag.setAttribute("aria-label", "Drag " + definition.label);
    drag.tabIndex = -1;

    var moveUp = document.createElement("button");
    moveUp.type = "button";
    moveUp.className = "sp-light-tab-move mdi mdi-chevron-up";
    moveUp.setAttribute("aria-label", "Move " + definition.label + " up");

    var moveDown = document.createElement("button");
    moveDown.type = "button";
    moveDown.className = "sp-light-tab-move mdi mdi-chevron-down";
    moveDown.setAttribute("aria-label", "Move " + definition.label + " down");

    controls.appendChild(drag);
    controls.appendChild(moveUp);
    controls.appendChild(moveDown);
    row.appendChild(controls);

    var label = document.createElement("label");
    label.className = "sp-light-tab-label";
    label.htmlFor = helpers.idPrefix + config.idPrefix + definition.value;
    label.textContent = definition.label;
    row.appendChild(label);

    var toggle = document.createElement("label");
    toggle.className = "sp-toggle";
    var input = document.createElement("input");
    input.type = "checkbox";
    input.id = helpers.idPrefix + config.idPrefix + definition.value;
    input.checked = visible;
    var track = document.createElement("span");
    track.className = "sp-toggle-track";
    toggle.appendChild(input);
    toggle.appendChild(track);
    row.appendChild(toggle);

    input.addEventListener("change", function () {
      if (!this.checked) {
        var visibleCount = listRows().filter(function (item) {
          var itemInput = item.querySelector("input[type=checkbox]");
          return itemInput && itemInput.checked;
        }).length;
        if (visibleCount < 1) {
          this.checked = true;
          return;
        }
      }
      saveTabsFromRows();
    });

    moveUp.addEventListener("click", function () { moveRow(row, -1); });
    moveDown.addEventListener("click", function () { moveRow(row, 1); });

    row.addEventListener("dragstart", function (event) {
      row.classList.add("sp-dragging");
      event.dataTransfer.effectAllowed = "move";
      event.dataTransfer.setData("text/plain", definition.value);
    });
    row.addEventListener("dragend", function () {
      row.classList.remove("sp-dragging");
    });
    row.addEventListener("dragover", function (event) {
      var dragging = list.querySelector(".sp-dragging");
      if (!dragging || dragging === row) return;
      event.preventDefault();
      var rect = row.getBoundingClientRect();
      var after = event.clientY > rect.top + rect.height / 2;
      list.insertBefore(dragging, after ? row.nextSibling : row);
    });
    row.addEventListener("drop", function (event) {
      event.preventDefault();
      saveTabsFromRows();
    });

    list.appendChild(row);
  });

  return section;
}

function normalizeSubpageKind(value) {
  value = String(value || "").trim();
  return subpagePresetDefaults(value) ? value : "";
}

function subpageKind(b) {
  return normalizeSubpageKind(configOptionValue(b && b.options, SUBPAGE_KIND_OPTION));
}

var SUBPAGE_KIND_PRESET_DEFINITIONS = [
  { value: "", label: "Generic" },
  { value: "switch", label: "Switch", preset: { label: "Switch", icon: "Power Plug", entityDomains: ["light", "switch", "input_boolean", "fan"], placeholder: "e.g. switch.living_room" } },
  { value: "lights", label: "Lights", preset: { label: "Lighting", icon: "Lightbulb", entityDomains: ["light"], placeholder: "e.g. light.living_room" } },
  { value: "climate", label: "Climate", preset: { label: "Climate", icon: "Thermostat", entityDomains: ["climate"], placeholder: "e.g. climate.living_room" } },
  { value: "presence", label: "Presence", preset: { label: "Presence", icon: "Account", entityDomains: ["person", "device_tracker", "binary_sensor", "input_boolean"], placeholder: "e.g. person.jane" } },
  { value: "media", label: "Media", preset: { label: "Media", icon: "Speaker", entityDomains: ["media_player"], placeholder: "e.g. media_player.living_room" } },
  { value: "alarm", label: "Alarm", preset: { label: "Alarm", icon: "Security", entityDomains: ["alarm_control_panel"], placeholder: "e.g. alarm_control_panel.home" } },
  { value: "cover", label: "Cover", preset: { label: "Cover", icon: "Blinds", entityDomains: ["cover"], placeholder: "e.g. cover.office_blind" } },
  { value: "garage", label: "Garage Door", preset: { label: "Garage", icon: "Garage", entityDomains: ["cover"], placeholder: "e.g. cover.garage_door" } },
  { value: "gate", label: "Gate", preset: { label: "Gate", icon: "Gate", entityDomains: ["cover"], placeholder: "e.g. cover.driveway_gate" } },
  { value: "lock", label: "Lock", preset: { label: "Lock", icon: "Lock", entityDomains: ["lock"], placeholder: "e.g. lock.front_door" } },
  { value: "vacuum", label: "Vacuum", preset: { label: "Vacuum", icon: "Robot Vacuum", entityDomains: ["vacuum"], placeholder: "e.g. vacuum.downstairs" } },
  { value: "lawn_mower", label: "Lawn Mower", preset: { label: "Lawn Mower", icon: "Robot Mower", entityDomains: ["lawn_mower"], placeholder: "e.g. lawn_mower.backyard" } },
  { value: "weather", label: "Weather", preset: { label: "Weather", icon: "Weather Partly Cloudy", entityDomains: ["weather"], placeholder: "e.g. weather.home" } },
  { value: "sensor", label: "Sensor", preset: { label: "Sensor", icon: "Gauge", entityDomains: ["sensor", "binary_sensor", "text_sensor"], placeholder: "e.g. sensor.open_windows" } },
  { value: "image", label: "Camera/Image", preset: { label: "Camera", icon: "Camera", entityDomains: ["camera", "image"], placeholder: "e.g. camera.front_door" } },
];

function subpageKindOptions() {
  return SUBPAGE_KIND_PRESET_DEFINITIONS.map(function (definition) {
    return [definition.value, definition.label];
  });
}

function subpagePresetDefaults(kind) {
  kind = String(kind || "").trim();
  for (var i = 0; i < SUBPAGE_KIND_PRESET_DEFINITIONS.length; i++) {
    var definition = SUBPAGE_KIND_PRESET_DEFINITIONS[i];
    if (definition.value === kind) return definition.preset || null;
  }
  return null;
}

function applySubpagePresetConfig(b, forceDisplayDefaults) {
  if (!b) return;
  var defaults = subpagePresetDefaults(subpageKind(b));
  if (!defaults) return;
  if (forceDisplayDefaults || !b.label) b.label = defaults.label;
  if (forceDisplayDefaults || !b.icon || b.icon === "Auto") b.icon = defaults.icon;
  b.icon_on = "Auto";
  b.sensor = "indicator";
  b.unit = "";
  b.precision = "";
}

function normalizeSubpageOptions(options, sensor, precision) {
  var out = "";
  var kind = normalizeSubpageKind(configOptionValue(options, SUBPAGE_KIND_OPTION));
  if (kind) out = setConfigOptionValue(out, SUBPAGE_KIND_OPTION, kind);
  if (sensor && sensor !== "indicator" && precision !== "text" &&
      (configOptionEnabled(options, SENSOR_LARGE_NUMBERS_OPTION) || largeNumbersExplicitlyDisabled(options))) {
    out = copyLargeNumbersOption(out, options);
  }
  return out;
}

function mediaVolumeMax(b) {
  return normalizeMediaVolumeMax(configOptionValue(b && b.options, MEDIA_VOLUME_MAX_OPTION));
}

function setMediaVolumeMax(b, value) {
  if (!b) return "";
  var normalized = normalizeMediaVolumeMax(value);
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_VOLUME_MAX_OPTION,
    normalized === "100" ? "" : normalized
  );
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaLabelDisplayMode(b) {
  return normalizeMediaLabelDisplayMode(
    configOptionValue(b && b.options, MEDIA_LABEL_DISPLAY_OPTION));
}

function setMediaLabelDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeMediaLabelDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_LABEL_DISPLAY_OPTION,
    normalized === "status" ? "" : normalized
  );
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaNumberDisplayMode(b) {
  return normalizeMediaNumberDisplayMode(
    configOptionValue(b && b.options, MEDIA_NUMBER_DISPLAY_OPTION));
}

function setMediaNumberDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeMediaNumberDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_NUMBER_DISPLAY_OPTION,
    normalized === "icon" ? "" : normalized
  );
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaPlaylistContentId(b) {
  return configOptionValue(b && b.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION);
}

function mediaPlaylistContentType(b) {
  return configOptionValue(b && b.options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION) ||
    cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
}

function setMediaPlaylistContentId(b, value) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION, value || "");
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function setMediaPlaylistContentType(b, value) {
  if (!b) return "";
  var defaultType = cardContractOptionDefaultValue("media", MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, "playlist");
  value = String(value || "").trim() || defaultType;
  b.options = setConfigOptionValue(
    b.options,
    MEDIA_PLAYLIST_CONTENT_TYPE_OPTION,
    value === defaultType ? "" : value);
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function mediaPlaylistPlayerSource(b) {
  return configOptionValue(b && b.options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION);
}

function setMediaPlaylistPlayerSource(b, value) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION, value || "");
  b.options = normalizeMediaOptions(b.options, b.sensor);
  return b.options;
}

function cardContractOptionSpec(type, name) {
  var options = cardContractOptions(type);
  for (var i = 0; i < options.length; i++) {
    if (options[i].name === name) return options[i];
  }
  return null;
}

function cardContractOptionSupportedFor(type, name, context) {
  var spec = cardContractOptionSpec(type, name);
  if (!spec) return false;
  var rule = spec.supportedWhen || {};
  if (rule.never) return false;
  context = context || {};
  var precision = context.precision || "";
  if (rule.precision && rule.precision.indexOf(precision) < 0) return false;
  if (rule.precisionNot && rule.precisionNot.indexOf(precision) >= 0) return false;
  return true;
}

function cardContractOptionDefaultValue(type, name, fallback) {
  var spec = cardContractOptionSpec(type, name);
  return spec && typeof spec.defaultValue === "string" ? spec.defaultValue : fallback;
}

function switchConfirmationModeStorage() {
  var spec = cardContractOptionSpec("", "confirmation_mode");
  return spec && spec.storage && spec.storage.length >= 2
    ? spec.storage
    : [SWITCH_CONFIRM_OFF_OPTION, SWITCH_CONFIRM_ON_OPTION];
}

function normalizeCardOnPattern(value) {
  value = String(value || "").trim();
  return value === "stripes" ? "stripes" : "";
}

function cardOnPattern(b) {
  return normalizeCardOnPattern(configOptionValue(b && b.options, CARD_ON_PATTERN_OPTION));
}

function setCardOnPattern(b, pattern) {
  if (!b) return "";
  b.options = setConfigOptionValue(
    b.options,
    CARD_ON_PATTERN_OPTION,
    normalizeCardOnPattern(pattern)
  );
  if (!b.type) b.options = normalizeSwitchConfirmationOptions(b.options);
  return b.options;
}

function cardLargeNumbersSupported(b) {
  if (!b) return false;
  if (typeof BUTTON_TYPES !== "undefined") {
    var typeDef = BUTTON_TYPES[b.type || ""];
    var large = typeDef && typeDef.cardMetadata && typeDef.cardMetadata.largeNumbers;
    if (large) {
      return typeof large.supported === "function" ? !!large.supported(b) : large.supported !== false;
    }
  }
  return cardContractLargeNumbersSupported(b.type, b.precision);
}

function cardLargeNumbersEnabled(b) {
  return !!(b && cardLargeNumbersSupported(b) &&
    configOptionEnabled(b.options, SENSOR_LARGE_NUMBERS_OPTION));
}

function sensorLargeNumbersEnabled(b) {
  return cardLargeNumbersEnabled(b);
}

function setSensorLargeNumbersEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, SENSOR_LARGE_NUMBERS_OPTION, false);
  b.options = setConfigOptionValue(
    b.options,
    SENSOR_LARGE_NUMBERS_OPTION,
    enabled ? "" : SENSOR_LARGE_NUMBERS_OFF_VALUE
  );
  if (enabled) b.options = setConfigOption(b.options, SENSOR_LARGE_NUMBERS_OPTION, true);
  return b.options;
}

function sensorActiveColorEnabled(b) {
  return !!(b && b.type === "sensor" &&
    configOptionEnabled(b.options, SENSOR_ACTIVE_COLOR_OPTION));
}

function setSensorActiveColorEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, SENSOR_ACTIVE_COLOR_OPTION, enabled);
  return b.options;
}

function sensorStateLabelsEnabled(b) {
  return !!(b && b.type === "sensor" && b.precision === "text" &&
    configOptionEnabled(b.options, SENSOR_STATE_LABELS_OPTION));
}

function legacySensorStateInput(options) {
  if (configOptionValue(options, SENSOR_STATE_HIGH_LABEL_OPTION)) return "high";
  if (configOptionValue(options, SENSOR_STATE_LOW_LABEL_OPTION)) return "low";
  return "";
}

function legacySensorStateOutput(options) {
  if (configOptionValue(options, SENSOR_STATE_HIGH_LABEL_OPTION)) {
    return configOptionValue(options, SENSOR_STATE_HIGH_LABEL_OPTION);
  }
  if (configOptionValue(options, SENSOR_STATE_LOW_LABEL_OPTION)) {
    return configOptionValue(options, SENSOR_STATE_LOW_LABEL_OPTION);
  }
  return "";
}

function sensorStateInput(b) {
  return sensorStateLabelsEnabled(b)
    ? (configOptionValue(b.options, SENSOR_STATE_INPUT_OPTION) || legacySensorStateInput(b.options))
    : "";
}

function sensorStateOutput(b) {
  return sensorStateLabelsEnabled(b)
    ? (configOptionValue(b.options, SENSOR_STATE_OUTPUT_OPTION) || legacySensorStateOutput(b.options))
    : "";
}

function sensorStateInput2(b) {
  return sensorStateLabelsEnabled(b)
    ? configOptionValue(b.options, SENSOR_STATE_INPUT_2_OPTION)
    : "";
}

function sensorStateOutput2(b) {
  return sensorStateLabelsEnabled(b)
    ? configOptionValue(b.options, SENSOR_STATE_OUTPUT_2_OPTION)
    : "";
}

function setSensorStateTranslation(b, enabled, inputText, outputText) {
  return setSensorStateTranslations(b, enabled, inputText, outputText, "", "");
}

function setSensorStateTranslations(b, enabled, inputText, outputText, inputText2, outputText2) {
  if (!b) return "";
  var options = b.options || "";
  options = setConfigOption(options, SENSOR_STATE_LABELS_OPTION, enabled);
  options = setConfigOptionValue(options, SENSOR_STATE_INPUT_OPTION, enabled ? inputText : "");
  options = setConfigOptionValue(options, SENSOR_STATE_OUTPUT_OPTION, enabled ? outputText : "");
  options = setConfigOptionValue(options, SENSOR_STATE_INPUT_2_OPTION, enabled ? inputText2 : "");
  options = setConfigOptionValue(options, SENSOR_STATE_OUTPUT_2_OPTION, enabled ? outputText2 : "");
  options = setConfigOptionValue(options, SENSOR_STATE_LOW_LABEL_OPTION, "");
  options = setConfigOptionValue(options, SENSOR_STATE_HIGH_LABEL_OPTION, "");
  b.options = normalizeSensorOptions(options, b.precision);
  return b.options;
}

function normalizeSensorOptions(options, precision) {
  var out = "";
  if (configOptionEnabled(options, SENSOR_LARGE_NUMBERS_OPTION) &&
      cardContractOptionSupportedFor("sensor", SENSOR_LARGE_NUMBERS_OPTION, { precision: precision })) {
    out = copyLargeNumbersOption(out, options);
  } else if (largeNumbersExplicitlyDisabled(options) &&
      cardContractOptionSupportedFor("sensor", SENSOR_LARGE_NUMBERS_OPTION, { precision: precision })) {
    out = copyLargeNumbersOption(out, options);
  }
  if (configOptionEnabled(options, SENSOR_ACTIVE_COLOR_OPTION) &&
      cardContractOptionSupportedFor("sensor", SENSOR_ACTIVE_COLOR_OPTION, { precision: precision })) {
    out = setConfigOption(out, SENSOR_ACTIVE_COLOR_OPTION, true);
  }
  if (precision === "text" && configOptionEnabled(options, SENSOR_STATE_LABELS_OPTION)) {
    out = setConfigOption(out, SENSOR_STATE_LABELS_OPTION, true);
    out = setConfigOptionValue(out, SENSOR_STATE_INPUT_OPTION,
      configOptionValue(options, SENSOR_STATE_INPUT_OPTION) || legacySensorStateInput(options));
    out = setConfigOptionValue(out, SENSOR_STATE_OUTPUT_OPTION,
      configOptionValue(options, SENSOR_STATE_OUTPUT_OPTION) || legacySensorStateOutput(options));
    out = setConfigOptionValue(out, SENSOR_STATE_INPUT_2_OPTION,
      configOptionValue(options, SENSOR_STATE_INPUT_2_OPTION));
    out = setConfigOptionValue(out, SENSOR_STATE_OUTPUT_2_OPTION,
      configOptionValue(options, SENSOR_STATE_OUTPUT_2_OPTION));
  }
  return out;
}

function normalizeDateTimeOptions(type, options, precision) {
  if (configOptionEnabled(options, SENSOR_LARGE_NUMBERS_OPTION) &&
      cardContractOptionSupportedFor(type, SENSOR_LARGE_NUMBERS_OPTION, { precision: precision })) {
    return copyLargeNumbersOption("", options);
  }
  if (largeNumbersExplicitlyDisabled(options) &&
      cardContractOptionSupportedFor(type, SENSOR_LARGE_NUMBERS_OPTION, { precision: precision })) {
    return copyLargeNumbersOption("", options);
  }
  return "";
}

function normalizeDoorWindowSubtype(value) {
  value = String(value || "").trim();
  return value === "window" ? "window" : "door";
}

function doorWindowClosedIcon(subtype) {
  return normalizeDoorWindowSubtype(subtype) === "window" ? "Window Closed" : "Door";
}

function doorWindowOpenIcon(subtype) {
  return normalizeDoorWindowSubtype(subtype) === "window" ? "Window Open" : "Door Open";
}

function doorWindowActiveColorEnabled(b) {
  return !!(b && b.type === "door_window" &&
    configOptionEnabled(b.options, SENSOR_ACTIVE_COLOR_OPTION));
}

function setDoorWindowActiveColorEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, SENSOR_ACTIVE_COLOR_OPTION, enabled);
  return b.options;
}

function normalizeDoorWindowOptions(options) {
  var out = "";
  if (configOptionEnabled(options, SENSOR_ACTIVE_COLOR_OPTION)) {
    out = setConfigOption(out, SENSOR_ACTIVE_COLOR_OPTION, true);
  }
  return out;
}

function presenceActiveColorEnabled(b) {
  return !!(b && b.type === "presence" &&
    configOptionEnabled(b.options, SENSOR_ACTIVE_COLOR_OPTION));
}

function setPresenceActiveColorEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, SENSOR_ACTIVE_COLOR_OPTION, enabled);
  return b.options;
}

function normalizePresenceOptions(options) {
  var out = "";
  if (configOptionEnabled(options, SENSOR_ACTIVE_COLOR_OPTION)) {
    out = setConfigOption(out, SENSOR_ACTIVE_COLOR_OPTION, true);
  }
  return out;
}

function normalizeTodoCountDisplay(value) {
  value = String(value || "").trim();
  return value === "icon" ? "icon" : "count";
}

function normalizeTodoOptions(options) {
  var showCount = normalizeTodoCountDisplay(configOptionValue(options, "count_display")) === "count";
  var out = showCount ? "" : setConfigOptionValue("", "count_display", "icon");
  if (showCount) out = copyLargeNumbersOption(out, options);
  return out;
}

function switchConfirmationEnabled(b) {
  return !!switchConfirmationMode(b);
}

function switchConfirmationMode(b) {
  var options = b && b.options;
  var storage = switchConfirmationModeStorage();
  var confirmOff = configOptionEnabled(options, storage[0]);
  var confirmOn = configOptionEnabled(options, storage[1]);
  if (confirmOff && confirmOn) return "both";
  if (confirmOn) return "on";
  if (confirmOff) return "off";
  return "";
}

function switchConfirmationDefaultMessageForMode(mode) {
  var spec = cardContractOptionSpec("", SWITCH_CONFIRM_MESSAGE_OPTION);
  var defaults = spec && spec.defaultValueByMode || {};
  if (mode && defaults[mode]) return defaults[mode];
  return cardContractOptionDefaultValue("", SWITCH_CONFIRM_MESSAGE_OPTION, SWITCH_CONFIRM_DEFAULT_MESSAGE);
}

function switchConfirmationMessage(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_MESSAGE_OPTION) ||
    switchConfirmationDefaultMessageForMode(switchConfirmationMode(b));
}

function switchConfirmationYesText(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_YES_OPTION) ||
    cardContractOptionDefaultValue("", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES);
}

function switchConfirmationNoText(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_NO_OPTION) ||
    cardContractOptionDefaultValue("", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO);
}

function normalizeSwitchConfirmationOptions(options) {
  var mode = switchConfirmationMode({ options: options });
  var out = "";
  out = copyLargeNumbersOption(out, options);
  var onPattern = normalizeCardOnPattern(configOptionValue(options, CARD_ON_PATTERN_OPTION));
  if (onPattern) out = setConfigOptionValue(out, CARD_ON_PATTERN_OPTION, onPattern);
  if (!mode) return out;
  var storage = switchConfirmationModeStorage();
  out = setConfigOption(out, storage[0], mode === "off" || mode === "both");
  out = setConfigOption(out, storage[1], mode === "on" || mode === "both");
  var msg = configOptionValue(options, SWITCH_CONFIRM_MESSAGE_OPTION);
  var yes = configOptionValue(options, SWITCH_CONFIRM_YES_OPTION);
  var no = configOptionValue(options, SWITCH_CONFIRM_NO_OPTION);
  if (msg && msg !== switchConfirmationDefaultMessageForMode(mode)) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_MESSAGE_OPTION, msg);
  }
  if (yes && yes !== cardContractOptionDefaultValue("", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES)) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_YES_OPTION, yes);
  }
  if (no && no !== cardContractOptionDefaultValue("", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO)) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_NO_OPTION, no);
  }
  return out;
}

function setSwitchConfirmationOptions(b, mode, message, yesText, noText) {
  if (!b) return "";
  mode = mode === true ? "off" : mode;
  mode = mode === "on" || mode === "both" || mode === "off" ? mode : "";
  var out = "";
  out = copyLargeNumbersOption(out, b.options);
  var storage = switchConfirmationModeStorage();
  out = setConfigOption(out, storage[0], mode === "off" || mode === "both");
  out = setConfigOption(out, storage[1], mode === "on" || mode === "both");
  if (mode) {
    if (message && message !== switchConfirmationDefaultMessageForMode(mode)) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_MESSAGE_OPTION, message);
    }
    if (yesText && yesText !== cardContractOptionDefaultValue("", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES)) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_YES_OPTION, yesText);
    }
    if (noText && noText !== cardContractOptionDefaultValue("", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO)) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_NO_OPTION, noText);
    }
  }
  b.options = out;
  return b.options;
}

function actionCardIsScript(b) {
  var value = typeof b === "string" ? b : b && b.sensor;
  return value === "script.turn_on";
}

function actionScriptConfirmationEnabled(b) {
  return !!(b && actionCardIsScript(b) &&
    configOptionEnabled(b.options, SWITCH_CONFIRM_ON_OPTION));
}

function actionScriptConfirmationDefaultMessage() {
  return cardContractOptionDefaultValue("action", SWITCH_CONFIRM_MESSAGE_OPTION,
    ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE);
}

function actionScriptConfirmationMessage(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_MESSAGE_OPTION) ||
    actionScriptConfirmationDefaultMessage();
}

function actionScriptConfirmationYesText(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_YES_OPTION) ||
    cardContractOptionDefaultValue("action", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES);
}

function actionScriptConfirmationNoText(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_NO_OPTION) ||
    cardContractOptionDefaultValue("action", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO);
}

function actionScriptFields(b) {
  return actionCardIsScript(b) ? configOptionValue(b && b.options, ACTION_SCRIPT_FIELDS_OPTION) : "";
}

function copyActionCardStateOptions(out, options) {
  var stateEntity = configOptionValue(options, ACTION_CARD_STATE_ENTITY_OPTION);
  if (!stateEntity) return out;
  out = setConfigOptionValue(out, ACTION_CARD_STATE_ENTITY_OPTION, stateEntity);
  var rawPrecision = configOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION);
  if (rawPrecision === "icon" || rawPrecision === "text") {
    out = setConfigOptionValue(out, ACTION_CARD_STATE_PRECISION_OPTION, rawPrecision);
    return out;
  }
  var stateUnit = configOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION);
  if (!stateUnit && rawPrecision !== "0" && rawPrecision !== "1" && rawPrecision !== "2") {
    return out;
  }
  var statePrecision = rawPrecision === "1" || rawPrecision === "2" ? rawPrecision : "0";
  if (stateUnit) out = setConfigOptionValue(out, ACTION_CARD_STATE_UNIT_OPTION, stateUnit);
  if (rawPrecision === "0" || statePrecision !== "0") {
    out = setConfigOptionValue(out, ACTION_CARD_STATE_PRECISION_OPTION, statePrecision);
  }
  out = copyLargeNumbersOption(out, options);
  return out;
}

function normalizeActionOptions(options, action) {
  if (action === ACTION_CARD_LOCAL_ACTION) return "";
  var out = copyActionCardStateOptions("", options);
  if (action !== "script.turn_on") {
    return out;
  }
  var fields = configOptionValue(options, ACTION_SCRIPT_FIELDS_OPTION);
  if (fields) out = setConfigOptionValue(out, ACTION_SCRIPT_FIELDS_OPTION, fields);
  if (!configOptionEnabled(options, SWITCH_CONFIRM_ON_OPTION)) return out;
  out = setConfigOption(out, SWITCH_CONFIRM_ON_OPTION, true);
  var msg = configOptionValue(options, SWITCH_CONFIRM_MESSAGE_OPTION);
  var yes = configOptionValue(options, SWITCH_CONFIRM_YES_OPTION);
  var no = configOptionValue(options, SWITCH_CONFIRM_NO_OPTION);
  if (msg && msg !== actionScriptConfirmationDefaultMessage()) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_MESSAGE_OPTION, msg);
  }
  if (yes && yes !== cardContractOptionDefaultValue("action", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES)) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_YES_OPTION, yes);
  }
  if (no && no !== cardContractOptionDefaultValue("action", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO)) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_NO_OPTION, no);
  }
  return out;
}

function setActionScriptConfirmationOptions(b, enabled, message, yesText, noText) {
  if (!b) return "";
  var out = copyActionCardStateOptions("", b.options);
  var fields = actionScriptFields(b);
  if (fields) out = setConfigOptionValue(out, ACTION_SCRIPT_FIELDS_OPTION, fields);
  if (enabled && actionCardIsScript(b)) {
    out = setConfigOption(out, SWITCH_CONFIRM_ON_OPTION, true);
    if (message && message !== actionScriptConfirmationDefaultMessage()) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_MESSAGE_OPTION, message);
    }
    if (yesText && yesText !== cardContractOptionDefaultValue("action", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES)) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_YES_OPTION, yesText);
    }
    if (noText && noText !== cardContractOptionDefaultValue("action", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO)) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_NO_OPTION, noText);
    }
  }
  b.options = out;
  return b.options;
}

function setActionScriptFields(b, fields) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options, ACTION_SCRIPT_FIELDS_OPTION, fields || "");
  b.options = normalizeActionOptions(b.options, b.sensor);
  return b.options;
}

function normalizeGarageLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("garage", GARAGE_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : [];
  var fallback = cardContractOptionDefaultValue("garage", GARAGE_LABEL_DISPLAY_OPTION, "label");
  return values.indexOf(value) >= 0 ? value : fallback;
}

function normalizeGarageOptions(options, mode) {
  var labelMode = normalizeGarageLabelDisplayMode(
    configOptionValue(options, GARAGE_LABEL_DISPLAY_OPTION));
  return labelMode !== cardContractOptionDefaultValue("garage", GARAGE_LABEL_DISPLAY_OPTION, "label")
    ? setConfigOptionValue("", GARAGE_LABEL_DISPLAY_OPTION, labelMode)
    : "";
}

function garageLabelDisplayMode(b) {
  return normalizeGarageLabelDisplayMode(
    configOptionValue(b && b.options, GARAGE_LABEL_DISPLAY_OPTION));
}

function setGarageLabelDisplayMode(b, mode) {
  if (!b) return "";
  b.options = setConfigOptionValue(
    b.options,
    GARAGE_LABEL_DISPLAY_OPTION,
    normalizeGarageLabelDisplayMode(mode) === "status" ? "status" : ""
  );
  b.options = normalizeGarageOptions(b.options, b.sensor);
  return b.options;
}

function normalizeGateLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("gate", GATE_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : [];
  var fallback = cardContractOptionDefaultValue("gate", GATE_LABEL_DISPLAY_OPTION, "label");
  return values.indexOf(value) >= 0 ? value : fallback;
}

function normalizeGateOptions(options, mode) {
  var labelMode = normalizeGateLabelDisplayMode(
    configOptionValue(options, GATE_LABEL_DISPLAY_OPTION));
  return labelMode !== cardContractOptionDefaultValue("gate", GATE_LABEL_DISPLAY_OPTION, "label")
    ? setConfigOptionValue("", GATE_LABEL_DISPLAY_OPTION, labelMode)
    : "";
}

function gateLabelDisplayMode(b) {
  return normalizeGateLabelDisplayMode(
    configOptionValue(b && b.options, GATE_LABEL_DISPLAY_OPTION));
}

function setGateLabelDisplayMode(b, mode) {
  if (!b) return "";
  b.options = setConfigOptionValue(
    b.options,
    GATE_LABEL_DISPLAY_OPTION,
    normalizeGateLabelDisplayMode(mode) === "status" ? "status" : ""
  );
  b.options = normalizeGateOptions(b.options, b.sensor);
  return b.options;
}

function normalizeClimateLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("climate", CLIMATE_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : [];
  return values.indexOf(value) >= 0 ? value : climateDefaultLabelDisplayMode();
}

function normalizeClimateNumberDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("climate", CLIMATE_NUMBER_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : [];
  return values.indexOf(value) >= 0 ? value : climateDefaultNumberDisplayMode();
}

function normalizeClimateTemperatureStep(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("climate", CLIMATE_TEMPERATURE_STEP_OPTION);
  var values = spec && spec.values ? spec.values : [];
  return values.indexOf(value) >= 0 ? value : climateDefaultTemperatureStep();
}

function normalizeClimateOptions(options, includeControlTabs) {
  var labelMode = normalizeClimateLabelDisplayMode(
    configOptionValue(options, CLIMATE_LABEL_DISPLAY_OPTION));
  var numberMode = normalizeClimateNumberDisplayMode(
    configOptionValue(options, CLIMATE_NUMBER_DISPLAY_OPTION));
  var temperatureStep = normalizeClimateTemperatureStep(
    configOptionValue(options, CLIMATE_TEMPERATURE_STEP_OPTION));
  var out = "";
  if (labelMode !== climateDefaultLabelDisplayMode()) {
    out = setConfigOptionValue(out, CLIMATE_LABEL_DISPLAY_OPTION, labelMode);
  }
  if (numberMode !== climateDefaultNumberDisplayMode()) {
    out = setConfigOptionValue(out, CLIMATE_NUMBER_DISPLAY_OPTION, numberMode);
  }
  if (temperatureStep !== climateDefaultTemperatureStep()) {
    out = setConfigOptionValue(out, CLIMATE_TEMPERATURE_STEP_OPTION, temperatureStep);
  }
  if (numberMode !== "icon") {
    out = copyLargeNumbersOption(out, options);
  }
  if (includeControlTabs) {
    var tabs = normalizeClimateControlTabs(configOptionValue(options, CLIMATE_CONTROL_TABS_OPTION));
    if (!climateControlTabsAreDefault(tabs)) {
      out = setConfigOptionValue(out, CLIMATE_CONTROL_TABS_OPTION, tabs.join("|"));
    }
  }
  return out;
}

function climateLabelDisplayMode(b) {
  return normalizeClimateLabelDisplayMode(
    configOptionValue(b && b.options, CLIMATE_LABEL_DISPLAY_OPTION));
}

function setClimateLabelDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeClimateLabelDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    CLIMATE_LABEL_DISPLAY_OPTION,
    normalized === climateDefaultLabelDisplayMode() ? "" : normalized
  );
  b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type) && b.type === "climate_control");
  return b.options;
}

function climateNumberDisplayMode(b) {
  return normalizeClimateNumberDisplayMode(
    configOptionValue(b && b.options, CLIMATE_NUMBER_DISPLAY_OPTION));
}

function setClimateNumberDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeClimateNumberDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    CLIMATE_NUMBER_DISPLAY_OPTION,
    normalized === climateDefaultNumberDisplayMode() ? "" : normalized
  );
  b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type) && b.type === "climate_control");
  return b.options;
}

function climateTemperatureStep(b) {
  return normalizeClimateTemperatureStep(
    configOptionValue(b && b.options, CLIMATE_TEMPERATURE_STEP_OPTION));
}

function setClimateTemperatureStep(b, step) {
  if (!b) return "";
  var normalized = normalizeClimateTemperatureStep(step);
  b.options = setConfigOptionValue(
    b.options,
    CLIMATE_TEMPERATURE_STEP_OPTION,
    normalized === climateDefaultTemperatureStep() ? "" : normalized
  );
  b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type) && b.type === "climate_control");
  return b.options;
}

function alarmActionInfo(value) {
  var actions = alarmActionSpecs();
  for (var i = 0; i < actions.length; i++) {
    if (actions[i].value === value) return actions[i];
  }
  return null;
}

function alarmActionValues() {
  return alarmDefaultActions();
}

function alarmPinRequired(b, mode) {
  var option = mode === "disarm" ? ALARM_PIN_DISARM_OPTION : ALARM_PIN_ARM_OPTION;
  return configOptionValue(b && b.options, option) !== "0";
}

function setAlarmPinRequired(b, mode, required) {
  if (!b) return "";
  var option = mode === "disarm" ? ALARM_PIN_DISARM_OPTION : ALARM_PIN_ARM_OPTION;
  b.options = setConfigOptionValue(b.options, option, required ? "" : "0");
  b.options = normalizeAlarmOptions(b.options);
  return b.options;
}

function normalizeAlarmActionList(value) {
  var raw = String(value || "");
  if (!raw) return alarmDefaultActions();
  var parts = raw.split("|");
  var out = [];
  for (var i = 0; i < parts.length; i++) {
    var action = parts[i];
    if (!alarmActionInfo(action) || out.indexOf(action) >= 0) continue;
    out.push(action);
    if (out.length >= alarmMaxVisibleActions()) break;
  }
  return out.length ? out : alarmDefaultActions();
}

function alarmVisibleActions(b) {
  return normalizeAlarmActionList(configOptionValue(b && b.options, ALARM_ACTIONS_OPTION));
}

function alarmActionsAreDefault(actions) {
  actions = actions || [];
  var defaults = alarmDefaultActions();
  if (actions.length !== defaults.length) return false;
  for (var i = 0; i < defaults.length; i++) {
    if (actions[i] !== defaults[i]) return false;
  }
  return true;
}

function setAlarmVisibleActions(b, actions) {
  if (!b) return "";
  var normalized = normalizeAlarmActionList((actions || []).join("|"));
  b.options = setConfigOptionValue(
    b.options,
    ALARM_ACTIONS_OPTION,
    alarmActionsAreDefault(normalized) ? "" : normalized.join("|")
  );
  b.options = normalizeAlarmOptions(b.options);
  return b.options;
}

function normalizeAlarmIconDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("alarm", ALARM_ICON_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["static", "status"];
  return values.indexOf(value) >= 0 ? value : "status";
}

function normalizeAlarmLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("alarm", ALARM_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["name", "status"];
  return values.indexOf(value) >= 0 ? value : "status";
}

function alarmIconDisplayMode(b) {
  return normalizeAlarmIconDisplayMode(
    configOptionValue(b && b.options, ALARM_ICON_DISPLAY_OPTION));
}

function setAlarmIconDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeAlarmIconDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    ALARM_ICON_DISPLAY_OPTION,
    normalized === "status" ? "" : normalized
  );
  b.options = normalizeAlarmOptions(b.options);
  return b.options;
}

function alarmLabelDisplayMode(b) {
  return normalizeAlarmLabelDisplayMode(
    configOptionValue(b && b.options, ALARM_LABEL_DISPLAY_OPTION));
}

function setAlarmLabelDisplayMode(b, mode) {
  if (!b) return "";
  var normalized = normalizeAlarmLabelDisplayMode(mode);
  b.options = setConfigOptionValue(
    b.options,
    ALARM_LABEL_DISPLAY_OPTION,
    normalized === "status" ? "" : normalized
  );
  b.options = normalizeAlarmOptions(b.options);
  return b.options;
}

function normalizeAlarmOptions(options) {
  var out = "";
  if (configOptionValue(options, ALARM_PIN_ARM_OPTION) === "0") {
    out = setConfigOptionValue(out, ALARM_PIN_ARM_OPTION, "0");
  }
  if (configOptionValue(options, ALARM_PIN_DISARM_OPTION) === "0") {
    out = setConfigOptionValue(out, ALARM_PIN_DISARM_OPTION, "0");
  }
  var rawActions = configOptionValue(options, ALARM_ACTIONS_OPTION);
  if (rawActions) {
    var actions = normalizeAlarmActionList(rawActions);
    if (!alarmActionsAreDefault(actions)) {
      out = setConfigOptionValue(out, ALARM_ACTIONS_OPTION, actions.join("|"));
    }
  }
  var iconMode = normalizeAlarmIconDisplayMode(
    configOptionValue(options, ALARM_ICON_DISPLAY_OPTION));
  if (iconMode !== "status") {
    out = setConfigOptionValue(out, ALARM_ICON_DISPLAY_OPTION, iconMode);
  }
  var labelMode = normalizeAlarmLabelDisplayMode(
    configOptionValue(options, ALARM_LABEL_DISPLAY_OPTION));
  if (labelMode !== "status") {
    out = setConfigOptionValue(out, ALARM_LABEL_DISPLAY_OPTION, labelMode);
  }
  return out;
}

function parseClimatePrecisionConfig(value) {
  var raw = String(value || "");
  var parts = raw.split(":");
  var precision = parts[0] || "";
  if (precision === "0") precision = "";
  if (climatePrecisionValues().indexOf(precision) < 0) precision = "";
  var min = parts.length > 1 ? sanitizeClimateRangeValue(parts[1]) : "";
  var max = parts.length > 2 ? sanitizeClimateRangeValue(parts[2]) : "";
  return { precision: precision, min: min, max: max };
}

function sanitizeClimateRangeValue(value) {
  var text = String(value || "").trim();
  if (!text) return "";
  var num = Number(text);
  if (!isFinite(num)) return "";
  return String(Math.round(num * 10) / 10).replace(/\.0$/, "");
}

function climatePrecisionConfig(precision, min, max) {
  var p = climatePrecisionValues().indexOf(String(precision || "")) >= 0 ? String(precision || "") : "";
  var lo = sanitizeClimateRangeValue(min);
  var hi = sanitizeClimateRangeValue(max);
  if (!lo && !hi) return p;
  return (p || "0") + ":" + lo + ":" + hi;
}

function climatePrecisionValues() {
  var behavior = climateBehaviorSpec();
  var values = behavior && behavior.precisionValues;
  return values && values.length ? values.slice() : ["", "1", "2", "3"];
}

function climateBehaviorSpec() {
  var card = cardContractCard("climate");
  return card && card.behavior && card.behavior.climate || null;
}

function climateDefaultLabelDisplayMode() {
  var behavior = climateBehaviorSpec();
  var fallback = behavior && behavior.defaultLabelDisplay || "label";
  return cardContractOptionDefaultValue("climate", CLIMATE_LABEL_DISPLAY_OPTION, fallback);
}

function climateDefaultNumberDisplayMode() {
  var behavior = climateBehaviorSpec();
  var fallback = behavior && behavior.defaultNumberDisplay || "target";
  return cardContractOptionDefaultValue("climate", CLIMATE_NUMBER_DISPLAY_OPTION, fallback);
}

function climateDefaultTemperatureStep() {
  var behavior = climateBehaviorSpec();
  var fallback = behavior && behavior.defaultTemperatureStep || "1";
  return cardContractOptionDefaultValue("climate", CLIMATE_TEMPERATURE_STEP_OPTION, fallback);
}

function normalizeClimatePrecisionConfig(value) {
  var parsed = parseClimatePrecisionConfig(value);
  return climatePrecisionConfig(parsed.precision, parsed.min, parsed.max);
}

function buttonConfigChangedByNormalize(raw) {
  var before = EspControlModel.cloneCardConfig(raw || {});
  var after = normalizeButtonConfig(EspControlModel.cloneCardConfig(before));
  return EspControlModel.cardConfigChanged(before, after);
}

function trimConfigFields(fields) {
  return EspControlModel.trimConfigFields(fields);
}

function buttonConfigFields(b) {
  var type = b && b.type || "";
  if (b && type === "subpage" && subpageKind(b)) {
    b = EspControlModel.cloneCardConfig(b);
    applySubpagePresetConfig(b);
  }
  var isActionOptionSelect = !!(b && (actionCardIsOptionSelect(b) || isOptionSelectType(type)));
  if (isActionOptionSelect) type = "action";
  if (type === "local") type = "action";
  if (type === "local_sensor") type = "sensor";
  var label = b && b.label || "";
  if (type === "calendar" || type === "clock" || type === "timezone") label = "";
  if (type === "screen_lock") label = "";
  var sensor = isActionOptionSelect ? ACTION_CARD_OPTION_SELECT_ACTION :
    (isBrightnessSliderType(type) || type === "calendar" || type === "clock" || isClimateCardType(type) || type === "light_switch" || type === "light_control" || type === "alarm" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.sensor || "");
  if (type === "lock" && sensor !== "lock" && sensor !== "unlock") sensor = "";
  if (b && b.type === "local") sensor = ACTION_CARD_LOCAL_ACTION;
  if (b && (b.type === "local_sensor" || sensorCardIsLocal(b))) sensor = SENSOR_CARD_LOCAL_SENSOR;
  var isLocalAction = type === "action" && sensor === ACTION_CARD_LOCAL_ACTION;
  var unit = (isActionOptionSelect || type === "calendar" || type === "clock" || isClimateCardType(type) || type === "light_switch" || type === "light_control" || type === "alarm" || type === "alarm_action" || type === "lock" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.unit || "");
  if (isLocalAction) unit = "";
  var icon = b && b.icon || "Auto";
  if (isActionOptionSelect && (!icon || icon === "Auto" || icon === "Chevron Down")) icon = "Flash";
  if (isLocalAction && (!icon || icon === "Auto" || icon === "Flash")) icon = "Gesture Tap";
  if (type === "alarm" && (!icon || icon === "Auto")) icon = "Security";
  if (type === "calendar" || type === "clock" || type === "timezone") icon = "Auto";
  if (type === "screen_lock") icon = "Lock";
  if (type === "alarm_action" && (!icon || icon === "Auto")) icon = (alarmActionInfo(sensor) || alarmActionSpecs()[0]).icon;
  if (isFanCardType(type) && (!icon || icon === "Auto")) icon = fanCardDefaultIcon(type);
  var iconOn = (isActionOptionSelect || type === "alarm" || type === "alarm_action" || (isFanCardType(type) && type !== "fan_switch")) ? "Auto" : (b && b.icon_on || "Auto");
  if (type === "calendar" || type === "clock" || type === "timezone") iconOn = "Auto";
  if (isLocalAction) iconOn = "Auto";
  if (type === "fan_switch" && (!iconOn || iconOn === "Auto")) iconOn = "Fan";
  if (type === "lock") iconOn = sensor ? "Auto" : ((!iconOn || iconOn === "Auto") ? "Lock Open" : iconOn);
  if (type === "screen_lock") iconOn = "Lock Open";
  var precision = (isActionOptionSelect || type === "clock" || type === "light_switch" || type === "light_control" || type === "alarm" || type === "alarm_action" || type === "lock" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.precision || "");
  if (isLocalAction) precision = "";
  if (sensor === SENSOR_CARD_LOCAL_SENSOR && precision !== "text" && precision !== "1" && precision !== "2") precision = "";
  if (type === "media") {
    sensor = mediaEditorMode(sensor);
    precision = sensor === "now_playing"
      ? mediaNowPlayingControls({ sensor: sensor, precision: precision })
      : (mediaStateDisplayModeSupported(sensor) && precision === "state" ? "state" : "");
  }
  if (type === "vacuum") {
    sensor = normalizeVacuumMode(sensor);
    unit = vacuumModeNeedsArea(sensor) ? unit : "";
    precision = "";
    iconOn = "Auto";
    if (!icon || icon === "Auto") icon = vacuumModeDefaultIcon(sensor);
  }
  if (type === "lawn_mower") {
    sensor = normalizeLawnMowerMode(sensor);
    unit = "";
    precision = "";
    iconOn = "Auto";
    if (!icon || icon === "Auto") icon = lawnMowerModeDefaultIcon(sensor);
  }
  if (isClimateCardType(type)) precision = normalizeClimatePrecisionConfig(precision);
  if (type === "calendar" && precision !== "datetime") precision = "";
  if (type === "weather") {
    sensor = "";
    precision = normalizeWeatherCardMode(precision);
  }
  if (type === "todo") {
    sensor = "";
    unit = "";
    precision = "";
    iconOn = "Auto";
    if (!icon || icon === "Auto") icon = "Check";
  }
  if (type === "image") {
    iconOn = "Auto";
    sensor = "";
    unit = "";
    precision = "";
    if (!imageLabelEnabled(b)) label = "";
  }
  if (type === "door_window") precision = normalizeDoorWindowSubtype(precision);
  var options = b && b.options || "";
  if (type === "") {
    options = normalizeSwitchConfirmationOptions(options);
  } else if (type === "alarm" || type === "alarm_action") {
    options = normalizeAlarmOptions(options);
  } else if (type === "garage") {
    options = normalizeGarageOptions(options, sensor);
  } else if (type === "gate") {
    options = normalizeGateOptions(options, sensor);
  } else if (type === "cover") {
    sensor = normalizeCoverMode(sensor, true);
    options = normalizeCoverOptionsForMode(options, sensor);
  } else if (isClimateCardType(type)) {
    options = normalizeClimateOptions(options, type === "climate_control");
  } else if (type === "media") {
    options = normalizeMediaOptions(options, sensor);
  } else if (type === "weather") {
    options = cardLargeNumbersSupported({ type: type, precision: precision }) ? copyLargeNumbersOption("", options) : "";
  } else if (type === "subpage") {
    options = normalizeSubpageOptions(options, sensor, precision);
  } else if (type === "webhook" && typeof normalizeWebhookConfig === "function") {
    var webhookButton = EspControlModel.cloneCardConfig(b || {});
    normalizeWebhookConfig(webhookButton);
    sensor = webhookButton.sensor;
    unit = webhookButton.unit;
    iconOn = webhookButton.icon_on || "Auto";
    precision = webhookButton.precision || "";
    options = webhookButton.options || "";
  } else if (type === "lock" || type === "screen_lock") {
    options = "";
  } else if (type === "calendar" || type === "clock" || type === "timezone") {
    options = normalizeDateTimeOptions(type, options, precision);
  } else if (type === "vacuum" || type === "lawn_mower") {
    options = "";
  } else if (type === "todo") {
    options = normalizeTodoOptions(options);
  } else if (type === "sensor") {
    options = sensor === SENSOR_CARD_LOCAL_SENSOR ? "" : normalizeSensorOptions(options, precision);
  } else if (type === "door_window") {
    options = normalizeDoorWindowOptions(options);
  } else if (type === "presence") {
    options = normalizePresenceOptions(options);
  } else if (type === "image") {
    options = normalizeImageOptions(options);
  } else if (type === "light_control") {
    options = normalizeLightControlOptions(options);
  } else if (type === "fan_control") {
    options = normalizeFanControlOptions(options);
  } else if (type === "action") {
    options = sensor === ACTION_CARD_LOCAL_ACTION ? "" : normalizeActionOptions(options, sensor);
  } else if (isActionOptionSelect || isFanCardType(type)) {
    options = "";
  } else if (type !== "action" && type !== "alarm_action" && !isClimateCardType(type) && type !== "cover" && type !== "garage" && type !== "gate" && type !== "webhook" && type !== "screen_lock" && type !== "media" && type !== "presence" && type !== "light_control" && type !== "fan_control" && !cardLargeNumbersSupported({ type: type, precision: precision })) {
    options = "";
  }
  if (type === "image") {
    icon = configOptionEnabled(options, IMAGE_ICON_OPTION)
      ? (icon && icon !== "Auto" ? icon : "Camera")
      : "Auto";
  }
  if (type === "door_window") {
    b = b || {};
    b.entity = "";
    unit = "";
    if (!icon || icon === "Auto") icon = doorWindowClosedIcon(precision);
    if (!iconOn || iconOn === "Auto") iconOn = doorWindowOpenIcon(precision);
  }
  if (type === "presence") {
    b = b || {};
    b.entity = "";
    unit = "";
    precision = "";
    if (!icon || icon === "Auto") icon = "Motion Sensor Off";
    if (!iconOn || iconOn === "Auto") iconOn = "Motion Sensor";
  }
  if (type === "calendar") {
    b = b || {};
    if (!b.entity) b.entity = cardContractDefaultConfig("calendar").entity;
  }
  if (type === "clock") {
    b = b || {};
    b.entity = "";
  }
  if (type === "timezone") {
    b = b || {};
    if (!b.entity) b.entity = cardContractDefaultConfig("timezone").entity;
  }
  if (!type && !sensor) {
    unit = "";
    precision = "";
  }
  return trimConfigFields([
    (type === "door_window" || type === "presence" || type === "screen_lock") ? "" : (b && b.entity || ""),
    label,
    icon,
    iconOn,
    sensor,
    unit,
    type,
    precision,
    options,
  ]);
}

function encodeConfigField(value) {
  return EspControlModel.encodeConfigField(value);
}

function decodeConfigField(value) {
  return EspControlModel.decodeConfigField(value);
}

function legacyButtonConfigSafe(fields) {
  return EspControlModel.legacyButtonConfigSafe(fields);
}

function serializeButtonConfig(b) {
  var fields = buttonConfigFields(b || {});
  if (legacyButtonConfigSafe(fields)) return fields.join(";");
  return "~" + fields.map(encodeConfigField).join(",");
}

function parseRawButtonConfig(str) {
  return EspControlModel.parseRawButtonConfig(str);
}

function parseButtonConfig(str) {
  return normalizeButtonConfig(parseRawButtonConfig(str));
}

function hasLegacySliderDirection(b) {
  return !!(b && isBrightnessSliderType(b.type) && b.sensor);
}

function buttonConfigHasLegacySliderDirection(str) {
  return hasLegacySliderDirection(parseRawButtonConfig(str || ""));
}

function buttonConfigNeedsMigration(str) {
  return buttonConfigChangedByNormalize(parseRawButtonConfig(str || ""));
}

function parseBackOrderToken(value) {
  return EspControlModel.parseBackOrderToken(value);
}

function backOrderToken(baseToken, label) {
  return EspControlModel.backOrderToken(baseToken, label);
}

function backLabelFromOrder(order) {
  return EspControlModel.backLabelFromOrder(order);
}

function parseSubpageOrder(orderStr) {
  return EspControlModel.parseSubpageOrder(orderStr);
}

function subpageOrderForSerialize(sp) {
  return EspControlModel.subpageOrderForSerialize((sp && sp.order) || [], sp && sp.backLabel);
}

function subpageSerializedOrder(sp) {
  if (!sp) return [];
  if (sp.order && sp.order.length) return subpageOrderForSerialize(sp);
  if (sp.grid && sp.grid.length) return serializeSubpageGrid(sp);
  return [];
}

function parseSubpageConfig(str, raw) {
  var parsed = EspControlModel.parseRawSubpageConfig(str, subpageTypeFromCode);
  if (raw) return parsed;
  var compactButtonTokens = String(str || "").charAt(0) === "~"
    ? String(str || "").split("|").slice(1)
    : [];
  parsed.buttons = parsed.buttons.map(function (button, index) {
    var normalized = normalizeButtonConfig(button);
    if (button && button.type === "calendar" && (!button.entity || compactButtonTokens[index] === "D")) normalized.entity = "";
    return normalized;
  });
  return parsed;
}

function subpageTypeCode(type) {
  return cardContractSubpageTypeCode(type);
}

function subpageTypeFromCode(code) {
  return cardContractSubpageTypeFromCode(code);
}

function encodeSubpageField(value) {
  return encodeConfigField(value);
}

function decodeSubpageField(value) {
  return decodeConfigField(value);
}

function parseCompactSubpageConfig(str, raw) {
  var parsed = EspControlModel.parseCompactSubpageConfig(str, subpageTypeFromCode);
  if (raw) return parsed;
  var compactButtonTokens = String(str || "").split("|").slice(1);
  parsed.buttons = parsed.buttons.map(function (button, index) {
    var normalized = normalizeButtonConfig(button);
    if (button && button.type === "calendar" && compactButtonTokens[index] === "D") normalized.entity = "";
    return normalized;
  });
  return parsed;
}

function subpageConfigHasLegacySliderDirection(str) {
  var sp = parseSubpageConfig(str, true);
  for (var i = 0; i < sp.buttons.length; i++) {
    if (hasLegacySliderDirection(sp.buttons[i])) return true;
  }
  return false;
}

function subpageConfigNeedsMigration(str) {
  var sp = parseSubpageConfig(str, true);
  for (var i = 0; i < sp.buttons.length; i++) {
    if (buttonConfigChangedByNormalize(sp.buttons[i])) return true;
  }
  return false;
}

function serializeSubpageConfig(sp) {
  var order = subpageSerializedOrder(sp);
  var legacy = legacySubpageConfigSafe(sp) ? serializeLegacySubpageConfig(sp) : "";
  var compact = serializeCompactSubpageConfig(sp);
  return EspControlModel.chooseSerializedSubpageConfig(
    order,
    sp && sp.buttons ? sp.buttons.length : 0,
    legacy,
    compact
  );
}

function subpageLegacyButtonFields(b) {
  var fields = buttonConfigFields(b || {});
  if (fields.length > 1 && fields[fields.length - 1] === "Auto") {
    while (fields.length > 1 && (fields[fields.length - 1] === "Auto" || !fields[fields.length - 1])) fields.pop();
  }
  return fields;
}

function subpageCompactButtonFields(b) {
  var fields = buttonConfigFields(b || {});
  var compact = [
    subpageTypeCode(fields[6] || ""),
    encodeSubpageField(fields[0]),
    encodeSubpageField(fields[1]),
    fields[2] && fields[2] !== "Auto" ? encodeSubpageField(fields[2]) : "",
    fields[3] && fields[3] !== "Auto" ? encodeSubpageField(fields[3]) : "",
    encodeSubpageField(fields[4]),
    encodeSubpageField(fields[5]),
    encodeSubpageField(fields[7]),
    encodeSubpageField(fields[8]),
  ];
  while (compact.length > 1 && !compact[compact.length - 1]) compact.pop();
  return compact;
}

function legacySubpageConfigSafe(sp) {
  var fields = ((sp && sp.buttons) || []).map(subpageLegacyButtonFields);
  return EspControlModel.legacySubpageFieldsSafe(fields);
}

function serializeLegacySubpageConfig(sp) {
  if (!sp) return "";
  return EspControlModel.serializeLegacySubpageConfig(
    subpageSerializedOrder(sp),
    ((sp && sp.buttons) || []).map(subpageLegacyButtonFields)
  );
}

function serializeCompactSubpageConfig(sp) {
  if (!sp || !sp.buttons || sp.buttons.length === 0) return "";
  return EspControlModel.serializeCompactSubpageConfig(
    subpageSerializedOrder(sp),
    sp.buttons.map(subpageCompactButtonFields)
  );
}

function applySubpageRaw(slot) {
  var raw = state.subpageRaw[slot];
  var combined = (raw && raw.main || "") + (raw && raw.ext || "") +
    (raw && raw.ext2 || "") + (raw && raw.ext3 || "") +
    (raw && raw.ext4 || "") + (raw && raw.ext5 || "") +
    (raw && raw.ext6 || "") + (raw && raw.ext7 || "");
  var pending = state.subpageSavePending[slot];
  if (pending) {
    if (combined !== pending) {
      if (state.editingSubpage === slot) scheduleRender();
      return;
    }
    delete state.subpageSavePending[slot];
  }
  var local = state.subpages[slot];
  var localHasData = local && (
    (local.buttons && local.buttons.length > 0) ||
    (local.order && local.order.length > 0)
  );
  if (state.editingSubpage === slot && localHasData) {
    var localSerialized = serializeSubpageConfig(local);
    if (combined !== localSerialized) {
      scheduleRender();
      return;
    }
  }
  if (combined) {
    var migrateConfig = subpageConfigNeedsMigration(combined);
    var sp = parseSubpageConfig(combined);
    sp.sizes = sp.sizes || {};
    buildSubpageGrid(sp);
    state.subpages[slot] = sp;
    if (migrateConfig) scheduleSliderSubpageMigration(slot);
  } else {
    delete state.subpages[slot];
  }
  if (state.editingSubpage === slot) {
    scheduleRender();
  }
}

function getSubpage(homeSlot) {
  if (!state.subpages[homeSlot]) {
    state.subpages[homeSlot] = { order: [], buttons: [], grid: [], sizes: {}, backLabel: "Back" };
  } else if (!state.subpages[homeSlot].backLabel) {
    state.subpages[homeSlot].backLabel = backLabelFromOrder(state.subpages[homeSlot].order);
  }
  return state.subpages[homeSlot];
}

function buildSubpageGrid(sp) {
  var result = EspControlModel.buildSubpageGrid(sp, NUM_SLOTS, GRID_COLS);
  sp.grid = result.grid;
  sp.sizes = result.sizes;
  return sp.grid;
}

function serializeSubpageGrid(sp) {
  return EspControlModel.serializeSubpageGrid(sp.grid, sp.sizes || {}, sp.backLabel || "Back");
}

function enterSubpage(homeSlot) {
  state.editingSubpage = homeSlot;
  state.subpageSelectedSlots = [];
  state.subpageLastClicked = -1;
  var sp = getSubpage(homeSlot);
  buildSubpageGrid(sp);
  renderPreview();
  renderButtonSettings();
}

function exitSubpage() {
  state.editingSubpage = null;
  state.subpageSelectedSlots = [];
  state.subpageLastClicked = -1;
  renderPreview();
  renderButtonSettings();
}

function saveSubpageConfig(homeSlot) {
  var sp = getSubpage(homeSlot);
  sp.order = serializeSubpageGrid(sp);
  saveSubpageEntity(homeSlot);
}

function subpageFirstFreeSlot(sp) {
  var used = {};
  sp.grid.forEach(function (s) { if (s > 0) used[s] = true; });
  for (var i = 1; i <= sp.buttons.length + 1; i++) {
    if (!used[i]) return i;
  }
  return sp.buttons.length + 1;
}

function bindTextPost(input, postName, opts) {
  input.addEventListener("blur", function () {
    if (opts && opts.onBlur) opts.onBlur(this.value);
    if (opts && opts.post) opts.post(this.value);
    else postText(postName, this.value);
    if (opts && opts.rerender) renderPreview();
  });
  input.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
}
