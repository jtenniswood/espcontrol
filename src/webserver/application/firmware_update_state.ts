import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installFirmwareUpdateStateModule(): GlobalDescriptors {
    // ── Firmware Update State ─────────────────────────────────────────────
    var firmwareInstallRefreshTimer: any = null;
    var firmwareInstallRefreshUntil: any = 0;
    var firmwareWebOtaFallbackTimer: any = null;
    var FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS: any = 12000;
    function firmwareUpdateAvailable(this: any) {
        return state.firmwareUpdateState === "UPDATE AVAILABLE" &&
            isSpecificFirmwareVersion(state.firmwareLatestVersion);
    }
    function publicFirmwareInstallAvailable(this: any) {
        return publicFirmwareReleaseKnown() && !installedFirmwareMatchesPublicRelease();
    }
    function latestFirmwareInfo(this: any) {
        return findFirmwareVersionInfo(state.firmwareLatestVersion) || latestFirmwareInfoFromState();
    }
    function latestFirmwareInstallAvailable(this: any) {
        var info: any = latestFirmwareInfo();
        return state.firmwareInstallControlsSupported === true &&
            !!info &&
            isSpecificFirmwareVersion(info.latest_version) &&
            !installedFirmwareMatchesPublicRelease();
    }
    function latestFirmwareInstallAction(this: any) {
        if (!latestFirmwareInstallAvailable())
            return "check";
        return firmwareUpdateAvailable() ? "install" : "check_then_install";
    }
    function latestFirmwareInfoFromState(this: any) {
        if (!isSpecificFirmwareVersion(state.firmwareLatestVersion))
            return null;
        return {
            latest_version: state.firmwareLatestVersion,
            release_url: state.firmwareReleaseUrl,
            ota_url: state.firmwareOtaUrl,
            ota_filename: state.firmwareOtaFilename || (DEVICE_ID + ".ota.bin"),
            ota_md5: state.firmwareOtaMd5,
        };
    }
    function findFirmwareVersionInfo(this: any, version?: any) {
        version = String(version || "").trim();
        if (!version)
            return null;
        for (var i: any = 0; i < state.firmwareVersionOptions.length; i++) {
            var info: any = state.firmwareVersionOptions[i];
            if (firmwareVersionsSame(info.latest_version, version))
                return info;
        }
        var latest: any = latestFirmwareInfoFromState();
        if (latest && firmwareVersionsSame(latest.latest_version, version))
            return latest;
        return null;
    }
    function firmwareInfoForVersion(this: any, version?: any) {
        return isSpecificFirmwareVersion(version) ? findFirmwareVersionInfo(version) : selectedFirmwareInfo();
    }
    function selectedFirmwareInfo(this: any) {
        return findFirmwareVersionInfo(state.firmwareSelectedVersion) ||
            (state.firmwareVersionOptions.length ? state.firmwareVersionOptions[0] : null) ||
            latestFirmwareInfoFromState();
    }
    function previousFirmwareInfos(this: any) {
        return state.firmwareVersionOptions.filter(function (this: any, info?: any) {
            var version: any = info && info.latest_version;
            return isSpecificFirmwareVersion(version) &&
                !firmwareVersionsSame(version, state.firmwareLatestVersion) &&
                !firmwareVersionsSame(version, state.firmwareVersion);
        });
    }
    function selectedPreviousFirmwareInfo(this: any) {
        var options: any = previousFirmwareInfos();
        for (var i: any = 0; i < options.length; i++) {
            if (firmwareVersionsSame(options[i].latest_version, state.firmwareSelectedVersion)) {
                return options[i];
            }
        }
        return options.length ? options[0] : null;
    }
    function previousFirmwareInstallAvailable(this: any) {
        var info: any = selectedPreviousFirmwareInfo();
        return state.firmwareInstallControlsSupported === true &&
            !!info &&
            !firmwareVersionsSame(info.latest_version, state.firmwareVersion);
    }
    function firmwareVersionSelectorVisible(this: any) {
        return state.firmwareVersionIndexLoaded && previousFirmwareInfos().length > 0;
    }
    function syncFirmwareVersionSelect(this: any) {
        if (!els.fwVersionSelect)
            return;
        var options: any = previousFirmwareInfos();
        els.fwVersionSelect.innerHTML = "";
        if (!options.length) {
            state.firmwareSelectedVersion = "";
            syncPreviousFirmwareUi();
            return;
        }
        state.firmwareSelectedVersion = selectedPreviousFirmwareInfo().latest_version;
        for (var i: any = 0; i < options.length; i++) {
            var info: any = options[i];
            var option: any = document.createElement("option");
            option.value = info.latest_version;
            option.textContent = info.latest_version;
            els.fwVersionSelect.appendChild(option);
        }
        els.fwVersionSelect.value = state.firmwareSelectedVersion;
        syncPreviousFirmwareUi();
    }
    function syncPreviousFirmwareUi(this: any) {
        var show: any = firmwareUpdateControlsVisible() && firmwareVersionSelectorVisible();
        if (els.fwPreviousPanel)
            els.fwPreviousPanel.style.display = show ? "" : "none";
        var busy: any = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
        if (els.fwVersionSelect)
            els.fwVersionSelect.disabled = busy || !show;
        if (els.fwPreviousInstallBtn) {
            els.fwPreviousInstallBtn.disabled = busy || !show || !previousFirmwareInstallAvailable();
            els.fwPreviousInstallBtn.className = "sp-fw-btn" + (busy ? " sp-fw-btn-busy" : "");
            els.fwPreviousInstallBtn.textContent = state.firmwareUpdateState === "INSTALLING" ? "Installing…" : "Install";
        }
    }
    function setPublicFirmwareInfo(this: any, info?: any) {
        if (!info)
            return false;
        var latest: any = String(info.latest_version || "").trim();
        if (!isSpecificFirmwareVersion(latest))
            return false;
        state.firmwareLatestVersion = latest;
        if (info.release_url)
            state.firmwareReleaseUrl = String(info.release_url).trim();
        if (info.ota_url)
            state.firmwareOtaUrl = String(info.ota_url).trim();
        if (info.ota_filename)
            state.firmwareOtaFilename = String(info.ota_filename).trim();
        if (info.ota_md5)
            state.firmwareOtaMd5 = String(info.ota_md5).trim();
        if (state.firmwareUpdateState === "NO UPDATE" &&
            !isSpecificFirmwareVersion(state.firmwareVersion)) {
            setFirmwareVersion(latest);
        }
        syncFirmwareVersionSelect();
        renderFirmwareUpdateStatus();
        return true;
    }
    function setPublicFirmwareVersions(this: any, infos?: any) {
        if (!Array.isArray(infos) || !infos.length)
            return false;
        state.firmwareVersionOptions = infos;
        state.firmwareVersionIndexLoaded = true;
        if (!state.firmwareSelectedVersion || !findFirmwareVersionInfo(state.firmwareSelectedVersion)) {
            state.firmwareSelectedVersion = infos[0].latest_version;
        }
        setPublicFirmwareInfo(infos[0]);
        syncFirmwareVersionSelect();
        renderFirmwareUpdateStatus();
        return true;
    }
    function publicFirmwareReleaseKnown(this: any) {
        return isSpecificFirmwareVersion(state.firmwareLatestVersion);
    }
    function installedFirmwareMatchesPublicRelease(this: any) {
        return publicFirmwareReleaseKnown() &&
            isSpecificFirmwareVersion(state.firmwareVersion) &&
            firmwareVersionsSame(state.firmwareVersion, state.firmwareLatestVersion);
    }
    function firmwareUpdateControlsVisible(this: any) {
        return state.firmwareUpdateControlsSupported === true;
    }
    function syncFirmwareCardBadge(this: any) {
        if (els.firmwareCardBadge) {
            els.firmwareCardBadge.classList.toggle("sp-hidden",
                !latestFirmwareInstallAvailable() && !c6FirmwareUpdateKnownAvailable());
        }
    }
    function syncFirmwareUpdateUi(this: any) {
        var show: any = firmwareUpdateControlsVisible();
        if (els.fwActions)
            els.fwActions.style.display = show ? "" : "none";
        if (els.fwStatus)
            els.fwStatus.style.display = show ? "" : "none";
        if (els.autoUpdatePanel)
            els.autoUpdatePanel.style.display = show ? "" : "none";
        if (els.autoUpdateBadge)
            els.autoUpdateBadge.classList.toggle("sp-hidden", !state.autoUpdate);
        if (els.firmwareUpdatesBadge)
            els.firmwareUpdatesBadge.classList.toggle("sp-hidden", !latestFirmwareInstallAvailable());
        syncFirmwareCardBadge();
        if (els.setAutoUpdateRow)
            els.setAutoUpdateRow.style.display = show ? "" : "none";
        if (els.updateFreqWrap) {
            els.updateFreqWrap.style.display = show && state.autoUpdate ? "" : "none";
        }
        syncPreviousFirmwareUi();
    }
    function renderFirmwareUpdateStatus(this: any) {
        if (!els.fwStatus)
            return;
        var cls: any = "sp-fw-status";
        var status: any = "";
        if (els.fwLatestVersion) {
            if (publicFirmwareReleaseKnown()) {
                els.fwLatestVersion.textContent = state.firmwareLatestVersion;
            }
            else if (state.firmwareChecking) {
                els.fwLatestVersion.textContent = "Checking\u2026";
            }
            else {
                els.fwLatestVersion.textContent = "Not checked";
            }
        }
        if (state.firmwareInstallError) {
            status = escHtml(state.firmwareInstallError);
            cls += " sp-update-error";
        }
        els.fwStatus.className = cls;
        els.fwStatus.innerHTML = status;
        if (els.fwCheckBtn) {
            var isBusy: any = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
            els.fwCheckBtn.className = "sp-fw-btn" + (isBusy ? " sp-fw-btn-busy" : "");
            if (state.firmwareUpdateState === "INSTALLING") {
                els.fwCheckBtn.disabled = true;
                els.fwCheckBtn.textContent = "Installing\u2026";
            }
            else if (latestFirmwareInstallAvailable()) {
                els.fwCheckBtn.disabled = false;
                els.fwCheckBtn.textContent = "Install Update";
            }
            else {
                els.fwCheckBtn.disabled = state.firmwareChecking;
                els.fwCheckBtn.textContent = state.firmwareChecking ? "Checking\u2026" : "Check for Update";
            }
        }
        syncFirmwareUpdateUi();
    }
    function setFirmwareUpdateInfo(this: any, d?: any) {
        state.firmwareUpdateControlsSupported = true;
        state.firmwareInstallControlsSupported = true;
        var latest: any = d.latest_version || d.value || "";
        var updateState: any = String(d.state || state.firmwareUpdateState || "").trim().toUpperCase();
        if (d.current_version)
            setFirmwareVersion(d.current_version);
        if (latest)
            state.firmwareLatestVersion = String(latest).trim();
        var installWindowActive: any = !!state.firmwareInstallTargetVersion &&
            Date.now() < firmwareInstallRefreshUntil;
        if (state.firmwareInstallPostPending) {
            if (installWindowActive && updateState === "UPDATE AVAILABLE") {
                state.firmwareInstallPostPending = false;
                clearFirmwareWebOtaFallback();
                state.firmwareInstallStatus = "Installing update\u2026";
                postFirmwareUpdateInstall();
                updateState = "INSTALLING";
            }
            else if (!installWindowActive || (updateState === "NO UPDATE" && !publicFirmwareInstallAvailable())) {
                state.firmwareInstallPostPending = false;
            }
        }
        if (installWindowActive && updateState === "UPDATE AVAILABLE") {
            updateState = "INSTALLING";
        }
        state.firmwareUpdateState = updateState;
        if (state.firmwareUpdateState)
            state.firmwareInstallError = "";
        state.firmwareReleaseUrl = d.release_url || state.firmwareReleaseUrl || "";
        if (state.firmwareUpdateState === "NO UPDATE" &&
            !isSpecificFirmwareVersion(state.firmwareVersion) &&
            isSpecificFirmwareVersion(state.firmwareLatestVersion)) {
            setFirmwareVersion(state.firmwareLatestVersion);
        }
        if (state.firmwareUpdateState)
            state.firmwareChecking = false;
        if (state.firmwareUpdateState === "INSTALLING") {
            startFirmwareInstallRefresh();
        }
        else {
            stopFirmwareInstallRefreshIfComplete();
        }
        renderFirmwareUpdateStatus();
    }
    function firmwareVersionMatches(this: any, version?: any, expected?: any) {
        return String(version == null ? "" : version).trim() ===
            String(expected == null ? "" : expected).trim();
    }
    function stopFirmwareInstallRefresh(this: any) {
        if (firmwareInstallRefreshTimer)
            clearTimeout(firmwareInstallRefreshTimer);
        firmwareInstallRefreshTimer = null;
        firmwareInstallRefreshUntil = 0;
        clearFirmwareWebOtaFallback();
        state.firmwareInstallTargetVersion = "";
        state.firmwareInstallPostPending = false;
        state.firmwareInstallStatus = "";
    }
    function stopFirmwareInstallRefreshIfComplete(this: any) {
        var target: any = state.firmwareInstallTargetVersion;
        if (!target || state.firmwareUpdateState !== "NO UPDATE")
            return false;
        if (isSpecificFirmwareVersion(target) && !firmwareVersionMatches(state.firmwareVersion, target)) {
            setFirmwareVersion(target);
        }
        stopFirmwareInstallRefresh();
        return true;
    }
    function pollFirmwareInstallRefresh(this: any) {
        firmwareInstallRefreshTimer = null;
        refreshFirmwareVersion();
        if (stopFirmwareInstallRefreshIfComplete())
            return;
        if (Date.now() >= firmwareInstallRefreshUntil) {
            stopFirmwareInstallRefresh();
            return;
        }
        firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
    }
    function startFirmwareInstallRefresh(this: any) {
        if (!state.firmwareInstallTargetVersion && isSpecificFirmwareVersion(state.firmwareLatestVersion)) {
            state.firmwareInstallTargetVersion = state.firmwareLatestVersion;
        }
        firmwareInstallRefreshUntil = Date.now() + 180000;
        if (firmwareInstallRefreshTimer)
            clearTimeout(firmwareInstallRefreshTimer);
        firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
    }
    function clearFirmwareWebOtaFallback(this: any) {
        if (firmwareWebOtaFallbackTimer)
            clearTimeout(firmwareWebOtaFallbackTimer);
        firmwareWebOtaFallbackTimer = null;
    }
    function scheduleFirmwareWebOtaFallback(this: any) {
        clearFirmwareWebOtaFallback();
        firmwareWebOtaFallbackTimer = setTimeout(function (this: any) {
            firmwareWebOtaFallbackTimer = null;
            if (!state.firmwareInstallPostPending)
                return;
            if (firmwareUpdateAvailable())
                return;
            if (!publicFirmwareInstallAvailable())
                return;
            installPublicFirmwareViaWebOta(latestFirmwareInfo());
        }, FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS);
    }
    return {
        "firmwareInstallRefreshTimer": liveGlobal(() => firmwareInstallRefreshTimer, (value?: any) => { firmwareInstallRefreshTimer = value; }),
        "firmwareInstallRefreshUntil": liveGlobal(() => firmwareInstallRefreshUntil, (value?: any) => { firmwareInstallRefreshUntil = value; }),
        "firmwareWebOtaFallbackTimer": liveGlobal(() => firmwareWebOtaFallbackTimer, (value?: any) => { firmwareWebOtaFallbackTimer = value; }),
        "FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS": liveGlobal(() => FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS, (value?: any) => { FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS = value; }),
        "firmwareUpdateAvailable": staticGlobal(firmwareUpdateAvailable),
        "publicFirmwareInstallAvailable": staticGlobal(publicFirmwareInstallAvailable),
        "latestFirmwareInfo": staticGlobal(latestFirmwareInfo),
        "latestFirmwareInstallAvailable": staticGlobal(latestFirmwareInstallAvailable),
        "latestFirmwareInstallAction": staticGlobal(latestFirmwareInstallAction),
        "latestFirmwareInfoFromState": staticGlobal(latestFirmwareInfoFromState),
        "findFirmwareVersionInfo": staticGlobal(findFirmwareVersionInfo),
        "firmwareInfoForVersion": staticGlobal(firmwareInfoForVersion),
        "selectedFirmwareInfo": staticGlobal(selectedFirmwareInfo),
        "previousFirmwareInfos": staticGlobal(previousFirmwareInfos),
        "selectedPreviousFirmwareInfo": staticGlobal(selectedPreviousFirmwareInfo),
        "previousFirmwareInstallAvailable": staticGlobal(previousFirmwareInstallAvailable),
        "firmwareVersionSelectorVisible": staticGlobal(firmwareVersionSelectorVisible),
        "syncFirmwareVersionSelect": staticGlobal(syncFirmwareVersionSelect),
        "syncPreviousFirmwareUi": staticGlobal(syncPreviousFirmwareUi),
        "setPublicFirmwareInfo": staticGlobal(setPublicFirmwareInfo),
        "setPublicFirmwareVersions": staticGlobal(setPublicFirmwareVersions),
        "publicFirmwareReleaseKnown": staticGlobal(publicFirmwareReleaseKnown),
        "installedFirmwareMatchesPublicRelease": staticGlobal(installedFirmwareMatchesPublicRelease),
        "firmwareUpdateControlsVisible": staticGlobal(firmwareUpdateControlsVisible),
        "syncFirmwareCardBadge": staticGlobal(syncFirmwareCardBadge),
        "syncFirmwareUpdateUi": staticGlobal(syncFirmwareUpdateUi),
        "renderFirmwareUpdateStatus": staticGlobal(renderFirmwareUpdateStatus),
        "setFirmwareUpdateInfo": staticGlobal(setFirmwareUpdateInfo),
        "firmwareVersionMatches": staticGlobal(firmwareVersionMatches),
        "stopFirmwareInstallRefresh": staticGlobal(stopFirmwareInstallRefresh),
        "stopFirmwareInstallRefreshIfComplete": staticGlobal(stopFirmwareInstallRefreshIfComplete),
        "pollFirmwareInstallRefresh": staticGlobal(pollFirmwareInstallRefresh),
        "startFirmwareInstallRefresh": staticGlobal(startFirmwareInstallRefresh),
        "clearFirmwareWebOtaFallback": staticGlobal(clearFirmwareWebOtaFallback),
        "scheduleFirmwareWebOtaFallback": staticGlobal(scheduleFirmwareWebOtaFallback),
    };
}
