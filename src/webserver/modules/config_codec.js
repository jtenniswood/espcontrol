// ── Subpage helpers ────────────────────────────────────────────────────

var SENSOR_STATE_LABELS_OPTION = "state_labels";
var SENSOR_STATE_INPUT_OPTION = "state_input";
var SENSOR_STATE_OUTPUT_OPTION = "state_output";
var SENSOR_STATE_INPUT_2_OPTION = "state_input_2";
var SENSOR_STATE_OUTPUT_2_OPTION = "state_output_2";
var SENSOR_STATE_LOW_LABEL_OPTION = "state_low_label";
var SENSOR_STATE_HIGH_LABEL_OPTION = "state_high_label";
var CARD_ON_PATTERN_OPTION = "on_pattern";

function normalizeButtonConfig(b) {
  if (b) b.options = b.options || "";
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
    b.options = "";
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
  if (b && b.type === "climate") {
    b.sensor = "";
    b.unit = "";
    if (!b.icon) b.icon = "Thermostat";
    if (!b.icon_on) b.icon_on = "Auto";
    b.precision = normalizeClimatePrecisionConfig(b.precision);
    b.options = normalizeClimateOptions(b.options);
  }
  if (b && b.type === "garage") {
    if (b.sensor !== "open" && b.sensor !== "close") b.sensor = "";
    b.unit = "";
    b.precision = "";
    if (b.sensor === "open" || b.sensor === "close") b.icon_on = "Auto";
    b.options = normalizeGarageOptions(b.options, b.sensor);
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
  if (b && b.type === "vacuum") {
    normalizeVacuumConfig(b);
  }
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
  } else if (b && b.type !== "action" && b.type !== "alarm" && b.type !== "alarm_action" && b.type !== "climate" && b.type !== "garage" && b.type !== "webhook" && b.type !== "screen_lock" && b.type !== "media" && b.type !== "presence" && b.type !== "subpage" && b.type !== "image" && b.type !== "vacuum" && !cardLargeNumbersSupported(b)) {
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

function isOptionSelectType(type) {
  return cardContractIsOptionSelectType(type);
}

function fanCardDefaultIcon(type) {
  return cardContractFanDefaultIcon(type);
}

var SENSOR_LARGE_NUMBERS_OPTION = "large_numbers";
var SENSOR_LARGE_NUMBERS_OFF_VALUE = "off";
var SENSOR_ACTIVE_COLOR_OPTION = "active_color";
var SWITCH_CONFIRM_OFF_OPTION = "confirm_off";
var SWITCH_CONFIRM_ON_OPTION = "confirm_on";
var SWITCH_CONFIRM_MESSAGE_OPTION = "confirm_message";
var SWITCH_CONFIRM_YES_OPTION = "confirm_yes";
var SWITCH_CONFIRM_NO_OPTION = "confirm_no";
var SWITCH_CONFIRM_DEFAULT_MESSAGE = "Turn off this device?";
var SWITCH_CONFIRM_ON_DEFAULT_MESSAGE = "Turn on this device?";
var SWITCH_CONFIRM_BOTH_DEFAULT_MESSAGE = "Toggle this device?";
var SWITCH_CONFIRM_DEFAULT_YES = "Yes";
var SWITCH_CONFIRM_DEFAULT_NO = "No";
var ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE = "Run this script?";
var ALARM_PIN_ARM_OPTION = "pin_arm";
var ALARM_PIN_DISARM_OPTION = "pin_disarm";
var ALARM_ACTIONS_OPTION = "actions";
var ALARM_ICON_DISPLAY_OPTION = "icon_display";
var ALARM_LABEL_DISPLAY_OPTION = "label_display";
var GARAGE_LABEL_DISPLAY_OPTION = "label_display";
var CLIMATE_LABEL_DISPLAY_OPTION = "label_display";
var CLIMATE_NUMBER_DISPLAY_OPTION = "number_display";
var MEDIA_VOLUME_MAX_OPTION = "volume_max";
var SUBPAGE_KIND_OPTION = "subpage_kind";
var IMAGE_LABEL_OPTION = "image_label";
var IMAGE_ICON_OPTION = "image_icon";
var IMAGE_MODAL_MODE_OPTION = "image_modal_mode";
var IMAGE_REFRESH_OPTION = "image_refresh";
var IMAGE_REFRESH_MODE_OPTION = "image_refresh_mode";
var IMAGE_CARD_LIMIT = Math.max(0, parseInt(CFG && CFG.imageCardLimit != null ? CFG.imageCardLimit : 4, 10) || 0);
var ALARM_ACTIONS = [
  { value: "away", label: "Arm Away", service: "alarm_control_panel.alarm_arm_away", icon: "Shield Lock" },
  { value: "home", label: "Arm Home", service: "alarm_control_panel.alarm_arm_home", icon: "Shield Home" },
  { value: "night", label: "Arm Night", service: "alarm_control_panel.alarm_arm_night", icon: "Weather Night" },
  { value: "vacation", label: "Arm Vacation", service: "alarm_control_panel.alarm_arm_vacation", icon: "Airplane" },
  { value: "disarm", label: "Disarm", service: "alarm_control_panel.alarm_disarm", icon: "Shield Off" },
];
var ALARM_DEFAULT_ACTIONS = ["away", "home", "disarm"];
var ALARM_MAX_VISIBLE_ACTIONS = 3;

function alarmBehaviorSpec() {
  var card = cardContractCard("alarm");
  return card && card.behavior && card.behavior.alarm || {};
}

function alarmActionSpecs() {
  var actions = alarmBehaviorSpec().actions;
  return actions && actions.length ? actions : ALARM_ACTIONS;
}

function alarmDefaultActions() {
  var actions = alarmBehaviorSpec().defaultActions;
  return actions && actions.length ? actions.slice() : ALARM_DEFAULT_ACTIONS.slice();
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
  if (!value) return "100";
  var parsed = parseInt(value, 10);
  if (!isFinite(parsed)) return "100";
  if (parsed < 1) parsed = 1;
  if (parsed > 100) parsed = 100;
  return String(parsed);
}

function normalizeMediaOptions(options, mode) {
  mode = mediaEditorMode(mode);
  if (mode !== "volume" && mode !== "position") return "";
  var out = "";
  var maxVolume = normalizeMediaVolumeMax(configOptionValue(options, MEDIA_VOLUME_MAX_OPTION));
  if (mode === "volume" && maxVolume !== "100") {
    out = setConfigOptionValue(out, MEDIA_VOLUME_MAX_OPTION, maxVolume);
  }
  out = copyLargeNumbersOption(out, options);
  return out;
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
  return spec && spec.values ? spec.values.slice() : ["fill", "fit"];
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
  return imageModalModeValues().indexOf(value) >= 0 ? value : "fill";
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
  if (modalMode !== "fill") {
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
  { value: "lock", label: "Lock", preset: { label: "Lock", icon: "Lock", entityDomains: ["lock"], placeholder: "e.g. lock.front_door" } },
  { value: "vacuum", label: "Vacuum", preset: { label: "Vacuum", icon: "Robot Vacuum", entityDomains: ["vacuum"], placeholder: "e.g. vacuum.downstairs" } },
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
  var out = copyActionCardStateOptions("", options);
  if (action !== "script.turn_on" || !configOptionEnabled(options, SWITCH_CONFIRM_ON_OPTION)) {
    return out;
  }
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

function normalizeGarageLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("garage", GARAGE_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["label", "status"];
  return values.indexOf(value) >= 0 ? value : "label";
}

function normalizeGarageOptions(options, mode) {
  var labelMode = normalizeGarageLabelDisplayMode(
    configOptionValue(options, GARAGE_LABEL_DISPLAY_OPTION));
  return labelMode === "status"
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

function normalizeClimateLabelDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("climate", CLIMATE_LABEL_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["label", "status", "actual", "target"];
  return values.indexOf(value) >= 0 ? value : climateDefaultLabelDisplayMode();
}

function normalizeClimateNumberDisplayMode(value) {
  value = String(value || "").trim();
  var spec = cardContractOptionSpec("climate", CLIMATE_NUMBER_DISPLAY_OPTION);
  var values = spec && spec.values ? spec.values : ["icon", "actual", "target"];
  return values.indexOf(value) >= 0 ? value : climateDefaultNumberDisplayMode();
}

function normalizeClimateOptions(options) {
  var labelMode = normalizeClimateLabelDisplayMode(
    configOptionValue(options, CLIMATE_LABEL_DISPLAY_OPTION));
  var numberMode = normalizeClimateNumberDisplayMode(
    configOptionValue(options, CLIMATE_NUMBER_DISPLAY_OPTION));
  var out = "";
  if (labelMode !== climateDefaultLabelDisplayMode()) {
    out = setConfigOptionValue(out, CLIMATE_LABEL_DISPLAY_OPTION, labelMode);
  }
  if (numberMode !== climateDefaultNumberDisplayMode()) {
    out = setConfigOptionValue(out, CLIMATE_NUMBER_DISPLAY_OPTION, numberMode);
  }
  if (numberMode !== "icon") {
    out = copyLargeNumbersOption(out, options);
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
  b.options = normalizeClimateOptions(b.options);
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
  b.options = normalizeClimateOptions(b.options);
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
    if (out.length >= ALARM_MAX_VISIBLE_ACTIONS) break;
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
  if (type === "local_sensor") type = "sensor";
  var label = b && b.label || "";
  if (type === "screen_lock") label = "";
  var sensor = isActionOptionSelect ? ACTION_CARD_OPTION_SELECT_ACTION :
    (isBrightnessSliderType(type) || type === "climate" || type === "light_switch" || type === "alarm" || type === "screen_lock" || isFanCardType(type)) ? "" : (b && b.sensor || "");
  if (b && (b.type === "local_sensor" || sensorCardIsLocal(b))) sensor = SENSOR_CARD_LOCAL_SENSOR;
  var unit = (isActionOptionSelect || type === "climate" || type === "light_switch" || type === "alarm" || type === "alarm_action" || type === "screen_lock" || isFanCardType(type)) ? "" : (b && b.unit || "");
  var icon = b && b.icon || "Auto";
  if (isActionOptionSelect && (!icon || icon === "Auto" || icon === "Chevron Down")) icon = "Flash";
  if (type === "alarm" && (!icon || icon === "Auto")) icon = "Security";
  if (type === "screen_lock") icon = "Lock";
  if (type === "alarm_action" && (!icon || icon === "Auto")) icon = (alarmActionInfo(sensor) || alarmActionSpecs()[0]).icon;
  if (isFanCardType(type) && (!icon || icon === "Auto")) icon = fanCardDefaultIcon(type);
  var iconOn = (isActionOptionSelect || type === "alarm" || type === "alarm_action" || (isFanCardType(type) && type !== "fan_switch")) ? "Auto" : (b && b.icon_on || "Auto");
  if (type === "fan_switch" && (!iconOn || iconOn === "Auto")) iconOn = "Fan";
  if (type === "screen_lock") iconOn = "Lock Open";
  var precision = (isActionOptionSelect || type === "light_switch" || type === "alarm" || type === "alarm_action" || type === "screen_lock" || isFanCardType(type)) ? "" : (b && b.precision || "");
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
  if (type === "climate") precision = normalizeClimatePrecisionConfig(precision);
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
  } else if (type === "climate") {
    options = normalizeClimateOptions(options);
  } else if (type === "media") {
    options = normalizeMediaOptions(options, sensor);
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
  } else if (type === "screen_lock") {
    options = "";
  } else if (type === "vacuum") {
    options = "";
  } else if (type === "sensor") {
    options = sensor === SENSOR_CARD_LOCAL_SENSOR ? "" : normalizeSensorOptions(options, precision);
  } else if (type === "door_window") {
    options = normalizeDoorWindowOptions(options);
  } else if (type === "presence") {
    options = normalizePresenceOptions(options);
  } else if (type === "image") {
    options = normalizeImageOptions(options);
  } else if (type === "action") {
    options = normalizeActionOptions(options, sensor);
  } else if (isActionOptionSelect || isFanCardType(type)) {
    options = "";
  } else if (type !== "action" && type !== "alarm_action" && type !== "garage" && type !== "webhook" && type !== "screen_lock" && type !== "media" && type !== "presence" && !cardLargeNumbersSupported({ type: type, precision: precision })) {
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
  parsed.buttons = parsed.buttons.map(function (button) {
    return normalizeButtonConfig(button);
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
  parsed.buttons = parsed.buttons.map(function (button) {
    return normalizeButtonConfig(button);
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
    postText(postName, this.value);
    if (opts && opts.rerender) renderPreview();
  });
  input.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
}
