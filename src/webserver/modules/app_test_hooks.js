if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
  globalThis.__ESPCONTROL_TEST_HOOKS__.config = {
    parseButtonConfig: parseButtonConfig,
    serializeButtonConfig: serializeButtonConfig,
    CARD_CONFIG_FIELDS: CARD_CONFIG_FIELDS,
    cardContractSubpageTypeCode: cardContractSubpageTypeCode,
    cardContractSubpageTypeFromCode: cardContractSubpageTypeFromCode,
    cardContractLargeNumbersSupported: cardContractLargeNumbersSupported,
    cardContractCardKeys: cardContractCardKeys,
    cardContractCardLabel: cardContractCardLabel,
    cardContractAllowInSubpage: cardContractAllowInSubpage,
    cardContractPickerKey: cardContractPickerKey,
    cardContractHidden: cardContractHidden,
    cardContractOptions: cardContractOptions,
    cardContractDefaultConfig: cardContractDefaultConfig,
    cardContractDomains: cardContractDomains,
    cardContractMigrationAlias: cardContractMigrationAlias,
    cardContractOptionSupportedFor: cardContractOptionSupportedFor,
    cardLargeNumbersEnabled: cardLargeNumbersEnabled,
    SSE_ALIAS_GROUPS: SSE_ALIAS_GROUPS,
    BACKUP_CONFIG_VERSION: BACKUP_CONFIG_VERSION,
    BACKUP_FORMAT: BACKUP_FORMAT,
    createBackupConfig: createBackupConfig,
    normalizeBackupConfig: normalizeBackupConfig,
    planBackupImport: planBackupImport,
    backupExportFileName: backupExportFileName,
    switchConfirmationEnabled: switchConfirmationEnabled,
    switchConfirmationMode: switchConfirmationMode,
    switchConfirmationMessage: switchConfirmationMessage,
    switchConfirmationDefaultMessageForMode: switchConfirmationDefaultMessageForMode,
    switchConfirmationYesText: switchConfirmationYesText,
    switchConfirmationNoText: switchConfirmationNoText,
    normalizeCardOnPattern: normalizeCardOnPattern,
    cardOnPattern: cardOnPattern,
    setCardOnPattern: setCardOnPattern,
    sensorActiveColorEnabled: sensorActiveColorEnabled,
    sensorStateLabelsEnabled: sensorStateLabelsEnabled,
    sensorStateInput: sensorStateInput,
    sensorStateOutput: sensorStateOutput,
    sensorStateInput2: sensorStateInput2,
    sensorStateOutput2: sensorStateOutput2,
    setSensorStateTranslation: setSensorStateTranslation,
    setSensorStateTranslations: setSensorStateTranslations,
    dateTimeModeOptionValues: dateTimeModeOptionValues,
    normalizeDateTimeCardMode: normalizeDateTimeCardMode,
    dateTimeLargeNumbersLabel: dateTimeLargeNumbersLabel,
    weatherModeOptionValues: weatherModeOptionValues,
    normalizeWeatherCardMode: normalizeWeatherCardMode,
    weatherCardIsForecastMode: weatherCardIsForecastMode,
    coverModeOptionValues: coverModeOptionValues,
    normalizeCoverMode: normalizeCoverMode,
    normalizeCoverPosition: normalizeCoverPosition,
    lightTempDefaultRange: lightTempDefaultRange,
    lightTempParseRange: lightTempParseRange,
    lightTempClampMin: lightTempClampMin,
    lightTempClampMax: lightTempClampMax,
    lightTempLegacySensorValues: lightTempLegacySensorValues,
    lightTempSensorNeedsCleanup: lightTempSensorNeedsCleanup,
    doorWindowActiveColorEnabled: doorWindowActiveColorEnabled,
    presenceActiveColorEnabled: presenceActiveColorEnabled,
    garageModeOptionValues: garageModeOptionValues,
    normalizeGarageMode: normalizeGarageMode,
    normalizeGarageLabelDisplayMode: normalizeGarageLabelDisplayMode,
    garageLabelDisplayMode: garageLabelDisplayMode,
    lockModeOptionValues: lockModeOptionValues,
    normalizeLockMode: normalizeLockMode,
    pushDefaultIcon: pushDefaultIcon,
    pushDefaultIconOn: pushDefaultIconOn,
    webhookMethod: webhookMethod,
    webhookHeaders: webhookHeaders,
    internalRelayModeOptionValues: internalRelayModeOptionValues,
    normalizeInternalRelayMode: normalizeInternalRelayMode,
    internalRelayDefaultIcon: internalRelayDefaultIcon,
    internalRelayDefaultOnIcon: internalRelayDefaultOnIcon,
    mediaModeOptionValues: mediaModeOptionValues,
    mediaEditorMode: mediaEditorMode,
    mediaNowPlayingControlValues: mediaNowPlayingControlValues,
    mediaNowPlayingControls: mediaNowPlayingControls,
    mediaStateDisplayModeSupported: mediaStateDisplayModeSupported,
    normalizeMediaOptions: normalizeMediaOptions,
    mediaVolumeMax: mediaVolumeMax,
    setMediaVolumeMax: setMediaVolumeMax,
    imageRefreshIntervalValues: imageRefreshIntervalValues,
    imageRefreshModeValues: imageRefreshModeValues,
    imageModalModeValues: imageModalModeValues,
    normalizeImageOptions: normalizeImageOptions,
    imageLabelEnabled: imageLabelEnabled,
    imageIconEnabled: imageIconEnabled,
    imageModalMode: imageModalMode,
    imageRefreshInterval: imageRefreshInterval,
    imageRefreshMode: imageRefreshMode,
    imageCardLimit: imageCardLimit,
    imageCardCountForTest: function (snapshot, candidate) {
      var oldGrid = state.grid;
      var oldButtons = state.buttons;
      var oldSubpages = state.subpages;
      state.grid = (snapshot && snapshot.grid) || [];
      state.buttons = (snapshot && snapshot.buttons) || [];
      state.subpages = (snapshot && snapshot.subpages) || {};
      try {
        return imageCardCountWithCandidate(candidate);
      } finally {
        state.grid = oldGrid;
        state.buttons = oldButtons;
        state.subpages = oldSubpages;
      }
    },
    imageCardCandidateAllowedForTest: function (snapshot, candidate) {
      var oldGrid = state.grid;
      var oldButtons = state.buttons;
      var oldSubpages = state.subpages;
      state.grid = (snapshot && snapshot.grid) || [];
      state.buttons = (snapshot && snapshot.buttons) || [];
      state.subpages = (snapshot && snapshot.subpages) || {};
      try {
        return imageCardCountWithCandidate(candidate) <= imageCardLimit();
      } finally {
        state.grid = oldGrid;
        state.buttons = oldButtons;
        state.subpages = oldSubpages;
      }
    },
    actionCardStateEntity: actionCardStateEntity,
    actionCardStateUnit: actionCardStateUnit,
    actionCardStatePrecision: actionCardStatePrecision,
    actionCardStateDisplayMode: actionCardStateDisplayMode,
    actionScriptConfirmationEnabled: actionScriptConfirmationEnabled,
    actionScriptConfirmationMessage: actionScriptConfirmationMessage,
    actionScriptConfirmationYesText: actionScriptConfirmationYesText,
    actionScriptConfirmationNoText: actionScriptConfirmationNoText,
    alarmPinRequired: alarmPinRequired,
    alarmIconDisplayMode: alarmIconDisplayMode,
    alarmLabelDisplayMode: alarmLabelDisplayMode,
    alarmControlPanelValue: alarmControlPanelValue,
    alarmActionValues: alarmActionValues,
    normalizeAlarmIconDisplayMode: normalizeAlarmIconDisplayMode,
    normalizeAlarmLabelDisplayMode: normalizeAlarmLabelDisplayMode,
    alarmVisibleActions: alarmVisibleActions,
    normalizeClimateLabelDisplayMode: normalizeClimateLabelDisplayMode,
    normalizeClimateNumberDisplayMode: normalizeClimateNumberDisplayMode,
    climateDefaultLabelDisplayMode: climateDefaultLabelDisplayMode,
    climateDefaultNumberDisplayMode: climateDefaultNumberDisplayMode,
    climatePrecisionValues: climatePrecisionValues,
    parseClimatePrecisionConfig: parseClimatePrecisionConfig,
    normalizeClimatePrecisionConfig: normalizeClimatePrecisionConfig,
    alarmCardTypeOptionValues: function (isSub) {
      return alarmCardTypeOptionsForSettings(!!isSub).map(function (option) {
        return option.value;
      });
    },
    coverModeOptionLabels: function (currentMode) {
      var options = coverModeOptionsForSettings(currentMode || "");
      return options.map(function (option) { return option[0] + ":" + option[1]; });
    },
    normalizeAlarmOptions: normalizeAlarmOptions,
    buttonTypePickerKeysFor: function (isSub, selectedTypeKey) {
      var keys = buttonTypePickerKeys(!!isSub, selectedTypeKey || "");
      return keys;
    },
    buttonTypeVisibleInPickerFor: function (key, isSub) {
      var visible = buttonTypeVisibleInPicker(key, !!isSub);
      return visible;
    },
    buttonTypePickerKeysForInfoOnly: function (enabled, selectedTypeKey) {
      var oldInfoOnly = CFG.infoOnly;
      CFG.infoOnly = !!enabled;
      var keys = buttonTypePickerKeys(false, selectedTypeKey);
      CFG.infoOnly = oldInfoOnly;
      return keys;
    },
    buttonTypePickerOptionsFor: function (isSub, selectedTypeKey) {
      return buttonTypePickerOptionList(!!isSub, selectedTypeKey == null ? null : selectedTypeKey);
    },
    defaultButtonTypeForPicker: defaultButtonTypeForPicker,
    buttonTypesMissingCardMetadata: function () {
      var missing = [];
      for (var key in BUTTON_TYPES) {
        if (!BUTTON_TYPES[key].cardMetadata) missing.push(key);
      }
      return missing.sort();
    },
    buttonTypeDefaultConfig: function (type) {
      var typeDef = BUTTON_TYPES[type || ""];
      var config = typeDef && typeDef.defaultConfig;
      if (typeof config === "function") config = config();
      return config ? EspControlModel.cloneCardConfig(config) : null;
    },
    buttonTypeRuntimeSpec: function (type) {
      var typeDef = BUTTON_TYPES[type || ""];
      var metadata = typeDef && typeDef.cardMetadata;
      var entity = metadata && metadata.entity;
      return typeDef ? {
        label: buttonTypeRegistryValue(typeDef, "label", typeDef.key || "Toggle"),
        allowInSubpage: !!buttonTypeRegistryValue(typeDef, "allowInSubpage", false),
        pickerKey: buttonTypeRegistryValue(typeDef, "pickerKey", "") || "",
        hidden: !!buttonTypeRegistryValue(typeDef, "hidden", false),
        domains: entity && entity.domains
          ? cardMetadataValue(entity.domains, {}, {}) || []
          : cardContractDomains(typeDef.key),
      } : null;
    },
    parseSubpageConfig: parseSubpageConfig,
    serializeSubpageConfig: serializeSubpageConfig,
    buildSubpageGrid: buildSubpageGrid,
    serializeSubpageGrid: serializeSubpageGrid,
    splitSubpageConfigChunks: EspControlModel.splitSubpageConfigChunks,
    subpageChunkPostKeysFor: function (full, raw, previousPending) {
      var oldRaw = state.subpageRaw[1];
      var oldPending = state.subpageSavePending[1];
      state.subpageRaw[1] = raw || {};
      state.subpageSavePending[1] = previousPending || "";
      var keys = subpageEntityKeys();
      var chunks = EspControlModel.splitSubpageConfigChunks(full || "", keys.length, 255) || [];
      var previousPendingChunks = EspControlModel.splitSubpageConfigChunks(
        state.subpageSavePending[1] || "", keys.length, 255) || [];
      var out = keys.filter(function (_key, index) {
        return subpageChunkShouldPost(1, keys, chunks, index, previousPendingChunks);
      });
      state.subpageRaw[1] = oldRaw;
      state.subpageSavePending[1] = oldPending;
      return out;
    },
    parseBackOrderToken: parseBackOrderToken,
    backOrderToken: backOrderToken,
    backLabelFromOrder: backLabelFromOrder,
    subpageStateDisplayMode: subpageStateDisplayMode,
    subpageKind: subpageKind,
    buttonConfigNeedsMigration: buttonConfigNeedsMigration,
    subpageConfigNeedsMigration: subpageConfigNeedsMigration,
    normalizeTemperatureUnit: normalizeTemperatureUnit,
    normalizeHomeAssistantArtworkPort: normalizeHomeAssistantArtworkPort,
    defaultTimezoneOptions: defaultTimezoneOptions,
    timezoneOptionsWithFallback: timezoneOptionsWithFallback,
    normalizeScreensaverAction: normalizeScreensaverAction,
    screensaverActionOption: screensaverActionOption,
    clockBarVisibleInPreviewFor: function (clockBarOn, screensaverAction) {
      var oldClockBarOn = state.clockBarOn;
      var oldScreensaverAction = state.screensaverAction;
      state.clockBarOn = !!clockBarOn;
      state.screensaverAction = normalizeScreensaverAction(screensaverAction);
      var visible = clockBarVisibleInPreview();
      state.clockBarOn = oldClockBarOn;
      state.screensaverAction = oldScreensaverAction;
      return visible;
    },
    clockBarStateAfterEvents: function (events) {
      var oldClockBarOn = state.clockBarOn;
      var oldSourceValues = state._clockBarStateValues;
      state.clockBarOn = false;
      state._clockBarStateValues = {};
      (events || []).forEach(function (event) {
        var keys = entityStateKeys(event || {});
        var matchedKey = "";
        for (var i = 0; i < keys.length; i++) {
          if (SSE_ALIAS_GROUPS.clockBar.indexOf(keys[i]) !== -1) {
            matchedKey = keys[i];
            break;
          }
        }
        applyClockBarStateValue(
          event && event.state != null ? String(event.state) : "",
          event || {},
          matchedKey
        );
      });
      var result = state.clockBarOn;
      state.clockBarOn = oldClockBarOn;
      state._clockBarStateValues = oldSourceValues;
      return result;
    },
    removedLegacyStateEvent: function (event) {
      var keys = entityStateKeys(event || {});
      var id = keys[0] || event && event.id || "";
      return isRemovedLegacyStateEvent(id, event || {});
    },
    normalizeScreensaverDimmedBrightness: normalizeScreensaverDimmedBrightness,
    previewHtmlValue: previewHtmlValue,
    buttonTypePreviewFor: function (type, button, options) {
      var oldTimezone = state.timezone;
      var oldUnit = state.temperatureUnit;
      var oldClockFormat = state.clockFormat;
      var oldLanguage = state.language;
      options = options || {};
      if (options.timezone != null) state.timezone = options.timezone;
      if (options.temperatureUnit != null) {
        state.temperatureUnit = normalizeTemperatureUnit(options.temperatureUnit);
      }
      if (options.clockFormat != null) state.clockFormat = options.clockFormat;
      if (options.language != null) state.language = normalizeLanguage(options.language);
      var typeDef = BUTTON_TYPES[type || ""];
      var preview = typeDef && typeDef.renderPreview
        ? typeDef.renderPreview(button || {}, { escHtml: escHtml, cardSize: options.cardSize || 1 })
        : null;
      state.timezone = oldTimezone;
      state.temperatureUnit = oldUnit;
      state.clockFormat = oldClockFormat;
      state.language = oldLanguage;
      return preview;
    },
    networkPreviewIconSlug: networkPreviewIconSlug,
    displayFirmwareVersion: displayFirmwareVersion,
    firmwareVersionFromMetadata: firmwareVersionFromMetadata,
    firmwareInfoFromPublicManifest: firmwareInfoFromPublicManifest,
    firmwareInfosFromPublicVersions: firmwareInfosFromPublicVersions,
    firmwareVersionLabelFor: function (version, pending) {
      var oldVersion = state.firmwareVersion;
      var oldPending = state.firmwareVersionRefreshPending;
      state.firmwareVersion = version;
      state.firmwareVersionRefreshPending = !!pending;
      var label = firmwareVersionLabel();
      state.firmwareVersion = oldVersion;
      state.firmwareVersionRefreshPending = oldPending;
      return label;
    },
    entityDetailPath: entityDetailPath,
    entityDetailPaths: entityDetailPaths,
    entityInitialDetail: entityInitialDetail,
    entityLookupNames: entityLookupNames,
    coverArtHideExternalInputPostUrls: coverArtHideExternalInputPostUrls,
    coverArtDelayPostUrls: coverArtDelayPostUrls,
    coverArtTrackOverlayDurationPostUrls: coverArtTrackOverlayDurationPostUrls,
    homeAssistantArtworkPortPostUrls: homeAssistantArtworkPortPostUrls,
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
    findDuplicatePlacementFor: function (grid, start, size, maxSlots) {
      return findDuplicatePlacement(grid.slice(), start, size, maxSlots || NUM_SLOTS);
    },
    importedButtonOrderFor: function (orderStr, existingSizes) {
      var oldSizes = state.sizes;
      var oldGrid = state.grid;
      state.sizes = existingSizes || {};
      state.grid = [];
      for (var i = 0; i < NUM_SLOTS; i++) state.grid.push(0);
      applyImportedButtonOrder(orderStr, {});
      var sizes = {};
      for (var k in state.sizes) sizes[k] = state.sizes[k];
      var grid = state.grid.slice();
      state.sizes = oldSizes;
      state.grid = oldGrid;
      return { grid: grid, sizes: sizes };
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
    temperatureUnitSymbolFor: function (timezone, unit) {
      var oldTimezone = state.timezone;
      var oldUnit = state.temperatureUnit;
      state.timezone = timezone || oldTimezone;
      state.temperatureUnit = normalizeTemperatureUnit(unit);
      var symbol = temperatureUnitSymbol();
      state.timezone = oldTimezone;
      state.temperatureUnit = oldUnit;
      return symbol;
    },
  };
}
