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
    cardContractExperimental: cardContractExperimental,
    cardContractHidden: cardContractHidden,
    cardContractOptions: cardContractOptions,
    cardContractDefaultConfig: cardContractDefaultConfig,
    cardContractDomains: cardContractDomains,
    cardContractMigrationAlias: cardContractMigrationAlias,
    cardContractOptionSupportedFor: cardContractOptionSupportedFor,
    SSE_ALIAS_GROUPS: SSE_ALIAS_GROUPS,
    BACKUP_CONFIG_VERSION: BACKUP_CONFIG_VERSION,
    BACKUP_FORMAT: BACKUP_FORMAT,
    createBackupConfig: createBackupConfig,
    normalizeBackupConfig: normalizeBackupConfig,
    planBackupImport: planBackupImport,
    switchConfirmationEnabled: switchConfirmationEnabled,
    switchConfirmationMode: switchConfirmationMode,
    switchConfirmationMessage: switchConfirmationMessage,
    switchConfirmationDefaultMessageForMode: switchConfirmationDefaultMessageForMode,
    switchConfirmationYesText: switchConfirmationYesText,
    switchConfirmationNoText: switchConfirmationNoText,
    sensorActiveColorEnabled: sensorActiveColorEnabled,
    normalizeTodoCountDisplayMode: normalizeTodoCountDisplayMode,
    normalizeTodoLabelDisplayMode: normalizeTodoLabelDisplayMode,
    normalizeTodoCompletedDisplayMode: normalizeTodoCompletedDisplayMode,
    todoCardShowCount: todoCardShowCount,
    todoCardLabelShowsCount: todoCardLabelShowsCount,
    todoCardShowsCompletedItems: todoCardShowsCompletedItems,
    dateTimeModeOptionValues: dateTimeModeOptionValues,
    normalizeDateTimeCardMode: normalizeDateTimeCardMode,
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
    actionCardStateEntity: actionCardStateEntity,
    actionCardStateUnit: actionCardStateUnit,
    actionCardStatePrecision: actionCardStatePrecision,
    actionCardStateDisplayMode: actionCardStateDisplayMode,
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
    normalizeAlarmOptions: normalizeAlarmOptions,
    buttonTypePickerKeysForExperimental: function (enabled, isSub, selectedTypeKey) {
      var oldExperimental = state.developerExperimentalFeatures;
      state.developerExperimentalFeatures = !!enabled;
      var keys = buttonTypePickerKeys(!!isSub, selectedTypeKey || "");
      state.developerExperimentalFeatures = oldExperimental;
      return keys;
    },
    buttonTypeVisibleInPickerForExperimental: function (key, enabled, isSub) {
      var oldExperimental = state.developerExperimentalFeatures;
      state.developerExperimentalFeatures = !!enabled;
      var visible = buttonTypeVisibleInPicker(key, !!isSub);
      state.developerExperimentalFeatures = oldExperimental;
      return visible;
    },
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
        experimental: buttonTypeRegistryValue(typeDef, "experimental", "") || "",
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
    parseBackOrderToken: parseBackOrderToken,
    backOrderToken: backOrderToken,
    backLabelFromOrder: backLabelFromOrder,
    subpageStateDisplayMode: subpageStateDisplayMode,
    buttonConfigNeedsMigration: buttonConfigNeedsMigration,
    subpageConfigNeedsMigration: subpageConfigNeedsMigration,
    normalizeTemperatureUnit: normalizeTemperatureUnit,
    normalizeScreensaverAction: normalizeScreensaverAction,
    screensaverActionOption: screensaverActionOption,
    normalizeScreensaverDimmedBrightness: normalizeScreensaverDimmedBrightness,
    previewHtmlValue: previewHtmlValue,
    buttonTypePreviewFor: function (type, button, options) {
      var oldTimezone = state.timezone;
      var oldUnit = state.temperatureUnit;
      var oldClockFormat = state.clockFormat;
      options = options || {};
      if (options.timezone != null) state.timezone = options.timezone;
      if (options.temperatureUnit != null) {
        state.temperatureUnit = normalizeTemperatureUnit(options.temperatureUnit);
      }
      if (options.clockFormat != null) state.clockFormat = options.clockFormat;
      var typeDef = BUTTON_TYPES[type || ""];
      var preview = typeDef && typeDef.renderPreview
        ? typeDef.renderPreview(button || {}, { escHtml: escHtml, cardSize: options.cardSize || 1 })
        : null;
      state.timezone = oldTimezone;
      state.temperatureUnit = oldUnit;
      state.clockFormat = oldClockFormat;
      return preview;
    },
    networkPreviewIconSlug: networkPreviewIconSlug,
    displayFirmwareVersion: displayFirmwareVersion,
    firmwareVersionFromMetadata: firmwareVersionFromMetadata,
    firmwareInfoFromPublicManifest: firmwareInfoFromPublicManifest,
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
    entityDetailPaths: entityDetailPaths,
    entityLookupNames: entityLookupNames,
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
      state.firmwareVersion = "";
      state.firmwareLatestVersion = "";
      state.firmwareUpdateState = "";
      state.firmwareReleaseUrl = "";
      state.firmwareChecking = false;
      state.firmwareUpdateControlsSupported = false;
      state.firmwareInstallControlsSupported = false;
      state.firmwareInstallTargetVersion = "";
      state.firmwareInstallPostPending = false;
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
      return result;
    },
    firmwareStateAfterPublicManifest: function (initialVersion, manifest) {
      var oldVersion = state.firmwareVersion;
      var oldLatest = state.firmwareLatestVersion;
      var oldUpdateState = state.firmwareUpdateState;
      var oldReleaseUrl = state.firmwareReleaseUrl;
      var oldInstallSupported = state.firmwareInstallControlsSupported;
      var oldInstallPostPending = state.firmwareInstallPostPending;
      state.firmwareVersion = "";
      state.firmwareLatestVersion = "";
      state.firmwareUpdateState = "";
      state.firmwareReleaseUrl = "";
      state.firmwareInstallControlsSupported = true;
      state.firmwareInstallPostPending = false;
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
