import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { postConfigurationValue } from "./configuration_transaction";
export function installApiModule(): GlobalDescriptors {
    // ── POST queue ─────────────────────────────────────────────────────────
    var _deviceApi: any = createDeviceApi(function (this: any, url?: any, init?: any) { return fetch(url, init); });
    var _postQueue: any = Promise.resolve(null);
    var _postQueueHadError: any = false;
    function setPostThrottle(this: any, ms?: any) {
        _deviceApi.setPostThrottle(ms);
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
        return _deviceApi.postQuiet(url).then(function (this: any, result?: any) {
            return result.ok || result.kind === "http-error" ? result.value : null;
        });
    }
    function post(this: any, url?: any, fallbackUrl?: any, errorMessage?: any) {
        var urls: any = Array.isArray(url) ? url.slice() : [url];
        if (fallbackUrl)
            urls.push(fallbackUrl);
        _postQueue = _deviceApi.enqueuePost(urls).then(function (this: any, result?: any) {
            var failure: any = requestFailureInfo(result, errorMessage);
            if (failure && failure.reconnect) {
                _postQueueHadError = true;
                setConfigLocked(true, "Reconnecting to device\u2026");
                showBanner(failure.message, "error");
                setTimeout(connectEvents, 5000);
                return null;
            }
            if (failure) {
                _postQueueHadError = true;
                showBanner(failure.message, "error");
            }
            return result.value;
        });
        return _postQueue;
    }
    function postOptional(this: any, url?: any) {
        var urls: any = Array.isArray(url) ? url.slice() : [url];
        _postQueue = _deviceApi.enqueuePost(urls).then(function (this: any, result?: any) {
            var failure: any = requestFailureInfo(result);
            if (failure && failure.reconnect) {
                _postQueueHadError = true;
                setConfigLocked(true, "Reconnecting to device\u2026");
                showBanner(failure.message, "error");
                setTimeout(connectEvents, 5000);
                return null;
            }
            return result.value;
        });
        return _postQueue;
    }
    function postFirstAvailable(this: any, urls?: any) {
        return _deviceApi.postFirstAvailable(urls).then(function (this: any, result?: any) {
            if (result.kind === "network-error")
                throw result.error;
            return result.value;
        });
    }
    function postConfiguration(this: any, domain?: any, name?: any, objectIds?: any, value?: any, fallback?: any) {
        _postQueue = postConfigurationValue(domain, name, objectIds || [], String(value), fallback).catch(function (this: any, error?: any) {
            _postQueueHadError = true;
            showBanner("Could not save configuration. The device will retry when it reconnects.", "error");
            console.warn("Configuration save failed", error);
            return null;
        });
        return _postQueue;
    }
    function postText(this: any, name?: any, value?: any) {
        var encodedValue: any = encodeURIComponent(value);
        return postConfiguration("text", name, [], value, function (this: any) {
            return post(entityPostUrls("text", name, [], "set?value=" + encodedValue));
        });
    }
    function postTextWithObjectIds(this: any, name?: any, objectIds?: any, value?: any, errorMessage?: any) {
        return postConfiguration("text", name, objectIds, value, function (this: any) {
            return postWithObjectIds("text", name, objectIds, "set?value=" + encodeURIComponent(value), errorMessage);
        });
    }
    function postOptionalTextWithObjectIds(this: any, name?: any, objectIds?: any, value?: any) {
        return postConfiguration("text", name, objectIds, value, function (this: any) {
            return postOptional(entityPostUrls("text", name, objectIds, "set?value=" + encodeURIComponent(value)));
        });
    }
    function postSelect(this: any, name?: any, option?: any) {
        return postConfiguration("select", name, [], option, function (this: any) {
            return post(entityPostUrls("select", name, [], "set?option=" + encodeURIComponent(option)));
        });
    }
    function postButtonPress(this: any, name?: any) {
        return post(entityPostUrls("button", name, [], "press"));
    }
    function postSwitch(this: any, name?: any, on?: any) {
        return postConfiguration("switch", name, [], on ? "1" : "0", function (this: any) {
            return post(entityPostUrls("switch", name, [], on ? "turn_on" : "turn_off"));
        });
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
        return postConfiguration("number", name, [], value, function (this: any) {
            return post(entityPostUrls("number", name, [], "set?value=" + encodeURIComponent(value)));
        });
    }
    function postWithObjectId(this: any, domain?: any, name?: any, objectId?: any, action?: any, errorMessage?: any) {
        postWithObjectIds(domain, name, [objectId], action, errorMessage);
    }
    function postWithObjectIds(this: any, domain?: any, name?: any, objectIds?: any, action?: any, errorMessage?: any) {
        return post(entityPostUrls(domain, name, objectIds, action), null, errorMessage);
    }
    function postNumberWithObjectId(this: any, name?: any, objectId?: any, value?: any, errorMessage?: any) {
        return postConfiguration("number", name, [objectId], value, function (this: any) {
            return postWithObjectId("number", name, objectId, "set?value=" + encodeURIComponent(value), errorMessage);
        });
    }
    function postNumberWithObjectIds(this: any, name?: any, objectIds?: any, value?: any, errorMessage?: any) {
        return postConfiguration("number", name, objectIds, value, function (this: any) {
            return postWithObjectIds("number", name, objectIds, "set?value=" + encodeURIComponent(value), errorMessage);
        });
    }
    function postSelectWithObjectId(this: any, name?: any, objectId?: any, option?: any, errorMessage?: any) {
        return postConfiguration("select", name, [objectId], option, function (this: any) {
            return postWithObjectId("select", name, objectId, "set?option=" + encodeURIComponent(option), errorMessage);
        });
    }
    function postSelectWithObjectIds(this: any, name?: any, objectIds?: any, option?: any, errorMessage?: any) {
        return postConfiguration("select", name, objectIds, option, function (this: any) {
            return postWithObjectIds("select", name, objectIds, "set?option=" + encodeURIComponent(option), errorMessage);
        });
    }
    function postScreensaverTimeout(this: any, value?: any) {
        if (!screensaverTimeoutSupported(value)) {
            showBanner("Update the device firmware before using shorter screensaver timers.", "error");
            syncScreensaverTimeoutUi();
            return;
        }
        postNumberWithObjectIds(entityName("screensaver_timeout"), entityObjectIds("screensaver_timeout"), value);
    }
    var SCREENSAVER_ACTION_UNAVAILABLE: any = "Screen dimmed screensaver is not available on this firmware. Update the device firmware, then reload this page.";
    function postScreensaverAction(this: any, value?: any) {
        postSelectWithObjectIds(entityName("screen_saver_action"), entityObjectIds("screen_saver_action"), screensaverActionOption(value), SCREENSAVER_ACTION_UNAVAILABLE);
    }
    function postScreensaverDimmedBrightness(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_dimmed_brightness"), entityObjectIds("screen_saver_dimmed_brightness"), value, SCREENSAVER_ACTION_UNAVAILABLE);
    }
    function postHomeScreenTimeout(this: any, value?: any) {
        postNumberWithObjectIds(entityName("home_screen_timeout"), entityObjectIds("home_screen_timeout"), value);
    }
    function postSwitchWithObjectId(this: any, name?: any, objectId?: any, on?: any, errorMessage?: any) {
        return postConfiguration("switch", name, [objectId], on ? "1" : "0", function (this: any) {
            return postWithObjectId("switch", name, objectId, on ? "turn_on" : "turn_off", errorMessage);
        });
    }
    function postSwitchWithObjectIds(this: any, name?: any, objectIds?: any, on?: any, errorMessage?: any) {
        return postConfiguration("switch", name, objectIds, on ? "1" : "0", function (this: any) {
            return postWithObjectIds("switch", name, objectIds, on ? "turn_on" : "turn_off", errorMessage);
        });
    }
    function getJsonQuietly(this: any, path?: any, callback?: any) {
        return _deviceApi.getJson(path).then(function (this: any, result?: any) {
            var data: any = result.ok ? result.value : null;
            if (data && callback)
                callback(data);
            return data;
        });
    }
    function getJsonFirst(this: any, paths?: any, callback?: any) {
        var index: any = 0;
        function tryNext(this: any) {
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
        "_deviceApi": liveGlobal(() => _deviceApi, (value?: any) => { _deviceApi = value; }),
        "_postQueue": liveGlobal(() => _postQueue, (value?: any) => { _postQueue = value; }),
        "_postQueueHadError": liveGlobal(() => _postQueueHadError, (value?: any) => { _postQueueHadError = value; }),
        "setPostThrottle": staticGlobal(setPostThrottle),
        "postQueueIdle": staticGlobal(postQueueIdle),
        "resetPostQueueError": staticGlobal(resetPostQueueError),
        "postQueueHadError": staticGlobal(postQueueHadError),
        "postQuiet": staticGlobal(postQuiet),
        "post": staticGlobal(post),
        "postOptional": staticGlobal(postOptional),
        "postFirstAvailable": staticGlobal(postFirstAvailable),
        "postText": staticGlobal(postText),
        "postTextWithObjectIds": staticGlobal(postTextWithObjectIds),
        "postOptionalTextWithObjectIds": staticGlobal(postOptionalTextWithObjectIds),
        "postSelect": staticGlobal(postSelect),
        "postButtonPress": staticGlobal(postButtonPress),
        "postSwitch": staticGlobal(postSwitch),
        "postScreensaverMode": staticGlobal(postScreensaverMode),
        "postFirmwareAutoUpdate": staticGlobal(postFirmwareAutoUpdate),
        "postC6FirmwareAutoUpdate": staticGlobal(postC6FirmwareAutoUpdate),
        "postFirmwareUpdateFrequency": staticGlobal(postFirmwareUpdateFrequency),
        "postNumber": staticGlobal(postNumber),
        "postWithObjectId": staticGlobal(postWithObjectId),
        "postWithObjectIds": staticGlobal(postWithObjectIds),
        "postNumberWithObjectId": staticGlobal(postNumberWithObjectId),
        "postNumberWithObjectIds": staticGlobal(postNumberWithObjectIds),
        "postSelectWithObjectId": staticGlobal(postSelectWithObjectId),
        "postSelectWithObjectIds": staticGlobal(postSelectWithObjectIds),
        "postScreensaverTimeout": staticGlobal(postScreensaverTimeout),
        "SCREENSAVER_ACTION_UNAVAILABLE": liveGlobal(() => SCREENSAVER_ACTION_UNAVAILABLE, (value?: any) => { SCREENSAVER_ACTION_UNAVAILABLE = value; }),
        "postScreensaverAction": staticGlobal(postScreensaverAction),
        "postScreensaverDimmedBrightness": staticGlobal(postScreensaverDimmedBrightness),
        "postHomeScreenTimeout": staticGlobal(postHomeScreenTimeout),
        "postSwitchWithObjectId": staticGlobal(postSwitchWithObjectId),
        "postSwitchWithObjectIds": staticGlobal(postSwitchWithObjectIds),
        "getJsonQuietly": staticGlobal(getJsonQuietly),
        "getJsonFirst": staticGlobal(getJsonFirst),
        "entityDetailPath": staticGlobal(entityDetailPath),
        "entityDetailPaths": staticGlobal(entityDetailPaths),
        "entityInitialDetail": staticGlobal(entityInitialDetail),
    };
}
