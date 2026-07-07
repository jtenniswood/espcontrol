// ── Config Option Core ─────────────────────────────────────────────
// @web-module-requires: card_contract_generated, state

var SENSOR_STATE_LABELS_OPTION = cardContractOptionName("state_labels");
var SENSOR_STATE_INPUT_OPTION = cardContractOptionName("state_input");
var SENSOR_STATE_OUTPUT_OPTION = cardContractOptionName("state_output");
var SENSOR_STATE_INPUT_2_OPTION = cardContractOptionName("state_input_2");
var SENSOR_STATE_OUTPUT_2_OPTION = cardContractOptionName("state_output_2");
var SENSOR_STATE_LOW_LABEL_OPTION = cardContractOptionName("state_low_label");
var SENSOR_STATE_HIGH_LABEL_OPTION = cardContractOptionName("state_high_label");
var CARD_ON_PATTERN_OPTION = cardContractOptionName("on_pattern");
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
var MEDIA_COVER_ART_OPTION = cardContractOptionName("media_cover_art");
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
