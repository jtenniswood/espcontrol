import { cardContractOptionSpec } from "../application/config_option_core";

export function entityModeValues(this: any, cardType?: any, optionName?: any, fallbackModes?: any) {
    const spec: any = cardContractOptionSpec(cardType, optionName);
    return spec && spec.values ? spec.values.slice() : fallbackModes.map(function (entry?: any) { return entry[0]; });
}

export function normalizeEntityMode(this: any, mode?: any, values?: any, fallback?: any) {
    mode = String(mode || "");
    return values.indexOf(mode) >= 0 ? mode : fallback;
}

export function entityModeCardUsesDefaultIcon(this: any, icon?: any, icons?: any) {
    if (!icon || icon === "Auto")
        return true;
    return icons.indexOf(icon) >= 0;
}

export function normalizeEntityModeCardConfig(this: any, button?: any, options?: any) {
    if (!button)
        return;
    const mode: any = options.normalizeMode(button.sensor);
    button.sensor = mode;
    if (options.keepUnit && options.keepUnit(mode)) {
        button.unit = button.unit || "";
    }
    else {
        button.unit = "";
    }
    button.precision = "";
    button.options = "";
    button.icon_on = "Auto";
    if (!button.icon || button.icon === "Auto")
        button.icon = options.defaultIcon(mode);
}

export function applyEntityModeCardModeChange(
    this: any,
    button?: any,
    helpers?: any,
    previousMode?: any,
    nextMode?: any,
    options?: any,
) {
    const hadDefaultIcon: any = options.usesDefaultIcon(button.icon);
    button.sensor = nextMode;
    if (options.keepUnit && options.keepUnit(nextMode)) {
        button.unit = button.unit || "";
    }
    else {
        button.unit = "";
        helpers.saveField("unit", "");
    }
    button.precision = "";
    button.options = "";
    button.icon_on = "Auto";
    helpers.saveField("sensor", nextMode);
    helpers.saveField("precision", "");
    helpers.saveField("options", "");
    helpers.saveField("icon_on", "Auto");
    if (hadDefaultIcon || button.icon === options.defaultIcon(previousMode)) {
        button.icon = options.defaultIcon(nextMode);
        helpers.saveField("icon", button.icon);
    }
}
