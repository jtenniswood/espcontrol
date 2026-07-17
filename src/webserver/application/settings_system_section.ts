import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installSettingsSystemSectionModule(): GlobalDescriptors {
    // ── Settings System Section ────────────────────────────────────────
    function buildSystemSettingsCards(this: any) {
        var backupBody: any = document.createElement("div");
        var backupRow: any = document.createElement("div");
        backupRow.className = "sp-backup-btns";
        var exportBtn: any = createActionButton("sp-backup-btn", "Export", "download");
        exportBtn.addEventListener("click", exportConfig);
        backupRow.appendChild(exportBtn);
        var importBtn: any = createActionButton("sp-backup-btn", "Import", "upload");
        importBtn.addEventListener("click", importConfig);
        backupRow.appendChild(importBtn);
        backupBody.appendChild(backupRow);
        var backupCard: any = makeCollapsibleCard("Backup", backupBody, true);
        var fwBody: any = document.createElement("div");
        var fwOverview: any = document.createElement("div");
        fwOverview.className = "sp-fw-overview";
        var fwCurrentRow: any = document.createElement("div");
        fwCurrentRow.className = "sp-fw-row sp-fw-info-row";
        var fwCurrentLabel: any = document.createElement("span");
        fwCurrentLabel.className = "sp-fw-label";
        fwCurrentLabel.textContent = "Current version";
        var fwVersionLabel: any = document.createElement("span");
        fwVersionLabel.className = "sp-fw-version";
        fwCurrentRow.appendChild(fwCurrentLabel);
        fwCurrentRow.appendChild(fwVersionLabel);
        els.fwVersionLabel = fwVersionLabel;
        renderFirmwareVersion();
        var fwLatestRow: any = document.createElement("div");
        fwLatestRow.className = "sp-fw-row sp-fw-info-row";
        var fwLatestLabel: any = document.createElement("span");
        fwLatestLabel.className = "sp-fw-label";
        fwLatestLabel.textContent = "Available version";
        var fwLatestValue: any = document.createElement("span");
        fwLatestValue.className = "sp-fw-version";
        fwLatestRow.appendChild(fwLatestLabel);
        fwLatestRow.appendChild(fwLatestValue);
        els.fwLatestVersion = fwLatestValue;
        fwOverview.appendChild(fwCurrentRow);
        fwOverview.appendChild(fwLatestRow);
        var fwActions: any = document.createElement("div");
        fwActions.className = "sp-fw-actions sp-fw-actions-full";
        els.fwActions = fwActions;
        var fwInlineStatus: any = document.createElement("span");
        fwInlineStatus.className = "sp-fw-inline-status";
        fwActions.appendChild(fwInlineStatus);
        els.fwInlineStatus = fwInlineStatus;
        var fwCheckBtn: any = createActionButton("sp-fw-btn", "Check for Update");
        fwCheckBtn.addEventListener("click", function (this: any) {
            if (!firmwareUpdateControlsVisible())
                return;
            var installAction: any = latestFirmwareInstallAction();
            if (installAction !== "check") {
                var latestInfo: any = latestFirmwareInfo();
                state.firmwareInstallTargetVersion = latestInfo && latestInfo.latest_version ?
                    latestInfo.latest_version :
                    state.firmwareLatestVersion;
                state.firmwareInstallPostPending = installAction === "check_then_install";
                state.firmwareChecking = false;
                if (installAction === "install") {
                    state.firmwareUpdateState = "INSTALLING";
                    state.firmwareInstallStatus = "Installing update\u2026";
                    renderFirmwareUpdateStatus();
                    clearFirmwareWebOtaFallback();
                    postFirmwareUpdateInstall();
                    startFirmwareInstallRefresh();
                }
                else {
                    state.firmwareUpdateState = "INSTALLING";
                    state.firmwareInstallStatus = "Checking update before install\u2026";
                    renderFirmwareUpdateStatus();
                    postFirmwareUpdateCheck();
                    scheduleFirmwareWebOtaFallback();
                    startFirmwareInstallRefresh();
                }
                return;
            }
            state.firmwareChecking = true;
            renderFirmwareUpdateStatus();
            postFirmwareUpdateCheck();
            getJsonQuietly(publicFirmwareManifestUrl(), function (this: any, d?: any) {
                setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
            });
            getJsonQuietly(publicFirmwareVersionsUrl(), function (this: any, d?: any) {
                setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
            });
            setTimeout(function (this: any) {
                state.firmwareChecking = false;
                refreshFirmwareVersion();
                renderFirmwareUpdateStatus();
            }, 10000);
        });
        fwActions.appendChild(fwCheckBtn);
        els.fwCheckBtn = fwCheckBtn;
        fwOverview.appendChild(fwActions);
        var fwStatus: any = document.createElement("div");
        fwStatus.className = "sp-fw-status";
        fwOverview.appendChild(fwStatus);
        els.fwStatus = fwStatus;
        renderFirmwareUpdateStatus();
        var firmwareSubpanels: any = document.createElement("div");
        firmwareSubpanels.className = "sp-fw-subpanels";
        var firmwareUpdatesBadge: any = disclosureBadge("Update available", "Firmware update available");
        var firmwareUpdatesPanel: any = inlineDisclosure("Firmware updates", fwOverview, false, firmwareUpdatesBadge);
        firmwareUpdatesPanel.id = "sp-fw-updates-panel";
        els.firmwareUpdatesBadge = firmwareUpdatesBadge;
        firmwareSubpanels.appendChild(firmwareUpdatesPanel);
        var autoUpdateBody: any = document.createElement("div");
        var autoUpdateToggle: any = toggleRow("Auto Update", "sp-set-auto-update", state.autoUpdate);
        autoUpdateBody.appendChild(autoUpdateToggle.row);
        autoUpdateToggle.input.addEventListener("change", function (this: any) {
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
        var freqWrap: any = document.createElement("div");
        freqWrap.className = "sp-field";
        freqWrap.style.display = state.autoUpdate ? "" : "none";
        freqWrap.appendChild(fieldLabel("Update Frequency", "sp-set-update-freq"));
        var freqSelect: any = document.createElement("select");
        freqSelect.className = "sp-select";
        freqSelect.id = "sp-set-update-freq";
        state.updateFreqOptions.forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt;
            o.textContent = opt;
            freqSelect.appendChild(o);
        });
        freqSelect.value = state.updateFrequency;
        freqSelect.addEventListener("change", function (this: any) {
            if (!firmwareUpdateControlsVisible())
                return;
            state.updateFrequency = this.value;
            postFirmwareUpdateFrequency(state.updateFrequency);
        });
        freqWrap.appendChild(freqSelect);
        autoUpdateBody.appendChild(freqWrap);
        els.updateFreqWrap = freqWrap;
        els.setUpdateFreq = freqSelect;
        var autoUpdateBadge: any = disclosureBadge("On", "Automatic firmware updates on");
        var autoUpdatePanel: any = inlineDisclosure("Auto updates", autoUpdateBody, false, autoUpdateBadge);
        autoUpdatePanel.id = "sp-fw-auto-panel";
        els.autoUpdateBadge = autoUpdateBadge;
        els.autoUpdatePanel = autoUpdatePanel;
        firmwareSubpanels.appendChild(autoUpdatePanel);
        var wifiFirmwareBody: any = document.createElement("div");
        var c6CurrentRow: any = document.createElement("div");
        c6CurrentRow.className = "sp-fw-row sp-fw-info-row";
        var c6CurrentLabel: any = document.createElement("span");
        c6CurrentLabel.className = "sp-fw-label";
        c6CurrentLabel.textContent = "Current";
        var c6CurrentValue: any = document.createElement("span");
        c6CurrentValue.className = "sp-fw-version";
        c6CurrentRow.appendChild(c6CurrentLabel);
        c6CurrentRow.appendChild(c6CurrentValue);
        wifiFirmwareBody.appendChild(c6CurrentRow);
        els.c6FirmwareCurrent = c6CurrentValue;
        var c6LatestRow: any = document.createElement("div");
        c6LatestRow.className = "sp-fw-row sp-fw-info-row";
        var c6LatestLabel: any = document.createElement("span");
        c6LatestLabel.className = "sp-fw-label";
        c6LatestLabel.textContent = "Available";
        var c6LatestValue: any = document.createElement("span");
        c6LatestValue.className = "sp-fw-version";
        c6LatestRow.appendChild(c6LatestLabel);
        c6LatestRow.appendChild(c6LatestValue);
        wifiFirmwareBody.appendChild(c6LatestRow);
        els.c6FirmwareLatest = c6LatestValue;
        var c6AutoUpdateToggle: any = toggleRow("Auto Update", "sp-set-c6-auto-update", state.c6FirmwareAutoUpdate);
        c6AutoUpdateToggle.input.addEventListener("change", function (this: any) {
            if (!state.c6FirmwareAutoUpdateSupported) {
                syncC6FirmwareUi();
                return;
            }
            state.c6FirmwareAutoUpdate = this.checked;
            postC6FirmwareAutoUpdate(state.c6FirmwareAutoUpdate);
            syncC6FirmwareUi();
        });
        wifiFirmwareBody.appendChild(c6AutoUpdateToggle.row);
        els.c6FirmwareAutoUpdateRow = c6AutoUpdateToggle.row;
        els.c6FirmwareAutoUpdate = c6AutoUpdateToggle.input;
        var c6Actions: any = document.createElement("div");
        c6Actions.className = "sp-fw-actions sp-fw-actions-full";
        var c6UpdateBtn: any = createActionButton("sp-fw-btn", "Check for Update");
        c6UpdateBtn.addEventListener("click", function (this: any) {
            if (!state.c6FirmwareUpdateControlsSupported)
                return;
            if (c6FirmwareUpdateKnownAvailable() && state.c6FirmwareInstallControlsSupported) {
                state.c6FirmwareInstalling = true;
                state.c6FirmwareChecking = false;
                syncC6FirmwareUi();
                postC6FirmwareUpdateInstall();
                setTimeout(function (this: any) {
                    refreshFirmwareVersion();
                }, 5000);
                return;
            }
            state.c6FirmwareChecking = true;
            syncC6FirmwareUi();
            postC6FirmwareUpdateCheck();
            setTimeout(function (this: any) {
                state.c6FirmwareChecking = false;
                refreshFirmwareVersion();
                syncC6FirmwareUi();
            }, 10000);
        });
        c6Actions.appendChild(c6UpdateBtn);
        wifiFirmwareBody.appendChild(c6Actions);
        els.c6FirmwareUpdateBtn = c6UpdateBtn;
        var c6Status: any = document.createElement("div");
        c6Status.className = "sp-fw-status";
        wifiFirmwareBody.appendChild(c6Status);
        els.c6FirmwareStatus = c6Status;
        var c6Badge: any = disclosureBadge("Update available", "WiFi firmware update available");
        var wifiFirmwarePanel: any = inlineDisclosure("WiFi firmware", wifiFirmwareBody, false, c6Badge);
        wifiFirmwarePanel.id = "sp-fw-wifi-panel";
        els.c6FirmwareBadge = c6Badge;
        els.c6FirmwareCard = wifiFirmwarePanel;
        firmwareSubpanels.appendChild(wifiFirmwarePanel);
        var previousFirmwareBody: any = document.createElement("div");
        var fwVersionField: any = document.createElement("div");
        fwVersionField.className = "sp-field sp-fw-version-field";
        fwVersionField.appendChild(fieldLabel("Version", "sp-set-firmware-version"));
        var fwVersionSelect: any = document.createElement("select");
        fwVersionSelect.className = "sp-select";
        fwVersionSelect.id = "sp-set-firmware-version";
        fwVersionSelect.addEventListener("change", function (this: any) {
            state.firmwareSelectedVersion = this.value;
            syncPreviousFirmwareUi();
        });
        fwVersionField.appendChild(fwVersionSelect);
        previousFirmwareBody.appendChild(fwVersionField);
        els.fwVersionField = fwVersionField;
        els.fwVersionSelect = fwVersionSelect;
        var previousFirmwareActions: any = document.createElement("div");
        previousFirmwareActions.className = "sp-fw-previous-actions";
        var previousFirmwareInstallBtn: any = createActionButton("sp-fw-btn", "Install");
        previousFirmwareInstallBtn.addEventListener("click", function (this: any) {
            var info: any = selectedPreviousFirmwareInfo();
            if (!info || !firmwareUpdateControlsVisible())
                return;
            if (!window.confirm("Install older firmware " + info.latest_version + "? The display will restart during installation.")) {
                return;
            }
            installPublicFirmwareViaWebOta(info);
        });
        previousFirmwareActions.appendChild(previousFirmwareInstallBtn);
        previousFirmwareBody.appendChild(previousFirmwareActions);
        els.fwPreviousInstallBtn = previousFirmwareInstallBtn;
        var previousFirmwarePanel: any = inlineDisclosure("Previous firmware", previousFirmwareBody, false);
        previousFirmwarePanel.id = "sp-fw-previous-panel";
        els.fwPreviousPanel = previousFirmwarePanel;
        firmwareSubpanels.appendChild(previousFirmwarePanel);
        fwBody.appendChild(firmwareSubpanels);
        var firmwareCard: any = makeCollapsibleCard("Firmware", fwBody, true);
        syncFirmwareVersionSelect();
        syncFirmwareUpdateUi();
        syncC6FirmwareUi();
        refreshFirmwareVersion();
        var homeAssistantSettingsBody: any = document.createElement("div");
        var haProtocolField: any = document.createElement("div");
        haProtocolField.className = "sp-field";
        haProtocolField.appendChild(fieldLabel("Home Assistant Protocol", "sp-set-ha-artwork-protocol"));
        var haProtocolSelect: any = document.createElement("select");
        haProtocolSelect.className = "sp-select";
        haProtocolSelect.id = "sp-set-ha-artwork-protocol";
        ["http", "https"].forEach(function (this: any, option?: any) {
            var item: any = document.createElement("option");
            item.value = option;
            item.textContent = option;
            haProtocolSelect.appendChild(item);
        });
        haProtocolSelect.value = normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
        haProtocolSelect.addEventListener("change", function (this: any) {
            state.homeAssistantArtworkProtocol = normalizeHomeAssistantArtworkProtocol(this.value);
            this.value = state.homeAssistantArtworkProtocol;
            postHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
        });
        haProtocolField.appendChild(haProtocolSelect);
        homeAssistantSettingsBody.appendChild(haProtocolField);
        els.setHomeAssistantArtworkProtocol = haProtocolSelect;
        var haPortField: any = document.createElement("div");
        haPortField.className = "sp-field";
        haPortField.appendChild(fieldLabel("Home Assistant Port", "sp-set-ha-artwork-port"));
        var haPortInput: any = document.createElement("input");
        haPortInput.className = "sp-input sp-input--no-stepper";
        haPortInput.id = "sp-set-ha-artwork-port";
        haPortInput.type = "number";
        haPortInput.min = "1";
        haPortInput.max = "65535";
        haPortInput.step = "1";
        haPortInput.inputMode = "numeric";
        haPortInput.value = String(normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort));
        haPortInput.addEventListener("change", function (this: any) {
            state.coverArtHomeAssistantPort = normalizeHomeAssistantArtworkPort(this.value);
            this.value = String(state.coverArtHomeAssistantPort);
            postHomeAssistantArtworkPort(state.coverArtHomeAssistantPort);
        });
        haPortField.appendChild(haPortInput);
        homeAssistantSettingsBody.appendChild(haPortField);
        els.setCoverArtHomeAssistantPort = haPortInput;
        var homeAssistantSettingsCard: any = makeCollapsibleCard("Home Assistant Settings", homeAssistantSettingsBody, true);
        return {
            backupCard: backupCard,
            firmwareCard: firmwareCard,
            homeAssistantSettingsCard: homeAssistantSettingsCard,
        };
    }
    return {
        "buildSystemSettingsCards": staticGlobal(buildSystemSettingsCards),
    };
}
