import type { NativePanelConfigController } from "../controllers/native_panel_config_controller";
import type { DeviceApi } from "../api/device_api";
import { requestFailureInfo } from "../api/request_failure";
import { screensaverActionOption } from "../model/settings";
import type { ScreensaverTimeoutFeature } from "./screensaver_timeout";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";

export interface ApplicationApiFeature {
    postQueue: Promise<any>;
    postQueueError: boolean;
    connectReconnect(callback: () => void): void;
    setPostThrottle(ms?: number): void;
    postQueueIdle(): Promise<any>;
    resetPostQueueError(): void;
    postQueueHadError(): boolean;
    postQuiet(url?: string): Promise<any>;
    post(url?: string | string[], fallbackUrl?: string | null, errorMessage?: string): Promise<any>;
    postTextLegacy(name?: string, value?: any): Promise<any>;
    postOptional(url?: string | string[]): Promise<any>;
    postFirstAvailable(urls?: string[]): Promise<any>;
    postText(name?: string, value?: any): Promise<any>;
    postTextWithObjectIds(name?: string, objectIds?: string[], value?: any, errorMessage?: string): Promise<any>;
    postSelect(name?: string, option?: any): Promise<any>;
    postButtonPress(name?: string): Promise<any>;
    postSwitch(name?: string, on?: boolean): Promise<any>;
    postScreensaverMode(value?: any): Promise<any>;
    postFirmwareAutoUpdate(on?: boolean): void;
    postC6FirmwareAutoUpdate(on?: boolean): void;
    postFirmwareUpdateFrequency(value?: any): void;
    postNumber(name?: string, value?: any): Promise<any>;
    postWithObjectId(domain?: string, name?: string, objectId?: string, action?: string, errorMessage?: string): void;
    postWithObjectIds(domain?: string, name?: string, objectIds?: string[], action?: string, errorMessage?: string): Promise<any>;
    postNumberWithObjectId(name?: string, objectId?: string, value?: any, errorMessage?: string): void;
    postNumberWithObjectIds(name?: string, objectIds?: string[], value?: any, errorMessage?: string): void;
    postSelectWithObjectId(name?: string, objectId?: string, option?: any, errorMessage?: string): void;
    postSelectWithObjectIds(name?: string, objectIds?: string[], option?: any, errorMessage?: string): void;
    postScreensaverTimeout(value?: any): void;
    postScreensaverAction(value?: any): void;
    postScreensaverDimmedBrightness(value?: any): void;
    postScreensaverDimmedBrightnessDay(value?: any): void;
    postScreensaverDimmedBrightnessNight(value?: any): void;
    postHomeScreenTimeout(value?: any): void;
    postSwitchWithObjectId(name?: string, objectId?: string, on?: boolean, errorMessage?: string): void;
    postSwitchWithObjectIds(name?: string, objectIds?: string[], on?: boolean, errorMessage?: string): void;
    getJsonQuietly(path?: string, callback?: (data: any) => void): Promise<any>;
    getJsonFirst(paths?: string[], callback?: (data: any) => void): Promise<any>;
    entityDetailPath(domain?: string, name?: string, detail?: string): string;
    entityDetailPaths(domain?: string, names?: string[], detail?: string): string[];
    entityInitialDetail(domain?: string): string;
}

export function createApplicationApiFeature(
    nativePanelConfig: NativePanelConfigController,
    deviceApi: DeviceApi,
    entityState: Pick<EntityStateFeature, "entityPostUrls" | "entityName" | "entityObjectIds">,
    screensaverTimeout: ScreensaverTimeoutFeature,
    shell: Pick<ControlsShellFeature, "setConfigLocked" | "showBanner">,
): ApplicationApiFeature {
    const { entityPostUrls, entityName, entityObjectIds } = entityState;
    const { supported: screensaverTimeoutSupported, syncUi: syncScreensaverTimeoutUi } = screensaverTimeout;
    const { setConfigLocked, showBanner } = shell;
    // ── POST queue ─────────────────────────────────────────────────────────
    var deviceApiClient: DeviceApi = deviceApi;
    var reconnect: () => void = function () {};
    var _postQueue: any = Promise.resolve(null);
    var _postQueueHadError: any = false;
    function connectReconnect(this: any, callback: () => void) {
        reconnect = callback;
    }
    function setPostThrottle(this: any, ms?: any) {
        deviceApiClient.setPostThrottle(ms);
    }
    function postQueueIdle(this: any) {
        return _postQueue;
    }
    function resetPostQueueError(this: any) {
        _postQueueHadError = false;
    }
    function postQueueHadError(this: any) {
        return _postQueueHadError;
    }
    function postQuiet(this: any, url?: any) {
        return deviceApiClient.postQuiet(url).then(function (this: any, result?: any) {
            return result.ok || result.kind === "http-error" ? result.value : null;
        });
    }
    function post(this: any, url?: any, fallbackUrl?: any, errorMessage?: any) {
        var urls: any = Array.isArray(url) ? url.slice() : [url];
        if (fallbackUrl)
            urls.push(fallbackUrl);
        _postQueue = enqueuePost(urls, errorMessage);
        return _postQueue;
    }
    function enqueuePost(this: any, urls?: any, errorMessage?: any) {
        return deviceApiClient.enqueuePost(urls).then(function (this: any, result?: any) {
            var failure: any = requestFailureInfo(result, errorMessage);
            if (failure && failure.reconnect) {
                _postQueueHadError = true;
                setConfigLocked(true, "Reconnecting to device\u2026");
                showBanner(failure.message, "error");
                setTimeout(reconnect, 5000);
                return null;
            }
            if (failure) {
                _postQueueHadError = true;
                showBanner(failure.message, "error");
            }
            return result.value;
        });
    }
    function postTextLegacy(this: any, name?: any, value?: any) {
        var encodedValue: any = encodeURIComponent(value);
        return enqueuePost(entityPostUrls("text", name, [], "set?value=" + encodedValue));
    }
    function postOptional(this: any, url?: any) {
        var urls: any = Array.isArray(url) ? url.slice() : [url];
        _postQueue = deviceApiClient.enqueuePost(urls).then(function (this: any, result?: any) {
            var failure: any = requestFailureInfo(result);
            if (failure && failure.reconnect) {
                _postQueueHadError = true;
                setConfigLocked(true, "Reconnecting to device\u2026");
                showBanner(failure.message, "error");
                setTimeout(reconnect, 5000);
                return null;
            }
            return result.value;
        });
        return _postQueue;
    }
    function postFirstAvailable(this: any, urls?: any) {
        return deviceApiClient.postFirstAvailable(urls).then(function (this: any, result?: any) {
            if (result.kind === "network-error")
                throw result.error;
            return result.value;
        });
    }
    function postText(this: any, name?: any, value?: any) {
        var nativeSave: any = nativePanelConfig
            ? nativePanelConfig.writeText(String(name || ""), String(value || ""))
            : null;
        if (nativeSave) {
            _postQueue = _postQueue.then(function () { return nativeSave; }).then(function (result: any) {
                if (result === "legacy-fallback")
                    return postTextLegacy(name, value);
                if (result !== "saved")
                    _postQueueHadError = true;
                return result;
            });
            return _postQueue;
        }
        var encodedValue: any = encodeURIComponent(value);
        return post(entityPostUrls("text", name, [], "set?value=" + encodedValue));
    }
    function postTextWithObjectIds(this: any, name?: any, objectIds?: any, value?: any, errorMessage?: any) {
        return postWithObjectIds("text", name, objectIds, "set?value=" + encodeURIComponent(value), errorMessage);
    }
    function postSelect(this: any, name?: any, option?: any) {
        return post(entityPostUrls("select", name, [], "set?option=" + encodeURIComponent(option)));
    }
    function postButtonPress(this: any, name?: any) {
        return post(entityPostUrls("button", name, [], "press"));
    }
    function postSwitch(this: any, name?: any, on?: any) {
        return post(entityPostUrls("switch", name, [], on ? "turn_on" : "turn_off"));
    }
    function postScreensaverMode(this: any, value?: any) {
        return postTextWithObjectIds(entityName("screensaver_mode"), entityObjectIds("screensaver_mode"), value);
    }
    function postFirmwareAutoUpdate(this: any, on?: any) {
        return postSwitchWithObjectIds(entityName("firmware_auto_update"), entityObjectIds("firmware_auto_update"), on);
    }
    function postC6FirmwareAutoUpdate(this: any, on?: any) {
        return postSwitchWithObjectIds(entityName("esp32_c6_auto_update"), entityObjectIds("esp32_c6_auto_update"), on);
    }
    function postFirmwareUpdateFrequency(this: any, value?: any) {
        return postSelectWithObjectIds(entityName("firmware_update_frequency"), entityObjectIds("firmware_update_frequency"), value);
    }
    function postNumber(this: any, name?: any, value?: any) {
        return post(entityPostUrls("number", name, [], "set?value=" + encodeURIComponent(value)));
    }
    function postWithObjectId(this: any, domain?: any, name?: any, objectId?: any, action?: any, errorMessage?: any) {
        postWithObjectIds(domain, name, [objectId], action, errorMessage);
    }
    function postWithObjectIds(this: any, domain?: any, name?: any, objectIds?: any, action?: any, errorMessage?: any) {
        return post(entityPostUrls(domain, name, objectIds, action), null, errorMessage);
    }
    function postNumberWithObjectId(this: any, name?: any, objectId?: any, value?: any, errorMessage?: any) {
        postWithObjectId("number", name, objectId, "set?value=" + encodeURIComponent(value), errorMessage);
    }
    function postNumberWithObjectIds(this: any, name?: any, objectIds?: any, value?: any, errorMessage?: any) {
        postWithObjectIds("number", name, objectIds, "set?value=" + encodeURIComponent(value), errorMessage);
    }
    function postSelectWithObjectId(this: any, name?: any, objectId?: any, option?: any, errorMessage?: any) {
        postWithObjectId("select", name, objectId, "set?option=" + encodeURIComponent(option), errorMessage);
    }
    function postSelectWithObjectIds(this: any, name?: any, objectIds?: any, option?: any, errorMessage?: any) {
        postWithObjectIds("select", name, objectIds, "set?option=" + encodeURIComponent(option), errorMessage);
    }
    function postScreensaverTimeout(this: any, value?: any) {
        if (!screensaverTimeoutSupported(value)) {
            showBanner("Update the device firmware before using shorter screensaver timers.", "error");
            syncScreensaverTimeoutUi();
            return;
        }
        postNumberWithObjectIds(entityName("screensaver_timeout"), entityObjectIds("screensaver_timeout"), value);
    }
    const SCREENSAVER_ACTION_UNAVAILABLE = "Screen dimmed screensaver is not available on this firmware. Update the device firmware, then reload this page.";
    function postScreensaverAction(this: any, value?: any) {
        postSelectWithObjectIds(entityName("screen_saver_action"), entityObjectIds("screen_saver_action"), screensaverActionOption(value), SCREENSAVER_ACTION_UNAVAILABLE);
    }
    function postScreensaverDimmedBrightness(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_dimmed_brightness"), entityObjectIds("screen_saver_dimmed_brightness"), value, SCREENSAVER_ACTION_UNAVAILABLE);
    }
    function postScreensaverDimmedBrightnessDay(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_daytime_dimmed_brightness"), entityObjectIds("screen_saver_daytime_dimmed_brightness"), value, SCREENSAVER_ACTION_UNAVAILABLE);
    }
    function postScreensaverDimmedBrightnessNight(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_nighttime_dimmed_brightness"), entityObjectIds("screen_saver_nighttime_dimmed_brightness"), value, SCREENSAVER_ACTION_UNAVAILABLE);
    }
    function postHomeScreenTimeout(this: any, value?: any) {
        postNumberWithObjectIds(entityName("home_screen_timeout"), entityObjectIds("home_screen_timeout"), value);
    }
    function postSwitchWithObjectId(this: any, name?: any, objectId?: any, on?: any, errorMessage?: any) {
        postWithObjectId("switch", name, objectId, on ? "turn_on" : "turn_off", errorMessage);
    }
    function postSwitchWithObjectIds(this: any, name?: any, objectIds?: any, on?: any, errorMessage?: any) {
        postWithObjectIds("switch", name, objectIds, on ? "turn_on" : "turn_off", errorMessage);
    }
    function getJsonQuietly(this: any, path?: any, callback?: any) {
        return deviceApiClient.getJson(path).then(function (this: any, result?: any) {
            var data: any = result.ok ? result.value : null;
            if (data && callback)
                callback(data);
            return data;
        });
    }
    function getJsonFirst(this: any, paths?: any, callback?: any) {
        var index: any = 0;
        function tryNext(this: any): Promise<any> {
            if (index >= paths.length)
                return Promise.resolve(null);
            return getJsonQuietly(paths[index++]).then(function (this: any, data?: any) {
                if (data) {
                    if (callback)
                        callback(data);
                    return data;
                }
                return tryNext();
            });
        }
        return tryNext();
    }
    function entityDetailPath(this: any, domain?: any, name?: any, detail?: any) {
        var query: any = detail === "state" ? "" : "?detail=all";
        return "/" + encodeURIComponent(domain) + "/" + encodeURIComponent(name) + query;
    }
    function entityDetailPaths(this: any, domain?: any, names?: any, detail?: any) {
        return names.map(function (this: any, name?: any) { return entityDetailPath(domain, name, detail); });
    }
    function entityInitialDetail(this: any, domain?: any) {
        return domain === "select" ? "state" : "all";
    }
    return {
        get postQueue() { return _postQueue; },
        set postQueue(value: Promise<any>) { _postQueue = value; },
        get postQueueError() { return _postQueueHadError; },
        set postQueueError(value: boolean) { _postQueueHadError = value; },
        connectReconnect,
        setPostThrottle,
        postQueueIdle,
        resetPostQueueError,
        postQueueHadError,
        postQuiet,
        post,
        postTextLegacy,
        postOptional,
        postFirstAvailable,
        postText,
        postTextWithObjectIds,
        postSelect,
        postButtonPress,
        postSwitch,
        postScreensaverMode,
        postFirmwareAutoUpdate,
        postC6FirmwareAutoUpdate,
        postFirmwareUpdateFrequency,
        postNumber,
        postWithObjectId,
        postWithObjectIds,
        postNumberWithObjectId,
        postNumberWithObjectIds,
        postSelectWithObjectId,
        postSelectWithObjectIds,
        postScreensaverTimeout,
        postScreensaverAction,
        postScreensaverDimmedBrightness,
        postScreensaverDimmedBrightnessDay,
        postScreensaverDimmedBrightnessNight,
        postHomeScreenTimeout,
        postSwitchWithObjectId,
        postSwitchWithObjectIds,
        getJsonQuietly,
        getJsonFirst,
        entityDetailPath,
        entityDetailPaths,
        entityInitialDetail,
    };
}
