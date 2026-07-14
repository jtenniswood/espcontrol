import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installC6FirmwareUiModule(): GlobalDescriptors {
    // WiFi co-processor firmware update UI helpers.
    function displayC6FirmwareVersion(this: any, version?: any) {
        version = String(version == null ? "" : version).trim();
        return version || "Unknown";
    }
    function c6FirmwareVersionLooksKnown(this: any, version?: any) {
        version = String(version == null ? "" : version).trim();
        return /\d/.test(version);
    }
    function c6FirmwareUpdateKnownAvailable(this: any) {
        var current: any = String(state.c6FirmwareCurrentVersion || "").trim();
        var latest: any = String(state.c6FirmwareLatestVersion || "").trim();
        return c6FirmwareVersionLooksKnown(current) &&
            c6FirmwareVersionLooksKnown(latest) &&
            current !== latest;
    }
    function syncC6FirmwareUi(this: any) {
        var show: any = state.c6FirmwareUpdateControlsSupported === true;
        if (els.c6FirmwareCard)
            els.c6FirmwareCard.style.display = show ? "" : "none";
        if (els.c6FirmwareCurrent) {
            els.c6FirmwareCurrent.textContent = displayC6FirmwareVersion(state.c6FirmwareCurrentVersion);
        }
        if (els.c6FirmwareLatest) {
            els.c6FirmwareLatest.textContent = displayC6FirmwareVersion(state.c6FirmwareLatestVersion);
        }
        if (els.c6FirmwareStatus) {
            var cls: any = "sp-fw-status";
            var status: any = "";
            if (state.c6FirmwareInstalling) {
                status = "Installing WiFi firmware update\u2026";
                cls += " sp-update-installing";
            }
            else if (state.c6FirmwareChecking) {
                status = "Checking WiFi firmware\u2026";
            }
            else if (c6FirmwareUpdateKnownAvailable()) {
                status = "WiFi firmware update available.";
                cls += " sp-update-available";
            }
            else if (state.c6FirmwareUpdateAvailable) {
                status = state.c6FirmwareUpdateAvailable;
            }
            els.c6FirmwareStatus.className = cls;
            els.c6FirmwareStatus.textContent = status;
        }
        if (els.c6FirmwareUpdateBtn) {
            var busy: any = state.c6FirmwareChecking || state.c6FirmwareInstalling;
            els.c6FirmwareUpdateBtn.className = "sp-fw-btn" + (busy ? " sp-fw-btn-busy" : "");
            els.c6FirmwareUpdateBtn.disabled = busy || !show ||
                (c6FirmwareUpdateKnownAvailable() && !state.c6FirmwareInstallControlsSupported);
            if (state.c6FirmwareInstalling) {
                els.c6FirmwareUpdateBtn.textContent = "Installing\u2026";
            }
            else if (state.c6FirmwareChecking) {
                els.c6FirmwareUpdateBtn.textContent = "Checking\u2026";
            }
            else if (c6FirmwareUpdateKnownAvailable()) {
                els.c6FirmwareUpdateBtn.textContent = "Update WiFi Firmware";
            }
            else {
                els.c6FirmwareUpdateBtn.textContent = "Check for Update";
            }
        }
    }
    function setC6FirmwareCurrentVersion(this: any, version?: any) {
        version = String(version == null ? "" : version).trim();
        if (!version)
            return;
        state.c6FirmwareCurrentVersion = version;
        state.c6FirmwareUpdateControlsSupported = true;
        state.c6FirmwareChecking = false;
        state.c6FirmwareInstalling = false;
        syncC6FirmwareUi();
    }
    function setC6FirmwareLatestVersion(this: any, version?: any) {
        version = String(version == null ? "" : version).trim();
        if (!version)
            return;
        state.c6FirmwareLatestVersion = version;
        state.c6FirmwareUpdateControlsSupported = true;
        state.c6FirmwareChecking = false;
        syncC6FirmwareUi();
    }
    function setC6FirmwareUpdateAvailable(this: any, value?: any) {
        value = String(value == null ? "" : value).trim();
        if (!value)
            return;
        state.c6FirmwareUpdateAvailable = value;
        state.c6FirmwareUpdateControlsSupported = true;
        state.c6FirmwareChecking = false;
        syncC6FirmwareUi();
    }
    return {
        "displayC6FirmwareVersion": staticGlobal(displayC6FirmwareVersion),
        "c6FirmwareVersionLooksKnown": staticGlobal(c6FirmwareVersionLooksKnown),
        "c6FirmwareUpdateKnownAvailable": staticGlobal(c6FirmwareUpdateKnownAvailable),
        "syncC6FirmwareUi": staticGlobal(syncC6FirmwareUi),
        "setC6FirmwareCurrentVersion": staticGlobal(setC6FirmwareCurrentVersion),
        "setC6FirmwareLatestVersion": staticGlobal(setC6FirmwareLatestVersion),
        "setC6FirmwareUpdateAvailable": staticGlobal(setC6FirmwareUpdateAvailable),
    };
}
