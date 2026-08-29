import { state } from "../state/app_instance";
import { isSpecificFirmwareVersion } from "./firmware_metadata";
import type { UiRuntimeState } from "./state";

export interface FirmwareVersionFeature {
    render(): void;
    set(version?: any): void;
    display(version?: any): string;
    label(): string;
}

export function createFirmwareVersionFeature(
    runtime: UiRuntimeState,
    dependencies: {
        syncVersionSelect(): void;
        renderUpdateStatus(): void;
        stopInstallRefreshIfComplete(): void;
    },
): FirmwareVersionFeature {
    const els = runtime.els;
    // ── Firmware Version State ─────────────────────────────────────────────
    const checkingLabel = "Checking version...";
    const devLabel = "Dev build";
    const unknownLabel = "Version unknown";
    function render() {
        if (!els.fwVersionLabel)
            return;
        els.fwVersionLabel.textContent = label();
    }
    function set(version?: any) {
        version = String(version == null ? "" : version).trim();
        if (!version)
            return;
        if (isSpecificFirmwareVersion(state.firmwareVersion) && !isSpecificFirmwareVersion(version))
            return;
        state.firmwareVersion = display(version);
        render();
        dependencies.syncVersionSelect();
        dependencies.renderUpdateStatus();
        dependencies.stopInstallRefreshIfComplete();
    }
    function display(version?: any) {
        version = String(version == null ? "" : version).trim();
        if (!version)
            return unknownLabel;
        if (version === unknownLabel)
            return unknownLabel;
        return isSpecificFirmwareVersion(version) ? version : devLabel;
    }
    function label() {
        if (!state.firmwareVersion && state.firmwareVersionRefreshPending) {
            return checkingLabel;
        }
        return display(state.firmwareVersion);
    }
    return {
        render,
        set,
        display,
        label,
    };
}
