import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { infoOnlyCardVisible } from "../features/preview";
export function installAppTestHooksConfig(): GlobalDescriptors {
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("config", {
            parseButtonConfig: parseButtonConfig,
            serializeButtonConfig: serializeButtonConfig,
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
            cardBackgroundSupported: cardBackgroundSupported,
            normalizeSwitchConfirmationOptions: normalizeSwitchConfirmationOptions,
            setSwitchConfirmationOptions: setSwitchConfirmationOptions,
            normalizeActionOptions: normalizeActionOptions,
            normalizeGarageOptions: normalizeGarageOptions,
            normalizeSubpageOptions: normalizeSubpageOptions,
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
            sensorTimeUnit: sensorTimeUnit,
            setSensorTimeUnit: setSensorTimeUnit,
            normalizeSensorOptions: normalizeSensorOptions,
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
            garageConfirmationEnabled: garageConfirmationEnabled,
            garageConfirmationMode: garageConfirmationMode,
            garageConfirmationMessage: garageConfirmationMessage,
            garageConfirmationDefaultMessageForMode: garageConfirmationDefaultMessageForMode,
            garageConfirmationYesText: garageConfirmationYesText,
            garageConfirmationNoText: garageConfirmationNoText,
            setGarageConfirmationOptions: setGarageConfirmationOptions,
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
            cardRequiresSquareSize: cardRequiresSquareSize,
            cardSupportsMaxSize: cardSupportsMaxSize,
            cardSupportsPortraitLargeSize: cardSupportsPortraitLargeSize,
            cardSizeMenuOptions: cardSizeMenuOptions,
            normalizeCardSizeForConfig: normalizeCardSizeForConfig,
            normalizeMediaOptions: normalizeMediaOptions,
            mediaCoverArtAction: mediaCoverArtAction,
            setMediaCoverArtAction: setMediaCoverArtAction,
            mediaCoverArtDetailsEnabled: mediaCoverArtDetailsEnabled,
            setMediaCoverArtDetailsEnabled: setMediaCoverArtDetailsEnabled,
            mediaVolumeMax: mediaVolumeMax,
            setMediaVolumeMax: setMediaVolumeMax,
            mediaLabelDisplayMode: mediaLabelDisplayMode,
            setMediaLabelDisplayMode: setMediaLabelDisplayMode,
            mediaNumberDisplayMode: mediaNumberDisplayMode,
            setMediaNumberDisplayMode: setMediaNumberDisplayMode,
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
            imageModalModeValues: imageModalModeValues,
            normalizeImageOptions: normalizeImageOptions,
            imageLabelEnabled: imageLabelEnabled,
            imageIconEnabled: imageIconEnabled,
            imageModalMode: imageModalMode,
            imageSlotCapacity: imageSlotCapacity,
            cardBackgroundImageLimit: cardBackgroundImageLimit,
            cardBackgroundImageCountForTest: function (this: any, snapshot?: any, candidate?: any) {
                var oldGrid: any = state.grid;
                var oldButtons: any = state.buttons;
                var oldSubpages: any = state.subpages;
                state.grid = (snapshot && snapshot.grid) || [];
                state.buttons = (snapshot && snapshot.buttons) || [];
                state.subpages = (snapshot && snapshot.subpages) || {};
                try {
                    return cardBackgroundImageCountWithCandidate(candidate);
                }
                finally {
                    state.grid = oldGrid;
                    state.buttons = oldButtons;
                    state.subpages = oldSubpages;
                }
            },
            imageSlotCapacityMessage: imageSlotCapacityMessage,
            imageCardCountForTest: function (this: any, snapshot?: any, candidate?: any) {
                var oldGrid: any = state.grid;
                var oldButtons: any = state.buttons;
                var oldSubpages: any = state.subpages;
                state.grid = (snapshot && snapshot.grid) || [];
                state.buttons = (snapshot && snapshot.buttons) || [];
                state.subpages = (snapshot && snapshot.subpages) || {};
                try {
                    return imageCardCountWithCandidate(candidate);
                }
                finally {
                    state.grid = oldGrid;
                    state.buttons = oldButtons;
                    state.subpages = oldSubpages;
                }
            },
            imageCardCandidateAllowedForTest: function (this: any, snapshot?: any, candidate?: any) {
                var oldGrid: any = state.grid;
                var oldButtons: any = state.buttons;
                var oldSubpages: any = state.subpages;
                state.grid = (snapshot && snapshot.grid) || [];
                state.buttons = (snapshot && snapshot.buttons) || [];
                state.subpages = (snapshot && snapshot.subpages) || {};
                try {
                    return imageCardCountWithCandidate(candidate) <= imageSlotCapacity();
                }
                finally {
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
            alarmCardTypeOptionValues: function (this: any, isSub?: any) {
                return alarmCardTypeOptionsForSettings(!!isSub).map(function (this: any, option?: any) {
                    return option.value;
                });
            },
            coverModeOptionLabels: function (this: any, currentMode?: any) {
                var options: any = coverModeOptionsForSettings(currentMode || "");
                return options.map(function (this: any, option?: any) { return option[0] + ":" + option[1]; });
            },
            normalizeAlarmOptions: normalizeAlarmOptions,
            buttonTypePickerKeysFor: function (this: any, isSub?: any, selectedTypeKey?: any) {
                var keys: any = buttonTypePickerKeys(!!isSub, selectedTypeKey || "");
                return keys;
            },
            buttonTypeVisibleInPickerFor: function (this: any, key?: any, isSub?: any) {
                var visible: any = buttonTypeVisibleInPicker(key, !!isSub);
                return visible;
            },
            cardTransferEntriesFromEnvelopeForTest: function (this: any, envelope?: any, targetIsSubpage?: any) {
                return clipboardEntriesFromCardTransfer(envelope, !!targetIsSubpage);
            },
            buttonTypePickerKeysForInfoOnly: function (this: any, enabled?: any, selectedTypeKey?: any) {
                var oldInfoOnly: any = CFG.infoOnly;
                CFG.infoOnly = !!enabled;
                var keys: any = buttonTypePickerKeys(false, selectedTypeKey);
                CFG.infoOnly = oldInfoOnly;
                return keys;
            },
            buttonTypePickerOptionsFor: function (this: any, isSub?: any, selectedTypeKey?: any) {
                return buttonTypePickerOptionList(!!isSub, selectedTypeKey == null ? null : selectedTypeKey);
            },
            defaultButtonTypeForPicker: defaultButtonTypeForPicker,
            buttonTypesMissingCardMetadata: function (this: any) {
                var missing: any = [];
                for (var key in BUTTON_TYPES) {
                    if (!BUTTON_TYPES[key].cardMetadata)
                        missing.push(key);
                }
                return missing.sort();
            },
            buttonTypesMissingRuntimeSpec: function (this: any) {
                var missing: any = [];
                for (var key in BUTTON_TYPES) {
                    if (!BUTTON_TYPES[key].runtimeSpec)
                        missing.push(key);
                }
                return missing.sort();
            },
            buttonTypeInfoOnlySupported: function (this: any, type?: any) {
                return infoOnlyCardVisible(type || "", true);
            },
            buttonTypeGeneratedRuntimeSpec: function (this: any, type?: any) {
                var typeDef: any = BUTTON_TYPES[type || ""];
                return typeDef && typeDef.runtimeSpec
                    ? JSON.parse(JSON.stringify(typeDef.runtimeSpec))
                    : null;
            },
            buttonTypeDefaultConfig: function (this: any, type?: any) {
                var typeDef: any = BUTTON_TYPES[type || ""];
                var config: any = typeDef && typeDef.defaultConfig;
                if (typeof config === "function")
                    config = config();
                return config ? EspControlModel.cloneCardConfig(config) : null;
            },
            buttonTypeRuntimeSpec: function (this: any, type?: any) {
                var typeDef: any = BUTTON_TYPES[type || ""];
                var metadata: any = typeDef && typeDef.cardMetadata;
                var entity: any = metadata && metadata.entity;
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
            subpageChunkPostKeysFor: function (this: any, full?: any, raw?: any, previousPending?: any) {
                var oldRaw: any = state.subpageRaw[1];
                var oldPending: any = state.subpageSavePending[1];
                state.subpageRaw[1] = raw || {};
                state.subpageSavePending[1] = previousPending || "";
                var keys: any = subpageEntityKeys();
                var chunks: any = EspControlModel.splitSubpageConfigChunks(full || "", keys.length, 255) || [];
                var previousPendingChunks: any = EspControlModel.splitSubpageConfigChunks(state.subpageSavePending[1] || "", keys.length, 255) || [];
                var out: any = keys.filter(function (this: any, _key?: any, index?: any) {
                    return subpageChunkShouldPost(1, keys, chunks, index, previousPendingChunks);
                });
                state.subpageRaw[1] = oldRaw;
                state.subpageSavePending[1] = oldPending;
                return out;
            },
            subpageStateDisplayMode: subpageStateDisplayMode,
            subpageKind: subpageKind,
            buttonConfigNeedsMigration: buttonConfigNeedsMigration,
            subpageConfigNeedsMigration: subpageConfigNeedsMigration,
        });
    }
    return {};
}
