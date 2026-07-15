import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installConfigSensorOptionsModule(): GlobalDescriptors {
    // ── Sensor Card Options ────────────────────────────────────────────
    function cardLargeNumbersSupported(this: any, b?: any) {
        if (!b)
            return false;
        if (typeof BUTTON_TYPES !== "undefined") {
            var typeDef: any = BUTTON_TYPES[b.type || ""];
            var large: any = typeDef && typeDef.cardMetadata && typeDef.cardMetadata.largeNumbers;
            if (large) {
                return typeof large.supported === "function" ? !!large.supported(b) : large.supported !== false;
            }
        }
        return cardContractLargeNumbersSupported(b.type, b.precision);
    }
    function cardLargeNumbersEnabled(this: any, b?: any) {
        return !!(b && cardLargeNumbersSupported(b) &&
            configOptionEnabled(b.options, SENSOR_LARGE_NUMBERS_OPTION));
    }
    function sensorLargeNumbersEnabled(this: any, b?: any) {
        return cardLargeNumbersEnabled(b);
    }
    function setSensorLargeNumbersEnabled(this: any, b?: any, enabled?: any) {
        if (!b)
            return "";
        b.options = setConfigOption(b.options, SENSOR_LARGE_NUMBERS_OPTION, false);
        b.options = setConfigOptionValue(b.options, SENSOR_LARGE_NUMBERS_OPTION, enabled ? "" : SENSOR_LARGE_NUMBERS_OFF_VALUE);
        if (enabled)
            b.options = setConfigOption(b.options, SENSOR_LARGE_NUMBERS_OPTION, true);
        return b.options;
    }
    function normalizeSensorTimeUnit(this: any, value?: any) {
        value = String(value || "").trim().toLowerCase();
        return value === "seconds" || value === "minutes" || value === "hours" || value === "days" ? value : "";
    }
    function sensorTimeUnit(this: any, b?: any) {
        return normalizeSensorTimeUnit(b ? configOptionValue(b.options, SENSOR_TIME_UNIT_OPTION) : "");
    }
    function setSensorTimeUnit(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, SENSOR_TIME_UNIT_OPTION, normalizeSensorTimeUnit(value));
        b.options = normalizeSensorOptions(b.options, b.precision);
        return b.options;
    }
    function sensorActiveColorEnabled(this: any, b?: any) {
        return !!(b && b.type === "sensor" &&
            configOptionEnabled(b.options, SENSOR_ACTIVE_COLOR_OPTION));
    }
    function setSensorActiveColorEnabled(this: any, b?: any, enabled?: any) {
        if (!b)
            return "";
        b.options = setConfigOption(b.options, SENSOR_ACTIVE_COLOR_OPTION, enabled);
        return b.options;
    }
    function sensorStateLabelsEnabled(this: any, b?: any) {
        return !!(b && b.type === "sensor" && b.precision === "text" &&
            configOptionEnabled(b.options, SENSOR_STATE_LABELS_OPTION));
    }
    function legacySensorStateInput(this: any, options?: any) {
        if (configOptionValue(options, SENSOR_STATE_HIGH_LABEL_OPTION))
            return "high";
        if (configOptionValue(options, SENSOR_STATE_LOW_LABEL_OPTION))
            return "low";
        return "";
    }
    function legacySensorStateOutput(this: any, options?: any) {
        if (configOptionValue(options, SENSOR_STATE_HIGH_LABEL_OPTION)) {
            return configOptionValue(options, SENSOR_STATE_HIGH_LABEL_OPTION);
        }
        if (configOptionValue(options, SENSOR_STATE_LOW_LABEL_OPTION)) {
            return configOptionValue(options, SENSOR_STATE_LOW_LABEL_OPTION);
        }
        return "";
    }
    function sensorStateInput(this: any, b?: any) {
        return sensorStateLabelsEnabled(b)
            ? (configOptionValue(b.options, SENSOR_STATE_INPUT_OPTION) || legacySensorStateInput(b.options))
            : "";
    }
    function sensorStateOutput(this: any, b?: any) {
        return sensorStateLabelsEnabled(b)
            ? (configOptionValue(b.options, SENSOR_STATE_OUTPUT_OPTION) || legacySensorStateOutput(b.options))
            : "";
    }
    function sensorStateInput2(this: any, b?: any) {
        return sensorStateLabelsEnabled(b)
            ? configOptionValue(b.options, SENSOR_STATE_INPUT_2_OPTION)
            : "";
    }
    function sensorStateOutput2(this: any, b?: any) {
        return sensorStateLabelsEnabled(b)
            ? configOptionValue(b.options, SENSOR_STATE_OUTPUT_2_OPTION)
            : "";
    }
    function setSensorStateTranslation(this: any, b?: any, enabled?: any, inputText?: any, outputText?: any) {
        return setSensorStateTranslations(b, enabled, inputText, outputText, "", "");
    }
    function setSensorStateTranslations(this: any, b?: any, enabled?: any, inputText?: any, outputText?: any, inputText2?: any, outputText2?: any) {
        if (!b)
            return "";
        var options: any = b.options || "";
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
    function normalizeSensorOptions(this: any, options?: any, precision?: any) {
        var out: any = "";
        if (configOptionEnabled(options, SENSOR_LARGE_NUMBERS_OPTION) &&
            cardContractOptionSupportedFor("sensor", SENSOR_LARGE_NUMBERS_OPTION, { precision: precision })) {
            out = copyLargeNumbersOption(out, options);
        }
        else if (largeNumbersExplicitlyDisabled(options) &&
            cardContractOptionSupportedFor("sensor", SENSOR_LARGE_NUMBERS_OPTION, { precision: precision })) {
            out = copyLargeNumbersOption(out, options);
        }
        if (configOptionEnabled(options, SENSOR_ACTIVE_COLOR_OPTION) &&
            cardContractOptionSupportedFor("sensor", SENSOR_ACTIVE_COLOR_OPTION, { precision: precision })) {
            out = setConfigOption(out, SENSOR_ACTIVE_COLOR_OPTION, true);
        }
        if (precision === "text" && configOptionEnabled(options, SENSOR_STATE_LABELS_OPTION)) {
            out = setConfigOption(out, SENSOR_STATE_LABELS_OPTION, true);
            out = setConfigOptionValue(out, SENSOR_STATE_INPUT_OPTION, configOptionValue(options, SENSOR_STATE_INPUT_OPTION) || legacySensorStateInput(options));
            out = setConfigOptionValue(out, SENSOR_STATE_OUTPUT_OPTION, configOptionValue(options, SENSOR_STATE_OUTPUT_OPTION) || legacySensorStateOutput(options));
            out = setConfigOptionValue(out, SENSOR_STATE_INPUT_2_OPTION, configOptionValue(options, SENSOR_STATE_INPUT_2_OPTION));
            out = setConfigOptionValue(out, SENSOR_STATE_OUTPUT_2_OPTION, configOptionValue(options, SENSOR_STATE_OUTPUT_2_OPTION));
        }
        if (precision === "time")
            out = setConfigOptionValue(out, SENSOR_TIME_UNIT_OPTION, normalizeSensorTimeUnit(configOptionValue(options, SENSOR_TIME_UNIT_OPTION)));
        return out;
    }
    function normalizeDateTimeOptions(this: any, type?: any, options?: any, precision?: any) {
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
    function normalizeDoorWindowSubtype(this: any, value?: any) {
        value = String(value || "").trim();
        return value === "window" ? "window" : "door";
    }
    function doorWindowClosedIcon(this: any, subtype?: any) {
        return normalizeDoorWindowSubtype(subtype) === "window" ? "Window Closed" : "Door";
    }
    function doorWindowOpenIcon(this: any, subtype?: any) {
        return normalizeDoorWindowSubtype(subtype) === "window" ? "Window Open" : "Door Open";
    }
    function doorWindowActiveColorEnabled(this: any, b?: any) {
        return !!(b && b.type === "door_window" &&
            configOptionEnabled(b.options, SENSOR_ACTIVE_COLOR_OPTION));
    }
    function setDoorWindowActiveColorEnabled(this: any, b?: any, enabled?: any) {
        if (!b)
            return "";
        b.options = setConfigOption(b.options, SENSOR_ACTIVE_COLOR_OPTION, enabled);
        return b.options;
    }
    function normalizeDoorWindowOptions(this: any, options?: any) {
        var out: any = "";
        if (configOptionEnabled(options, SENSOR_ACTIVE_COLOR_OPTION)) {
            out = setConfigOption(out, SENSOR_ACTIVE_COLOR_OPTION, true);
        }
        return out;
    }
    function presenceActiveColorEnabled(this: any, b?: any) {
        return !!(b && b.type === "presence" &&
            configOptionEnabled(b.options, SENSOR_ACTIVE_COLOR_OPTION));
    }
    function setPresenceActiveColorEnabled(this: any, b?: any, enabled?: any) {
        if (!b)
            return "";
        b.options = setConfigOption(b.options, SENSOR_ACTIVE_COLOR_OPTION, enabled);
        return b.options;
    }
    function normalizePresenceOptions(this: any, options?: any) {
        var out: any = "";
        if (configOptionEnabled(options, SENSOR_ACTIVE_COLOR_OPTION)) {
            out = setConfigOption(out, SENSOR_ACTIVE_COLOR_OPTION, true);
        }
        return out;
    }
    function normalizeTodoCountDisplay(this: any, value?: any) {
        value = String(value || "").trim();
        return value === "icon" ? "icon" : "count";
    }
    function normalizeTodoOptions(this: any, options?: any) {
        var showCount: any = normalizeTodoCountDisplay(configOptionValue(options, "count_display")) === "count";
        var out: any = showCount ? "" : setConfigOptionValue("", "count_display", "icon");
        if (showCount)
            out = copyLargeNumbersOption(out, options);
        return out;
    }
    return {
        "cardLargeNumbersSupported": staticGlobal(cardLargeNumbersSupported),
        "cardLargeNumbersEnabled": staticGlobal(cardLargeNumbersEnabled),
        "sensorLargeNumbersEnabled": staticGlobal(sensorLargeNumbersEnabled),
        "setSensorLargeNumbersEnabled": staticGlobal(setSensorLargeNumbersEnabled),
        "normalizeSensorTimeUnit": staticGlobal(normalizeSensorTimeUnit),
        "sensorTimeUnit": staticGlobal(sensorTimeUnit),
        "setSensorTimeUnit": staticGlobal(setSensorTimeUnit),
        "sensorActiveColorEnabled": staticGlobal(sensorActiveColorEnabled),
        "setSensorActiveColorEnabled": staticGlobal(setSensorActiveColorEnabled),
        "sensorStateLabelsEnabled": staticGlobal(sensorStateLabelsEnabled),
        "legacySensorStateInput": staticGlobal(legacySensorStateInput),
        "legacySensorStateOutput": staticGlobal(legacySensorStateOutput),
        "sensorStateInput": staticGlobal(sensorStateInput),
        "sensorStateOutput": staticGlobal(sensorStateOutput),
        "sensorStateInput2": staticGlobal(sensorStateInput2),
        "sensorStateOutput2": staticGlobal(sensorStateOutput2),
        "setSensorStateTranslation": staticGlobal(setSensorStateTranslation),
        "setSensorStateTranslations": staticGlobal(setSensorStateTranslations),
        "normalizeSensorOptions": staticGlobal(normalizeSensorOptions),
        "normalizeDateTimeOptions": staticGlobal(normalizeDateTimeOptions),
        "normalizeDoorWindowSubtype": staticGlobal(normalizeDoorWindowSubtype),
        "doorWindowClosedIcon": staticGlobal(doorWindowClosedIcon),
        "doorWindowOpenIcon": staticGlobal(doorWindowOpenIcon),
        "doorWindowActiveColorEnabled": staticGlobal(doorWindowActiveColorEnabled),
        "setDoorWindowActiveColorEnabled": staticGlobal(setDoorWindowActiveColorEnabled),
        "normalizeDoorWindowOptions": staticGlobal(normalizeDoorWindowOptions),
        "presenceActiveColorEnabled": staticGlobal(presenceActiveColorEnabled),
        "setPresenceActiveColorEnabled": staticGlobal(setPresenceActiveColorEnabled),
        "normalizePresenceOptions": staticGlobal(normalizePresenceOptions),
        "normalizeTodoCountDisplay": staticGlobal(normalizeTodoCountDisplay),
        "normalizeTodoOptions": staticGlobal(normalizeTodoOptions),
    };
}
