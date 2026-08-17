import { state } from "../state/app_instance";
import * as EspControlModel from "../model";
import { normalizeLanguage, normalizeScreensaverAction, normalizeTemperatureUnit } from "../model/settings";
import { escHtml } from "../application/ui_primitives";
import type { AppTestHookRegistrar } from "./app_test_hooks";
import type { CardRegistry } from "../application/card_registry";
import type { ConfigCodecFeature } from "../application/config_codec";
import type { UiRuntimeState } from "../application/state";
import type { CoreFeature } from "../application/core";
import type { ApplicationLayoutState } from "../application/application_context";
import type { ScreenRotationFeature } from "../application/screen_rotation_state";
import type { FirmwareVersionFeature } from "../application/firmware_version_state";
import type { AppStatusPreviewFeature } from "../application/app_status_preview";
import type { GridFeature } from "../application/grid";
export function installAppTestHooksPreview(
    cardRegistry: CardRegistry,
    codec: ConfigCodecFeature,
    runtime: UiRuntimeState,
    core: Pick<CoreFeature, "mockNow" | "now" | "withMockNow" | "normalizeGridSpansForLayout" | "clockBarVisibleInPreview">,
    layout: ApplicationLayoutState,
    screenRotation: ScreenRotationFeature,
    firmwareVersion: FirmwareVersionFeature,
    statusPreview: Pick<AppStatusPreviewFeature, "networkPreviewIconSlug">,
    grid: Pick<GridFeature, "applyImportedButtonOrder">,
    registerEspControlTestHookGroup: AppTestHookRegistrar,
): void {
    const {
        mockNow: webserverMockNow,
        now: webserverNow,
        withMockNow: withWebserverMockNow,
        normalizeGridSpansForLayout,
        clockBarVisibleInPreview,
    } = core;
    const { applyDeferredButtonOrder: applyDeferredButtonOrderValue } = screenRotation;
    const { display: displayFirmwareVersion, label: firmwareVersionLabel } = firmwareVersion;
    const {
        buildSubpageGrid,
        buildSubpageGridAndNormalizeOrder,
        serializeSubpageGrid,
    } = codec;
    if (typeof globalThis !== "undefined" && globalThis.__ESPCONTROL_TEST_HOOKS__) {
        registerEspControlTestHookGroup("preview", {
            clockBarVisibleInPreviewFor: function (this: any, clockBarOn?: any, screensaverAction?: any) {
                var oldClockBarOn: any = state.clockBarOn;
                var oldScreensaverAction: any = state.screensaverAction;
                state.clockBarOn = !!clockBarOn;
                state.screensaverAction = normalizeScreensaverAction(screensaverAction);
                var visible: any = clockBarVisibleInPreview();
                state.clockBarOn = oldClockBarOn;
                state.screensaverAction = oldScreensaverAction;
                return visible;
            },
            webserverMockNow: webserverMockNow,
            webserverNow: webserverNow,
            buttonTypePreviewFor: function (this: any, type?: any, button?: any, options?: any) {
                var oldTimezone: any = state.timezone;
                var oldActiveTimezone: any = state.activeTimezone;
                var oldUnit: any = state.temperatureUnit;
                var oldClockFormat: any = state.clockFormat;
                var oldLanguage: any = state.language;
                options = options || {};
                if (options.timezone != null)
                    state.timezone = options.timezone;
                if (options.activeTimezone != null)
                    state.activeTimezone = options.activeTimezone;
                if (options.temperatureUnit != null) {
                    state.temperatureUnit = normalizeTemperatureUnit(options.temperatureUnit);
                }
                if (options.clockFormat != null)
                    state.clockFormat = options.clockFormat;
                if (options.language != null)
                    state.language = normalizeLanguage(options.language);
                var typeDef: any = cardRegistry.definitions[type || ""];
                var preview: any = typeDef && typeDef.renderPreview
                    ? typeDef.renderPreview(button || {}, { escHtml: escHtml, cardSize: options.cardSize || 1 })
                    : null;
                state.timezone = oldTimezone;
                state.activeTimezone = oldActiveTimezone;
                state.temperatureUnit = oldUnit;
                state.clockFormat = oldClockFormat;
                state.language = oldLanguage;
                return preview;
            },
            buttonTypePreviewForMockNow: function (this: any, type?: any, button?: any, options?: any) {
                return withWebserverMockNow(function (this: any) {
                    return globalThis.__ESPCONTROL_TEST_HOOKS__.config.buttonTypePreviewFor(type, button, options);
                });
            },
            networkPreviewIconSlug: statusPreview.networkPreviewIconSlug,
            displayFirmwareVersion: displayFirmwareVersion,
            firmwareVersionLabelFor: function (this: any, version?: any, pending?: any) {
                var oldVersion: any = state.firmwareVersion;
                var oldPending: any = state.firmwareVersionRefreshPending;
                state.firmwareVersion = version;
                state.firmwareVersionRefreshPending = !!pending;
                var label: any = firmwareVersionLabel();
                state.firmwareVersion = oldVersion;
                state.firmwareVersionRefreshPending = oldPending;
                return label;
            },
            importedButtonOrderFor: function (this: any, orderStr?: any, existingSizes?: any, gridCols?: any) {
                var oldSizes: any = state.sizes;
                var oldGrid: any = state.grid;
                var oldGridCols: any = layout.gridCols;
                state.sizes = existingSizes || {};
                state.grid = [];
                for (var i: any = 0; i < layout.numSlots; i++)
                    state.grid.push(0);
                if (gridCols)
                    layout.gridCols = gridCols;
                try {
                    var normalizedOrder: any = grid.applyImportedButtonOrder(orderStr, {});
                    var sizes: any = {};
                    for (var k in state.sizes)
                        sizes[k] = state.sizes[k];
                    return { grid: state.grid.slice(), sizes: sizes, order: normalizedOrder };
                }
                finally {
                    layout.gridCols = oldGridCols;
                    state.sizes = oldSizes;
                    state.grid = oldGrid;
                }
            },
            normalizeGridOrderForLayoutChange: function (this: any, orderStr?: any, maxSlots?: any, fromCols?: any, toCols?: any) {
                var parsed: any = EspControlModel.parseGridOrder(orderStr, maxSlots, fromCols);
                var persistedOrder: any = null;
                var normalizedOrder: any = normalizeGridSpansForLayout(parsed.grid, parsed.sizes, maxSlots, toCols, function (this: any, value?: any) {
                    persistedOrder = value;
                });
                return { order: normalizedOrder, persistedOrder: persistedOrder, sizes: parsed.sizes };
            },
            normalizeDeferredGridOrderForLayoutChange: function (this: any, orderStr?: any, toCols?: any) {
                var oldGridCols: any = layout.gridCols;
                var oldGrid: any = state.grid;
                var oldSizes: any = state.sizes;
                var oldSelectedSlots: any = state.selectedSlots;
                var oldOrderReceived: any = runtime.orderReceived;
                layout.gridCols = toCols;
                state.grid = [];
                state.sizes = {};
                state.selectedSlots = [];
                runtime.orderReceived = !!(orderStr && orderStr.trim());
                var persistedOrder: any = null;
                var normalizedOrder: any = applyDeferredButtonOrderValue(orderStr, function (this: any, value?: any) {
                    persistedOrder = value;
                });
                var sizes: any = Object.assign({}, state.sizes);
                layout.gridCols = oldGridCols;
                state.grid = oldGrid;
                state.sizes = oldSizes;
                state.selectedSlots = oldSelectedSlots;
                runtime.orderReceived = oldOrderReceived;
                return { order: normalizedOrder, persistedOrder: persistedOrder, sizes: sizes };
            },
            normalizeSubpageOrderForLayoutChange: function (this: any, order?: any, maxSlots?: any, fromCols?: any, toCols?: any) {
                var source: any = { order: order, buttons: [{}], sizes: {}, backLabel: "Back" };
                var parsed: any = EspControlModel.buildSubpageGrid(source, maxSlots, fromCols);
                var previousOrder: any = EspControlModel.serializeSubpageGrid(parsed.grid, parsed.sizes, source.backLabel);
                normalizeGridSpansForLayout(parsed.grid, parsed.sizes, maxSlots, toCols);
                var normalizedOrder: any = EspControlModel.serializeSubpageGrid(parsed.grid, parsed.sizes, source.backLabel);
                return { changed: JSON.stringify(normalizedOrder) !== JSON.stringify(previousOrder), order: normalizedOrder };
            },
            normalizeLoadedSubpageOrderForLayout: function (this: any, order?: any, toCols?: any) {
                var oldGridCols: any = layout.gridCols;
                layout.gridCols = toCols;
                var source: any = { order: order, buttons: [{}], sizes: {}, backLabel: "Back" };
                var changed: any = buildSubpageGridAndNormalizeOrder(source);
                layout.gridCols = oldGridCols;
                return { changed: changed, order: source.order, sizes: source.sizes };
            },
        });
    }
}
