import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
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
    function postText(this: any, name?: any, value?: any) {
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
    function postAgendaViewDays(this: any, value?: any) {
        postNumberWithObjectIds(entityName("agenda_view_days"), entityObjectIds("agenda_view_days"), value);
    }
    function postAgendaViewHideEmpty(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("agenda_view_hide_empty_days"), entityObjectIds("agenda_view_hide_empty_days"), on);
    }
    var SCREENSAVER_ACTION_UNAVAILABLE: any = "Screen dimmed screensaver is not available on this firmware. Update the device firmware, then reload this page.";
    function postScreensaverAction(this: any, value?: any) {
        postSelectWithObjectIds(entityName("screen_saver_action"), entityObjectIds("screen_saver_action"), screensaverActionOption(value), SCREENSAVER_ACTION_UNAVAILABLE);
    }
    function postScreensaverDimmedBrightness(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_dimmed_brightness"), entityObjectIds("screen_saver_dimmed_brightness"), value, SCREENSAVER_ACTION_UNAVAILABLE);
    }
    var PHOTOS_UNAVAILABLE: any = "The photo screensaver is not available on this firmware. Update the device firmware, then reload this page.";
    function postScreensaverPhotosFolder(this: any, value?: any) {
        postTextWithObjectIds(entityName("screen_saver_photos_folder"), entityObjectIds("screen_saver_photos_folder"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosInterval(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_photos_interval"), entityObjectIds("screen_saver_photos_interval"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosShuffle(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_saver_photos_shuffle"), entityObjectIds("screen_saver_photos_shuffle"), on, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosShowDatetime(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_saver_photos_show_datetime"), entityObjectIds("screen_saver_photos_show_datetime"), on, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosShowWeather(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_saver_photos_show_weather"), entityObjectIds("screen_saver_photos_show_weather"), on, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosWeatherEntity(this: any, value?: any) {
        postTextWithObjectIds(entityName("screen_saver_photos_weather_entity"), entityObjectIds("screen_saver_photos_weather_entity"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosShowAgenda(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_saver_photos_show_agenda"), entityObjectIds("screen_saver_photos_show_agenda"), on, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosAgendaEntities(this: any, value?: any) {
        postTextWithObjectIds(entityName("screen_saver_photos_agenda_entities"), entityObjectIds("screen_saver_photos_agenda_entities"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosAgendaStyle(this: any, value?: any) {
        postSelectWithObjectIds(entityName("screen_saver_photos_agenda_style"), entityObjectIds("screen_saver_photos_agenda_style"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosAgendaOpacity(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_photos_agenda_opacity"), entityObjectIds("screen_saver_photos_agenda_opacity"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosAgendaLimit(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_photos_agenda_limit"), entityObjectIds("screen_saver_photos_agenda_limit"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosAgendaDays(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_photos_agenda_days"), entityObjectIds("screen_saver_photos_agenda_days"), value, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosAgendaHideEmpty(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_saver_photos_agenda_hide_empty"), entityObjectIds("screen_saver_photos_agenda_hide_empty"), on, PHOTOS_UNAVAILABLE);
    }
    function postScreensaverPhotosShowDate(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_saver_photos_show_date"), entityObjectIds("screen_saver_photos_show_date"), on, PHOTOS_UNAVAILABLE);
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
        "postAgendaViewDays": staticGlobal(postAgendaViewDays),
        "postAgendaViewHideEmpty": staticGlobal(postAgendaViewHideEmpty),
        "SCREENSAVER_ACTION_UNAVAILABLE": liveGlobal(() => SCREENSAVER_ACTION_UNAVAILABLE, (value?: any) => { SCREENSAVER_ACTION_UNAVAILABLE = value; }),
        "postScreensaverAction": staticGlobal(postScreensaverAction),
        "postScreensaverDimmedBrightness": staticGlobal(postScreensaverDimmedBrightness),
        "postScreensaverPhotosFolder": staticGlobal(postScreensaverPhotosFolder),
        "postScreensaverPhotosShowAgenda": staticGlobal(postScreensaverPhotosShowAgenda),
        "postScreensaverPhotosAgendaEntities": staticGlobal(postScreensaverPhotosAgendaEntities),
        "postScreensaverPhotosAgendaStyle": staticGlobal(postScreensaverPhotosAgendaStyle),
        "postScreensaverPhotosAgendaOpacity": staticGlobal(postScreensaverPhotosAgendaOpacity),
        "postScreensaverPhotosInterval": staticGlobal(postScreensaverPhotosInterval),
        "postScreensaverPhotosShuffle": staticGlobal(postScreensaverPhotosShuffle),
        "postScreensaverPhotosShowDatetime": staticGlobal(postScreensaverPhotosShowDatetime),
        "postScreensaverPhotosShowDate": staticGlobal(postScreensaverPhotosShowDate),
        "postScreensaverPhotosAgendaLimit": staticGlobal(postScreensaverPhotosAgendaLimit),
        "postScreensaverPhotosAgendaDays": staticGlobal(postScreensaverPhotosAgendaDays),
        "postScreensaverPhotosAgendaHideEmpty": staticGlobal(postScreensaverPhotosAgendaHideEmpty),
        "postScreensaverPhotosShowWeather": staticGlobal(postScreensaverPhotosShowWeather),
        "postScreensaverPhotosWeatherEntity": staticGlobal(postScreensaverPhotosWeatherEntity),
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
