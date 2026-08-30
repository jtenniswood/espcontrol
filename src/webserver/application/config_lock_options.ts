import { cardContractOptionSpec } from "./config_option_core";

export function createConfigLockOptionsFeature() {
    function lockCommandMode(this: any, mode?: any) {
        return mode === "lock" || mode === "unlock";
    }

    function lockModeOptionValues(this: any) {
        const spec: any = cardContractOptionSpec("lock", "lock_mode");
        return spec && spec.values ? spec.values.slice() : [];
    }

    function normalizeLockMode(this: any, mode?: any) {
        mode = String(mode || "");
        return lockModeOptionValues().indexOf(mode) >= 0 ? mode : "";
    }

    function lockModeDefaultIcon(this: any, mode?: any) {
        return mode === "unlock" ? "Lock Open" : "Lock";
    }

    function lockModeDefaultLabel(this: any, mode?: any) {
        return mode === "unlock" ? "Unlock" : "Lock";
    }

    function lockUsesDefaultIcon(this: any, icon?: any) {
        return !icon || icon === "Auto" || icon === "Lock" || icon === "Lock Open";
    }

    return {
        lockCommandMode,
        lockModeDefaultIcon,
        lockModeDefaultLabel,
        lockModeOptionValues,
        lockUsesDefaultIcon,
        normalizeLockMode,
    };
}

export type ConfigLockOptionsFeature = ReturnType<typeof createConfigLockOptionsFeature>;
