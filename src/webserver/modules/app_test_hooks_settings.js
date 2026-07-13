// @web-module-requires: environment_state, ntp_state, artwork_state, artwork_post_api, screensaver_state, firmware_version_state, clock_bar_state, clock_bar_post_api, entity_state, firmware_update_state, screensaver_timeout, public_firmware_install, state_loader_api

if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
  registerEspControlTestHookGroup("settings", {
    normalizeTemperatureUnit: normalizeTemperatureUnit,
    normalizeHomeAssistantArtworkPort: normalizeHomeAssistantArtworkPort,
    defaultTimezoneOptions: defaultTimezoneOptions,
    timezoneOptionsWithFallback: timezoneOptionsWithFallback,
    effectiveTimezoneOptionForWeb: effectiveTimezoneOptionForWeb,
    normalizeScreensaverAction: normalizeScreensaverAction,
    screensaverActionOption: screensaverActionOption,
    normalizeScreensaverDimmedBrightness: normalizeScreensaverDimmedBrightness,
    firmwareVersionFromMetadata: firmwareVersionFromMetadata,
    firmwareInfoFromPublicManifest: firmwareInfoFromPublicManifest,
    firmwareInfosFromPublicVersions: firmwareInfosFromPublicVersions,
    entityDetailPath: entityDetailPath,
    entityDetailPaths: entityDetailPaths,
    entityInitialDetail: entityInitialDetail,
    entityLookupNames: entityLookupNames,
    coverArtHideExternalInputPostUrls: coverArtHideExternalInputPostUrls,
    coverArtDelayPostUrls: coverArtDelayPostUrls,
    coverArtTrackOverlayDurationPostUrls: coverArtTrackOverlayDurationPostUrls,
    homeAssistantArtworkPortPostUrls: homeAssistantArtworkPortPostUrls,
    voiceServicesPostUrls: voiceServicesPostUrls,
    firmwareUpdateControlsVisibleFor: function (transport, supported) {
      var oldTransport = state.networkTransport;
      var oldSupported = state.firmwareUpdateControlsSupported;
      state.networkTransport = normalizeNetworkTransport(transport);
      state.firmwareUpdateControlsSupported = supported;
      var visible = firmwareUpdateControlsVisible();
      state.networkTransport = oldTransport;
      state.firmwareUpdateControlsSupported = oldSupported;
      return visible;
    },
    firmwareVersionAfterUpdateInfo: function (initialVersion, updateInfo) {
      var oldVersion = state.firmwareVersion;
      var oldLatest = state.firmwareLatestVersion;
      var oldUpdateState = state.firmwareUpdateState;
      var oldReleaseUrl = state.firmwareReleaseUrl;
      var oldChecking = state.firmwareChecking;
      var oldSupported = state.firmwareUpdateControlsSupported;
      var oldInstallSupported = state.firmwareInstallControlsSupported;
      var oldInstallTarget = state.firmwareInstallTargetVersion;
      var oldInstallPostPending = state.firmwareInstallPostPending;
      var oldOptions = state.firmwareVersionOptions;
      var oldSelected = state.firmwareSelectedVersion;
      var oldIndexLoaded = state.firmwareVersionIndexLoaded;
      state.firmwareVersion = "";
      state.firmwareLatestVersion = "";
      state.firmwareUpdateState = "";
      state.firmwareReleaseUrl = "";
      state.firmwareChecking = false;
      state.firmwareUpdateControlsSupported = false;
      state.firmwareInstallControlsSupported = false;
      state.firmwareInstallTargetVersion = "";
      state.firmwareInstallPostPending = false;
      state.firmwareVersionOptions = [];
      state.firmwareSelectedVersion = "";
      state.firmwareVersionIndexLoaded = false;
      setFirmwareVersion(initialVersion);
      setFirmwareUpdateInfo(updateInfo || {});
      var result = {
        version: state.firmwareVersion,
        latest: state.firmwareLatestVersion,
        updateState: state.firmwareUpdateState,
        installAvailable: firmwareInstallAvailable(),
      };
      state.firmwareVersion = oldVersion;
      state.firmwareLatestVersion = oldLatest;
      state.firmwareUpdateState = oldUpdateState;
      state.firmwareReleaseUrl = oldReleaseUrl;
      state.firmwareChecking = oldChecking;
      state.firmwareUpdateControlsSupported = oldSupported;
      state.firmwareInstallControlsSupported = oldInstallSupported;
      state.firmwareInstallTargetVersion = oldInstallTarget;
      state.firmwareInstallPostPending = oldInstallPostPending;
      state.firmwareVersionOptions = oldOptions;
      state.firmwareSelectedVersion = oldSelected;
      state.firmwareVersionIndexLoaded = oldIndexLoaded;
      return result;
    },
    firmwareStateAfterPublicManifest: function (initialVersion, manifest) {
      var oldVersion = state.firmwareVersion;
      var oldLatest = state.firmwareLatestVersion;
      var oldUpdateState = state.firmwareUpdateState;
      var oldReleaseUrl = state.firmwareReleaseUrl;
      var oldInstallSupported = state.firmwareInstallControlsSupported;
      var oldInstallPostPending = state.firmwareInstallPostPending;
      var oldOptions = state.firmwareVersionOptions;
      var oldSelected = state.firmwareSelectedVersion;
      var oldIndexLoaded = state.firmwareVersionIndexLoaded;
      state.firmwareVersion = "";
      state.firmwareLatestVersion = "";
      state.firmwareUpdateState = "";
      state.firmwareReleaseUrl = "";
      state.firmwareInstallControlsSupported = true;
      state.firmwareInstallPostPending = false;
      state.firmwareVersionOptions = [];
      state.firmwareSelectedVersion = "";
      state.firmwareVersionIndexLoaded = false;
      setFirmwareVersion(initialVersion);
      setPublicFirmwareInfo(firmwareInfoFromPublicManifest(manifest));
      var result = {
        version: state.firmwareVersion,
        latest: state.firmwareLatestVersion,
        updateState: state.firmwareUpdateState,
        releaseUrl: state.firmwareReleaseUrl,
        updateAvailable: firmwareUpdateAvailable(),
        installAvailable: firmwareInstallAvailable(),
      };
      state.firmwareVersion = oldVersion;
      state.firmwareLatestVersion = oldLatest;
      state.firmwareUpdateState = oldUpdateState;
      state.firmwareReleaseUrl = oldReleaseUrl;
      state.firmwareInstallControlsSupported = oldInstallSupported;
      state.firmwareInstallPostPending = oldInstallPostPending;
      state.firmwareVersionOptions = oldOptions;
      state.firmwareSelectedVersion = oldSelected;
      state.firmwareVersionIndexLoaded = oldIndexLoaded;
      return result;
    },
    firmwareStateAfterVersionIndex: function (initialVersion, versionIndex, selectedVersion) {
      var oldVersion = state.firmwareVersion;
      var oldLatest = state.firmwareLatestVersion;
      var oldUpdateState = state.firmwareUpdateState;
      var oldReleaseUrl = state.firmwareReleaseUrl;
      var oldOtaUrl = state.firmwareOtaUrl;
      var oldOtaFilename = state.firmwareOtaFilename;
      var oldOtaMd5 = state.firmwareOtaMd5;
      var oldInstallSupported = state.firmwareInstallControlsSupported;
      var oldOptions = state.firmwareVersionOptions;
      var oldSelected = state.firmwareSelectedVersion;
      var oldIndexLoaded = state.firmwareVersionIndexLoaded;
      state.firmwareVersion = "";
      state.firmwareLatestVersion = "";
      state.firmwareUpdateState = "";
      state.firmwareReleaseUrl = "";
      state.firmwareOtaUrl = "";
      state.firmwareOtaFilename = "";
      state.firmwareOtaMd5 = "";
      state.firmwareInstallControlsSupported = true;
      state.firmwareVersionOptions = [];
      state.firmwareSelectedVersion = "";
      state.firmwareVersionIndexLoaded = false;
      setFirmwareVersion(initialVersion);
      setPublicFirmwareVersions(firmwareInfosFromPublicVersions(versionIndex));
      if (selectedVersion) state.firmwareSelectedVersion = selectedVersion;
      var selected = selectedFirmwareInfo();
      var result = {
        latest: state.firmwareLatestVersion,
        selected: selected && selected.latest_version,
        installAvailable: firmwareInstallAvailable(),
        selectorVisible: firmwareVersionSelectorVisible(),
        installedSelected: selectedFirmwareMatchesInstalled(),
      };
      state.firmwareVersion = oldVersion;
      state.firmwareLatestVersion = oldLatest;
      state.firmwareUpdateState = oldUpdateState;
      state.firmwareReleaseUrl = oldReleaseUrl;
      state.firmwareOtaUrl = oldOtaUrl;
      state.firmwareOtaFilename = oldOtaFilename;
      state.firmwareOtaMd5 = oldOtaMd5;
      state.firmwareInstallControlsSupported = oldInstallSupported;
      state.firmwareVersionOptions = oldOptions;
      state.firmwareSelectedVersion = oldSelected;
      state.firmwareVersionIndexLoaded = oldIndexLoaded;
      return result;
    },
    firmwareFailureStatusFor: function (message) {
      var oldError = state.firmwareInstallError;
      var oldStatus = state.firmwareInstallStatus;
      var oldUpdateState = state.firmwareUpdateState;
      var oldTarget = state.firmwareInstallTargetVersion;
      var oldPostPending = state.firmwareInstallPostPending;
      failPublicFirmwareUpload(message);
      var result = {
        error: state.firmwareInstallError,
        updateState: state.firmwareUpdateState,
        installStatus: state.firmwareInstallStatus,
      };
      state.firmwareInstallError = oldError;
      state.firmwareInstallStatus = oldStatus;
      state.firmwareUpdateState = oldUpdateState;
      state.firmwareInstallTargetVersion = oldTarget;
      state.firmwareInstallPostPending = oldPostPending;
      return result;
    },
    screensaverTimeoutSupportedFor: function (value, limitsLoaded, min, max) {
      var oldLoaded = state.screensaverTimeoutLimitsLoaded;
      var oldMin = state.screensaverTimeoutMin;
      var oldMax = state.screensaverTimeoutMax;
      state.screensaverTimeoutLimitsLoaded = !!limitsLoaded;
      state.screensaverTimeoutMin = min;
      state.screensaverTimeoutMax = max;
      var supported = screensaverTimeoutSupported(value);
      state.screensaverTimeoutLimitsLoaded = oldLoaded;
      state.screensaverTimeoutMin = oldMin;
      state.screensaverTimeoutMax = oldMax;
      return supported;
    },
    temperatureUnitSymbolFor: function (timezone, unit, activeTimezone) {
      var oldTimezone = state.timezone;
      var oldActiveTimezone = state.activeTimezone;
      var oldUnit = state.temperatureUnit;
      state.timezone = timezone || oldTimezone;
      if (activeTimezone != null) state.activeTimezone = activeTimezone;
      state.temperatureUnit = normalizeTemperatureUnit(unit);
      var symbol = temperatureUnitSymbol();
      state.timezone = oldTimezone;
      state.activeTimezone = oldActiveTimezone;
      state.temperatureUnit = oldUnit;
      return symbol;
    },
  });
}
