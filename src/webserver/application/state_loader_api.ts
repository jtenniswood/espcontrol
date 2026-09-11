import { state } from "../state/app_instance";
import { ENTITY_CATALOG } from "../generated/entity_catalog";
import type { UiRuntimeState } from "./state";
import type { ApplicationLayoutState } from "./application_context";
import {
    FIRMWARE_VERSION_METADATA_PATH,
    firmwareInfoFromPublicManifest,
    firmwareInfosFromPublicVersions,
    firmwareVersionFromMetadata,
    publicFirmwareManifestUrl,
    publicFirmwareVersionsUrl,
} from "./firmware_metadata";
import type { ScreensaverTimeoutFeature } from "./screensaver_timeout";
import type { FirmwareVersionFeature } from "./firmware_version_state";
import type { FirmwareUpdateFeature } from "./firmware_update_state";
import type { C6FirmwareFeature } from "./c6_firmware_ui";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { ApplicationApiFeature } from "./api";
import type { GridMigrationFeature } from "./grid_migration";
export interface StateLoaderDependencies {
    readonly subpageEntityKeys: () => string[];
    readonly connectEvents: () => void;
}

export interface StateLoaderFeature {
    eventStreamEnabled(): boolean;
    cardStateEntities(): any[];
    settingsStateEntities(): any[];
    subpageStateEntities(): any[];
    loadStateItems(items?: any[], handleState?: (state: any) => void, concurrency?: number): Promise<number>;
    loadInitialState(handleState?: (state: any) => void, onLoaded?: () => void): void;
    refreshFirmwareVersion(): void;
    refreshScreensaverTimeout(): void;
    waitForReboot(): void;
}

export function createStateLoaderFeature(runtime: UiRuntimeState, layout: ApplicationLayoutState, screensaverTimeout: ScreensaverTimeoutFeature, firmwareVersion: FirmwareVersionFeature, firmwareUpdate: FirmwareUpdateFeature, c6Firmware: C6FirmwareFeature, entityState: Pick<EntityStateFeature, "entityStateItems" | "entityStateItemsForSlots" | "entityLookupNames" | "rememberEntityPostPath" | "entityName" | "entityObjectIds">, shell: Pick<ControlsShellFeature, "setConfigLocked" | "showBanner">, requestApi: Pick<ApplicationApiFeature, "getJsonQuietly" | "getJsonFirst" | "entityDetailPath" | "entityDetailPaths" | "entityInitialDetail">, gridMigration: Pick<GridMigrationFeature, "schedule">, dependencies: StateLoaderDependencies): StateLoaderFeature {
    const { entityStateItems, entityStateItemsForSlots, entityLookupNames, rememberEntityPostPath, entityName, entityObjectIds } = entityState;
    const { setConfigLocked, showBanner } = shell;
    const { getJsonQuietly, getJsonFirst, entityDetailPath, entityDetailPaths, entityInitialDetail } = requestApi;
    const { applyState: applyScreensaverTimeoutState } = screensaverTimeout;
    const { render: renderFirmwareVersion, set: setFirmwareVersion } = firmwareVersion;
    const {
        setPublicInfo: setPublicFirmwareInfo,
        setPublicVersions: setPublicFirmwareVersions,
        setInfo: setFirmwareUpdateInfo,
        syncUi: syncFirmwareUpdateUi,
        renderStatus: renderFirmwareUpdateStatus,
    } = firmwareUpdate;
    const {
        setCurrentVersion: setC6FirmwareCurrentVersion,
        setLatestVersion: setC6FirmwareLatestVersion,
        setUpdateAvailable: setC6FirmwareUpdateAvailable,
        syncUi: syncC6FirmwareUi,
    } = c6Firmware;
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
        if (layout.config.features && layout.config.features.screenRotation) {
            items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_optional));
        }
        if (layout.config.features && layout.config.features.voiceServices) {
            items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_voice));
        }
        if (layout.config.features && layout.config.features.battery) {
            items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_battery));
        }
        if (layout.config.features && layout.config.features.alarmDelayAudio) {
            items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_alarm_audio));
        }
        return items;
    }
    function subpageStateEntities(this: any) {
        return entityStateItemsForSlots(dependencies.subpageEntityKeys());
    }
    function loadStateItems(this: any, items?: any, handleState?: any, concurrency?: any) {
        var index: any = 0;
        var active: any = 0;
        var loadedCount: any = 0;
        var limit: any = Math.max(1, concurrency || 1);
        return new Promise<number>(function (this: any, resolve?: any) {
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
                setTimeout(dependencies.connectEvents, 5000);
                return;
            }
            if (onLoaded)
                onLoaded();
            clearTimeout(runtime.migrationTimer as any);
            runtime.migrationTimer = setTimeout(gridMigration.schedule, 5000);
            clearTimeout(runtime.sliderMigrationTimer as any);
            runtime.pendingSliderSubpageMigrations = {};
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
                getJsonQuietly("/number/" + encodeURIComponent(entityObjectIds("screensaver_timeout")[0]!) + "?detail=all", applyScreensaverTimeoutState);
            }
        });
    }
    function waitForReboot(this: any) {
        if (runtime.eventSource) {
            runtime.eventSource.close();
            runtime.eventSource = null;
        }
        setConfigLocked(true, "Restarting device\u2026");
        showBanner("Restarting device\u2026", "offline");
        setTimeout(function (this: any) {
            dependencies.connectEvents();
        }, 15000);
    }
    return {
        eventStreamEnabled,
        cardStateEntities,
        settingsStateEntities,
        subpageStateEntities,
        loadStateItems,
        loadInitialState,
        refreshFirmwareVersion,
        refreshScreensaverTimeout,
        waitForReboot,
    };
}
