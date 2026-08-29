import { state } from "../state/app_instance";
import * as EspControlModel from "../model";
import { uniqueOptions } from "./ui_primitives";
import type { UiRuntimeState } from "./state";
import type { ApplicationLayoutState } from "./application_context";

export interface ScreenRotationFeature {
    normalize(value?: any): string;
    activeOptions(): any[];
    allOptions(): any[];
    syncSelect(): void;
    display(value?: any): any;
    sortValue(value?: any): number;
    sortOptions(options?: any): any[];
    appendOption(select?: any, option?: any): void;
    startupRequired(): boolean;
    gridPreviewBlocked(): boolean;
    clearInitialTimer(): void;
    startInitialCheck(): void;
    applyDeferredButtonOrder(rawOrder?: any, onNormalized?: any): any;
    resolveInitialCheck(preservePendingButtonOrder?: any): void;
}

export function createScreenRotationFeature(
    runtime: UiRuntimeState,
    layout: ApplicationLayoutState,
    dependencies: {
        applyButtonOrder(value: string, skipSpanNormalization: boolean): void;
        postNormalizedOrder(value: string): void;
        renderPreview(): void;
    },
): ScreenRotationFeature {
    const els = runtime.els;
    // ── Screen Rotation State ──────────────────────────────────────────────
    const startupFallbackMs = 1200;
    function normalize(value?: any) {
        value = String(value == null ? "" : value);
        return allOptions().indexOf(value) !== -1 ? value : "0";
    }
    function activeOptions() {
        return sortOptions(uniqueOptions(state.screenRotationOptions || []));
    }
    function allOptions() {
        return uniqueOptions((state.screenRotationOptions || [])
            .concat(state.screenRotationDeviceOptions || []));
    }
    function syncSelect() {
        if (!els.setScreenRotation)
            return;
        els.setScreenRotation.innerHTML = "";
        activeOptions().forEach(function (this: any, opt?: any) {
            appendOption(els.setScreenRotation, opt);
        });
        els.setScreenRotation.value = state.screenRotation;
    }
    function display(value?: any) {
        var labels: any = layout.config.features && layout.config.features.screenRotationDisplayLabels;
        value = String(value == null ? "" : value);
        if (labels && Object.prototype.hasOwnProperty.call(labels, value))
            return labels[value];
        var offset: any = (layout.config.features && parseInt(String(layout.config.features.screenRotationDisplayOffset || 0), 10)) || 0;
        var n: any = parseInt(value, 10);
        if (!isFinite(n))
            return value;
        return String((n + offset + 360) % 360);
    }
    function sortValue(value?: any) {
        var displayed: any = parseInt(display(value), 10);
        if (isFinite(displayed))
            return (displayed + 360) % 360;
        var raw: any = parseInt(value, 10);
        return isFinite(raw) ? (raw + 360) % 360 : 999;
    }
    function sortOptions(options?: any) {
        return (options || []).slice().sort(function (this: any, a?: any, b?: any) {
            return sortValue(a) - sortValue(b);
        });
    }
    function appendOption(select?: any, opt?: any) {
        var o: any = document.createElement("option");
        o.value = opt;
        o.textContent = display(opt) + " deg";
        select.appendChild(o);
    }
    function startupRequired() {
        return !!(layout.config.features && layout.config.features.screenRotation);
    }
    function gridPreviewBlocked() {
        return startupRequired() && !state.screenRotationInitialReady;
    }
    function clearInitialTimer() {
        if (!state.screenRotationInitialTimer)
            return;
        clearTimeout(state.screenRotationInitialTimer);
        state.screenRotationInitialTimer = null;
    }
    function startInitialCheck() {
        clearInitialTimer();
        state.pendingButtonOrderRaw = null;
        state.screenRotationInitialFallbackActive = false;
        state.screenRotationInitialReady = !startupRequired();
        if (!state.screenRotationInitialReady) {
            state.screenRotationInitialTimer = setTimeout(function (this: any) {
                resolveInitialCheck(true);
            }, startupFallbackMs);
        }
    }
    function applyDeferredButtonOrder(rawOrder?: any, onNormalized?: any) {
        var receivedOrder: any = String(rawOrder || "").trim();
        dependencies.applyButtonOrder(receivedOrder, true);
        var normalizedOrder: any = EspControlModel.serializeGridOrder(state.grid, state.sizes);
        if (normalizedOrder !== receivedOrder && typeof onNormalized === "function")
            onNormalized(normalizedOrder);
        return normalizedOrder;
    }
    function resolveInitialCheck(preservePendingButtonOrder?: any) {
        if (state.screenRotationInitialReady && state.pendingButtonOrderRaw === null &&
            !state.screenRotationInitialFallbackActive)
            return;
        clearInitialTimer();
        state.screenRotationInitialReady = true;
        if (state.pendingButtonOrderRaw !== null) {
            if (preservePendingButtonOrder) {
                dependencies.applyButtonOrder(state.pendingButtonOrderRaw, true);
            }
            else {
                applyDeferredButtonOrder(state.pendingButtonOrderRaw, function (this: any, normalizedOrder?: any) {
                    if (runtime.orderReceived)
                        dependencies.postNormalizedOrder(normalizedOrder);
                });
                state.pendingButtonOrderRaw = null;
            }
        }
        state.screenRotationInitialFallbackActive = !!preservePendingButtonOrder;
        if (els.previewMain)
            dependencies.renderPreview();
    }
    return {
        normalize,
        activeOptions,
        allOptions,
        syncSelect,
        display,
        sortValue,
        sortOptions,
        appendOption,
        startupRequired,
        gridPreviewBlocked,
        clearInitialTimer,
        startInitialCheck,
        applyDeferredButtonOrder,
        resolveInitialCheck,
    };
}
