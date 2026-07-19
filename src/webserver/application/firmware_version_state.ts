import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installFirmwareVersionStateModule(): GlobalDescriptors {
    // ── Firmware Version State ─────────────────────────────────────────────
    var FIRMWARE_CHECKING_VERSION_LABEL: any = "Checking version...";
    var FIRMWARE_DEV_VERSION_LABEL: any = "Dev build";
    var FIRMWARE_UNKNOWN_VERSION_LABEL: any = "Version unknown";
    function renderFirmwareVersion(this: any) {
        if (!els.fwVersionLabel)
            return;
        els.fwVersionLabel.textContent = firmwareVersionLabel();
    }
    function setFirmwareVersion(this: any, version?: any) {
        version = String(version == null ? "" : version).trim();
        if (!version)
            return;
        if (isSpecificFirmwareVersion(state.firmwareVersion) && !isSpecificFirmwareVersion(version))
            return;
        state.firmwareVersion = displayFirmwareVersion(version);
        renderFirmwareVersion();
        syncFirmwareVersionSelect();
        renderFirmwareUpdateStatus();
        stopFirmwareInstallRefreshIfComplete();
    }
    function displayFirmwareVersion(this: any, version?: any) {
        version = String(version == null ? "" : version).trim();
        if (!version)
            return FIRMWARE_UNKNOWN_VERSION_LABEL;
        if (version === FIRMWARE_UNKNOWN_VERSION_LABEL)
            return FIRMWARE_UNKNOWN_VERSION_LABEL;
        return isSpecificFirmwareVersion(version) ? version : FIRMWARE_DEV_VERSION_LABEL;
    }
    function firmwareVersionLabel(this: any) {
        if (!state.firmwareVersion && state.firmwareVersionRefreshPending) {
            return FIRMWARE_CHECKING_VERSION_LABEL;
        }
        return displayFirmwareVersion(state.firmwareVersion);
    }
    return {
        "FIRMWARE_CHECKING_VERSION_LABEL": liveGlobal(() => FIRMWARE_CHECKING_VERSION_LABEL, (value?: any) => { FIRMWARE_CHECKING_VERSION_LABEL = value; }),
        "FIRMWARE_DEV_VERSION_LABEL": liveGlobal(() => FIRMWARE_DEV_VERSION_LABEL, (value?: any) => { FIRMWARE_DEV_VERSION_LABEL = value; }),
        "FIRMWARE_UNKNOWN_VERSION_LABEL": liveGlobal(() => FIRMWARE_UNKNOWN_VERSION_LABEL, (value?: any) => { FIRMWARE_UNKNOWN_VERSION_LABEL = value; }),
        "renderFirmwareVersion": staticGlobal(renderFirmwareVersion),
        "setFirmwareVersion": staticGlobal(setFirmwareVersion),
        "displayFirmwareVersion": staticGlobal(displayFirmwareVersion),
        "firmwareVersionLabel": staticGlobal(firmwareVersionLabel),
    };
}
