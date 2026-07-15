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
    function firmwareInstallAvailable(this: any) {
        var info: any = selectedFirmwareInfo();
        return state.firmwareInstallControlsSupported === true &&
            !!info &&
            isSpecificFirmwareVersion(info.latest_version) &&
            !selectedFirmwareMatchesInstalled();
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
    function selectedFirmwareInfo(this: any) {
        return findFirmwareVersionInfo(state.firmwareSelectedVersion) ||
            (state.firmwareVersionOptions.length ? state.firmwareVersionOptions[0] : null) ||
            latestFirmwareInfoFromState();
    }
    function selectedFirmwareVersion(this: any) {
        var info: any = selectedFirmwareInfo();
        return info ? info.latest_version : "";
    }
    function selectedFirmwareIsLatest(this: any) {
        var version: any = selectedFirmwareVersion();
        return !version || !publicFirmwareReleaseKnown() ||
            firmwareVersionsSame(version, state.firmwareLatestVersion);
    }
    function selectedFirmwareMatchesInstalled(this: any) {
        var version: any = selectedFirmwareVersion();
        return isSpecificFirmwareVersion(version) &&
            isSpecificFirmwareVersion(state.firmwareVersion) &&
            firmwareVersionsSame(state.firmwareVersion, version);
    }
    function firmwareVersionSelectorVisible(this: any) {
        return state.firmwareVersionIndexLoaded && state.firmwareVersionOptions.length > 1;
    }
    function syncFirmwareVersionSelect(this: any) {
        if (!els.fwVersionSelect)
            return;
        var options: any = state.firmwareVersionOptions;
        els.fwVersionSelect.innerHTML = "";
        if (!options.length) {
            if (els.fwVersionField)
                els.fwVersionField.style.display = "none";
            return;
        }
        if (!findFirmwareVersionInfo(state.firmwareSelectedVersion)) {
            state.firmwareSelectedVersion = options[0].latest_version;
        }
        for (var i: any = 0; i < options.length; i++) {
            var info: any = options[i];
            var option: any = document.createElement("option");
            option.value = info.latest_version;
            option.textContent = info.latest_version +
                (i === 0 || firmwareVersionsSame(info.latest_version, state.firmwareLatestVersion) ? " (Latest)" : "");
            if (firmwareVersionsSame(info.latest_version, state.firmwareVersion)) {
                option.textContent += " (Installed)";
            }
            els.fwVersionSelect.appendChild(option);
        }
        els.fwVersionSelect.value = state.firmwareSelectedVersion;
        if (els.fwVersionField) {
            els.fwVersionField.style.display =
                firmwareUpdateControlsVisible() && firmwareVersionSelectorVisible() ? "" : "none";
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
    function publicFirmwareStatusHtml(this: any) {
        var info: any = selectedFirmwareInfo() || latestFirmwareInfoFromState();
        var isLatest: any = selectedFirmwareIsLatest();
        var version: any = info && info.latest_version ? info.latest_version : state.firmwareLatestVersion;
        var releaseUrl: any = info && info.release_url ? info.release_url : state.firmwareReleaseUrl;
        var status: any = (isLatest ? "Latest public version: " : "Selected firmware version: ") + escHtml(version);
        if (releaseUrl) {
            status += ' <a href="' + escAttr(releaseUrl) + '" target="_blank" rel="noopener">release notes</a>';
        }
        return status;
    }
    function firmwareUpdateControlsVisible(this: any) {
        return state.firmwareUpdateControlsSupported === true;
    }
    function syncFirmwareUpdateUi(this: any) {
        var show: any = firmwareUpdateControlsVisible();
        if (els.fwActions)
            els.fwActions.style.display = show ? "" : "none";
        if (els.fwStatus)
            els.fwStatus.style.display = show ? "" : "none";
        if (els.fwVersionField) {
            els.fwVersionField.style.display = show && firmwareVersionSelectorVisible() ? "" : "none";
        }
        if (els.setAutoUpdateRow)
            els.setAutoUpdateRow.style.display = show ? "" : "none";
        if (els.updateFreqWrap) {
            els.updateFreqWrap.style.display = show && state.autoUpdate ? "" : "none";
        }
    }
    function renderFirmwareUpdateStatus(this: any) {
        if (!els.fwStatus)
            return;
        var cls: any = "sp-fw-status";
        var status: any = "";
        var inlineStatus: any = "";
        if (state.firmwareUpdateState === "INSTALLING") {
            status = state.firmwareInstallStatus || "Installing update\u2026";
            cls += " sp-update-installing";
        }
        else if (state.firmwareInstallError) {
            status = escHtml(state.firmwareInstallError);
            cls += " sp-update-error";
        }
        else if (firmwareInstallAvailable()) {
            status = publicFirmwareStatusHtml();
            cls += " sp-update-available";
        }
        else if (state.firmwareUpdateState === "NO UPDATE") {
            if (selectedFirmwareMatchesInstalled()) {
                inlineStatus = selectedFirmwareIsLatest() ? "Up to date" : "Installed";
            }
            else if (publicFirmwareReleaseKnown() &&
                isSpecificFirmwareVersion(state.firmwareVersion) &&
                !installedFirmwareMatchesPublicRelease()) {
                status = publicFirmwareStatusHtml();
            }
            else {
                inlineStatus = "Up to date";
            }
        }
        else if (publicFirmwareReleaseKnown()) {
            if (selectedFirmwareMatchesInstalled()) {
                inlineStatus = selectedFirmwareIsLatest() ? "Up to date" : "Installed";
            }
            else {
                status = publicFirmwareStatusHtml();
            }
        }
        else if (state.firmwareChecking) {
            status = "Checking public firmware\u2026";
        }
        els.fwStatus.className = cls;
        els.fwStatus.innerHTML = status;
        if (els.fwInlineStatus) {
            els.fwInlineStatus.className = "sp-fw-inline-status" + (inlineStatus ? " sp-visible" : "");
            els.fwInlineStatus.textContent = inlineStatus;
        }
        if (els.fwCheckBtn) {
            var isBusy: any = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
            els.fwCheckBtn.className = "sp-fw-btn" + (isBusy ? " sp-fw-btn-busy" : "");
            if (state.firmwareUpdateState === "INSTALLING") {
                els.fwCheckBtn.disabled = true;
                els.fwCheckBtn.textContent = "Installing\u2026";
            }
            else if (selectedFirmwareMatchesInstalled() && !selectedFirmwareIsLatest()) {
                els.fwCheckBtn.disabled = true;
                els.fwCheckBtn.textContent = "Installed";
            }
            else if (firmwareInstallAvailable()) {
                els.fwCheckBtn.disabled = false;
                els.fwCheckBtn.textContent = selectedFirmwareIsLatest() ? "Install Update" : "Install Version";
            }
            else {
                els.fwCheckBtn.disabled = state.firmwareChecking;
                els.fwCheckBtn.textContent = state.firmwareChecking ? "Checking\u2026" : "Check for Update";
            }
        }
        if (els.fwVersionSelect) {
            els.fwVersionSelect.disabled = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
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
            installPublicFirmwareViaWebOta();
        }, FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS);
    }
    return {
        "firmwareInstallRefreshTimer": liveGlobal(() => firmwareInstallRefreshTimer, (value?: any) => { firmwareInstallRefreshTimer = value; }),
        "firmwareInstallRefreshUntil": liveGlobal(() => firmwareInstallRefreshUntil, (value?: any) => { firmwareInstallRefreshUntil = value; }),
        "firmwareWebOtaFallbackTimer": liveGlobal(() => firmwareWebOtaFallbackTimer, (value?: any) => { firmwareWebOtaFallbackTimer = value; }),
        "FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS": liveGlobal(() => FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS, (value?: any) => { FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS = value; }),
        "firmwareUpdateAvailable": staticGlobal(firmwareUpdateAvailable),
        "publicFirmwareInstallAvailable": staticGlobal(publicFirmwareInstallAvailable),
        "firmwareInstallAvailable": staticGlobal(firmwareInstallAvailable),
        "latestFirmwareInfoFromState": staticGlobal(latestFirmwareInfoFromState),
        "findFirmwareVersionInfo": staticGlobal(findFirmwareVersionInfo),
        "selectedFirmwareInfo": staticGlobal(selectedFirmwareInfo),
        "selectedFirmwareVersion": staticGlobal(selectedFirmwareVersion),
        "selectedFirmwareIsLatest": staticGlobal(selectedFirmwareIsLatest),
        "selectedFirmwareMatchesInstalled": staticGlobal(selectedFirmwareMatchesInstalled),
        "firmwareVersionSelectorVisible": staticGlobal(firmwareVersionSelectorVisible),
        "syncFirmwareVersionSelect": staticGlobal(syncFirmwareVersionSelect),
        "setPublicFirmwareInfo": staticGlobal(setPublicFirmwareInfo),
        "setPublicFirmwareVersions": staticGlobal(setPublicFirmwareVersions),
        "publicFirmwareReleaseKnown": staticGlobal(publicFirmwareReleaseKnown),
        "installedFirmwareMatchesPublicRelease": staticGlobal(installedFirmwareMatchesPublicRelease),
        "publicFirmwareStatusHtml": staticGlobal(publicFirmwareStatusHtml),
        "firmwareUpdateControlsVisible": staticGlobal(firmwareUpdateControlsVisible),
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
