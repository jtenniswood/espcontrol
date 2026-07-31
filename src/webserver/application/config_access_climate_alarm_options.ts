import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installConfigAccessClimateAlarmOptionsModule(): GlobalDescriptors {
    // ── Access, Climate, and Alarm Card Options ───────────────────────
    function alarmBehaviorSpec(this: any) {
        var card: any = cardContractCard("alarm");
        return card && card.behavior && card.behavior.alarm || {};
    }
    function alarmActionSpecs(this: any) {
        var actions: any = alarmBehaviorSpec().actions;
        return actions && actions.length ? actions : [];
    }
    function alarmDefaultActions(this: any) {
        var actions: any = alarmBehaviorSpec().defaultActions;
        return actions && actions.length ? actions.slice() : [];
    }
    function alarmMaxVisibleActions(this: any) {
        var max: any = parseInt(alarmBehaviorSpec().maxVisibleActions, 10);
        return isFinite(max) && max > 0 ? max : alarmDefaultActions().length;
    }
    function alarmActionLegacyIcon(this: any, value?: any) {
        var info: any = alarmActionInfo(value);
        if (info && info.legacyIcon)
            return info.legacyIcon;
        return "";
    }
    function alarmActionIconIsGenerated(this: any, value?: any, icon?: any) {
        var info: any = alarmActionInfo(value);
        return !!info && (icon === info.icon || icon === alarmActionLegacyIcon(value));
    }
    function normalizeGarageLabelDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("garage", GARAGE_LABEL_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : [];
        var fallback: any = cardContractOptionDefaultValue("garage", GARAGE_LABEL_DISPLAY_OPTION, "label");
        return values.indexOf(value) >= 0 ? value : fallback;
    }
    function normalizeGarageOptions(this: any, options?: any, mode?: any) {
        var labelMode: any = normalizeGarageLabelDisplayMode(configOptionValue(options, GARAGE_LABEL_DISPLAY_OPTION));
        var out: any = labelMode !== cardContractOptionDefaultValue("garage", GARAGE_LABEL_DISPLAY_OPTION, "label")
            ? setConfigOptionValue("", GARAGE_LABEL_DISPLAY_OPTION, labelMode)
            : "";
        var confirmationMode: any = mode === "open" ? "on" : mode === "close" ? "off" : "";
        return normalizeGarageConfirmationOptions(out, options, confirmationMode);
    }
    function garageLabelDisplayMode(this: any, b?: any) {
        return normalizeGarageLabelDisplayMode(configOptionValue(b && b.options, GARAGE_LABEL_DISPLAY_OPTION));
    }
    function setGarageLabelDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, GARAGE_LABEL_DISPLAY_OPTION, normalizeGarageLabelDisplayMode(mode) === "status" ? "status" : "");
        b.options = normalizeGarageOptions(b.options, b.sensor);
        return b.options;
    }
    function normalizeGateLabelDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("gate", GATE_LABEL_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : [];
        var fallback: any = cardContractOptionDefaultValue("gate", GATE_LABEL_DISPLAY_OPTION, "label");
        return values.indexOf(value) >= 0 ? value : fallback;
    }
    function normalizeGateOptions(this: any, options?: any, mode?: any) {
        var labelMode: any = normalizeGateLabelDisplayMode(configOptionValue(options, GATE_LABEL_DISPLAY_OPTION));
        return labelMode !== cardContractOptionDefaultValue("gate", GATE_LABEL_DISPLAY_OPTION, "label")
            ? setConfigOptionValue("", GATE_LABEL_DISPLAY_OPTION, labelMode)
            : "";
    }
    function gateLabelDisplayMode(this: any, b?: any) {
        return normalizeGateLabelDisplayMode(configOptionValue(b && b.options, GATE_LABEL_DISPLAY_OPTION));
    }
    function setGateLabelDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, GATE_LABEL_DISPLAY_OPTION, normalizeGateLabelDisplayMode(mode) === "status" ? "status" : "");
        b.options = normalizeGateOptions(b.options, b.sensor);
        return b.options;
    }
    function normalizeClimateLabelDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("climate", CLIMATE_LABEL_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : [];
        return values.indexOf(value) >= 0 ? value : climateDefaultLabelDisplayMode();
    }
    function normalizeClimateNumberDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("climate", CLIMATE_NUMBER_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : [];
        return values.indexOf(value) >= 0 ? value : climateDefaultNumberDisplayMode();
    }
    function normalizeClimateTemperatureStep(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("climate", CLIMATE_TEMPERATURE_STEP_OPTION);
        var values: any = spec && spec.values ? spec.values : [];
        return values.indexOf(value) >= 0 ? value : climateDefaultTemperatureStep();
    }
    function normalizeClimateOptions(this: any, options?: any, includeControlTabs?: any) {
        var labelMode: any = normalizeClimateLabelDisplayMode(configOptionValue(options, CLIMATE_LABEL_DISPLAY_OPTION));
        var numberMode: any = normalizeClimateNumberDisplayMode(configOptionValue(options, CLIMATE_NUMBER_DISPLAY_OPTION));
        var temperatureStep: any = normalizeClimateTemperatureStep(configOptionValue(options, CLIMATE_TEMPERATURE_STEP_OPTION));
        var out: any = "";
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
            var tabs: any = normalizeClimateControlTabs(configOptionValue(options, CLIMATE_CONTROL_TABS_OPTION));
            if (!climateControlTabsAreDefault(tabs)) {
                out = setConfigOptionValue(out, CLIMATE_CONTROL_TABS_OPTION, tabs.join("|"));
            }
        }
        return out;
    }
    function climateLabelDisplayMode(this: any, b?: any) {
        return normalizeClimateLabelDisplayMode(configOptionValue(b && b.options, CLIMATE_LABEL_DISPLAY_OPTION));
    }
    function setClimateLabelDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeClimateLabelDisplayMode(mode);
        b.options = setConfigOptionValue(b.options, CLIMATE_LABEL_DISPLAY_OPTION, normalized === climateDefaultLabelDisplayMode() ? "" : normalized);
        b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type));
        return b.options;
    }
    function climateNumberDisplayMode(this: any, b?: any) {
        return normalizeClimateNumberDisplayMode(configOptionValue(b && b.options, CLIMATE_NUMBER_DISPLAY_OPTION));
    }
    function setClimateNumberDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeClimateNumberDisplayMode(mode);
        b.options = setConfigOptionValue(b.options, CLIMATE_NUMBER_DISPLAY_OPTION, normalized === climateDefaultNumberDisplayMode() ? "" : normalized);
        b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type));
        return b.options;
    }
    function climateTemperatureStep(this: any, b?: any) {
        return normalizeClimateTemperatureStep(configOptionValue(b && b.options, CLIMATE_TEMPERATURE_STEP_OPTION));
    }
    function setClimateTemperatureStep(this: any, b?: any, step?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeClimateTemperatureStep(step);
        b.options = setConfigOptionValue(b.options, CLIMATE_TEMPERATURE_STEP_OPTION, normalized === climateDefaultTemperatureStep() ? "" : normalized);
        b.options = normalizeClimateOptions(b.options, isClimateCardType(b.type));
        return b.options;
    }
    function alarmActionInfo(this: any, value?: any) {
        var actions: any = alarmActionSpecs();
        for (var i: any = 0; i < actions.length; i++) {
            if (actions[i].value === value)
                return actions[i];
        }
        return null;
    }
    function alarmActionValues(this: any) {
        return alarmDefaultActions();
    }
    function alarmPinRequired(this: any, b?: any, mode?: any) {
        var option: any = mode === "disarm" ? ALARM_PIN_DISARM_OPTION : ALARM_PIN_ARM_OPTION;
        return configOptionValue(b && b.options, option) !== "0";
    }
    function setAlarmPinRequired(this: any, b?: any, mode?: any, required?: any) {
        if (!b)
            return "";
        var option: any = mode === "disarm" ? ALARM_PIN_DISARM_OPTION : ALARM_PIN_ARM_OPTION;
        b.options = setConfigOptionValue(b.options, option, required ? "" : "0");
        b.options = normalizeAlarmOptions(b.options);
        return b.options;
    }
    function normalizeAlarmActionList(this: any, value?: any) {
        var raw: any = String(value || "");
        if (!raw)
            return alarmDefaultActions();
        var parts: any = raw.split("|");
        var out: any = [];
        for (var i: any = 0; i < parts.length; i++) {
            var action: any = parts[i];
            if (!alarmActionInfo(action) || out.indexOf(action) >= 0)
                continue;
            out.push(action);
            if (out.length >= alarmMaxVisibleActions())
                break;
        }
        return out.length ? out : alarmDefaultActions();
    }
    function alarmVisibleActions(this: any, b?: any) {
        return normalizeAlarmActionList(configOptionValue(b && b.options, ALARM_ACTIONS_OPTION));
    }
    function alarmActionsAreDefault(this: any, actions?: any) {
        actions = actions || [];
        var defaults: any = alarmDefaultActions();
        if (actions.length !== defaults.length)
            return false;
        for (var i: any = 0; i < defaults.length; i++) {
            if (actions[i] !== defaults[i])
                return false;
        }
        return true;
    }
    function setAlarmVisibleActions(this: any, b?: any, actions?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeAlarmActionList((actions || []).join("|"));
        b.options = setConfigOptionValue(b.options, ALARM_ACTIONS_OPTION, alarmActionsAreDefault(normalized) ? "" : normalized.join("|"));
        b.options = normalizeAlarmOptions(b.options);
        return b.options;
    }
    function normalizeAlarmIconDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("alarm", ALARM_ICON_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : ["static", "status"];
        return values.indexOf(value) >= 0 ? value : "status";
    }
    function normalizeAlarmLabelDisplayMode(this: any, value?: any) {
        value = String(value || "").trim();
        var spec: any = cardContractOptionSpec("alarm", ALARM_LABEL_DISPLAY_OPTION);
        var values: any = spec && spec.values ? spec.values : ["name", "status"];
        return values.indexOf(value) >= 0 ? value : "status";
    }
    function alarmIconDisplayMode(this: any, b?: any) {
        return normalizeAlarmIconDisplayMode(configOptionValue(b && b.options, ALARM_ICON_DISPLAY_OPTION));
    }
    function setAlarmIconDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeAlarmIconDisplayMode(mode);
        b.options = setConfigOptionValue(b.options, ALARM_ICON_DISPLAY_OPTION, normalized === "status" ? "" : normalized);
        b.options = normalizeAlarmOptions(b.options);
        return b.options;
    }
    function alarmLabelDisplayMode(this: any, b?: any) {
        return normalizeAlarmLabelDisplayMode(configOptionValue(b && b.options, ALARM_LABEL_DISPLAY_OPTION));
    }
    function setAlarmLabelDisplayMode(this: any, b?: any, mode?: any) {
        if (!b)
            return "";
        var normalized: any = normalizeAlarmLabelDisplayMode(mode);
        b.options = setConfigOptionValue(b.options, ALARM_LABEL_DISPLAY_OPTION, normalized === "status" ? "" : normalized);
        b.options = normalizeAlarmOptions(b.options);
        return b.options;
    }
    function normalizeAlarmOptions(this: any, options?: any) {
        var out: any = "";
        if (configOptionValue(options, ALARM_PIN_ARM_OPTION) === "0") {
            out = setConfigOptionValue(out, ALARM_PIN_ARM_OPTION, "0");
        }
        if (configOptionValue(options, ALARM_PIN_DISARM_OPTION) === "0") {
            out = setConfigOptionValue(out, ALARM_PIN_DISARM_OPTION, "0");
        }
        var rawActions: any = configOptionValue(options, ALARM_ACTIONS_OPTION);
        if (rawActions) {
            var actions: any = normalizeAlarmActionList(rawActions);
            if (!alarmActionsAreDefault(actions)) {
                out = setConfigOptionValue(out, ALARM_ACTIONS_OPTION, actions.join("|"));
            }
        }
        var iconMode: any = normalizeAlarmIconDisplayMode(configOptionValue(options, ALARM_ICON_DISPLAY_OPTION));
        if (iconMode !== "status") {
            out = setConfigOptionValue(out, ALARM_ICON_DISPLAY_OPTION, iconMode);
        }
        var labelMode: any = normalizeAlarmLabelDisplayMode(configOptionValue(options, ALARM_LABEL_DISPLAY_OPTION));
        if (labelMode !== "status") {
            out = setConfigOptionValue(out, ALARM_LABEL_DISPLAY_OPTION, labelMode);
        }
        return out;
    }
    function parseClimatePrecisionConfig(this: any, value?: any) {
        var raw: any = String(value || "");
        var parts: any = raw.split(":");
        var precision: any = parts[0] || "";
        if (precision === "0")
            precision = "";
        if (climatePrecisionValues().indexOf(precision) < 0)
            precision = "";
        var min: any = parts.length > 1 ? sanitizeClimateRangeValue(parts[1]) : "";
        var max: any = parts.length > 2 ? sanitizeClimateRangeValue(parts[2]) : "";
        return { precision: precision, min: min, max: max };
    }
    function sanitizeClimateRangeValue(this: any, value?: any) {
        var text: any = String(value || "").trim();
        if (!text)
            return "";
        var num: any = Number(text);
        if (!isFinite(num))
            return "";
        return String(Math.round(num * 10) / 10).replace(/\.0$/, "");
    }
    function climatePrecisionConfig(this: any, precision?: any, min?: any, max?: any) {
        var p: any = climatePrecisionValues().indexOf(String(precision || "")) >= 0 ? String(precision || "") : "";
        var lo: any = sanitizeClimateRangeValue(min);
        var hi: any = sanitizeClimateRangeValue(max);
        if (!lo && !hi)
            return p;
        return (p || "0") + ":" + lo + ":" + hi;
    }
    function climatePrecisionValues(this: any) {
        var behavior: any = climateBehaviorSpec();
        var values: any = behavior && behavior.precisionValues;
        return values && values.length ? values.slice() : ["", "1", "2", "3"];
    }
    function climateBehaviorSpec(this: any) {
        var card: any = cardContractCard("climate");
        return card && card.behavior && card.behavior.climate || null;
    }
    function climateDefaultLabelDisplayMode(this: any) {
        var behavior: any = climateBehaviorSpec();
        var fallback: any = behavior && behavior.defaultLabelDisplay || "label";
        return cardContractOptionDefaultValue("climate", CLIMATE_LABEL_DISPLAY_OPTION, fallback);
    }
    function climateDefaultNumberDisplayMode(this: any) {
        var behavior: any = climateBehaviorSpec();
        var fallback: any = behavior && behavior.defaultNumberDisplay || "target";
        return cardContractOptionDefaultValue("climate", CLIMATE_NUMBER_DISPLAY_OPTION, fallback);
    }
    function climateDefaultTemperatureStep(this: any) {
        var behavior: any = climateBehaviorSpec();
        var fallback: any = behavior && behavior.defaultTemperatureStep || "1";
        return cardContractOptionDefaultValue("climate", CLIMATE_TEMPERATURE_STEP_OPTION, fallback);
    }
    function normalizeClimatePrecisionConfig(this: any, value?: any) {
        var parsed: any = parseClimatePrecisionConfig(value);
        return climatePrecisionConfig(parsed.precision, parsed.min, parsed.max);
    }
    return {
        "alarmBehaviorSpec": staticGlobal(alarmBehaviorSpec),
        "alarmActionSpecs": staticGlobal(alarmActionSpecs),
        "alarmDefaultActions": staticGlobal(alarmDefaultActions),
        "alarmMaxVisibleActions": staticGlobal(alarmMaxVisibleActions),
        "alarmActionLegacyIcon": staticGlobal(alarmActionLegacyIcon),
        "alarmActionIconIsGenerated": staticGlobal(alarmActionIconIsGenerated),
        "normalizeGarageLabelDisplayMode": staticGlobal(normalizeGarageLabelDisplayMode),
        "normalizeGarageOptions": staticGlobal(normalizeGarageOptions),
        "garageLabelDisplayMode": staticGlobal(garageLabelDisplayMode),
        "setGarageLabelDisplayMode": staticGlobal(setGarageLabelDisplayMode),
        "normalizeGateLabelDisplayMode": staticGlobal(normalizeGateLabelDisplayMode),
        "normalizeGateOptions": staticGlobal(normalizeGateOptions),
        "gateLabelDisplayMode": staticGlobal(gateLabelDisplayMode),
        "setGateLabelDisplayMode": staticGlobal(setGateLabelDisplayMode),
        "normalizeClimateLabelDisplayMode": staticGlobal(normalizeClimateLabelDisplayMode),
        "normalizeClimateNumberDisplayMode": staticGlobal(normalizeClimateNumberDisplayMode),
        "normalizeClimateTemperatureStep": staticGlobal(normalizeClimateTemperatureStep),
        "normalizeClimateOptions": staticGlobal(normalizeClimateOptions),
        "climateLabelDisplayMode": staticGlobal(climateLabelDisplayMode),
        "setClimateLabelDisplayMode": staticGlobal(setClimateLabelDisplayMode),
        "climateNumberDisplayMode": staticGlobal(climateNumberDisplayMode),
        "setClimateNumberDisplayMode": staticGlobal(setClimateNumberDisplayMode),
        "climateTemperatureStep": staticGlobal(climateTemperatureStep),
        "setClimateTemperatureStep": staticGlobal(setClimateTemperatureStep),
        "alarmActionInfo": staticGlobal(alarmActionInfo),
        "alarmActionValues": staticGlobal(alarmActionValues),
        "alarmPinRequired": staticGlobal(alarmPinRequired),
        "setAlarmPinRequired": staticGlobal(setAlarmPinRequired),
        "normalizeAlarmActionList": staticGlobal(normalizeAlarmActionList),
        "alarmVisibleActions": staticGlobal(alarmVisibleActions),
        "alarmActionsAreDefault": staticGlobal(alarmActionsAreDefault),
        "setAlarmVisibleActions": staticGlobal(setAlarmVisibleActions),
        "normalizeAlarmIconDisplayMode": staticGlobal(normalizeAlarmIconDisplayMode),
        "normalizeAlarmLabelDisplayMode": staticGlobal(normalizeAlarmLabelDisplayMode),
        "alarmIconDisplayMode": staticGlobal(alarmIconDisplayMode),
        "setAlarmIconDisplayMode": staticGlobal(setAlarmIconDisplayMode),
        "alarmLabelDisplayMode": staticGlobal(alarmLabelDisplayMode),
        "setAlarmLabelDisplayMode": staticGlobal(setAlarmLabelDisplayMode),
        "normalizeAlarmOptions": staticGlobal(normalizeAlarmOptions),
        "parseClimatePrecisionConfig": staticGlobal(parseClimatePrecisionConfig),
        "sanitizeClimateRangeValue": staticGlobal(sanitizeClimateRangeValue),
        "climatePrecisionConfig": staticGlobal(climatePrecisionConfig),
        "climatePrecisionValues": staticGlobal(climatePrecisionValues),
        "climateBehaviorSpec": staticGlobal(climateBehaviorSpec),
        "climateDefaultLabelDisplayMode": staticGlobal(climateDefaultLabelDisplayMode),
        "climateDefaultNumberDisplayMode": staticGlobal(climateDefaultNumberDisplayMode),
        "climateDefaultTemperatureStep": staticGlobal(climateDefaultTemperatureStep),
        "normalizeClimatePrecisionConfig": staticGlobal(normalizeClimatePrecisionConfig),
    };
}
