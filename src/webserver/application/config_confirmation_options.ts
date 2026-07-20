import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installConfigConfirmationOptionsModule(): GlobalDescriptors {
    // ── Confirmation Card Options ─────────────────────────────────────
    function switchConfirmationModeStorage(this: any) {
        var spec: any = cardContractOptionSpec("", "confirmation_mode");
        return spec && spec.storage && spec.storage.length >= 2
            ? spec.storage
            : [SWITCH_CONFIRM_OFF_OPTION, SWITCH_CONFIRM_ON_OPTION];
    }
    function normalizeCardOnPattern(this: any, value?: any) {
        value = String(value || "").trim();
        return value === "stripes" ? "stripes" : "";
    }
    function cardOnPattern(this: any, b?: any) {
        return normalizeCardOnPattern(configOptionValue(b && b.options, CARD_ON_PATTERN_OPTION));
    }
    function setCardOnPattern(this: any, b?: any, pattern?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, CARD_ON_PATTERN_OPTION, normalizeCardOnPattern(pattern));
        if (!b.type)
            b.options = normalizeSwitchConfirmationOptions(b.options);
        return b.options;
    }
    function switchConfirmationEnabled(this: any, b?: any) {
        return !!switchConfirmationMode(b);
    }
    function switchConfirmationMode(this: any, b?: any) {
        var options: any = b && b.options;
        var storage: any = switchConfirmationModeStorage();
        var confirmOff: any = configOptionEnabled(options, storage[0]);
        var confirmOn: any = configOptionEnabled(options, storage[1]);
        if (confirmOff && confirmOn)
            return "both";
        if (confirmOn)
            return "on";
        if (confirmOff)
            return "off";
        return "";
    }
    function switchConfirmationDefaultMessageForMode(this: any, mode?: any) {
        var spec: any = cardContractOptionSpec("", SWITCH_CONFIRM_MESSAGE_OPTION);
        var defaults: any = spec && spec.defaultValueByMode || {};
        if (mode && defaults[mode])
            return defaults[mode];
        return cardContractOptionDefaultValue("", SWITCH_CONFIRM_MESSAGE_OPTION, SWITCH_CONFIRM_DEFAULT_MESSAGE);
    }
    function switchConfirmationMessage(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_MESSAGE_OPTION) ||
            switchConfirmationDefaultMessageForMode(switchConfirmationMode(b));
    }
    function switchConfirmationYesText(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_YES_OPTION) ||
            cardContractOptionDefaultValue("", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES);
    }
    function switchConfirmationNoText(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_NO_OPTION) ||
            cardContractOptionDefaultValue("", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO);
    }
    function normalizeSwitchConfirmationOptions(this: any, options?: any) {
        var mode: any = switchConfirmationMode({ options: options });
        var out: any = copyCardBackgroundOptions("", options, { type: "" });
        out = copyLargeNumbersOption(out, options);
        var onPattern: any = normalizeCardOnPattern(configOptionValue(options, CARD_ON_PATTERN_OPTION));
        if (onPattern)
            out = setConfigOptionValue(out, CARD_ON_PATTERN_OPTION, onPattern);
        if (!mode)
            return out;
        var storage: any = switchConfirmationModeStorage();
        out = setConfigOption(out, storage[0], mode === "off" || mode === "both");
        out = setConfigOption(out, storage[1], mode === "on" || mode === "both");
        var msg: any = configOptionValue(options, SWITCH_CONFIRM_MESSAGE_OPTION);
        var yes: any = configOptionValue(options, SWITCH_CONFIRM_YES_OPTION);
        var no: any = configOptionValue(options, SWITCH_CONFIRM_NO_OPTION);
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
    function setSwitchConfirmationOptions(this: any, b?: any, mode?: any, message?: any, yesText?: any, noText?: any) {
        if (!b)
            return "";
        mode = mode === true ? "off" : mode;
        mode = mode === "on" || mode === "both" || mode === "off" ? mode : "";
        var out: any = "";
        out = copyLargeNumbersOption(out, b.options);
        out = copyCardBackgroundOptions(out, b.options, b);
        var storage: any = switchConfirmationModeStorage();
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
    function garageConfirmationModeStorage(this: any) {
        var spec: any = cardContractOptionSpec("garage", "confirmation_mode");
        return spec && spec.storage && spec.storage.length >= 2
            ? spec.storage
            : [SWITCH_CONFIRM_OFF_OPTION, SWITCH_CONFIRM_ON_OPTION];
    }
    function garageConfirmationEnabled(this: any, b?: any) {
        return !!garageConfirmationMode(b);
    }
    function garageConfirmationMode(this: any, b?: any) {
        var options: any = b && b.options;
        var storage: any = garageConfirmationModeStorage();
        var confirmClose: any = configOptionEnabled(options, storage[0]);
        var confirmOpen: any = configOptionEnabled(options, storage[1]);
        if (confirmClose && confirmOpen)
            return "both";
        if (confirmOpen)
            return "on";
        if (confirmClose)
            return "off";
        return "";
    }
    function garageConfirmationDefaultMessageForMode(this: any, mode?: any) {
        var spec: any = cardContractOptionSpec("garage", SWITCH_CONFIRM_MESSAGE_OPTION);
        var defaults: any = spec && spec.defaultValueByMode || {};
        if (mode && defaults[mode])
            return defaults[mode];
        return cardContractOptionDefaultValue("garage", SWITCH_CONFIRM_MESSAGE_OPTION, SWITCH_CONFIRM_DEFAULT_MESSAGE);
    }
    function garageConfirmationMessage(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_MESSAGE_OPTION) ||
            garageConfirmationDefaultMessageForMode(garageConfirmationMode(b));
    }
    function garageConfirmationYesText(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_YES_OPTION) ||
            cardContractOptionDefaultValue("garage", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES);
    }
    function garageConfirmationNoText(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_NO_OPTION) ||
            cardContractOptionDefaultValue("garage", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO);
    }
    function normalizeGarageConfirmationOptions(this: any, out?: any, options?: any, requestedMode?: any) {
        var storedMode: any = garageConfirmationMode({ options: options });
        var mode: any = storedMode && (requestedMode === "on" || requestedMode === "off")
            ? requestedMode
            : storedMode;
        if (!mode)
            return out;
        var storage: any = garageConfirmationModeStorage();
        out = setConfigOption(out, storage[0], mode === "off" || mode === "both");
        out = setConfigOption(out, storage[1], mode === "on" || mode === "both");
        var msg: any = configOptionValue(options, SWITCH_CONFIRM_MESSAGE_OPTION);
        var yes: any = configOptionValue(options, SWITCH_CONFIRM_YES_OPTION);
        var no: any = configOptionValue(options, SWITCH_CONFIRM_NO_OPTION);
        var storedDefaultMessage: any = garageConfirmationDefaultMessageForMode(storedMode);
        if (msg && msg !== garageConfirmationDefaultMessageForMode(mode) && msg !== storedDefaultMessage) {
            out = setConfigOptionValue(out, SWITCH_CONFIRM_MESSAGE_OPTION, msg);
        }
        if (yes && yes !== cardContractOptionDefaultValue("garage", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES)) {
            out = setConfigOptionValue(out, SWITCH_CONFIRM_YES_OPTION, yes);
        }
        if (no && no !== cardContractOptionDefaultValue("garage", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO)) {
            out = setConfigOptionValue(out, SWITCH_CONFIRM_NO_OPTION, no);
        }
        return out;
    }
    function setGarageConfirmationOptions(this: any, b?: any, mode?: any, message?: any, yesText?: any, noText?: any) {
        if (!b)
            return "";
        mode = mode === true ? "off" : mode;
        mode = mode === "on" || mode === "both" || mode === "off" ? mode : "";
        var storage: any = garageConfirmationModeStorage();
        b.options = setConfigOption(b.options, storage[0], mode === "off" || mode === "both");
        b.options = setConfigOption(b.options, storage[1], mode === "on" || mode === "both");
        var msgDefault: any = mode ? garageConfirmationDefaultMessageForMode(mode) : "";
        b.options = setConfigOptionValue(b.options, SWITCH_CONFIRM_MESSAGE_OPTION, mode && message && message !== msgDefault ? message : "");
        b.options = setConfigOptionValue(b.options, SWITCH_CONFIRM_YES_OPTION, mode && yesText && yesText !== cardContractOptionDefaultValue("garage", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES) ? yesText : "");
        b.options = setConfigOptionValue(b.options, SWITCH_CONFIRM_NO_OPTION, mode && noText && noText !== cardContractOptionDefaultValue("garage", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO) ? noText : "");
        b.options = normalizeGarageOptions(b.options, b.sensor);
        return b.options;
    }
    function actionCardIsScript(this: any, b?: any) {
        var value: any = typeof b === "string" ? b : b && b.sensor;
        return value === "script.turn_on";
    }
    function actionScriptConfirmationEnabled(this: any, b?: any) {
        return !!(b && actionCardIsScript(b) &&
            configOptionEnabled(b.options, SWITCH_CONFIRM_ON_OPTION));
    }
    function actionScriptConfirmationDefaultMessage(this: any) {
        return cardContractOptionDefaultValue("action", SWITCH_CONFIRM_MESSAGE_OPTION, ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE);
    }
    function actionScriptConfirmationMessage(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_MESSAGE_OPTION) ||
            actionScriptConfirmationDefaultMessage();
    }
    function actionScriptConfirmationYesText(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_YES_OPTION) ||
            cardContractOptionDefaultValue("action", SWITCH_CONFIRM_YES_OPTION, SWITCH_CONFIRM_DEFAULT_YES);
    }
    function actionScriptConfirmationNoText(this: any, b?: any) {
        return configOptionValue(b && b.options, SWITCH_CONFIRM_NO_OPTION) ||
            cardContractOptionDefaultValue("action", SWITCH_CONFIRM_NO_OPTION, SWITCH_CONFIRM_DEFAULT_NO);
    }
    function actionScriptFields(this: any, b?: any) {
        return actionCardIsScript(b) ? configOptionValue(b && b.options, ACTION_SCRIPT_FIELDS_OPTION) : "";
    }
    function copyActionCardStateOptions(this: any, out?: any, options?: any) {
        var stateEntity: any = configOptionValue(options, ACTION_CARD_STATE_ENTITY_OPTION).trim();
        if (!stateEntity)
            return out;
        out = setConfigOptionValue(out, ACTION_CARD_STATE_ENTITY_OPTION, stateEntity);
        var rawPrecision: any = configOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION);
        if (rawPrecision === "icon" || rawPrecision === "text") {
            out = setConfigOptionValue(out, ACTION_CARD_STATE_PRECISION_OPTION, rawPrecision);
            return out;
        }
        var stateUnit: any = configOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION).trim();
        if (!stateUnit && rawPrecision !== "0" && rawPrecision !== "1" && rawPrecision !== "2") {
            return copyLargeNumbersOption(out, options);
        }
        var statePrecision: any = rawPrecision === "1" || rawPrecision === "2" ? rawPrecision : "0";
        if (stateUnit)
            out = setConfigOptionValue(out, ACTION_CARD_STATE_UNIT_OPTION, stateUnit);
        if (rawPrecision === "0" || statePrecision !== "0") {
            out = setConfigOptionValue(out, ACTION_CARD_STATE_PRECISION_OPTION, statePrecision);
        }
        out = copyLargeNumbersOption(out, options);
        return out;
    }
    function normalizeActionOptions(this: any, options?: any, action?: any) {
        var out: any = copyCardBackgroundOptions("", options, { type: "action" });
        if (action === ACTION_CARD_LOCAL_ACTION)
            return out;
        out = copyActionCardStateOptions(out, options);
        if (action !== "script.turn_on") {
            return out;
        }
        var fields: any = configOptionValue(options, ACTION_SCRIPT_FIELDS_OPTION).trim();
        if (fields)
            out = setConfigOptionValue(out, ACTION_SCRIPT_FIELDS_OPTION, fields);
        if (!configOptionEnabled(options, SWITCH_CONFIRM_ON_OPTION))
            return out;
        out = setConfigOption(out, SWITCH_CONFIRM_ON_OPTION, true);
        var msg: any = configOptionValue(options, SWITCH_CONFIRM_MESSAGE_OPTION).trim();
        var yes: any = configOptionValue(options, SWITCH_CONFIRM_YES_OPTION).trim();
        var no: any = configOptionValue(options, SWITCH_CONFIRM_NO_OPTION).trim();
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
    function setActionScriptConfirmationOptions(this: any, b?: any, enabled?: any, message?: any, yesText?: any, noText?: any) {
        if (!b)
            return "";
        var out: any = copyCardBackgroundOptions("", b.options, b);
        out = copyActionCardStateOptions(out, b.options);
        var fields: any = actionScriptFields(b);
        if (fields)
            out = setConfigOptionValue(out, ACTION_SCRIPT_FIELDS_OPTION, fields);
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
    function setActionScriptFields(this: any, b?: any, fields?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, ACTION_SCRIPT_FIELDS_OPTION, fields || "");
        b.options = normalizeActionOptions(b.options, b.sensor);
        return b.options;
    }
    return {
        "switchConfirmationModeStorage": staticGlobal(switchConfirmationModeStorage),
        "normalizeCardOnPattern": staticGlobal(normalizeCardOnPattern),
        "cardOnPattern": staticGlobal(cardOnPattern),
        "setCardOnPattern": staticGlobal(setCardOnPattern),
        "switchConfirmationEnabled": staticGlobal(switchConfirmationEnabled),
        "switchConfirmationMode": staticGlobal(switchConfirmationMode),
        "switchConfirmationDefaultMessageForMode": staticGlobal(switchConfirmationDefaultMessageForMode),
        "switchConfirmationMessage": staticGlobal(switchConfirmationMessage),
        "switchConfirmationYesText": staticGlobal(switchConfirmationYesText),
        "switchConfirmationNoText": staticGlobal(switchConfirmationNoText),
        "normalizeSwitchConfirmationOptions": staticGlobal(normalizeSwitchConfirmationOptions),
        "setSwitchConfirmationOptions": staticGlobal(setSwitchConfirmationOptions),
        "garageConfirmationModeStorage": staticGlobal(garageConfirmationModeStorage),
        "garageConfirmationEnabled": staticGlobal(garageConfirmationEnabled),
        "garageConfirmationMode": staticGlobal(garageConfirmationMode),
        "garageConfirmationDefaultMessageForMode": staticGlobal(garageConfirmationDefaultMessageForMode),
        "garageConfirmationMessage": staticGlobal(garageConfirmationMessage),
        "garageConfirmationYesText": staticGlobal(garageConfirmationYesText),
        "garageConfirmationNoText": staticGlobal(garageConfirmationNoText),
        "normalizeGarageConfirmationOptions": staticGlobal(normalizeGarageConfirmationOptions),
        "setGarageConfirmationOptions": staticGlobal(setGarageConfirmationOptions),
        "actionCardIsScript": staticGlobal(actionCardIsScript),
        "actionScriptConfirmationEnabled": staticGlobal(actionScriptConfirmationEnabled),
        "actionScriptConfirmationDefaultMessage": staticGlobal(actionScriptConfirmationDefaultMessage),
        "actionScriptConfirmationMessage": staticGlobal(actionScriptConfirmationMessage),
        "actionScriptConfirmationYesText": staticGlobal(actionScriptConfirmationYesText),
        "actionScriptConfirmationNoText": staticGlobal(actionScriptConfirmationNoText),
        "actionScriptFields": staticGlobal(actionScriptFields),
        "copyActionCardStateOptions": staticGlobal(copyActionCardStateOptions),
        "normalizeActionOptions": staticGlobal(normalizeActionOptions),
        "setActionScriptConfirmationOptions": staticGlobal(setActionScriptConfirmationOptions),
        "setActionScriptFields": staticGlobal(setActionScriptFields),
    };
}
