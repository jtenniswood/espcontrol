// ── Confirmation Card Options ─────────────────────────────────────
// @web-module-requires: config_option_core, config_sensor_options

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
