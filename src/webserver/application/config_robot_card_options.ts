import {
    normalizeSavedConfigVacuumIconOn,
    normalizeSavedConfigVacuumOptions,
    normalizeSavedConfigVacuumPrecision,
    normalizeSavedConfigVacuumSensor,
} from "../generated/saved_config_vacuum";
import {
    entityModeCardUsesDefaultIcon,
    entityModeValues,
    normalizeEntityMode,
    normalizeEntityModeCardConfig,
} from "../cards/entity_mode_card";

export const LAWN_MOWER_CARD_MODES = [
    ["status", "Status"],
    ["start_mowing", "Start Mowing"],
    ["dock", "Dock"],
    ["pause_resume", "Pause / Resume"],
] as const;

export const VACUUM_CARD_MODES = [
    ["status", "Status"],
    ["start_stop", "Start / Stop"],
    ["dock", "Dock"],
    ["pause_resume", "Pause / Resume"],
    ["clean_spot", "Spot Clean"],
    ["locate", "Locate"],
    ["clean_area", "Clean Area"],
] as const;

export function createConfigRobotCardOptionsFeature() {
    function lawnMowerModeValues(this: any) {
        return entityModeValues("lawn_mower", "lawn_mower_mode", LAWN_MOWER_CARD_MODES);
    }

    function normalizeLawnMowerMode(this: any, mode?: any) {
        return normalizeEntityMode(mode, lawnMowerModeValues(), "start_mowing");
    }

    function lawnMowerModeDefaultIcon(this: any, mode?: any) {
        mode = normalizeLawnMowerMode(mode);
        return mode === "dock" ? "Robot Mower Outline" : "Robot Mower";
    }

    function lawnMowerModeBadgeIcon(this: any, mode?: any) {
        mode = normalizeLawnMowerMode(mode);
        if (mode === "status") return "format-text";
        if (mode === "dock") return "home-import-outline";
        if (mode === "pause_resume") return "play-pause";
        return "robot-mower";
    }

    function lawnMowerUsesDefaultIcon(this: any, icon?: any) {
        return entityModeCardUsesDefaultIcon(icon, ["Lawnmower", "Robot Mower", "Robot Mower Outline"]);
    }

    function normalizeLawnMowerConfig(this: any, button?: any) {
        normalizeEntityModeCardConfig(button, {
            normalizeMode: normalizeLawnMowerMode,
            defaultIcon: lawnMowerModeDefaultIcon,
        });
    }

    function vacuumModeValues(this: any) {
        return entityModeValues("vacuum", "vacuum_mode", VACUUM_CARD_MODES);
    }

    function normalizeVacuumMode(this: any, mode?: any) {
        return normalizeSavedConfigVacuumSensor(String(mode || ""));
    }

    function vacuumModeNeedsArea(this: any, mode?: any) {
        return normalizeVacuumMode(mode) === "clean_area";
    }

    function vacuumModeDefaultIcon(this: any, mode?: any) {
        mode = normalizeVacuumMode(mode);
        if (mode === "dock") return "Robot Vacuum Variant";
        if (mode === "clean_spot") return "Vacuum";
        if (mode === "locate") return "Robot Vacuum Alert";
        if (mode === "clean_area") return "Vacuum Outline";
        return "Robot Vacuum";
    }

    function vacuumModeBadgeIcon(this: any, mode?: any) {
        mode = normalizeVacuumMode(mode);
        if (mode === "dock") return "home-import-outline";
        if (mode === "pause_resume") return "play-pause";
        if (mode === "clean_spot") return "vacuum";
        if (mode === "locate") return "map-marker-question";
        if (mode === "clean_area") return "map-marker-path";
        return "robot-vacuum";
    }

    function vacuumUsesDefaultIcon(this: any, icon?: any) {
        return entityModeCardUsesDefaultIcon(icon, [
            "Robot Vacuum", "Robot Vacuum Alert", "Robot Vacuum Off",
            "Robot Vacuum Variant", "Robot Vacuum Variant Alert", "Robot Vacuum Variant Off",
            "Vacuum", "Vacuum Outline",
        ]);
    }

    function normalizeVacuumConfig(this: any, button?: any) {
        if (!button) return;
        button.sensor = normalizeSavedConfigVacuumSensor(String(button.sensor || ""));
        button.unit = vacuumModeNeedsArea(button.sensor) ? (button.unit || "") : "";
        button.precision = normalizeSavedConfigVacuumPrecision(String(button.precision || ""));
        button.options = normalizeSavedConfigVacuumOptions(String(button.options || ""));
        button.icon_on = normalizeSavedConfigVacuumIconOn(String(button.icon_on || ""));
        if (!button.icon || button.icon === "Auto")
            button.icon = vacuumModeDefaultIcon(button.sensor);
    }

    return {
        lawnMowerModes: LAWN_MOWER_CARD_MODES,
        lawnMowerModeBadgeIcon,
        lawnMowerModeDefaultIcon,
        lawnMowerUsesDefaultIcon,
        normalizeLawnMowerConfig,
        normalizeLawnMowerMode,
        vacuumModes: VACUUM_CARD_MODES,
        normalizeVacuumConfig,
        normalizeVacuumMode,
        vacuumModeBadgeIcon,
        vacuumModeDefaultIcon,
        vacuumModeNeedsArea,
        vacuumUsesDefaultIcon,
    };
}

export type ConfigRobotCardOptionsFeature = ReturnType<typeof createConfigRobotCardOptionsFeature>;
