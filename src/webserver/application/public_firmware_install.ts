import { state } from "../state/app_instance";
import type { DeviceApi } from "../api/device_api";
import {
    firmwareInfoFromPublicManifest,
    firmwareInfosFromPublicVersions,
    firmwareVersionsSame,
    isSpecificFirmwareVersion,
    publicFirmwareManifestUrl,
    publicFirmwareVersionsUrl,
} from "./firmware_metadata";
import type { FirmwareUpdateFeature } from "./firmware_update_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { ApplicationApiFeature } from "./api";
import type { AppEventsFeature } from "./app_events";

export interface PublicFirmwareInstallFeature {
    ensurePublicFirmwareOtaUrl(info?: any): Promise<any>;
    publicFirmwareOtaFilename(info?: any): string;
    installPublicFirmwareViaWebOta(info?: any): Promise<any>;
    waitForFirmwareRestart(): void;
    failPublicFirmwareUpload(message?: any): void;
}

export function createPublicFirmwareInstallFeature(
    deviceApi: DeviceApi,
    deviceId: string,
    firmwareUpdate: FirmwareUpdateFeature,
    shell: Pick<ControlsShellFeature, "setConfigLocked" | "showBanner">,
    requestApi: Pick<ApplicationApiFeature, "getJsonQuietly">,
    appEvents: Pick<AppEventsFeature, "connect">,
): PublicFirmwareInstallFeature {
    const { setConfigLocked, showBanner } = shell;
    const {
        selectedInfo: selectedFirmwareInfo,
        setPublicVersions: setPublicFirmwareVersions,
        infoForVersion: firmwareInfoForVersion,
        setPublicInfo: setPublicFirmwareInfo,
        clearWebOtaFallback: clearFirmwareWebOtaFallback,
        renderStatus: renderFirmwareUpdateStatus,
        startInstallRefresh: startFirmwareInstallRefresh,
        stopInstallRefresh: stopFirmwareInstallRefresh,
    } = firmwareUpdate;
    // ── Public Firmware Web OTA ────────────────────────────────────────────
    function ensurePublicFirmwareOtaUrl(this: any, info?: any) {
        info = info || selectedFirmwareInfo();
        var requestedVersion: any = info && info.latest_version ? info.latest_version : "";
        if (info && info.ota_url)
            return Promise.resolve(info.ota_url);
        if (!isSpecificFirmwareVersion(requestedVersion) && state.firmwareOtaUrl)
            return Promise.resolve(state.firmwareOtaUrl);
        return requestApi.getJsonQuietly(publicFirmwareVersionsUrl(), function (this: any, d?: any) {
            setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
        }).then(function (this: any) {
            info = firmwareInfoForVersion(requestedVersion);
            if (info && info.ota_url)
                return info.ota_url;
            return requestApi.getJsonQuietly(publicFirmwareManifestUrl(), function (this: any, d?: any) {
                setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
            }).then(function (this: any) {
                info = firmwareInfoForVersion(requestedVersion);
                return info && info.ota_url ? info.ota_url : "";
            });
        });
    }
    function publicFirmwareOtaFilename(this: any, info?: any) {
        return info && info.ota_filename ? info.ota_filename :
            (state.firmwareOtaFilename || (deviceId + ".ota.bin"));
    }
    function installPublicFirmwareViaWebOta(this: any, info?: any) {
        info = info || selectedFirmwareInfo();
        var installingLatest: any = !info ||
            firmwareVersionsSame(info.latest_version, state.firmwareLatestVersion);
        return requestApi.getJsonQuietly(publicFirmwareManifestUrl(), function (this: any, d?: any) {
            if (installingLatest)
                setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
        }).then(function (this: any) {
            info = info || selectedFirmwareInfo();
            var targetVersion: any = info && info.latest_version ? info.latest_version : state.firmwareLatestVersion;
            if (isSpecificFirmwareVersion(targetVersion)) {
                state.firmwareInstallTargetVersion = targetVersion;
            }
        }).then(function (this: any) {
            clearFirmwareWebOtaFallback();
            state.firmwareInstallPostPending = false;
            state.firmwareChecking = false;
            state.firmwareUpdateState = "INSTALLING";
            state.firmwareInstallError = "";
            state.firmwareInstallStatus = state.firmwareInstallTargetVersion ?
                "Uploading firmware " + state.firmwareInstallTargetVersion + "\u2026" :
                "Uploading firmware update\u2026";
            renderFirmwareUpdateStatus();
            startFirmwareInstallRefresh();
            var uploadStarted: any = false;
            var uploadResponseReceived: any = false;
            return ensurePublicFirmwareOtaUrl(info).then(function (this: any, otaUrl?: any) {
                if (!otaUrl)
                    throw new Error("Firmware file is not available yet.");
                return deviceApi.request(otaUrl, { cache: "no-store" });
            }).then(function (this: any, result?: any) {
                if (result.kind === "network-error")
                    throw result.error;
                var response: any = result.value;
                if (!response.ok)
                    throw new Error("Could not download firmware file (" + response.status + ").");
                return response.blob();
            }).then(function (this: any, blob?: any) {
                var filename: any = publicFirmwareOtaFilename(info);
                var form: any = new FormData();
                form.append("file", blob, filename);
                uploadStarted = true;
                return deviceApi.request("/update", { method: "POST", body: form });
            }).then(function (this: any, result?: any) {
                if (result.kind === "network-error")
                    throw result.error;
                var response: any = result.value;
                uploadResponseReceived = true;
                return response.text().catch(function (this: any) {
                    return "";
                }).then(function (this: any, text?: any) {
                    if (!response.ok) {
                        throw new Error("Device rejected firmware upload (" + response.status + ").");
                    }
                    if (/update failed/i.test(text)) {
                        throw new Error("Device reported that the firmware upload failed.");
                    }
                    waitForFirmwareRestart();
                    return true;
                });
            }).catch(function (this: any, err?: any) {
                if (uploadStarted && !uploadResponseReceived) {
                    waitForFirmwareRestart();
                    return true;
                }
                failPublicFirmwareUpload(err && err.message);
                return false;
            });
        });
    }
    function waitForFirmwareRestart(this: any) {
        state.firmwareInstallError = "";
        state.firmwareInstallStatus = "Waiting for device to restart\u2026";
        renderFirmwareUpdateStatus();
        setConfigLocked(true, "Waiting for device to restart\u2026");
        showBanner("Firmware uploaded. Waiting for device to restart\u2026", "offline");
        setTimeout(appEvents.connect, 5000);
    }
    function failPublicFirmwareUpload(this: any, message?: any) {
        var reason: any = message || "Could not upload firmware update.";
        stopFirmwareInstallRefresh();
        state.firmwareUpdateState = "";
        state.firmwareInstallError = "Firmware update failed: " + reason;
        renderFirmwareUpdateStatus();
        showBanner(reason, "error");
    }
    return {
        ensurePublicFirmwareOtaUrl,
        publicFirmwareOtaFilename,
        installPublicFirmwareViaWebOta,
        waitForFirmwareRestart,
        failPublicFirmwareUpload,
    };
}
