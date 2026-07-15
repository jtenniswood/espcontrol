import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerGarageCardTypes(): GlobalDescriptors {
    // Garage door card: cover toggle or one-tap open/close commands.
    var GARAGE_MODE_OPTIONS: any = [
        ["", "Toggle"],
        ["open", "Open"],
        ["close", "Close"],
    ];
    function garageCommandMode(this: any, mode?: any) {
        return mode === "open" || mode === "close";
    }
    function garageModeOptionValues(this: any) {
        return coverLikeModeValues("garage", "garage_mode", GARAGE_MODE_OPTIONS);
    }
    function normalizeGarageMode(this: any, mode?: any) {
        return normalizeCoverLikeMode(mode, garageModeOptionValues());
    }
    function garageModeDefaultIcon(this: any, mode?: any) {
        return mode === "open" ? "Garage Open" : "Garage";
    }
    function garageModeDefaultLabel(this: any, mode?: any) {
        if (mode === "open")
            return "Open";
        if (mode === "close")
            return "Close";
        return "Garage Door";
    }
    function garageUsesDefaultIcon(this: any, icon?: any) {
        return !icon || icon === "Auto" || icon === "Garage" || icon === "Garage Open";
    }
    var GARAGE_CARD_METADATA: any = {
        mode: {
            label: "Interaction",
            idSuffix: "garage-interaction",
            options: GARAGE_MODE_OPTIONS,
            value: function (this: any, b?: any) {
                return normalizeGarageMode(b.sensor);
            },
        },
        display: {
            label: "Display",
            options: [
                ["label", "Label"],
                ["status", "Status"],
            ],
        },
        entity: {
            label: "Entity",
            idSuffix: "entity",
            placeholder: "e.g. cover.garage_door",
            domains: function (this: any) { return cardContractDomains("garage"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add an entity before saving.",
        },
        labelField: {
            label: "Label",
            idSuffix: "label",
            field: "label",
            rerender: true,
        },
        preview: {
            badge: "garage",
        },
    };
    registerCoverLikeCardType({
        type: "garage",
        optionName: "garage_mode",
        metadata: GARAGE_CARD_METADATA,
        commandModes: ["open", "close"],
        closedIcon: "Garage",
        openIcon: "Garage Open",
        shortLabel: "Garage",
        defaultCardLabel: "Garage Door",
        labelPlaceholder: "e.g. Garage Door",
        defaultIcon: garageModeDefaultIcon,
        defaultLabel: garageModeDefaultLabel,
        usesDefaultIcon: garageUsesDefaultIcon,
        normalizeOptions: normalizeGarageOptions,
        labelDisplayMode: garageLabelDisplayMode,
        setLabelDisplayMode: setGarageLabelDisplayMode,
    });
    return {
        "GARAGE_MODE_OPTIONS": liveGlobal(() => GARAGE_MODE_OPTIONS, (value?: any) => { GARAGE_MODE_OPTIONS = value; }),
        "garageCommandMode": staticGlobal(garageCommandMode),
        "garageModeOptionValues": staticGlobal(garageModeOptionValues),
        "normalizeGarageMode": staticGlobal(normalizeGarageMode),
        "garageModeDefaultIcon": staticGlobal(garageModeDefaultIcon),
        "garageModeDefaultLabel": staticGlobal(garageModeDefaultLabel),
        "garageUsesDefaultIcon": staticGlobal(garageUsesDefaultIcon),
        "GARAGE_CARD_METADATA": liveGlobal(() => GARAGE_CARD_METADATA, (value?: any) => { GARAGE_CARD_METADATA = value; }),
    };
}
