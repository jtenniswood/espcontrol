import { state } from "../state/app_instance";
import {
    normalizeCoverArtDelay,
    normalizeHomeAssistantArtworkPort,
    normalizeScreensaverAction,
    normalizeScreensaverDimmedBrightness,
    normalizeTemperatureUnit,
    screensaverActionOption,
} from "../model/settings";
import {
    firmwareInfoFromPublicManifest,
    firmwareInfosFromPublicVersions,
    firmwareVersionFromMetadata,
    firmwareVersionsSame,
} from "../application/firmware_metadata";
import type { AppTestHookRegistrar } from "./app_test_hooks";
import type { EnvironmentStateFeature } from "../application/environment_state";
import type { ScreensaverTimeoutFeature } from "../application/screensaver_timeout";
import type { FirmwareVersionFeature } from "../application/firmware_version_state";
import type { FirmwareUpdateFeature } from "../application/firmware_update_state";
import type { ClockBarFeature } from "../application/clock_bar_state";
import type { EntityStateFeature } from "../application/entity_state";
import type { ApplicationApiFeature } from "../application/api";
import type { AppStatusPreviewFeature } from "../application/app_status_preview";
import type { ArtworkPostApiFeature } from "../application/artwork_post_api";
import type { ClockBarPostApiFeature } from "../application/clock_bar_post_api";
import type { PublicFirmwareInstallFeature } from "../application/public_firmware_install";
export function installAppTestHooksSettings(defaultTimezoneOptions: () => string[], environment: EnvironmentStateFeature, screensaverTimeout: ScreensaverTimeoutFeature, firmwareVersion: FirmwareVersionFeature, firmwareUpdate: FirmwareUpdateFeature, clockBar: Pick<ClockBarFeature, "temperatureUnitSymbol">, entityState: Pick<EntityStateFeature, "entityLookupNames">, requestApi: Pick<ApplicationApiFeature, "entityDetailPath" | "entityDetailPaths" | "entityInitialDetail">, statusPreview: Pick<AppStatusPreviewFeature, "normalizeNetworkTransport">, artworkPostApi: Pick<ArtworkPostApiFeature, "coverArtHideExternalInputPostUrls" | "coverArtDelayPostUrls" | "coverArtTrackOverlayDurationPostUrls" | "homeAssistantArtworkPortPostUrls">, clockBarPostApi: Pick<ClockBarPostApiFeature, "voiceServicesPostUrls">, publicFirmwareInstall: Pick<PublicFirmwareInstallFeature, "failPublicFirmwareUpload">, registerEspControlTestHookGroup: AppTestHookRegistrar): void {
    const { timezoneOptionsWithFallback, effectiveTimezoneOptionForWeb } = environment;
    const { supported: screensaverTimeoutSupported } = screensaverTimeout;
    const { set: setFirmwareVersion } = firmwareVersion;
    const {
        controlsVisible: firmwareUpdateControlsVisible,
        setInfo: setFirmwareUpdateInfo,
        latestInstallAvailable: latestFirmwareInstallAvailable,
        latestInstallAction: latestFirmwareInstallAction,
        setPublicInfo: setPublicFirmwareInfo,
        updateAvailable: firmwareUpdateAvailable,
        setPublicVersions: setPublicFirmwareVersions,
        selectedPreviousInfo: selectedPreviousFirmwareInfo,
        previousInstallAvailable: previousFirmwareInstallAvailable,
        versionSelectorVisible: firmwareVersionSelectorVisible,
        previousInfos: previousFirmwareInfos,
        infoForVersion: firmwareInfoForVersion,
    } = firmwareUpdate;
    const { temperatureUnitSymbol } = clockBar;
    const { entityLookupNames } = entityState;
    const { coverArtHideExternalInputPostUrls, coverArtDelayPostUrls, coverArtTrackOverlayDurationPostUrls, homeAssistantArtworkPortPostUrls } = artworkPostApi;
    const { voiceServicesPostUrls } = clockBarPostApi;
    const { failPublicFirmwareUpload } = publicFirmwareInstall;
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
            entityDetailPath: requestApi.entityDetailPath,
            entityDetailPaths: requestApi.entityDetailPaths,
            entityInitialDetail: requestApi.entityInitialDetail,
            entityLookupNames: entityLookupNames,
            coverArtHideExternalInputPostUrls: coverArtHideExternalInputPostUrls,
            coverArtDelayPostUrls: coverArtDelayPostUrls,
            coverArtTrackOverlayDurationPostUrls: coverArtTrackOverlayDurationPostUrls,
            homeAssistantArtworkPortPostUrls: homeAssistantArtworkPortPostUrls,
            voiceServicesPostUrls: voiceServicesPostUrls,
            firmwareUpdateControlsVisibleFor: function (this: any, transport?: any, supported?: any) {
                var oldTransport: any = state.networkTransport;
                var oldSupported: any = state.firmwareUpdateControlsSupported;
                state.networkTransport = statusPreview.normalizeNetworkTransport(transport);
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
                    installAvailable: latestFirmwareInstallAvailable(),
                    installAction: latestFirmwareInstallAction(),
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
                    installAvailable: latestFirmwareInstallAvailable(),
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
                var selected: any = selectedPreviousFirmwareInfo();
                var result: any = {
                    latest: state.firmwareLatestVersion,
                    selected: selected && selected.latest_version,
                    installAvailable: previousFirmwareInstallAvailable(),
                    selectorVisible: firmwareVersionSelectorVisible(),
                    installedSelected: !!selected && firmwareVersionsSame(selected.latest_version, state.firmwareVersion),
                    previous: previousFirmwareInfos().map(function (this: any, info?: any) { return info.latest_version; }),
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
            firmwareOtaUrlAfterVersionIndex: function (this: any, requestedVersion?: any, versionIndex?: any, selectedVersion?: any) {
                var oldLatest: any = state.firmwareLatestVersion;
                var oldOtaUrl: any = state.firmwareOtaUrl;
                var oldOtaFilename: any = state.firmwareOtaFilename;
                var oldOtaMd5: any = state.firmwareOtaMd5;
                var oldOptions: any = state.firmwareVersionOptions;
                var oldSelected: any = state.firmwareSelectedVersion;
                var oldIndexLoaded: any = state.firmwareVersionIndexLoaded;
                state.firmwareLatestVersion = "";
                state.firmwareOtaUrl = "";
                state.firmwareOtaFilename = "";
                state.firmwareOtaMd5 = "";
                state.firmwareVersionOptions = [];
                state.firmwareSelectedVersion = "";
                state.firmwareVersionIndexLoaded = false;
                setPublicFirmwareVersions(firmwareInfosFromPublicVersions(versionIndex));
                if (selectedVersion)
                    state.firmwareSelectedVersion = selectedVersion;
                var info: any = firmwareInfoForVersion(requestedVersion);
                var result: any = info && info.ota_url ? info.ota_url : "";
                state.firmwareLatestVersion = oldLatest;
                state.firmwareOtaUrl = oldOtaUrl;
                state.firmwareOtaFilename = oldOtaFilename;
                state.firmwareOtaMd5 = oldOtaMd5;
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
}
