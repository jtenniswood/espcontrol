import { state } from "../state/app_instance";
import { applySseHandlerAliases } from "../state/event_aliases";
import {
    entityStateKeys,
    isRemovedLegacyStateEvent,
    parseEntityEventData,
    resetStateForConnection,
} from "../state/event_state";
import {
    isC6FirmwareAutoUpdateEvent,
    isC6FirmwareCheckButtonEvent,
    isC6FirmwareCurrentEvent,
    isC6FirmwareInstallButtonEvent,
    isC6FirmwareLatestEvent,
    isC6FirmwareUpdateAvailableEvent,
    isFirmwareCheckButtonEvent,
    isFirmwareInstallButtonEvent,
    isFirmwareUpdateEvent,
    isFirmwareVersionEvent,
} from "../state/firmware_events";
import type { ReconnectController } from "../features/reconnect";
import type { AppStateEventHandlersFeature } from "./app_state_event_handlers";
import type { UiRuntimeState } from "./state";
import type { AppTitleFeature } from "./app_title";
import type { FirmwareVersionFeature } from "./firmware_version_state";
import type { FirmwareUpdateFeature } from "./firmware_update_state";
import type { C6FirmwareFeature } from "./c6_firmware_ui";
import type { EntityStateFeature } from "./entity_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { StateLoaderFeature } from "./state_loader_api";
import type { GridMigrationFeature } from "./grid_migration";
import type { AppConfigEventsFeature } from "./app_config_events";

export interface AppEventsFeature {
    connect(): void;
}

export function createAppEventsFeature(
    reconnectController: ReconnectController<unknown>,
    stateEventHandlers: AppStateEventHandlersFeature,
    configEvents: Pick<AppConfigEventsFeature, "patterns">,
    runtime: UiRuntimeState,
    pageTitle: AppTitleFeature,
    firmwareVersion: FirmwareVersionFeature,
    firmwareUpdate: FirmwareUpdateFeature,
    c6Firmware: C6FirmwareFeature,
    entityState: Pick<EntityStateFeature, "rememberEntityPostPath">,
    shell: Pick<ControlsShellFeature, "setConfigLocked" | "showBanner">,
    stateLoader: Pick<StateLoaderFeature, "refreshFirmwareVersion" | "refreshScreensaverTimeout">,
    gridMigration: Pick<GridMigrationFeature, "schedule">,
): AppEventsFeature {
    const { rememberEntityPostPath } = entityState;
    const { setConfigLocked, showBanner } = shell;
    const els = runtime.els;
    const { set: setFirmwareVersion } = firmwareVersion;
    const { setInfo: setFirmwareUpdateInfo, renderStatus: renderFirmwareUpdateStatus } = firmwareUpdate;
    const {
        setCurrentVersion: setC6FirmwareCurrentVersion,
        setLatestVersion: setC6FirmwareLatestVersion,
        setUpdateAvailable: setC6FirmwareUpdateAvailable,
        syncUi: syncC6FirmwareUi,
    } = c6Firmware;
    // ── SSE ────────────────────────────────────────────────────────────────
    function connectEvents(this: any) {
        function markConnected(this: any) {
            resetStateForConnection(state);
            runtime.orderReceived = false;
            setConfigLocked(false);
            if (els.banner)
                els.banner.className = "sp-banner";
            els.root.querySelectorAll(".sp-apply-btn").forEach(function (this: any, btn?: any) {
                btn.disabled = false;
                btn.textContent = "Apply Configuration";
            });
            clearTimeout(runtime.migrationTimer as any);
            runtime.migrationTimer = setTimeout(gridMigration.schedule, 5000);
            clearTimeout(runtime.sliderMigrationTimer as any);
            runtime.pendingSliderSubpageMigrations = {};
            stateLoader.refreshFirmwareVersion();
            stateLoader.refreshScreensaverTimeout();
        }
        function handleDisconnected(this: any) {
            setConfigLocked(true, "Reconnecting to device\u2026");
            showBanner("Reconnecting to device\u2026", "offline");
        }
        var sseHandlers: any = stateEventHandlers.createHandlers();
        applySseHandlerAliases(sseHandlers);
        var ssePatterns: any = configEvents.patterns();
        function handleState(this: any, d?: any) {
            rememberEntityPostPath(d);
            var keys: any = entityStateKeys(d);
            var id: any = keys[0] || d.id;
            var val: any = d.state != null ? String(d.state) : "";
            for (var ki: any = 0; ki < keys.length; ki++) {
                if (sseHandlers[keys[ki]]) {
                    sseHandlers[keys[ki]](val, d, keys[ki]);
                    return;
                }
            }
            if (isFirmwareVersionEvent(id, d)) {
                setFirmwareVersion(val);
                return;
            }
            if (isFirmwareUpdateEvent(id, d)) {
                setFirmwareUpdateInfo(d);
                return;
            }
            if (isFirmwareInstallButtonEvent(id, d)) {
                state.firmwareUpdateControlsSupported = true;
                state.firmwareInstallControlsSupported = true;
                renderFirmwareUpdateStatus();
                return;
            }
            if (isFirmwareCheckButtonEvent(id, d)) {
                state.firmwareUpdateControlsSupported = true;
                renderFirmwareUpdateStatus();
                return;
            }
            if (isC6FirmwareCurrentEvent(id, d)) {
                setC6FirmwareCurrentVersion(val);
                return;
            }
            if (isC6FirmwareLatestEvent(id, d)) {
                setC6FirmwareLatestVersion(val);
                return;
            }
            if (isC6FirmwareUpdateAvailableEvent(id, d)) {
                setC6FirmwareUpdateAvailable(val);
                return;
            }
            if (isC6FirmwareAutoUpdateEvent(id, d)) {
                state.c6FirmwareUpdateControlsSupported = true;
                state.c6FirmwareAutoUpdateSupported = true;
                state.c6FirmwareAutoUpdate = d.value === true || val === "ON";
                syncC6FirmwareUi();
                return;
            }
            if (isC6FirmwareInstallButtonEvent(id, d)) {
                state.c6FirmwareUpdateControlsSupported = true;
                state.c6FirmwareInstallControlsSupported = true;
                syncC6FirmwareUi();
                return;
            }
            if (isC6FirmwareCheckButtonEvent(id, d)) {
                state.c6FirmwareUpdateControlsSupported = true;
                syncC6FirmwareUi();
                return;
            }
            if (isRemovedLegacyStateEvent(id, d))
                return;
            for (var i: any = 0; i < ssePatterns.length; i++) {
                for (var pk: any = 0; pk < keys.length; pk++) {
                    var m: any = keys[pk].match(ssePatterns[i].re);
                    if (m) {
                        ssePatterns[i].fn(m, val, d);
                        return;
                    }
                }
            }
            console.log("[state] unhandled:", id, val);
        }
        reconnectController.connect({
            "onConnected": markConnected,
            "onDisconnected": handleDisconnected,
            "onPing": pageTitle.handleWebServerPingEvent,
            "parseState": function (e: any) { return parseEntityEventData(e.data); },
            "onState": handleState,
        });
    }
    return { connect: connectEvents };
}
