import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installScreensaverTimeoutModule(): GlobalDescriptors {
    // Screensaver timeout options and UI syncing.
    var SCREENSAVER_TIMEOUT_OPTIONS: any = [
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
    function readNumberMeta(this: any, d?: any, keys?: any, fallback?: any) {
        for (var i: any = 0; i < keys.length; i++) {
            if (d[keys[i]] == null)
                continue;
            var n: any = parseFloat(d[keys[i]]);
            if (isFinite(n))
                return n;
        }
        return fallback;
    }
    function syncScreensaverTimeoutLimits(this: any, d?: any) {
        state.screensaverTimeoutMin = readNumberMeta(d, ["min", "min_value"], state.screensaverTimeoutMin);
        state.screensaverTimeoutMax = readNumberMeta(d, ["max", "max_value"], state.screensaverTimeoutMax);
        state.screensaverTimeoutLimitsLoaded = true;
    }
    function screensaverTimeoutSupported(this: any, value?: any) {
        var n: any = parseFloat(value);
        if (!isFinite(n))
            return false;
        if (!state.screensaverTimeoutLimitsLoaded) {
            return n > 0 && n <= state.screensaverTimeoutMax;
        }
        return n >= state.screensaverTimeoutMin && n <= state.screensaverTimeoutMax;
    }
    function syncScreensaverTimeoutUi(this: any) {
        var select: any = els.setSSTimeout;
        if (!select)
            return;
        var current: any = String(state.screensaverTimeout);
        select.innerHTML = "";
        SCREENSAVER_TIMEOUT_OPTIONS.forEach(function (this: any, opt?: any) {
            if (!screensaverTimeoutSupported(opt.value))
                return;
            var o: any = document.createElement("option");
            o.value = opt.value;
            o.textContent = opt.label;
            select.appendChild(o);
        });
        if (screensaverTimeoutSupported(state.screensaverTimeout)) {
            setSelectValue(select, state.screensaverTimeout, formatDuration(state.screensaverTimeout));
            select.value = current;
        }
    }
    function applyScreensaverTimeoutState(this: any, d?: any) {
        if (!d)
            return;
        syncScreensaverTimeoutLimits(d);
        var n: any = parseFloat(d.value != null ? d.value : d.state);
        if (!isFinite(n))
            return;
        state.screensaverTimeout = n;
        syncScreensaverTimeoutUi();
    }
    return {
        "SCREENSAVER_TIMEOUT_OPTIONS": liveGlobal(() => SCREENSAVER_TIMEOUT_OPTIONS, (value?: any) => { SCREENSAVER_TIMEOUT_OPTIONS = value; }),
        "readNumberMeta": staticGlobal(readNumberMeta),
        "syncScreensaverTimeoutLimits": staticGlobal(syncScreensaverTimeoutLimits),
        "screensaverTimeoutSupported": staticGlobal(screensaverTimeoutSupported),
        "syncScreensaverTimeoutUi": staticGlobal(syncScreensaverTimeoutUi),
        "applyScreensaverTimeoutState": staticGlobal(applyScreensaverTimeoutState),
    };
}
