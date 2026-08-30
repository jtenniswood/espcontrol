import type { DeviceConfig } from "../state/types";
import { cardContractCard } from "../generated/card_contract";
import { cardContractOptionSpec } from "./config_option_core";

export function createConfigInternalRelayOptionsFeature(
    deviceProfile: Pick<DeviceConfig, "features">,
) {
    function internalRelayOptions(this: any) {
        return (deviceProfile.features && deviceProfile.features.internalRelays) || [];
    }

    function internalRelaySpec(this: any) {
        const card: any = cardContractCard("internal");
        return card && card.behavior && card.behavior.internalRelay || {};
    }

    function internalRelayModeOptionValues(this: any) {
        const spec: any = cardContractOptionSpec("internal", "internal_mode");
        return spec && spec.values ? spec.values.slice() : ["switch", "push"];
    }

    function normalizeInternalRelayMode(this: any, mode?: any) {
        mode = String(mode || "");
        return internalRelayModeOptionValues().indexOf(mode) >= 0 ? mode : "switch";
    }

    function internalRelayDefaultIcon(this: any, mode?: any) {
        const icons: any = internalRelaySpec().defaultIcons || {};
        return icons[normalizeInternalRelayMode(mode)] || (mode === "push" ? "Gesture Tap" : "Lightbulb Outline");
    }

    function internalRelayDefaultOnIcon(this: any) {
        return internalRelaySpec().defaultIconOn || "Lightbulb";
    }

    function internalRelayUsesDefaultIcon(this: any, mode?: any, icon?: any) {
        if (!icon || icon === "Auto" || icon === internalRelayDefaultIcon(mode))
            return true;
        return mode === "switch" && icon === "Power Plug";
    }

    function internalRelayUsesDefaultOnIcon(this: any, icon?: any) {
        return !icon || icon === "Auto" || icon === internalRelayDefaultOnIcon() || icon === "Power";
    }

    function internalRelayMode(this: any, button?: any) {
        return normalizeInternalRelayMode(button && button.sensor === "push" ? "push" : "switch");
    }

    function internalRelayLabelFor(this: any, key?: any) {
        const relays = internalRelayOptions();
        for (let index = 0; index < relays.length; index++) {
            const relay = relays[index];
            if (relay && relay.key === key)
                return relay.label;
        }
        return key ? key.replace(/_/g, " ").replace(/\b\w/g, function (character: string) { return character.toUpperCase(); }) : "Relay";
    }

    return {
        internalRelayDefaultIcon,
        internalRelayDefaultOnIcon,
        internalRelayLabelFor,
        internalRelayMode,
        internalRelayModeOptionValues,
        internalRelayOptions,
        internalRelayUsesDefaultIcon,
        internalRelayUsesDefaultOnIcon,
        normalizeInternalRelayMode,
    };
}

export type ConfigInternalRelayOptionsFeature = ReturnType<typeof createConfigInternalRelayOptionsFeature>;
