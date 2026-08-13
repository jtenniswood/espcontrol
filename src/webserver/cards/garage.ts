import { cardContractDomains } from "../generated/card_contract";
import type { CoverLikeCardRegistration } from "./cover_like_card";
import type { ConfigAccessClimateAlarmOptionsFeature } from "../application/config_access_climate_alarm_options";
import type { ConfigConfirmationOptionsFeature } from "../application/config_confirmation_options";
import {
    SWITCH_CONFIRM_DEFAULT_MESSAGE,
    SWITCH_CONFIRM_DEFAULT_NO,
    SWITCH_CONFIRM_DEFAULT_YES,
} from "../application/config_option_core";
export function registerGarageCardTypes(
    registerCard: CoverLikeCardRegistration["register"],
    accessOptions: ConfigAccessClimateAlarmOptionsFeature,
    confirmationOptions: ConfigConfirmationOptionsFeature,
): void {
    const {
        normalizeGarageOptions,
        garageModeOptionValues,
        normalizeGarageMode,
        garageLabelDisplayMode,
        setGarageLabelDisplayMode,
    } = accessOptions;
    const {
        garageConfirmationDefaultMessageForMode,
        garageConfirmationEnabled,
        garageConfirmationMessage,
        garageConfirmationMode,
        garageConfirmationNoText,
        garageConfirmationYesText,
        setGarageConfirmationOptions,
    } = confirmationOptions;
    // Garage door card: cover toggle or one-tap open/close commands.
    var GARAGE_MODE_OPTIONS: any = [
        ["", "Toggle"],
        ["open", "Open"],
        ["close", "Close"],
    ];
    function garageCommandMode(this: any, mode?: any) {
        return mode === "open" || mode === "close";
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
            label: "Type",
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
        confirmationToggle: {
            label: "Confirmation Required",
            idSuffix: "garage-confirm-toggle",
            checked: function (this: any, b?: any) { return garageConfirmationEnabled(b); },
        },
        confirmationMode: {
            label: "When",
            options: [
                ["off", "Close"],
                ["on", "Open"],
                ["both", "Both"],
            ],
        },
        confirmationMessage: {
            label: "Message",
            idSuffix: "garage-confirm-message",
            placeholder: SWITCH_CONFIRM_DEFAULT_MESSAGE,
            bindName: null,
            value: function (this: any, b?: any) { return garageConfirmationMessage(b); },
        },
        confirmationYes: {
            label: "Confirm Button",
            idSuffix: "garage-confirm-yes",
            placeholder: SWITCH_CONFIRM_DEFAULT_YES,
            bindName: null,
            value: function (this: any, b?: any) { return garageConfirmationYesText(b); },
        },
        confirmationNo: {
            label: "Cancel Button",
            idSuffix: "garage-confirm-no",
            placeholder: SWITCH_CONFIRM_DEFAULT_NO,
            bindName: null,
            value: function (this: any, b?: any) { return garageConfirmationNoText(b); },
        },
        preview: {
            badge: "garage",
        },
    };
    registerCard({
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
        confirmation: {
            metadata: GARAGE_CARD_METADATA,
            enabled: garageConfirmationEnabled,
            mode: garageConfirmationMode,
            defaultMessageForMode: garageConfirmationDefaultMessageForMode,
            setOptions: setGarageConfirmationOptions,
        },
    });
}
