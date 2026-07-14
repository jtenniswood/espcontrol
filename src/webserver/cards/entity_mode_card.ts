import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerEntityModeCardHelpers(): GlobalDescriptors {
    function entityModeValues(this: any, cardType?: any, optionName?: any, fallbackModes?: any) {
        var spec: any = cardContractOptionSpec(cardType, optionName);
        return spec && spec.values ? spec.values.slice() : fallbackModes.map(function (this: any, entry?: any) { return entry[0]; });
    }
    function normalizeEntityMode(this: any, mode?: any, values?: any, fallback?: any) {
        mode = String(mode || "");
        return values.indexOf(mode) >= 0 ? mode : fallback;
    }
    function entityModeCardUsesDefaultIcon(this: any, icon?: any, icons?: any) {
        if (!icon || icon === "Auto")
            return true;
        return icons.indexOf(icon) >= 0;
    }
    function normalizeEntityModeCardConfig(this: any, b?: any, options?: any) {
        if (!b)
            return;
        var mode: any = options.normalizeMode(b.sensor);
        b.sensor = mode;
        if (options.keepUnit && options.keepUnit(mode)) {
            b.unit = b.unit || "";
        }
        else {
            b.unit = "";
        }
        b.precision = "";
        b.options = "";
        b.icon_on = "Auto";
        if (!b.icon || b.icon === "Auto")
            b.icon = options.defaultIcon(mode);
    }
    function applyEntityModeCardModeChange(this: any, b?: any, helpers?: any, previousMode?: any, nextMode?: any, options?: any) {
        var hadDefaultIcon: any = options.usesDefaultIcon(b.icon);
        b.sensor = nextMode;
        if (options.keepUnit && options.keepUnit(nextMode)) {
            b.unit = b.unit || "";
        }
        else {
            b.unit = "";
            helpers.saveField("unit", "");
        }
        b.precision = "";
        b.options = "";
        b.icon_on = "Auto";
        helpers.saveField("sensor", nextMode);
        helpers.saveField("precision", "");
        helpers.saveField("options", "");
        helpers.saveField("icon_on", "Auto");
        if (hadDefaultIcon || b.icon === options.defaultIcon(previousMode)) {
            b.icon = options.defaultIcon(nextMode);
            helpers.saveField("icon", b.icon);
        }
    }
    return {
        "entityModeValues": staticGlobal(entityModeValues),
        "normalizeEntityMode": staticGlobal(normalizeEntityMode),
        "entityModeCardUsesDefaultIcon": staticGlobal(entityModeCardUsesDefaultIcon),
        "normalizeEntityModeCardConfig": staticGlobal(normalizeEntityModeCardConfig),
        "applyEntityModeCardModeChange": staticGlobal(applyEntityModeCardModeChange),
    };
}
