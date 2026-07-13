// ── Public Firmware Web OTA ────────────────────────────────────────────
// @web-module-requires: state, firmware_metadata, firmware_version_state, api

function ensurePublicFirmwareOtaUrl(info) {
  info = info || selectedFirmwareInfo();
  if (info && info.ota_url) return Promise.resolve(info.ota_url);
  if (state.firmwareOtaUrl) return Promise.resolve(state.firmwareOtaUrl);
  return getJsonQuietly(publicFirmwareVersionsUrl(), function (d) {
    setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
  }).then(function () {
    info = selectedFirmwareInfo();
    if (info && info.ota_url) return info.ota_url;
    return getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
      setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
    }).then(function () {
      return state.firmwareOtaUrl || "";
    });
  });
}

function publicFirmwareOtaFilename(info) {
  return info && info.ota_filename ? info.ota_filename :
    (state.firmwareOtaFilename || (DEVICE_ID + ".ota.bin"));
}

function installPublicFirmwareViaWebOta(info) {
  info = info || selectedFirmwareInfo();
  return getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
    if (!info || selectedFirmwareIsLatest()) setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
  }).then(function () {
    info = info || selectedFirmwareInfo();
    var targetVersion = info && info.latest_version ? info.latest_version : state.firmwareLatestVersion;
    if (isSpecificFirmwareVersion(targetVersion)) {
      state.firmwareInstallTargetVersion = targetVersion;
    }
  }).then(function () {
    clearFirmwareWebOtaFallback();
    state.firmwareInstallPostPending = false;
    state.firmwareChecking = false;
    state.firmwareUpdateState = "INSTALLING";
    state.firmwareInstallError = "";
    state.firmwareInstallStatus = state.firmwareInstallTargetVersion ?
      "Uploading firmware " + state.firmwareInstallTargetVersion + "\u2026" :
      "Uploading firmware update\u2026";
    renderFirmwareUpdateStatus();
    startFirmwareInstallRefresh();

    var uploadStarted = false;
    var uploadResponseReceived = false;
    return ensurePublicFirmwareOtaUrl(info).then(function (otaUrl) {
      if (!otaUrl) throw new Error("Firmware file is not available yet.");
      return _deviceApi.request(otaUrl, { cache: "no-store" });
    }).then(function (result) {
      if (result.kind === "network-error") throw result.error;
      var response = result.value;
      if (!response.ok) throw new Error("Could not download firmware file (" + response.status + ").");
      return response.blob();
    }).then(function (blob) {
      var filename = publicFirmwareOtaFilename(info);
      var form = new FormData();
      form.append("file", blob, filename);
      uploadStarted = true;
      return _deviceApi.request("/update", { method: "POST", body: form });
    }).then(function (result) {
      if (result.kind === "network-error") throw result.error;
      var response = result.value;
      uploadResponseReceived = true;
      return response.text().catch(function () {
        return "";
      }).then(function (text) {
        if (!response.ok) {
          throw new Error("Device rejected firmware upload (" + response.status + ").");
        }
        if (/update failed/i.test(text)) {
          throw new Error("Device reported that the firmware upload failed.");
        }
        waitForFirmwareRestart();
        return true;
      });
    }).catch(function (err) {
      if (uploadStarted && !uploadResponseReceived) {
        waitForFirmwareRestart();
        return true;
      }
      failPublicFirmwareUpload(err && err.message);
      return false;
    });
  });
}

function waitForFirmwareRestart() {
  state.firmwareInstallError = "";
  state.firmwareInstallStatus = "Waiting for device to restart\u2026";
  renderFirmwareUpdateStatus();
  setConfigLocked(true, "Waiting for device to restart\u2026");
  showBanner("Firmware uploaded. Waiting for device to restart\u2026", "offline");
  setTimeout(connectEvents, 5000);
}

function failPublicFirmwareUpload(message) {
  var reason = message || "Could not upload firmware update.";
  stopFirmwareInstallRefresh();
  state.firmwareUpdateState = "";
  state.firmwareInstallError = "Firmware update failed: " + reason;
  renderFirmwareUpdateStatus();
  showBanner(reason, "error");
}
