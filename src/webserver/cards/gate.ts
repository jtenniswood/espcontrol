import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerGateCardTypes(): GlobalDescriptors {
    // Gate card: cover toggle or one-tap open/close/stop commands.
    var GATE_MODE_OPTIONS: any = [
        ["", "Toggle"],
        ["open", "Open"],
        ["close", "Close"],
        ["stop", "Stop"],
    ];
    function gateCommandMode(this: any, mode?: any) {
        return mode === "open" || mode === "close" || mode === "stop";
    }
    function gateModeOptionValues(this: any) {
        return coverLikeModeValues("gate", "gate_mode", GATE_MODE_OPTIONS);
    }
    function normalizeGateMode(this: any, mode?: any) {
        return normalizeCoverLikeMode(mode, gateModeOptionValues());
    }
    function gateModeDefaultIcon(this: any, mode?: any) {
        if (mode === "open")
            return "Gate Open";
        if (mode === "stop")
            return "Stop";
        return "Gate";
    }
    function gateModeDefaultLabel(this: any, mode?: any) {
        if (mode === "open")
            return "Open";
        if (mode === "close")
            return "Close";
        if (mode === "stop")
            return "Stop";
        return "Gate";
    }
    function gateUsesDefaultIcon(this: any, icon?: any) {
        return !icon || icon === "Auto" || icon === "Gate" || icon === "Gate Open" || icon === "Stop";
    }
    var GATE_CARD_METADATA: any = {
        mode: {
            label: "Interaction",
            idSuffix: "gate-interaction",
            options: GATE_MODE_OPTIONS,
            value: function (this: any, b?: any) {
                return normalizeGateMode(b.sensor);
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
            placeholder: "e.g. cover.driveway_gate",
            domains: function (this: any) { return cardContractDomains("gate"); },
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
            badge: "gate",
        },
    };
    registerCoverLikeCardType({
        type: "gate",
        optionName: "gate_mode",
        metadata: GATE_CARD_METADATA,
        commandModes: ["open", "close", "stop"],
        closedIcon: "Gate",
        openIcon: "Gate Open",
        shortLabel: "Gate",
        defaultCardLabel: "Gate",
        labelPlaceholder: "e.g. Gate",
        defaultIcon: gateModeDefaultIcon,
        defaultLabel: gateModeDefaultLabel,
        usesDefaultIcon: gateUsesDefaultIcon,
        normalizeOptions: normalizeGateOptions,
        labelDisplayMode: gateLabelDisplayMode,
        setLabelDisplayMode: setGateLabelDisplayMode,
    });
    return {
        "GATE_MODE_OPTIONS": liveGlobal(() => GATE_MODE_OPTIONS, (value?: any) => { GATE_MODE_OPTIONS = value; }),
        "gateCommandMode": staticGlobal(gateCommandMode),
        "gateModeOptionValues": staticGlobal(gateModeOptionValues),
        "normalizeGateMode": staticGlobal(normalizeGateMode),
        "gateModeDefaultIcon": staticGlobal(gateModeDefaultIcon),
        "gateModeDefaultLabel": staticGlobal(gateModeDefaultLabel),
        "gateUsesDefaultIcon": staticGlobal(gateUsesDefaultIcon),
        "GATE_CARD_METADATA": liveGlobal(() => GATE_CARD_METADATA, (value?: any) => { GATE_CARD_METADATA = value; }),
    };
}
