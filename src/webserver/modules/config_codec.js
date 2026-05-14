// ── Subpage helpers ────────────────────────────────────────────────────

function normalizeButtonConfig(b) {
  if (b) b.options = b.options || "";
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
    b.type = "weather";
    b.precision = "tomorrow";
    if (b.label === "Weather") b.label = "";
  }
  if (b && b.type === "text_sensor") {
    b.type = "sensor";
    b.precision = "text";
    b.entity = "";
    b.label = "";
    b.unit = "";
    b.icon_on = "Auto";
    if (!b.icon) b.icon = "Auto";
  }
  if (b && b.type === "media") {
    if (b.sensor === "controls") {
      if (!b.icon || b.icon === "Speaker") b.icon = "Auto";
      b.sensor = "play_pause";
    } else if (!b.sensor) {
      b.sensor = "play_pause";
    }
    if (["play_pause", "previous", "next", "volume", "position", "now_playing"].indexOf(b.sensor) < 0) {
      b.sensor = "play_pause";
    }
    if (b.sensor === "previous" && b.label === "Skip Previous") b.label = "Previous";
    if (b.sensor === "next" && b.label === "Skip Next") b.label = "Next";
    if (b.sensor === "volume") {
      if (!b.label || b.label === "Media") b.label = "Volume";
      b.icon = "Auto";
    }
    if (b.sensor === "position" && (!b.label || b.label === "Track")) b.label = "Position";
    if (b.sensor === "now_playing") {
      b.precision = b.precision === "progress" || b.precision === "play_pause" ? b.precision : "";
    } else if ((b.sensor === "play_pause" || b.sensor === "position") && b.precision === "state") {
      b.precision = "state";
    } else {
      b.precision = "";
    }
  }
  if (b && b.type === "climate") {
    b.sensor = "";
    b.unit = "";
    if (!b.icon) b.icon = "Thermostat";
    b.icon_on = "Auto";
    b.precision = normalizeClimatePrecisionConfig(b.precision);
  }
  if (b && b.type === "garage") {
    if (b.sensor !== "open" && b.sensor !== "close") b.sensor = "";
    b.unit = "";
    b.precision = "";
    if (b.sensor === "open" || b.sensor === "close") b.icon_on = "Auto";
  }
  if (b && b.type === "alarm") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Security";
    b.options = normalizeAlarmOptions(b.options);
  }
  if (b && b.type === "light_switch") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
  }
  if (b && b.type === "option_select") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = "";
  }
  if (b && !b.type) {
    b.options = normalizeSwitchConfirmationOptions(b.options);
  } else if (b && b.type !== "action" && b.type !== "alarm" && !cardLargeNumbersSupported(b)) {
    b.options = "";
  }
  return b;
}

function isBrightnessSliderType(type) {
  return type === "slider" || type === "light_brightness" || type === "fan_speed";
}

function isFanCardType(type) {
  return type === "fan_switch" ||
    type === "fan_speed" ||
    type === "fan_oscillate" ||
    type === "fan_direction" ||
    type === "fan_preset";
}

function isOptionSelectType(type) {
  return type === "option_select";
}

function fanCardDefaultIcon(type) {
  if (type === "fan_switch") return "Fan Off";
  if (type === "fan_oscillate") return "Fan";
  if (type === "fan_direction") return "Swap Horizontal";
  if (type === "fan_preset") return "Fan Auto";
  return "Fan Speed 2";
}

var SENSOR_LARGE_NUMBERS_OPTION = "large_numbers";
var SWITCH_CONFIRM_OFF_OPTION = "confirm_off";
var SWITCH_CONFIRM_MESSAGE_OPTION = "confirm_message";
var SWITCH_CONFIRM_YES_OPTION = "confirm_yes";
var SWITCH_CONFIRM_NO_OPTION = "confirm_no";
var SWITCH_CONFIRM_DEFAULT_MESSAGE = "Turn off this device?";
var SWITCH_CONFIRM_DEFAULT_YES = "Yes";
var SWITCH_CONFIRM_DEFAULT_NO = "No";
var ALARM_PIN_ARM_OPTION = "pin_arm";
var ALARM_PIN_DISARM_OPTION = "pin_disarm";
var ALARM_ACTIONS_OPTION = "actions";
var ALARM_ACTIONS = [
  { value: "away", label: "Arm Away", service: "alarm_control_panel.alarm_arm_away", icon: "Security" },
  { value: "home", label: "Arm Home", service: "alarm_control_panel.alarm_arm_home", icon: "Home" },
  { value: "night", label: "Arm Night", service: "alarm_control_panel.alarm_arm_night", icon: "Weather Night" },
  { value: "disarm", label: "Disarm", service: "alarm_control_panel.alarm_disarm", icon: "Lock Open" },
];
var ALARM_DEFAULT_ACTIONS = ["away", "home", "night", "disarm"];

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

function cardLargeNumbersSupported(b) {
  if (!b) return false;
  return (b.type === "sensor" && b.precision !== "text") ||
    (b.type === "weather" && (b.precision === "today" || b.precision === "tomorrow")) ||
    b.type === "calendar" ||
    b.type === "timezone";
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
  b.options = setConfigOption(b.options, SENSOR_LARGE_NUMBERS_OPTION, enabled);
  return b.options;
}

function switchConfirmationEnabled(b) {
  return !!(b && configOptionEnabled(b.options, SWITCH_CONFIRM_OFF_OPTION));
}

function switchConfirmationMessage(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_MESSAGE_OPTION) ||
    SWITCH_CONFIRM_DEFAULT_MESSAGE;
}

function switchConfirmationYesText(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_YES_OPTION) ||
    SWITCH_CONFIRM_DEFAULT_YES;
}

function switchConfirmationNoText(b) {
  return configOptionValue(b && b.options, SWITCH_CONFIRM_NO_OPTION) ||
    SWITCH_CONFIRM_DEFAULT_NO;
}

function normalizeSwitchConfirmationOptions(options) {
  if (!configOptionEnabled(options, SWITCH_CONFIRM_OFF_OPTION)) return "";
  var out = setConfigOption("", SWITCH_CONFIRM_OFF_OPTION, true);
  var msg = configOptionValue(options, SWITCH_CONFIRM_MESSAGE_OPTION);
  var yes = configOptionValue(options, SWITCH_CONFIRM_YES_OPTION);
  var no = configOptionValue(options, SWITCH_CONFIRM_NO_OPTION);
  if (msg && msg !== SWITCH_CONFIRM_DEFAULT_MESSAGE) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_MESSAGE_OPTION, msg);
  }
  if (yes && yes !== SWITCH_CONFIRM_DEFAULT_YES) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_YES_OPTION, yes);
  }
  if (no && no !== SWITCH_CONFIRM_DEFAULT_NO) {
    out = setConfigOptionValue(out, SWITCH_CONFIRM_NO_OPTION, no);
  }
  return out;
}

function setSwitchConfirmationOptions(b, enabled, message, yesText, noText) {
  if (!b) return "";
  var out = setConfigOption("", SWITCH_CONFIRM_OFF_OPTION, !!enabled);
  if (enabled) {
    if (message && message !== SWITCH_CONFIRM_DEFAULT_MESSAGE) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_MESSAGE_OPTION, message);
    }
    if (yesText && yesText !== SWITCH_CONFIRM_DEFAULT_YES) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_YES_OPTION, yesText);
    }
    if (noText && noText !== SWITCH_CONFIRM_DEFAULT_NO) {
      out = setConfigOptionValue(out, SWITCH_CONFIRM_NO_OPTION, noText);
    }
  }
  b.options = out;
  return b.options;
}

function alarmActionInfo(value) {
  for (var i = 0; i < ALARM_ACTIONS.length; i++) {
    if (ALARM_ACTIONS[i].value === value) return ALARM_ACTIONS[i];
  }
  return null;
}

function alarmActionValues() {
  return ALARM_DEFAULT_ACTIONS.slice();
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
  if (!raw) return ALARM_DEFAULT_ACTIONS.slice();
  var parts = raw.split("|");
  var out = [];
  for (var i = 0; i < parts.length; i++) {
    var action = parts[i];
    if (!alarmActionInfo(action) || out.indexOf(action) >= 0) continue;
    out.push(action);
  }
  return out.length ? out : ALARM_DEFAULT_ACTIONS.slice();
}

function alarmVisibleActions(b) {
  return normalizeAlarmActionList(configOptionValue(b && b.options, ALARM_ACTIONS_OPTION));
}

function alarmActionsAreDefault(actions) {
  actions = actions || [];
  if (actions.length !== ALARM_DEFAULT_ACTIONS.length) return false;
  for (var i = 0; i < ALARM_DEFAULT_ACTIONS.length; i++) {
    if (actions[i] !== ALARM_DEFAULT_ACTIONS[i]) return false;
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
  return out;
}

function parseClimatePrecisionConfig(value) {
  var raw = String(value || "");
  var parts = raw.split(":");
  var precision = parts[0] || "";
  if (precision === "0") precision = "";
  if (["", "1", "2", "3"].indexOf(precision) < 0) precision = "";
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
  var p = ["", "1", "2", "3"].indexOf(String(precision || "")) >= 0 ? String(precision || "") : "";
  var lo = sanitizeClimateRangeValue(min);
  var hi = sanitizeClimateRangeValue(max);
  if (!lo && !hi) return p;
  return (p || "0") + ":" + lo + ":" + hi;
}

function normalizeClimatePrecisionConfig(value) {
  var parsed = parseClimatePrecisionConfig(value);
  return climatePrecisionConfig(parsed.precision, parsed.min, parsed.max);
}

function buttonConfigChangedByNormalize(raw) {
  var before = {
    entity: raw && raw.entity || "",
    label: raw && raw.label || "",
    icon: raw && raw.icon || "Auto",
    icon_on: raw && raw.icon_on || "Auto",
    sensor: raw && raw.sensor || "",
    unit: raw && raw.unit || "",
    type: raw && raw.type || "",
    precision: raw && raw.precision || "",
    options: raw && raw.options || "",
  };
  var after = normalizeButtonConfig({
    entity: before.entity,
    label: before.label,
    icon: before.icon,
    icon_on: before.icon_on,
    sensor: before.sensor,
    unit: before.unit,
    type: before.type,
    precision: before.precision,
    options: before.options,
  });
  return before.entity !== after.entity ||
    before.label !== after.label ||
    before.icon !== after.icon ||
    before.icon_on !== after.icon_on ||
    before.sensor !== after.sensor ||
    before.unit !== after.unit ||
    before.type !== after.type ||
    before.precision !== after.precision ||
    before.options !== after.options;
}

function trimConfigFields(fields) {
  while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
  return fields;
}

function buttonConfigFields(b) {
  var type = b && b.type || "";
  var sensor = (isBrightnessSliderType(type) || type === "climate" || type === "light_switch" || type === "alarm" || isFanCardType(type) || isOptionSelectType(type)) ? "" : (b && b.sensor || "");
  var unit = (type === "climate" || type === "light_switch" || type === "alarm" || isFanCardType(type) || isOptionSelectType(type)) ? "" : (b && b.unit || "");
  var icon = b && b.icon || "Auto";
  if (type === "alarm" && (!icon || icon === "Auto")) icon = "Security";
  if (isFanCardType(type) && (!icon || icon === "Auto")) icon = fanCardDefaultIcon(type);
  var iconOn = (type === "climate" || type === "alarm" || isOptionSelectType(type) || (isFanCardType(type) && type !== "fan_switch")) ? "Auto" : (b && b.icon_on || "Auto");
  if (type === "fan_switch" && (!iconOn || iconOn === "Auto")) iconOn = "Fan";
  var precision = (type === "light_switch" || type === "alarm" || isFanCardType(type) || isOptionSelectType(type)) ? "" : (b && b.precision || "");
  if (type === "climate") precision = normalizeClimatePrecisionConfig(precision);
  var options = b && b.options || "";
  if (type === "") {
    options = normalizeSwitchConfirmationOptions(options);
  } else if (type === "alarm") {
    options = normalizeAlarmOptions(options);
  } else if (isFanCardType(type) || isOptionSelectType(type)) {
    options = "";
  } else if (type !== "action" && !cardLargeNumbersSupported({ type: type, precision: precision })) {
    options = "";
  }
  if (!type && !sensor) {
    unit = "";
    precision = "";
  }
  return trimConfigFields([
    b && b.entity || "",
    b && b.label || "",
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
  return String(value || "").replace(/[%,;|:]/g, function (ch) {
    var hex = ch.charCodeAt(0).toString(16).toUpperCase();
    return "%" + (hex.length < 2 ? "0" : "") + hex;
  });
}

function decodeConfigField(value) {
  return String(value || "").replace(/%([0-9a-fA-F]{2})/g, function (_, hex) {
    return String.fromCharCode(parseInt(hex, 16));
  });
}

function legacyButtonConfigSafe(fields) {
  return fields.join(";").charAt(0) !== "~" && fields.every(function (field) {
    return String(field || "").indexOf(";") < 0;
  });
}

function serializeButtonConfig(b) {
  var fields = buttonConfigFields(b || {});
  if (legacyButtonConfigSafe(fields)) return fields.join(";");
  return "~" + fields.map(encodeConfigField).join(",");
}

function parseRawButtonConfig(str) {
  var compact = str && str.charAt(0) === "~";
  var parts = compact ? str.substring(1).split(",") : (str || "").split(";");
  if (compact) {
    parts = parts.map(decodeConfigField);
  }
  return {
    entity: parts[0] || "",
    label: parts[1] || "",
    icon: parts[2] || "Auto",
    icon_on: parts[3] || "Auto",
    sensor: parts[4] || "",
    unit: parts[5] || "",
    type: parts[6] || "",
    precision: parts[7] || "",
    options: parts[8] || "",
  };
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
  var raw = String(value || "").trim();
  var eq = raw.indexOf("=");
  var token = eq >= 0 ? raw.substring(0, eq) : raw;
  var label = eq >= 0 ? decodeSubpageField(raw.substring(eq + 1)) : "Back";
  if (token !== "B" && token !== "Bd" && token !== "Bw" && token !== "Bb" &&
      token !== "Bt" && token !== "Bx") {
    return { token: raw, label: "Back" };
  }
  return { token: token, label: label || "Back" };
}

function backOrderToken(baseToken, label) {
  var token = parseBackOrderToken(baseToken).token;
  var text = label || "Back";
  return text === "Back" ? token : token + "=" + encodeSubpageField(text);
}

function backLabelFromOrder(order) {
  for (var i = 0; i < (order || []).length; i++) {
    var parsed = parseBackOrderToken(order[i]);
    if (parsed.token === "B" || parsed.token === "Bd" || parsed.token === "Bw" ||
        parsed.token === "Bb" || parsed.token === "Bt" || parsed.token === "Bx") {
      return parsed.label || "Back";
    }
  }
  return "Back";
}

function parseSubpageOrder(orderStr) {
  var order = [];
  var backLabel = "Back";
  if (orderStr) {
    var op = orderStr.split(",");
    for (var i = 0; i < op.length; i++) {
      var parsed = parseBackOrderToken(op[i]);
      order.push(parsed.token);
      if (parsed.token === "B" || parsed.token === "Bd" || parsed.token === "Bw" ||
          parsed.token === "Bb" || parsed.token === "Bt" || parsed.token === "Bx") {
        backLabel = parsed.label || "Back";
      }
    }
  }
  return { order: order, backLabel: backLabel };
}

function subpageOrderForSerialize(sp) {
  var order = [];
  for (var i = 0; i < ((sp && sp.order) || []).length; i++) {
    var parsed = parseBackOrderToken(sp.order[i]);
    if (parsed.token === "B" || parsed.token === "Bd" || parsed.token === "Bw" ||
        parsed.token === "Bb" || parsed.token === "Bt" || parsed.token === "Bx") {
      order.push(backOrderToken(parsed.token, sp.backLabel || parsed.label || "Back"));
    } else {
      order.push(parsed.token);
    }
  }
  return order;
}

function parseSubpageConfig(str, raw) {
  if (str && str.charAt(0) === "~") return parseCompactSubpageConfig(str, raw);
  if (!str || !str.trim()) return { order: [], buttons: [], backLabel: "Back" };
  var parts = str.split("|");
  var parsedOrder = parseSubpageOrder(parts[0] || "");
  var order = parsedOrder.order;
  var backLabel = parsedOrder.backLabel;
  var buttons = [];
  for (var i = 1; i < parts.length; i++) {
    var f = parts[i].split(":");
    var button = {
      entity: f[0] || "",
      label: f[1] || "",
      icon: f[2] || "Auto",
      icon_on: f[3] || "Auto",
      sensor: f[4] || "",
      unit: f[5] || "",
      type: f[6] || "",
      precision: f[7] || "",
      options: f[8] || "",
    };
    buttons.push(raw ? button : normalizeButtonConfig(button));
  }
  return { order: order, buttons: buttons, backLabel: backLabel };
}

function subpageTypeCode(type) {
  var map = {
    action: "A",
    calendar: "D",
    timezone: "T",
    sensor: "S",
    weather: "W",
    weather_forecast: "F",
    option_select: "U",
    fan_switch: "B",
    fan_speed: "J",
    fan_oscillate: "O",
    fan_direction: "E",
    fan_preset: "Z",
    light_brightness: "V",
    light_switch: "Q",
    alarm: "Y",
    slider: "L",
    cover: "C",
    light_temperature: "N",
    garage: "R",
    lock: "K",
    media: "M",
    climate: "H",
    push: "P",
    internal: "I",
    subpage: "G",
  };
  return map[type || ""] || (type || "");
}

function subpageTypeFromCode(code) {
  var map = {
    A: "action",
    D: "calendar",
    T: "timezone",
    S: "sensor",
    W: "weather",
    F: "weather_forecast",
    U: "option_select",
    B: "fan_switch",
    J: "fan_speed",
    O: "fan_oscillate",
    E: "fan_direction",
    Z: "fan_preset",
    V: "light_brightness",
    Q: "light_switch",
    Y: "alarm",
    L: "slider",
    C: "cover",
    N: "light_temperature",
    R: "garage",
    K: "lock",
    M: "media",
    H: "climate",
    P: "push",
    I: "internal",
    G: "subpage",
  };
  return map[code || ""] || (code || "");
}

function encodeSubpageField(value) {
  return encodeConfigField(value);
}

function decodeSubpageField(value) {
  return decodeConfigField(value);
}

function parseCompactSubpageConfig(str, raw) {
  if (!str || str.length < 2) return { order: [], buttons: [], backLabel: "Back" };
  var parts = str.substring(1).split("|");
  var parsedOrder = parseSubpageOrder(parts[0] || "");
  var order = parsedOrder.order;
  var backLabel = parsedOrder.backLabel;
  var buttons = [];
  for (var i = 1; i < parts.length; i++) {
    var f = parts[i].split(",");
    var button = {
      type: subpageTypeFromCode(f[0] || ""),
      entity: decodeSubpageField(f[1]),
      label: decodeSubpageField(f[2]),
      icon: decodeSubpageField(f[3]) || "Auto",
      icon_on: decodeSubpageField(f[4]) || "Auto",
      sensor: decodeSubpageField(f[5]),
      unit: decodeSubpageField(f[6]),
      precision: decodeSubpageField(f[7]),
      options: decodeSubpageField(f[8]),
    };
    buttons.push(raw ? button : normalizeButtonConfig(button));
  }
  return { order: order, buttons: buttons, backLabel: backLabel };
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
  var legacy = legacySubpageConfigSafe(sp) ? serializeLegacySubpageConfig(sp) : "";
  var compact = serializeCompactSubpageConfig(sp);
  if (!compact) return legacy;
  if (!legacy) return compact;
  return compact.length < legacy.length ? compact : legacy;
}

function legacySubpageConfigSafe(sp) {
  if (!sp || !sp.buttons) return true;
  for (var i = 0; i < sp.buttons.length; i++) {
    var b = sp.buttons[i];
    var sensor = (b.type === "climate" || b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.sensor || "");
    var unit = (b.type === "climate" || b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.unit || "");
    var icon = b.icon || "Auto";
    if (b.type === "alarm" && (!icon || icon === "Auto")) icon = "Security";
    if (isFanCardType(b.type) && (!icon || icon === "Auto")) icon = fanCardDefaultIcon(b.type);
    var iconOn = (b.type === "climate" || b.type === "alarm" || isOptionSelectType(b.type) || (isFanCardType(b.type) && b.type !== "fan_switch")) ? "Auto" : (b.icon_on || "Auto");
    if (b.type === "fan_switch" && (!iconOn || iconOn === "Auto")) iconOn = "Fan";
    var precision = (b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.precision || "");
    if (b.type === "climate") precision = normalizeClimatePrecisionConfig(precision);
    var options = b.options || "";
    if (!b.type) {
      options = normalizeSwitchConfirmationOptions(options);
    } else if (b.type === "alarm") {
      options = normalizeAlarmOptions(options);
    } else if (isFanCardType(b.type) || isOptionSelectType(b.type)) {
      options = "";
    } else if (b.type !== "action" && !cardLargeNumbersSupported({ type: b.type || "", precision: precision })) {
      options = "";
    }
    var fields = [b.entity || "", b.label || "", icon, iconOn, sensor, unit, b.type || "", precision, options];
    for (var j = 0; j < fields.length; j++) {
      if (String(fields[j] || "").indexOf("|") >= 0 || String(fields[j] || "").indexOf(":") >= 0) {
        return false;
      }
    }
  }
  return true;
}

function serializeLegacySubpageConfig(sp) {
  if (!sp || !sp.buttons || sp.buttons.length === 0) return "";
  var out = subpageOrderForSerialize(sp).join(",");
  for (var i = 0; i < sp.buttons.length; i++) {
    var b = sp.buttons[i];
    var sensor = (isBrightnessSliderType(b.type) || b.type === "climate" || b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.sensor || "");
    var unit = (b.type === "climate" || b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.unit || "");
    var icon = b.icon || "Auto";
    if (b.type === "alarm" && (!icon || icon === "Auto")) icon = "Security";
    if (isFanCardType(b.type) && (!icon || icon === "Auto")) icon = fanCardDefaultIcon(b.type);
    var iconOn = (b.type === "climate" || b.type === "alarm" || isOptionSelectType(b.type) || (isFanCardType(b.type) && b.type !== "fan_switch")) ? "Auto" : (b.icon_on || "Auto");
    if (b.type === "fan_switch" && (!iconOn || iconOn === "Auto")) iconOn = "Fan";
    var precision = (b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.precision || "");
    if (b.type === "climate") precision = normalizeClimatePrecisionConfig(precision);
    var options = b.options || "";
    if (!b.type) {
      options = normalizeSwitchConfirmationOptions(options);
    } else if (b.type === "alarm") {
      options = normalizeAlarmOptions(options);
    } else if (isFanCardType(b.type) || isOptionSelectType(b.type)) {
      options = "";
    } else if (b.type !== "action" && !cardLargeNumbersSupported({ type: b.type || "", precision: precision })) {
      options = "";
    }
    var fields = [b.entity || "", b.label || "", icon, iconOn, sensor, unit, b.type || "", precision, options];
    while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
    if (fields.length > 1 && fields[fields.length - 1] === "Auto") {
      while (fields.length > 1 && (fields[fields.length - 1] === "Auto" || !fields[fields.length - 1])) fields.pop();
    }
    out += "|" + fields.join(":");
  }
  return out;
}

function serializeCompactSubpageConfig(sp) {
  if (!sp || !sp.buttons || sp.buttons.length === 0) return "";
  var out = "~" + subpageOrderForSerialize(sp).join(",");
  for (var i = 0; i < sp.buttons.length; i++) {
    var b = sp.buttons[i];
    var sensor = (isBrightnessSliderType(b.type) || b.type === "climate" || b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.sensor || "");
    var unit = (b.type === "climate" || b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.unit || "");
    var icon = b.icon || "Auto";
    if (b.type === "alarm" && (!icon || icon === "Auto")) icon = "Security";
    if (isFanCardType(b.type) && (!icon || icon === "Auto")) icon = fanCardDefaultIcon(b.type);
    var iconOn = (b.type === "climate" || b.type === "alarm" || isOptionSelectType(b.type) || (isFanCardType(b.type) && b.type !== "fan_switch")) ? "Auto" : (b.icon_on || "Auto");
    if (b.type === "fan_switch" && (!iconOn || iconOn === "Auto")) iconOn = "Fan";
    var precision = (b.type === "light_switch" || b.type === "alarm" || isFanCardType(b.type) || isOptionSelectType(b.type)) ? "" : (b.precision || "");
    if (b.type === "climate") precision = normalizeClimatePrecisionConfig(precision);
    var options = b.options || "";
    if (!b.type) {
      options = normalizeSwitchConfirmationOptions(options);
    } else if (b.type === "alarm") {
      options = normalizeAlarmOptions(options);
    } else if (isFanCardType(b.type) || isOptionSelectType(b.type)) {
      options = "";
    } else if (b.type !== "action" && !cardLargeNumbersSupported({ type: b.type || "", precision: precision })) {
      options = "";
    }
    var fields = [
      subpageTypeCode(b.type || ""),
      encodeSubpageField(b.entity),
      encodeSubpageField(b.label),
      icon && icon !== "Auto" ? encodeSubpageField(icon) : "",
      iconOn && iconOn !== "Auto" ? encodeSubpageField(iconOn) : "",
      encodeSubpageField(sensor),
      encodeSubpageField(unit),
      encodeSubpageField(precision),
      encodeSubpageField(options),
    ];
    while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
    out += "|" + fields.join(",");
  }
  return out;
}

function applySubpageRaw(slot) {
  var raw = state.subpageRaw[slot];
  var combined = (raw ? raw.main : "") + (raw ? raw.ext : "") +
    (raw ? raw.ext2 : "") + (raw ? raw.ext3 : "");
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
  var grid = [];
  for (var i = 0; i < NUM_SLOTS; i++) grid.push(0);
  sp.sizes = sp.sizes || {};
  if (sp.order.length > 0) {
    var hasBack = false;
    for (var i = 0; i < sp.order.length; i++) {
      var t = parseBackOrderToken(sp.order[i]).token;
      if (t === "B" || t === "Bd" || t === "Bw" || t === "Bb" || t === "Bt" || t === "Bx") { hasBack = true; break; }
    }
    if (hasBack) {
      for (var i = 0; i < sp.order.length && i < NUM_SLOTS; i++) {
        var s = parseBackOrderToken(sp.order[i]).token;
        if (!s) continue;
        if (s === "B" || s === "Bd" || s === "Bw" || s === "Bb" || s === "Bt" || s === "Bx") {
          grid[i] = -2;
          var backSize = sizeFromToken(s.charAt(1));
          if (backSize > 1) sp.sizes[-2] = backSize;
          else delete sp.sizes[-2];
          continue;
        }
        var last = s.charAt(s.length - 1);
        var parsedSize = sizeFromToken(last);
        var n = parseInt(s, 10);
        if (n >= 1 && n <= sp.buttons.length && !isNaN(n)) {
          grid[i] = n;
          if (parsedSize > 1) sp.sizes[n] = parsedSize;
        }
      }
    } else {
      grid[0] = -2;
      delete sp.sizes[-2];
      for (var i = 0; i < sp.order.length && i + 1 < NUM_SLOTS; i++) {
        var s = parseBackOrderToken(sp.order[i]).token;
        if (!s) continue;
        var last = s.charAt(s.length - 1);
        var parsedSize = sizeFromToken(last);
        var n = parseInt(s, 10);
        if (n >= 1 && n <= sp.buttons.length && !isNaN(n)) {
          grid[i + 1] = n;
          if (parsedSize > 1) sp.sizes[n] = parsedSize;
        }
      }
    }
  } else {
    grid[0] = -2;
    delete sp.sizes[-2];
  }
  applySpans(grid, sp.sizes, NUM_SLOTS);
  sp.grid = grid;
  return grid;
}

function serializeSubpageGrid(sp) {
  var grid = sp.grid;
  var last = -1;
  for (var i = grid.length - 1; i >= 0; i--) {
    if (grid[i] > 0 || grid[i] === -2) { last = i; break; }
  }
  if (last < 0) return [];
  var order = [];
  for (var i = 0; i <= last; i++) {
    if (grid[i] === -2) {
      var bsz = sp.sizes[-2];
      order.push(backOrderToken("B" + sizeToken(bsz), sp.backLabel || "Back"));
    } else if (grid[i] <= 0) {
      order.push("");
    } else {
      var ssz = sp.sizes[grid[i]];
      order.push(grid[i] + sizeToken(ssz));
    }
  }
  return order;
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
