if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
  registerEspControlTestHookGroup("config", {
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
    sensorCardIsLocal: sensorCardIsLocal,
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
    coverControlTabDefinitions: coverControlTabDefinitions,
    coverControlTabs: coverControlTabs,
    normalizeCoverControlTabs: normalizeCoverControlTabs,
    normalizeCoverOptions: normalizeCoverOptions,
    fanControlTabDefinitions: fanControlTabDefinitions,
    fanControlTabs: fanControlTabs,
    normalizeFanControlTabs: normalizeFanControlTabs,
    normalizeFanControlOptions: normalizeFanControlOptions,
    lightTempDefaultRange: lightTempDefaultRange,
    lightTempParseRange: lightTempParseRange,
    lightTempClampMin: lightTempClampMin,
    lightTempClampMax: lightTempClampMax,
    lightTempLegacySensorValues: lightTempLegacySensorValues,
    lightTempSensorNeedsCleanup: lightTempSensorNeedsCleanup,
    lightControlTabDefinitions: lightControlTabDefinitions,
    lightControlTabs: lightControlTabs,
    normalizeLightControlTabs: normalizeLightControlTabs,
    normalizeLightControlOptions: normalizeLightControlOptions,
    doorWindowActiveColorEnabled: doorWindowActiveColorEnabled,
    presenceActiveColorEnabled: presenceActiveColorEnabled,
    garageModeOptionValues: garageModeOptionValues,
    normalizeGarageMode: normalizeGarageMode,
    normalizeGarageLabelDisplayMode: normalizeGarageLabelDisplayMode,
    garageLabelDisplayMode: garageLabelDisplayMode,
    gateModeOptionValues: gateModeOptionValues,
    normalizeGateMode: normalizeGateMode,
    normalizeGateLabelDisplayMode: normalizeGateLabelDisplayMode,
    gateLabelDisplayMode: gateLabelDisplayMode,
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
    mediaLabelDisplayMode: mediaLabelDisplayMode,
    setMediaLabelDisplayMode: setMediaLabelDisplayMode,
    mediaNumberDisplayMode: mediaNumberDisplayMode,
    setMediaNumberDisplayMode: setMediaNumberDisplayMode,
    mediaCoverArtEnabled: mediaCoverArtEnabled,
    setMediaCoverArtEnabled: setMediaCoverArtEnabled,
    mediaPlaylistContentId: mediaPlaylistContentId,
    mediaPlaylistContentType: mediaPlaylistContentType,
    mediaPlaylistPlayerSource: mediaPlaylistPlayerSource,
    mediaPlaylistSourceOptions: mediaPlaylistSourceOptions,
    parseMediaPlaylistContentId: parseMediaPlaylistContentId,
    buildMediaPlaylistContentId: buildMediaPlaylistContentId,
    mediaPlaylistContentTypeOptions: mediaPlaylistContentTypeOptions,
    mediaPlaylistContentTypeKnown: mediaPlaylistContentTypeKnown,
    setMediaPlaylistContentId: setMediaPlaylistContentId,
    setMediaPlaylistContentType: setMediaPlaylistContentType,
    setMediaPlaylistPlayerSource: setMediaPlaylistPlayerSource,
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
    actionCardIsLocal: actionCardIsLocal,
    actionScriptConfirmationEnabled: actionScriptConfirmationEnabled,
    actionScriptConfirmationMessage: actionScriptConfirmationMessage,
    actionScriptConfirmationYesText: actionScriptConfirmationYesText,
    actionScriptConfirmationNoText: actionScriptConfirmationNoText,
    actionScriptFields: actionScriptFields,
    setActionScriptFields: setActionScriptFields,
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
    normalizeClimateTemperatureStep: normalizeClimateTemperatureStep,
    climateControlTabDefinitions: climateControlTabDefinitions,
    climateControlTabs: climateControlTabs,
    normalizeClimateControlTabs: normalizeClimateControlTabs,
    normalizeClimateOptions: normalizeClimateOptions,
    climateDefaultLabelDisplayMode: climateDefaultLabelDisplayMode,
    climateDefaultNumberDisplayMode: climateDefaultNumberDisplayMode,
    climateDefaultTemperatureStep: climateDefaultTemperatureStep,
    climateTemperatureStep: climateTemperatureStep,
    setClimateTemperatureStep: setClimateTemperatureStep,
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
  });
}
