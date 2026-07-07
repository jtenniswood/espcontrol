// ── Access, Climate, and Alarm Card Options ───────────────────────
// @web-module-requires: config_option_core, config_modal_tab_options

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
  b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type));
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
  b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type));
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
  b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type));
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
