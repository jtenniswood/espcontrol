import { state } from "../state/app_instance";
import { setSelectValue } from "./ui_primitives";
import type { UiRuntimeState } from "./state";
import type { ScreenScheduleStateFeature } from "./screen_schedule_state";

export interface ScreensaverTimeoutFeature {
    readonly options: readonly { label: string; value: number }[];
    readNumberMeta(data: any, keys: any, fallback: any): any;
    syncLimits(data: any): void;
    supported(value: any): boolean;
    syncUi(): void;
    applyState(data: any): void;
}

export function createScreensaverTimeoutFeature(runtime: UiRuntimeState, schedule: Pick<ScreenScheduleStateFeature, "formatDuration">): ScreensaverTimeoutFeature {
    const els = runtime.els;
    const { formatDuration } = schedule;
    // Screensaver timeout options and UI syncing.
    const options = [
        { label: "10 seconds", value: 10 },
        { label: "30 seconds", value: 30 },
        { label: "1 minute", value: 60 },
        { label: "5 minutes", value: 300 },
        { label: "10 minutes", value: 600 },
        { label: "15 minutes", value: 900 },
        { label: "20 minutes", value: 1200 },
        { label: "30 minutes", value: 1800 },
        { label: "45 minutes", value: 2700 },
        { label: "1 hour", value: 3600 },
    ];
    function readNumberMeta(d?: any, keys?: any, fallback?: any) {
        for (var i: any = 0; i < keys.length; i++) {
            if (d[keys[i]] == null)
                continue;
            var n: any = parseFloat(d[keys[i]]);
            if (isFinite(n))
                return n;
        }
        return fallback;
    }
    function syncLimits(d?: any) {
        state.screensaverTimeoutMin = readNumberMeta(d, ["min", "min_value"], state.screensaverTimeoutMin);
        state.screensaverTimeoutMax = readNumberMeta(d, ["max", "max_value"], state.screensaverTimeoutMax);
        state.screensaverTimeoutLimitsLoaded = true;
    }
    function supported(value?: any) {
        var n: any = parseFloat(value);
        if (!isFinite(n))
            return false;
        if (!state.screensaverTimeoutLimitsLoaded) {
            return n > 0 && n <= state.screensaverTimeoutMax;
        }
        return n >= state.screensaverTimeoutMin && n <= state.screensaverTimeoutMax;
    }
    function syncUi() {
        var select: any = els.setSSTimeout;
        if (!select)
            return;
        var current: any = String(state.screensaverTimeout);
        select.innerHTML = "";
        options.forEach(function (this: any, opt?: any) {
            if (!supported(opt.value))
                return;
            var o: any = document.createElement("option");
            o.value = opt.value;
            o.textContent = opt.label;
            select.appendChild(o);
        });
        if (supported(state.screensaverTimeout)) {
            setSelectValue(select, state.screensaverTimeout, formatDuration(state.screensaverTimeout));
            select.value = current;
        }
    }
    function applyState(d?: any) {
        if (!d)
            return;
        syncLimits(d);
        var n: any = parseFloat(d.value != null ? d.value : d.state);
        if (!isFinite(n))
            return;
        state.screensaverTimeout = n;
        syncUi();
    }
    return {
        options,
        readNumberMeta,
        syncLimits,
        supported,
        syncUi,
        applyState,
    };
}
