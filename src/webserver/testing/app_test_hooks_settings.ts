import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppTestHooksSettings(): GlobalDescriptors {
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("settings", {
            normalizeTemperatureUnit: normalizeTemperatureUnit,
            normalizeCoverArtDelay: normalizeCoverArtDelay,
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
            firmwareUpdateControlsVisibleFor: function (this: any, transport?: any, supported?: any) {
                var oldTransport: any = state.networkTransport;
                var oldSupported: any = state.firmwareUpdateControlsSupported;
                state.networkTransport = normalizeNetworkTransport(transport);
                state.firmwareUpdateControlsSupported = supported;
                var visible: any = firmwareUpdateControlsVisible();
                state.networkTransport = oldTransport;
                state.firmwareUpdateControlsSupported = oldSupported;
                return visible;
            },
            firmwareVersionAfterUpdateInfo: function (this: any, initialVersion?: any, updateInfo?: any) {
                var oldVersion: any = state.firmwareVersion;
                var oldLatest: any = state.firmwareLatestVersion;
                var oldUpdateState: any = state.firmwareUpdateState;
                var oldReleaseUrl: any = state.firmwareReleaseUrl;
                var oldChecking: any = state.firmwareChecking;
                var oldSupported: any = state.firmwareUpdateControlsSupported;
                var oldInstallSupported: any = state.firmwareInstallControlsSupported;
                var oldInstallTarget: any = state.firmwareInstallTargetVersion;
                var oldInstallPostPending: any = state.firmwareInstallPostPending;
                var oldOptions: any = state.firmwareVersionOptions;
                var oldSelected: any = state.firmwareSelectedVersion;
                var oldIndexLoaded: any = state.firmwareVersionIndexLoaded;
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
                var result: any = {
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
            firmwareStateAfterPublicManifest: function (this: any, initialVersion?: any, manifest?: any) {
                var oldVersion: any = state.firmwareVersion;
                var oldLatest: any = state.firmwareLatestVersion;
                var oldUpdateState: any = state.firmwareUpdateState;
                var oldReleaseUrl: any = state.firmwareReleaseUrl;
                var oldInstallSupported: any = state.firmwareInstallControlsSupported;
                var oldInstallPostPending: any = state.firmwareInstallPostPending;
                var oldOptions: any = state.firmwareVersionOptions;
                var oldSelected: any = state.firmwareSelectedVersion;
                var oldIndexLoaded: any = state.firmwareVersionIndexLoaded;
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
                var result: any = {
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
            firmwareStateAfterVersionIndex: function (this: any, initialVersion?: any, versionIndex?: any, selectedVersion?: any) {
                var oldVersion: any = state.firmwareVersion;
                var oldLatest: any = state.firmwareLatestVersion;
                var oldUpdateState: any = state.firmwareUpdateState;
                var oldReleaseUrl: any = state.firmwareReleaseUrl;
                var oldOtaUrl: any = state.firmwareOtaUrl;
                var oldOtaFilename: any = state.firmwareOtaFilename;
                var oldOtaMd5: any = state.firmwareOtaMd5;
                var oldInstallSupported: any = state.firmwareInstallControlsSupported;
                var oldOptions: any = state.firmwareVersionOptions;
                var oldSelected: any = state.firmwareSelectedVersion;
                var oldIndexLoaded: any = state.firmwareVersionIndexLoaded;
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
                if (selectedVersion)
                    state.firmwareSelectedVersion = selectedVersion;
                var selected: any = selectedFirmwareInfo();
                var result: any = {
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
            firmwareFailureStatusFor: function (this: any, message?: any) {
                var oldError: any = state.firmwareInstallError;
                var oldStatus: any = state.firmwareInstallStatus;
                var oldUpdateState: any = state.firmwareUpdateState;
                var oldTarget: any = state.firmwareInstallTargetVersion;
                var oldPostPending: any = state.firmwareInstallPostPending;
                failPublicFirmwareUpload(message);
                var result: any = {
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
            screensaverTimeoutSupportedFor: function (this: any, value?: any, limitsLoaded?: any, min?: any, max?: any) {
                var oldLoaded: any = state.screensaverTimeoutLimitsLoaded;
                var oldMin: any = state.screensaverTimeoutMin;
                var oldMax: any = state.screensaverTimeoutMax;
                state.screensaverTimeoutLimitsLoaded = !!limitsLoaded;
                state.screensaverTimeoutMin = min;
                state.screensaverTimeoutMax = max;
                var supported: any = screensaverTimeoutSupported(value);
                state.screensaverTimeoutLimitsLoaded = oldLoaded;
                state.screensaverTimeoutMin = oldMin;
                state.screensaverTimeoutMax = oldMax;
                return supported;
            },
            temperatureUnitSymbolFor: function (this: any, timezone?: any, unit?: any, activeTimezone?: any) {
                var oldTimezone: any = state.timezone;
                var oldActiveTimezone: any = state.activeTimezone;
                var oldUnit: any = state.temperatureUnit;
                state.timezone = timezone || oldTimezone;
                if (activeTimezone != null)
                    state.activeTimezone = activeTimezone;
                state.temperatureUnit = normalizeTemperatureUnit(unit);
                var symbol: any = temperatureUnitSymbol();
                state.timezone = oldTimezone;
                state.activeTimezone = oldActiveTimezone;
                state.temperatureUnit = oldUnit;
                return symbol;
            },
        });
    }
    return {};
}
