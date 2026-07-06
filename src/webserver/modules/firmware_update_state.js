// ── Firmware Update State ─────────────────────────────────────────────
// @web-module-requires: state, firmware_metadata, firmware_version_state

var firmwareInstallRefreshTimer = null;
var firmwareInstallRefreshUntil = 0;
var firmwareWebOtaFallbackTimer = null;
var FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS = 12000;

function firmwareUpdateAvailable() {
  return state.firmwareUpdateState === "UPDATE AVAILABLE" &&
    isSpecificFirmwareVersion(state.firmwareLatestVersion);
}

function publicFirmwareInstallAvailable() {
  return publicFirmwareReleaseKnown() && !installedFirmwareMatchesPublicRelease();
}

function firmwareInstallAvailable() {
  var info = selectedFirmwareInfo();
  return state.firmwareInstallControlsSupported === true &&
    !!info &&
    isSpecificFirmwareVersion(info.latest_version) &&
    !selectedFirmwareMatchesInstalled();
}

function latestFirmwareInfoFromState() {
  if (!isSpecificFirmwareVersion(state.firmwareLatestVersion)) return null;
  return {
    latest_version: state.firmwareLatestVersion,
    release_url: state.firmwareReleaseUrl,
    ota_url: state.firmwareOtaUrl,
    ota_filename: state.firmwareOtaFilename || (DEVICE_ID + ".ota.bin"),
    ota_md5: state.firmwareOtaMd5,
  };
}

function findFirmwareVersionInfo(version) {
  version = String(version || "").trim();
  if (!version) return null;
  for (var i = 0; i < state.firmwareVersionOptions.length; i++) {
    var info = state.firmwareVersionOptions[i];
    if (firmwareVersionsSame(info.latest_version, version)) return info;
  }
  var latest = latestFirmwareInfoFromState();
  if (latest && firmwareVersionsSame(latest.latest_version, version)) return latest;
  return null;
}

function selectedFirmwareInfo() {
  return findFirmwareVersionInfo(state.firmwareSelectedVersion) ||
    (state.firmwareVersionOptions.length ? state.firmwareVersionOptions[0] : null) ||
    latestFirmwareInfoFromState();
}

function selectedFirmwareVersion() {
  var info = selectedFirmwareInfo();
  return info ? info.latest_version : "";
}

function selectedFirmwareIsLatest() {
  var version = selectedFirmwareVersion();
  return !version || !publicFirmwareReleaseKnown() ||
    firmwareVersionsSame(version, state.firmwareLatestVersion);
}

function selectedFirmwareMatchesInstalled() {
  var version = selectedFirmwareVersion();
  return isSpecificFirmwareVersion(version) &&
    isSpecificFirmwareVersion(state.firmwareVersion) &&
    firmwareVersionsSame(state.firmwareVersion, version);
}

function firmwareVersionSelectorVisible() {
  return state.firmwareVersionIndexLoaded && state.firmwareVersionOptions.length > 1;
}

function syncFirmwareVersionSelect() {
  if (!els.fwVersionSelect) return;
  var options = state.firmwareVersionOptions;
  els.fwVersionSelect.innerHTML = "";
  if (!options.length) {
    if (els.fwVersionField) els.fwVersionField.style.display = "none";
    return;
  }
  if (!findFirmwareVersionInfo(state.firmwareSelectedVersion)) {
    state.firmwareSelectedVersion = options[0].latest_version;
  }
  for (var i = 0; i < options.length; i++) {
    var info = options[i];
    var option = document.createElement("option");
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

function setPublicFirmwareInfo(info) {
  if (!info) return false;
  var latest = String(info.latest_version || "").trim();
  if (!isSpecificFirmwareVersion(latest)) return false;
  state.firmwareLatestVersion = latest;
  if (info.release_url) state.firmwareReleaseUrl = String(info.release_url).trim();
  if (info.ota_url) state.firmwareOtaUrl = String(info.ota_url).trim();
  if (info.ota_filename) state.firmwareOtaFilename = String(info.ota_filename).trim();
  if (info.ota_md5) state.firmwareOtaMd5 = String(info.ota_md5).trim();
  if (state.firmwareUpdateState === "NO UPDATE" &&
      !isSpecificFirmwareVersion(state.firmwareVersion)) {
    setFirmwareVersion(latest);
  }
  syncFirmwareVersionSelect();
  renderFirmwareUpdateStatus();
  return true;
}

function setPublicFirmwareVersions(infos) {
  if (!Array.isArray(infos) || !infos.length) return false;
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

function publicFirmwareReleaseKnown() {
  return isSpecificFirmwareVersion(state.firmwareLatestVersion);
}

function installedFirmwareMatchesPublicRelease() {
  return publicFirmwareReleaseKnown() &&
    isSpecificFirmwareVersion(state.firmwareVersion) &&
    firmwareVersionsSame(state.firmwareVersion, state.firmwareLatestVersion);
}

function publicFirmwareStatusHtml() {
  var info = selectedFirmwareInfo() || latestFirmwareInfoFromState();
  var isLatest = selectedFirmwareIsLatest();
  var version = info && info.latest_version ? info.latest_version : state.firmwareLatestVersion;
  var releaseUrl = info && info.release_url ? info.release_url : state.firmwareReleaseUrl;
  var status = (isLatest ? "Latest public version: " : "Selected firmware version: ") + escHtml(version);
  if (releaseUrl) {
    status += ' <a href="' + escAttr(releaseUrl) + '" target="_blank" rel="noopener">release notes</a>';
  }
  return status;
}

function firmwareUpdateControlsVisible() {
  return state.firmwareUpdateControlsSupported === true;
}

function syncFirmwareUpdateUi() {
  var show = firmwareUpdateControlsVisible();
  if (els.fwActions) els.fwActions.style.display = show ? "" : "none";
  if (els.fwStatus) els.fwStatus.style.display = show ? "" : "none";
  if (els.fwVersionField) {
    els.fwVersionField.style.display = show && firmwareVersionSelectorVisible() ? "" : "none";
  }
  if (els.setAutoUpdateRow) els.setAutoUpdateRow.style.display = show ? "" : "none";
  if (els.updateFreqWrap) {
    els.updateFreqWrap.style.display = show && state.autoUpdate ? "" : "none";
  }
}

function renderFirmwareUpdateStatus() {
  if (!els.fwStatus) return;
  var cls = "sp-fw-status";
  var status = "";
  var inlineStatus = "";
  if (state.firmwareUpdateState === "INSTALLING") {
    status = state.firmwareInstallStatus || "Installing update\u2026";
    cls += " sp-update-installing";
  } else if (state.firmwareInstallError) {
    status = escHtml(state.firmwareInstallError);
    cls += " sp-update-error";
  } else if (firmwareInstallAvailable()) {
    status = publicFirmwareStatusHtml();
    cls += " sp-update-available";
  } else if (state.firmwareUpdateState === "NO UPDATE") {
    if (selectedFirmwareMatchesInstalled()) {
      inlineStatus = selectedFirmwareIsLatest() ? "Up to date" : "Installed";
    } else if (publicFirmwareReleaseKnown() &&
        isSpecificFirmwareVersion(state.firmwareVersion) &&
        !installedFirmwareMatchesPublicRelease()) {
      status = publicFirmwareStatusHtml();
    } else {
      inlineStatus = "Up to date";
    }
  } else if (publicFirmwareReleaseKnown()) {
    if (selectedFirmwareMatchesInstalled()) {
      inlineStatus = selectedFirmwareIsLatest() ? "Up to date" : "Installed";
    } else {
      status = publicFirmwareStatusHtml();
    }
  } else if (state.firmwareChecking) {
    status = "Checking public firmware\u2026";
  }
  els.fwStatus.className = cls;
  els.fwStatus.innerHTML = status;
  if (els.fwInlineStatus) {
    els.fwInlineStatus.className = "sp-fw-inline-status" + (inlineStatus ? " sp-visible" : "");
    els.fwInlineStatus.textContent = inlineStatus;
  }
  if (els.fwCheckBtn) {
    var isBusy = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
    els.fwCheckBtn.className = "sp-fw-btn" + (isBusy ? " sp-fw-btn-busy" : "");
    if (state.firmwareUpdateState === "INSTALLING") {
      els.fwCheckBtn.disabled = true;
      els.fwCheckBtn.textContent = "Installing\u2026";
    } else if (selectedFirmwareMatchesInstalled() && !selectedFirmwareIsLatest()) {
      els.fwCheckBtn.disabled = true;
      els.fwCheckBtn.textContent = "Installed";
    } else if (firmwareInstallAvailable()) {
      els.fwCheckBtn.disabled = false;
      els.fwCheckBtn.textContent = selectedFirmwareIsLatest() ? "Install Update" : "Install Version";
    } else {
      els.fwCheckBtn.disabled = state.firmwareChecking;
      els.fwCheckBtn.textContent = state.firmwareChecking ? "Checking\u2026" : "Check for Update";
    }
  }
  if (els.fwVersionSelect) {
    els.fwVersionSelect.disabled = state.firmwareUpdateState === "INSTALLING" || state.firmwareChecking;
  }
  syncFirmwareUpdateUi();
}

function setFirmwareUpdateInfo(d) {
  state.firmwareUpdateControlsSupported = true;
  state.firmwareInstallControlsSupported = true;
  var latest = d.latest_version || d.value || "";
  var updateState = String(d.state || state.firmwareUpdateState || "").trim().toUpperCase();
  if (d.current_version) setFirmwareVersion(d.current_version);
  if (latest) state.firmwareLatestVersion = String(latest).trim();
  var installWindowActive = !!state.firmwareInstallTargetVersion &&
    Date.now() < firmwareInstallRefreshUntil;
  if (state.firmwareInstallPostPending) {
    if (installWindowActive && updateState === "UPDATE AVAILABLE") {
      state.firmwareInstallPostPending = false;
      clearFirmwareWebOtaFallback();
      state.firmwareInstallStatus = "Installing update\u2026";
      postFirmwareUpdateInstall();
      updateState = "INSTALLING";
    } else if (!installWindowActive || (updateState === "NO UPDATE" && !publicFirmwareInstallAvailable())) {
      state.firmwareInstallPostPending = false;
    }
  }
  if (installWindowActive && updateState === "UPDATE AVAILABLE") {
    updateState = "INSTALLING";
  }
  state.firmwareUpdateState = updateState;
  if (state.firmwareUpdateState) state.firmwareInstallError = "";
  state.firmwareReleaseUrl = d.release_url || state.firmwareReleaseUrl || "";
  if (state.firmwareUpdateState === "NO UPDATE" &&
      !isSpecificFirmwareVersion(state.firmwareVersion) &&
      isSpecificFirmwareVersion(state.firmwareLatestVersion)) {
    setFirmwareVersion(state.firmwareLatestVersion);
  }
  if (state.firmwareUpdateState) state.firmwareChecking = false;
  if (state.firmwareUpdateState === "INSTALLING") {
    startFirmwareInstallRefresh();
  } else {
    stopFirmwareInstallRefreshIfComplete();
  }
  renderFirmwareUpdateStatus();
}

function firmwareVersionMatches(version, expected) {
  return String(version == null ? "" : version).trim() ===
    String(expected == null ? "" : expected).trim();
}

function stopFirmwareInstallRefresh() {
  if (firmwareInstallRefreshTimer) clearTimeout(firmwareInstallRefreshTimer);
  firmwareInstallRefreshTimer = null;
  firmwareInstallRefreshUntil = 0;
  clearFirmwareWebOtaFallback();
  state.firmwareInstallTargetVersion = "";
  state.firmwareInstallPostPending = false;
  state.firmwareInstallStatus = "";
}

function stopFirmwareInstallRefreshIfComplete() {
  var target = state.firmwareInstallTargetVersion;
  if (!target || state.firmwareUpdateState !== "NO UPDATE") return false;
  if (isSpecificFirmwareVersion(target) && !firmwareVersionMatches(state.firmwareVersion, target)) {
    setFirmwareVersion(target);
  }
  stopFirmwareInstallRefresh();
  return true;
}

function pollFirmwareInstallRefresh() {
  firmwareInstallRefreshTimer = null;
  refreshFirmwareVersion();
  if (stopFirmwareInstallRefreshIfComplete()) return;
  if (Date.now() >= firmwareInstallRefreshUntil) {
    stopFirmwareInstallRefresh();
    return;
  }
  firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
}

function startFirmwareInstallRefresh() {
  if (!state.firmwareInstallTargetVersion && isSpecificFirmwareVersion(state.firmwareLatestVersion)) {
    state.firmwareInstallTargetVersion = state.firmwareLatestVersion;
  }
  firmwareInstallRefreshUntil = Date.now() + 180000;
  if (firmwareInstallRefreshTimer) clearTimeout(firmwareInstallRefreshTimer);
  firmwareInstallRefreshTimer = setTimeout(pollFirmwareInstallRefresh, 5000);
}

function clearFirmwareWebOtaFallback() {
  if (firmwareWebOtaFallbackTimer) clearTimeout(firmwareWebOtaFallbackTimer);
  firmwareWebOtaFallbackTimer = null;
}

function scheduleFirmwareWebOtaFallback() {
  clearFirmwareWebOtaFallback();
  firmwareWebOtaFallbackTimer = setTimeout(function () {
    firmwareWebOtaFallbackTimer = null;
    if (!state.firmwareInstallPostPending) return;
    if (firmwareUpdateAvailable()) return;
    if (!publicFirmwareInstallAvailable()) return;
    installPublicFirmwareViaWebOta();
  }, FIRMWARE_WEB_OTA_FALLBACK_DELAY_MS);
}
