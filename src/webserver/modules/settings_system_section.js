// ── Settings System Section ────────────────────────────────────────
// @web-module-requires: state, firmware_version_state, firmware_update_state, c6_firmware_ui, api, public_firmware_install, config_codec, controls, controls_shell

function buildSystemSettingsCards() {
  var backupBody = document.createElement("div");

  var backupRow = document.createElement("div");
  backupRow.className = "sp-backup-btns";

  var exportBtn = createActionButton("sp-backup-btn", "Export", "download");
  exportBtn.addEventListener("click", exportConfig);
  backupRow.appendChild(exportBtn);

  var importBtn = createActionButton("sp-backup-btn", "Import", "upload");
  importBtn.addEventListener("click", importConfig);
  backupRow.appendChild(importBtn);

  backupBody.appendChild(backupRow);
  var backupCard = makeCollapsibleCard("Backup", backupBody, true);

  var fwBody = document.createElement("div");

  var fwVersionRow = document.createElement("div");
  fwVersionRow.className = "sp-fw-row";
  var fwVersionLabel = document.createElement("span");
  fwVersionLabel.className = "sp-fw-version";
  fwVersionRow.appendChild(fwVersionLabel);
  els.fwVersionLabel = fwVersionLabel;
  renderFirmwareVersion();
  refreshFirmwareVersion();

  var fwActions = document.createElement("div");
  fwActions.className = "sp-fw-actions";
  els.fwActions = fwActions;
  var fwInlineStatus = document.createElement("span");
  fwInlineStatus.className = "sp-fw-inline-status";
  fwActions.appendChild(fwInlineStatus);
  els.fwInlineStatus = fwInlineStatus;

  var fwCheckBtn = createActionButton("sp-fw-btn", "Check for Update");
  fwCheckBtn.addEventListener("click", function () {
    if (!firmwareUpdateControlsVisible()) return;
    if (firmwareInstallAvailable()) {
      var selectedInfo = selectedFirmwareInfo();
      var installingLatest = selectedFirmwareIsLatest();
      var updateReady = installingLatest && firmwareUpdateAvailable();
      state.firmwareInstallTargetVersion = selectedInfo && selectedInfo.latest_version ?
        selectedInfo.latest_version :
        state.firmwareLatestVersion;
      state.firmwareInstallPostPending = installingLatest && !updateReady;
      state.firmwareChecking = false;
      if (updateReady) {
        state.firmwareUpdateState = "INSTALLING";
        state.firmwareInstallStatus = "Installing update\u2026";
        renderFirmwareUpdateStatus();
        clearFirmwareWebOtaFallback();
        postFirmwareUpdateInstall();
        startFirmwareInstallRefresh();
      } else if (installingLatest) {
        state.firmwareUpdateState = "INSTALLING";
        state.firmwareInstallStatus = "Checking update before install\u2026";
        renderFirmwareUpdateStatus();
        postFirmwareUpdateCheck();
        scheduleFirmwareWebOtaFallback();
        startFirmwareInstallRefresh();
      } else {
        installPublicFirmwareViaWebOta(selectedInfo);
      }
      return;
    }
    state.firmwareChecking = true;
    renderFirmwareUpdateStatus();
    postFirmwareUpdateCheck();
    getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
      setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
    });
    getJsonQuietly(publicFirmwareVersionsUrl(), function (d) {
      setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
    });
    setTimeout(function () {
      state.firmwareChecking = false;
      refreshFirmwareVersion();
      renderFirmwareUpdateStatus();
    }, 10000);
  });
  fwActions.appendChild(fwCheckBtn);
  fwVersionRow.appendChild(fwActions);
  els.fwCheckBtn = fwCheckBtn;
  fwBody.appendChild(fwVersionRow);

  var fwStatus = document.createElement("div");
  fwStatus.className = "sp-fw-status";
  fwBody.appendChild(fwStatus);
  els.fwStatus = fwStatus;
  renderFirmwareUpdateStatus();

  var fwVersionField = document.createElement("div");
  fwVersionField.className = "sp-field sp-fw-version-field";
  fwVersionField.style.display = "none";
  fwVersionField.appendChild(fieldLabel("Install Version", "sp-set-firmware-version"));
  var fwVersionSelect = document.createElement("select");
  fwVersionSelect.className = "sp-select";
  fwVersionSelect.id = "sp-set-firmware-version";
  fwVersionSelect.addEventListener("change", function () {
    state.firmwareSelectedVersion = this.value;
    renderFirmwareUpdateStatus();
  });
  fwVersionField.appendChild(fwVersionSelect);
  fwBody.appendChild(fwVersionField);
  els.fwVersionField = fwVersionField;
  els.fwVersionSelect = fwVersionSelect;
  syncFirmwareVersionSelect();

  var autoUpdateToggle = toggleRow("Auto Update", "sp-set-auto-update", state.autoUpdate);
  fwBody.appendChild(autoUpdateToggle.row);
  autoUpdateToggle.input.addEventListener("change", function () {
    if (!firmwareUpdateControlsVisible()) {
      syncFirmwareUpdateUi();
      return;
    }
    state.autoUpdate = this.checked;
    postFirmwareAutoUpdate(state.autoUpdate);
    syncFirmwareUpdateUi();
  });
  els.setAutoUpdateRow = autoUpdateToggle.row;
  els.setAutoUpdate = autoUpdateToggle.input;

  var freqWrap = document.createElement("div");
  freqWrap.style.display = state.autoUpdate ? "" : "none";
  var freqSelect = document.createElement("select");
  freqSelect.className = "sp-select";
  freqSelect.id = "sp-set-update-freq";
  state.updateFreqOptions.forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt;
    o.textContent = opt;
    freqSelect.appendChild(o);
  });
  freqSelect.value = state.updateFrequency;
  freqSelect.addEventListener("change", function () {
    if (!firmwareUpdateControlsVisible()) return;
    state.updateFrequency = this.value;
    postFirmwareUpdateFrequency(state.updateFrequency);
  });
  freqWrap.appendChild(freqSelect);
  fwBody.appendChild(freqWrap);
  els.updateFreqWrap = freqWrap;
  els.setUpdateFreq = freqSelect;
  syncFirmwareUpdateUi();

  var firmwareCard = makeCollapsibleCard("Firmware", fwBody, true);

  var wifiFirmwareBody = document.createElement("div");
  var c6CurrentRow = document.createElement("div");
  c6CurrentRow.className = "sp-fw-row sp-fw-info-row";
  var c6CurrentLabel = document.createElement("span");
  c6CurrentLabel.className = "sp-fw-label";
  c6CurrentLabel.textContent = "Current C6 Firmware";
  var c6CurrentValue = document.createElement("span");
  c6CurrentValue.className = "sp-fw-version";
  c6CurrentRow.appendChild(c6CurrentLabel);
  c6CurrentRow.appendChild(c6CurrentValue);
  wifiFirmwareBody.appendChild(c6CurrentRow);
  els.c6FirmwareCurrent = c6CurrentValue;

  var c6LatestRow = document.createElement("div");
  c6LatestRow.className = "sp-fw-row sp-fw-info-row";
  var c6LatestLabel = document.createElement("span");
  c6LatestLabel.className = "sp-fw-label";
  c6LatestLabel.textContent = "Available C6 Firmware";
  var c6LatestValue = document.createElement("span");
  c6LatestValue.className = "sp-fw-version";
  c6LatestRow.appendChild(c6LatestLabel);
  c6LatestRow.appendChild(c6LatestValue);
  wifiFirmwareBody.appendChild(c6LatestRow);
  els.c6FirmwareLatest = c6LatestValue;

  var c6Actions = document.createElement("div");
  c6Actions.className = "sp-fw-actions sp-fw-actions-full";
  var c6UpdateBtn = document.createElement("button");
  c6UpdateBtn.className = "sp-fw-btn";
  c6UpdateBtn.textContent = "Check for Update";
  c6UpdateBtn.addEventListener("click", function () {
    if (!state.c6FirmwareUpdateControlsSupported) return;
    if (c6FirmwareUpdateKnownAvailable() && state.c6FirmwareInstallControlsSupported) {
      state.c6FirmwareInstalling = true;
      state.c6FirmwareChecking = false;
      syncC6FirmwareUi();
      postC6FirmwareUpdateInstall();
      setTimeout(function () {
        refreshFirmwareVersion();
      }, 5000);
      return;
    }
    state.c6FirmwareChecking = true;
    syncC6FirmwareUi();
    postC6FirmwareUpdateCheck();
    setTimeout(function () {
      state.c6FirmwareChecking = false;
      refreshFirmwareVersion();
      syncC6FirmwareUi();
    }, 10000);
  });
  c6Actions.appendChild(c6UpdateBtn);
  wifiFirmwareBody.appendChild(c6Actions);
  els.c6FirmwareUpdateBtn = c6UpdateBtn;

  var c6Status = document.createElement("div");
  c6Status.className = "sp-fw-status";
  wifiFirmwareBody.appendChild(c6Status);
  els.c6FirmwareStatus = c6Status;
  var wifiFirmwareCard = makeCollapsibleCard("WiFi", wifiFirmwareBody, true);
  els.c6FirmwareCard = wifiFirmwareCard;
  syncC6FirmwareUi();

  var homeAssistantSettingsBody = document.createElement("div");
  var haProtocolField = document.createElement("div");
  haProtocolField.className = "sp-field";
  haProtocolField.appendChild(fieldLabel("Home Assistant Protocol", "sp-set-ha-artwork-protocol"));
  var haProtocolSelect = document.createElement("select");
  haProtocolSelect.className = "sp-select";
  haProtocolSelect.id = "sp-set-ha-artwork-protocol";
  ["http", "https"].forEach(function (option) {
    var item = document.createElement("option");
    item.value = option;
    item.textContent = option;
    haProtocolSelect.appendChild(item);
  });
  haProtocolSelect.value = normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
  haProtocolSelect.addEventListener("change", function () {
    state.homeAssistantArtworkProtocol = normalizeHomeAssistantArtworkProtocol(this.value);
    this.value = state.homeAssistantArtworkProtocol;
    postHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
  });
  haProtocolField.appendChild(haProtocolSelect);
  homeAssistantSettingsBody.appendChild(haProtocolField);
  els.setHomeAssistantArtworkProtocol = haProtocolSelect;

  var haPortField = document.createElement("div");
  haPortField.className = "sp-field";
  haPortField.appendChild(fieldLabel("Home Assistant Port", "sp-set-ha-artwork-port"));
  var haPortInput = document.createElement("input");
  haPortInput.className = "sp-input sp-input--no-stepper";
  haPortInput.id = "sp-set-ha-artwork-port";
  haPortInput.type = "number";
  haPortInput.min = "1";
  haPortInput.max = "65535";
  haPortInput.step = "1";
  haPortInput.inputMode = "numeric";
  haPortInput.value = String(normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort));
  haPortInput.addEventListener("change", function () {
    state.coverArtHomeAssistantPort = normalizeHomeAssistantArtworkPort(this.value);
    this.value = String(state.coverArtHomeAssistantPort);
    postHomeAssistantArtworkPort(state.coverArtHomeAssistantPort);
  });
  haPortField.appendChild(haPortInput);
  homeAssistantSettingsBody.appendChild(haPortField);
  els.setCoverArtHomeAssistantPort = haPortInput;
  var homeAssistantSettingsCard = makeCollapsibleCard(
    "Home Assistant Settings",
    homeAssistantSettingsBody,
    true);


  return {
    backupCard: backupCard,
    firmwareCard: firmwareCard,
    wifiFirmwareCard: wifiFirmwareCard,
    homeAssistantSettingsCard: homeAssistantSettingsCard,
  };
}
