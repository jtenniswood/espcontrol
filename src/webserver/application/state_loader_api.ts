import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installStateLoaderApiModule(): GlobalDescriptors {
    // ── State Loader API ──────────────────────────────────────────────────
    function eventStreamEnabled(this: any) {
        try {
            return new URLSearchParams(window.location.search).get("events") === "1";
        }
        catch (_) {
            return false;
        }
    }
    function cardStateEntities(this: any) {
        return entityStateItems(ENTITY_CATALOG.groups.card)
            .concat(entityStateItemsForSlots(ENTITY_CATALOG.groups.card_slot));
    }
    function settingsStateEntities(this: any) {
        var items: any = entityStateItems(ENTITY_CATALOG.groups.settings);
        if (CFG.features && CFG.features.screenRotation) {
            items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_optional));
        }
        if (CFG.features && CFG.features.voiceServices) {
            items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_voice));
        }
        return items;
    }
    function subpageStateEntities(this: any) {
        return entityStateItemsForSlots(subpageEntityKeys());
    }
    function loadStateItems(this: any, items?: any, handleState?: any, concurrency?: any) {
        var index: any = 0;
        var active: any = 0;
        var loadedCount: any = 0;
        var limit: any = Math.max(1, concurrency || 1);
        return new Promise(function (this: any, resolve?: any) {
            function done(this: any) {
                active--;
                run();
            }
            function run(this: any) {
                if (index >= items.length && active === 0) {
                    resolve(loadedCount);
                    return;
                }
                while (active < limit && index < items.length) {
                    var item: any = items[index++];
                    active++;
                    getJsonQuietly(entityDetailPath(item[0], item[1], entityInitialDetail(item[0]))).then(function (this: any, data?: any) {
                        if (data) {
                            loadedCount++;
                            handleState(data);
                        }
                    }).then(done, done);
                }
            }
            run();
        });
    }
    function loadInitialState(this: any, handleState?: any, onLoaded?: any) {
        loadStateItems(cardStateEntities(), handleState, 4).then(function (this: any, loadedCount?: any) {
            if (loadedCount === 0) {
                setConfigLocked(true, "Reconnecting to device\u2026");
                showBanner("Reconnecting to device\u2026", "offline");
                setTimeout(connectEvents, 5000);
                return;
            }
            if (onLoaded)
                onLoaded();
            clearTimeout(migrationTimer);
            migrationTimer = setTimeout(scheduleMigration, 5000);
            clearTimeout(sliderMigrationTimer);
            pendingSliderSubpageMigrations = {};
            loadStateItems(settingsStateEntities(), handleState, 2).then(function (this: any) {
                loadStateItems(subpageStateEntities(), handleState, 2);
            });
        });
    }
    function refreshFirmwareVersion(this: any) {
        var pending: any = 13;
        if (!state.firmwareVersion) {
            state.firmwareVersionRefreshPending = true;
            renderFirmwareVersion();
        }
        function finishFirmwareVersionRefresh(this: any) {
            pending--;
            if (pending > 0)
                return;
            state.firmwareVersionRefreshPending = false;
            renderFirmwareVersion();
        }
        getJsonQuietly(FIRMWARE_VERSION_METADATA_PATH, function (this: any, d?: any) {
            setFirmwareVersion(firmwareVersionFromMetadata(d));
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonQuietly(publicFirmwareManifestUrl(), function (this: any, d?: any) {
            setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonQuietly(publicFirmwareVersionsUrl(), function (this: any, d?: any) {
            setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("firmware_version")), function (this: any, d?: any) {
            setFirmwareVersion(d.state || d.value);
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("update", entityLookupNames("firmware_update")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            setFirmwareUpdateInfo(d);
        }).then(function (this: any, data?: any) {
            if (!data && state.firmwareUpdateControlsSupported !== true) {
                state.firmwareUpdateControlsSupported = false;
                syncFirmwareUpdateUi();
            }
            finishFirmwareVersionRefresh();
        }, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("button", entityLookupNames("firmware_install_update")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            state.firmwareUpdateControlsSupported = true;
            state.firmwareInstallControlsSupported = true;
            renderFirmwareUpdateStatus();
            syncFirmwareUpdateUi();
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("button", entityLookupNames("firmware_check_for_update")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            state.firmwareUpdateControlsSupported = true;
            renderFirmwareUpdateStatus();
            syncFirmwareUpdateUi();
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_current_firmware")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            setC6FirmwareCurrentVersion(d.state || d.value);
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_latest_firmware")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            setC6FirmwareLatestVersion(d.state || d.value);
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_update_available")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            setC6FirmwareUpdateAvailable(d.state || d.value);
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("button", entityLookupNames("esp32_c6_install_update")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            state.c6FirmwareUpdateControlsSupported = true;
            state.c6FirmwareInstallControlsSupported = true;
            syncC6FirmwareUi();
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("button", entityLookupNames("esp32_c6_check_for_update")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            state.c6FirmwareUpdateControlsSupported = true;
            syncC6FirmwareUi();
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
        getJsonFirst(entityDetailPaths("switch", entityLookupNames("esp32_c6_auto_update")), function (this: any, d?: any) {
            rememberEntityPostPath(d);
            state.c6FirmwareUpdateControlsSupported = true;
            state.c6FirmwareAutoUpdateSupported = true;
            state.c6FirmwareAutoUpdate = d.value === true || d.state === "ON";
            syncC6FirmwareUi();
        }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
    }
    function refreshScreensaverTimeout(this: any) {
        getJsonQuietly("/number/" + encodeURIComponent(entityName("screensaver_timeout")) + "?detail=all", applyScreensaverTimeoutState)
            .then(function (this: any, data?: any) {
            if (!data) {
                getJsonQuietly("/number/" + encodeURIComponent(entityObjectIds("screensaver_timeout")[0]) + "?detail=all", applyScreensaverTimeoutState);
            }
        });
    }
    function waitForReboot(this: any) {
        if (_eventSource) {
            _eventSource.close();
            _eventSource = null;
        }
        setConfigLocked(true, "Restarting device\u2026");
        showBanner("Restarting device\u2026", "offline");
        setTimeout(function (this: any) {
            connectEvents();
        }, 15000);
    }
    return {
        "eventStreamEnabled": staticGlobal(eventStreamEnabled),
        "cardStateEntities": staticGlobal(cardStateEntities),
        "settingsStateEntities": staticGlobal(settingsStateEntities),
        "subpageStateEntities": staticGlobal(subpageStateEntities),
        "loadStateItems": staticGlobal(loadStateItems),
        "loadInitialState": staticGlobal(loadInitialState),
        "refreshFirmwareVersion": staticGlobal(refreshFirmwareVersion),
        "refreshScreensaverTimeout": staticGlobal(refreshScreensaverTimeout),
        "waitForReboot": staticGlobal(waitForReboot),
    };
}
