import { state } from "../state/app_instance";
import * as EspControlModel from "../model";
import {
    cardContractAllowInSubpage,
    cardContractCardKeys,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractLargeNumbersSupported,
    cardContractMigrationAlias,
    cardContractOptions,
    cardContractPickerKey,
    cardContractSubpageTypeCode,
    cardContractSubpageTypeFromCode,
} from "../generated/card_contract";
import type { AppTestHookRegistrar } from "./app_test_hooks";
import { infoOnlyCardVisible } from "../features/preview";
import type { CardRegistry } from "../application/card_registry";
import type { ConfigSensorOptionsFeature } from "../application/config_sensor_options";
import type { ConfigMediaOptionsFeature } from "../application/config_media_options";
import type { ConfigImageOptionsFeature } from "../application/config_image_options";
import type { ConfigWeatherOptionsFeature } from "../application/config_weather_options";
import type { ConfigWebhookOptionsFeature } from "../application/config_webhook_options";
import type { ConfigInternalRelayOptionsFeature } from "../application/config_internal_relay_options";
import type { ConfigLockOptionsFeature } from "../application/config_lock_options";
import type { ConfigDateTimeOptionsFeature } from "../application/config_date_time_options";
import type { ConfigModalTabOptionsFeature } from "../application/config_modal_tab_options";
import type { ConfigAccessClimateAlarmOptionsFeature } from "../application/config_access_climate_alarm_options";
import type { ConfigConfirmationOptionsFeature } from "../application/config_confirmation_options";
import type { ConfigCodecFeature } from "../application/config_codec";
import type { LightCardRegistration } from "../cards/light_temperature";
import type { CoreFeature } from "../application/core";
import type { ApplicationLayoutState } from "../application/application_context";
import type { ConfigPersistenceFeature } from "../application/config_post_api";
import type { PreviewRenderFeature } from "../application/preview_render";
import type { PreviewClipboardFeature } from "../application/preview_clipboard";
import type { PreviewContextMenuFeature } from "../application/preview_context_menu";
import type { ControlsFieldsFeature } from "../application/controls_fields";
import { cardContractOptionSupportedFor } from "../application/config_option_core";
import { subpageKind } from "../application/config_subpage_options";
import { entityMatchesDomains } from "../application/button_settings";
import { pushDefaultIcon, pushDefaultIconOn } from "../cards/push";
import {
    coverModeOptionValues,
    coverModeOptionsForSettings,
    normalizeCoverMode,
    normalizeCoverPosition,
} from "../application/config_cover_contract";
export function installAppTestHooksConfig(
    cardRegistry: CardRegistry,
    sensorOptions: ConfigSensorOptionsFeature,
    mediaOptions: ConfigMediaOptionsFeature,
    imageOptions: ConfigImageOptionsFeature,
    weatherOptions: ConfigWeatherOptionsFeature,
    webhookOptions: ConfigWebhookOptionsFeature,
    internalRelayOptions: ConfigInternalRelayOptionsFeature,
    lockOptions: ConfigLockOptionsFeature,
    dateTimeOptions: ConfigDateTimeOptionsFeature,
    modalTabs: ConfigModalTabOptionsFeature,
    accessOptions: ConfigAccessClimateAlarmOptionsFeature,
    confirmationOptions: ConfigConfirmationOptionsFeature,
    codec: ConfigCodecFeature,
    lightCards: LightCardRegistration,
    core: Pick<CoreFeature, "subpageStateDisplayMode">,
    layout: ApplicationLayoutState,
    configPersistence: Pick<ConfigPersistenceFeature, "subpageEntityKeys" | "subpageChunkShouldPost">,
    preview: Pick<PreviewRenderFeature, "defaultTypeForPicker" | "pickerKeys" | "pickerOptions" | "registryValue" | "typeVisibleInPicker">,
    clipboard: Pick<PreviewClipboardFeature, "entriesFromTransfer">,
    contextMenu: Pick<PreviewContextMenuFeature, "cardSizeOptions">,
    fields: Pick<ControlsFieldsFeature, "cardMetadataValue">,
    registerEspControlTestHookGroup: AppTestHookRegistrar,
): void {
    const { subpageEntityKeys, subpageChunkShouldPost } = configPersistence;
    const { defaultTypeForPicker: defaultButtonTypeForPicker, pickerKeys: buttonTypePickerKeys, pickerOptions: buttonTypePickerOptionList, registryValue: buttonTypeRegistryValue, typeVisibleInPicker: buttonTypeVisibleInPicker } = preview;
    const { entriesFromTransfer: clipboardEntriesFromCardTransfer } = clipboard;
    const { cardSizeOptions: cardSizeMenuOptions } = contextMenu;
    const { cardMetadataValue } = fields;
    const { subpageStateDisplayMode } = core;
    const {
        sensorCardIsLocal,
        cardLargeNumbersEnabled,
        sensorActiveColorEnabled,
        sensorTimeUnit,
        setSensorTimeUnit,
        normalizeSensorOptions,
        sensorStateLabelsEnabled,
        sensorStateInput,
        sensorStateOutput,
        sensorStateInput2,
        sensorStateOutput2,
        setSensorStateTranslation,
        setSensorStateTranslations,
        doorWindowActiveColorEnabled,
        presenceActiveColorEnabled,
    } = sensorOptions;
    const {
        mediaModeOptionValues,
        mediaEditorMode,
        mediaNowPlayingControls,
        mediaNowPlayingControlValues,
        mediaStateDisplayModeSupported,
        mediaPlaylistSourceOptions,
        parseMediaPlaylistContentId,
        buildMediaPlaylistContentId,
        mediaPlaylistContentTypeKnown,
        mediaPlaylistContentTypeOptions,
        normalizeMediaOptions,
        mediaCoverArtDetailsEnabled,
        setMediaCoverArtDetailsEnabled,
        mediaVolumeMax,
        setMediaVolumeMax,
        mediaSpeakerGroupEntity,
        setMediaSpeakerGroupEntity,
        mediaLabelDisplayMode,
        setMediaLabelDisplayMode,
        mediaNumberDisplayMode,
        setMediaNumberDisplayMode,
        mediaPlaylistContentId,
        mediaPlaylistContentType,
        mediaPlaylistPlayerSource,
        setMediaPlaylistContentId,
        setMediaPlaylistContentType,
        setMediaPlaylistPlayerSource,
    } = mediaOptions;
    const {
        imageModalModeValues,
        imageSlotCapacity,
        imageSlotCapacityMessage,
        imageCardCountWithCandidate,
        imageModalMode,
        imageLabelEnabled,
        imageIconEnabled,
        normalizeImageOptions,
    } = imageOptions;
    const {
        weatherCardIsForecastMode,
        weatherModeOptionValues,
        normalizeWeatherCardMode,
    } = weatherOptions;
    const {
        webhookHeaders,
        webhookMethod,
    } = webhookOptions;
    const {
        internalRelayDefaultIcon,
        internalRelayDefaultOnIcon,
        internalRelayModeOptionValues,
        normalizeInternalRelayMode,
    } = internalRelayOptions;
    const {
        lockModeOptionValues,
        normalizeLockMode,
    } = lockOptions;
    const {
        dateTimeLargeNumbersLabel,
        dateTimeModeOptionValues,
        normalizeDateTimeCardMode,
    } = dateTimeOptions;
    const {
        coverControlTabDefinitions,
        coverControlTabs,
        normalizeCoverControlTabs,
        normalizeCoverOptions,
        fanControlTabDefinitions,
        fanControlTabs,
        normalizeFanControlTabs,
        fanLightEntity,
        normalizeFanControlOptions,
        setFanControlTabs,
        setFanLightEntity,
        lightControlTabDefinitions,
        lightControlTabs,
        normalizeLightControlTabs,
        normalizeLightControlOptions,
        climateControlTabDefinitions,
        climateControlTabs,
        normalizeClimateControlTabs,
    } = modalTabs;
    const {
        alarmBehaviorSpec,
        alarmActionSpecs,
        normalizeGarageLabelDisplayMode,
        garageModeOptionValues,
        normalizeGarageMode,
        garageLabelDisplayMode,
        normalizeGateLabelDisplayMode,
        gateModeOptionValues,
        normalizeGateMode,
        gateLabelDisplayMode,
        normalizeClimateLabelDisplayMode,
        normalizeClimateNumberDisplayMode,
        normalizeClimateTemperatureStep,
        normalizeClimateOptions,
        climateTemperatureStep,
        setClimateTemperatureStep,
        alarmActionValues,
        alarmPinRequired,
        alarmVisibleActions,
        normalizeAlarmIconDisplayMode,
        normalizeAlarmLabelDisplayMode,
        alarmIconDisplayMode,
        alarmLabelDisplayMode,
        normalizeAlarmOptions,
        parseClimatePrecisionConfig,
        climatePrecisionValues,
        climateDefaultLabelDisplayMode,
        climateDefaultNumberDisplayMode,
        climateDefaultTemperatureStep,
        normalizeClimatePrecisionConfig,
    } = accessOptions;
    function alarmControlPanelValue(this: any) {
        return alarmBehaviorSpec().controlPanelValue || "control_panel";
    }
    function alarmCardTypeOptionsForSettings(this: any, _isSub?: any) {
        return [{ value: alarmControlPanelValue(), label: "All Controls" }].concat(alarmActionSpecs());
    }
    const {
        actionCardIsLocal,
        actionCardStateEntity,
        actionCardStateUnit,
        actionCardStatePrecision,
        actionCardStateDisplayMode,
        switchConfirmationEnabled,
        switchConfirmationMode,
        switchConfirmationMessage,
        switchConfirmationDefaultMessageForMode,
        switchConfirmationYesText,
        switchConfirmationNoText,
        normalizeCardOnPattern,
        cardOnPattern,
        setCardOnPattern,
        garageConfirmationEnabled,
        garageConfirmationMode,
        garageConfirmationMessage,
        garageConfirmationDefaultMessageForMode,
        garageConfirmationYesText,
        garageConfirmationNoText,
        setGarageConfirmationOptions,
        actionScriptConfirmationEnabled,
        actionScriptConfirmationMessage,
        actionScriptConfirmationYesText,
        actionScriptConfirmationNoText,
        actionScriptFields,
        setActionScriptFields,
    } = confirmationOptions;
    const {
        lightTempDefaultRange,
        lightTempParseRange,
        lightTempClampMin,
        lightTempClampMax,
        lightTempLegacySensorValues,
        lightTempSensorNeedsCleanup,
    } = lightCards;
    const {
        cardRequiresSquareSize,
        cardIsWifiSharing,
        cardSupportsWifiPortraitSizes,
        cardSupportsExtraLargeSize,
        cardSupportsMaxSize,
        cardSupportsPortraitLargeSize,
        cardSupportsLandscapeLargeSize,
        normalizeCardSizeForConfig,
        serializeButtonConfig,
        parseButtonConfig,
        buttonConfigNeedsMigration,
        parseSubpageConfig,
        subpageConfigNeedsMigration,
        serializeSubpageConfig,
        buildSubpageGrid,
        serializeSubpageGrid,
    } = codec;
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("config", {
            setGridDimensions: function (cols?: number, rows?: number) {
                if (cols) layout.gridCols = cols;
                if (rows) layout.gridRows = rows;
            },
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
            entityMatchesDomains: entityMatchesDomains,
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
            fanLightEntity: fanLightEntity,
            normalizeFanControlOptions: normalizeFanControlOptions,
            setFanControlTabs: setFanControlTabs,
            setFanLightEntity: setFanLightEntity,
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
            cardIsWifiSharing: cardIsWifiSharing,
            cardSupportsWifiPortraitSizes: cardSupportsWifiPortraitSizes,
            cardSupportsExtraLargeSize: cardSupportsExtraLargeSize,
            cardSupportsMaxSize: cardSupportsMaxSize,
            cardSupportsPortraitLargeSize: cardSupportsPortraitLargeSize,
            cardSupportsLandscapeLargeSize: cardSupportsLandscapeLargeSize,
            cardSizeMenuOptions: cardSizeMenuOptions,
            normalizeCardSizeForConfig: normalizeCardSizeForConfig,
            normalizeMediaOptions: normalizeMediaOptions,
            mediaCoverArtDetailsEnabled: mediaCoverArtDetailsEnabled,
            setMediaCoverArtDetailsEnabled: setMediaCoverArtDetailsEnabled,
            mediaVolumeMax: mediaVolumeMax,
            setMediaVolumeMax: setMediaVolumeMax,
            mediaSpeakerGroupEntity: mediaSpeakerGroupEntity,
            setMediaSpeakerGroupEntity: setMediaSpeakerGroupEntity,
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
                var oldInfoOnly: any = layout.config.infoOnly;
                (layout.config as any).infoOnly = !!enabled;
                var keys: any = buttonTypePickerKeys(false, selectedTypeKey);
                (layout.config as any).infoOnly = oldInfoOnly;
                return keys;
            },
            buttonTypePickerOptionsFor: function (this: any, isSub?: any, selectedTypeKey?: any) {
                return buttonTypePickerOptionList(!!isSub, selectedTypeKey == null ? null : selectedTypeKey);
            },
            defaultButtonTypeForPicker: defaultButtonTypeForPicker,
            buttonTypesMissingCardMetadata: function (this: any) {
                var missing: any = [];
                for (var key in cardRegistry.definitions) {
                    var definition: any = cardRegistry.definitions[key];
                    if (!definition.cardMetadata)
                        missing.push(key);
                }
                return missing.sort();
            },
            buttonTypesMissingRuntimeSpec: function (this: any) {
                var missing: any = [];
                for (var key in cardRegistry.definitions) {
                    var definition: any = cardRegistry.definitions[key];
                    if (!definition.runtimeSpec)
                        missing.push(key);
                }
                return missing.sort();
            },
            buttonTypeInfoOnlySupported: function (this: any, type?: any) {
                return infoOnlyCardVisible(type || "", true);
            },
            buttonTypeGeneratedRuntimeSpec: function (this: any, type?: any) {
                var typeDef: any = cardRegistry.definitions[type || ""];
                return typeDef && typeDef.runtimeSpec
                    ? JSON.parse(JSON.stringify(typeDef.runtimeSpec))
                    : null;
            },
            buttonTypeDefaultConfig: function (this: any, type?: any) {
                var typeDef: any = cardRegistry.definitions[type || ""];
                var config: any = typeDef && typeDef.defaultConfig;
                if (typeof config === "function")
                    config = config();
                return config ? EspControlModel.cloneCardConfig(config) : null;
            },
            buttonTypeRuntimeSpec: function (this: any, type?: any) {
                var typeDef: any = cardRegistry.definitions[type || ""];
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
}
