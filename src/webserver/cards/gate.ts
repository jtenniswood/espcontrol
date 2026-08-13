import { cardContractDomains } from "../generated/card_contract";
import type { CoverLikeCardRegistration } from "./cover_like_card";
import type { ConfigAccessClimateAlarmOptionsFeature } from "../application/config_access_climate_alarm_options";
export function registerGateCardTypes(
    registerCard: CoverLikeCardRegistration["register"],
    accessOptions: ConfigAccessClimateAlarmOptionsFeature,
): void {
    const {
        normalizeGateOptions,
        gateModeOptionValues,
        normalizeGateMode,
        gateLabelDisplayMode,
        setGateLabelDisplayMode,
    } = accessOptions;
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
            label: "Type",
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
    registerCard({
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
}
