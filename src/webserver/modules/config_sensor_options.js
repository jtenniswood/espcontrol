// ── Sensor Card Options ────────────────────────────────────────────
// @web-module-requires: config_option_core

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
