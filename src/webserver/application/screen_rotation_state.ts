import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installScreenRotationStateModule(): GlobalDescriptors {
    // ── Screen Rotation State ──────────────────────────────────────────────
    var SCREEN_ROTATION_STARTUP_FALLBACK_MS: any = 1200;
    function normalizeScreenRotation(this: any, value?: any) {
        value = String(value == null ? "" : value);
        return allScreenRotationOptions().indexOf(value) !== -1 ? value : "0";
    }
    function activeScreenRotationOptions(this: any) {
        return sortScreenRotationOptions(uniqueOptions(state.screenRotationOptions || []));
    }
    function allScreenRotationOptions(this: any) {
        return uniqueOptions((state.screenRotationOptions || [])
            .concat(state.screenRotationDeviceOptions || []));
    }
    function syncScreenRotationSelect(this: any) {
        if (!els.setScreenRotation)
            return;
        els.setScreenRotation.innerHTML = "";
        activeScreenRotationOptions().forEach(function (this: any, opt?: any) {
            appendScreenRotationOption(els.setScreenRotation, opt);
        });
        els.setScreenRotation.value = state.screenRotation;
    }
    function displayScreenRotation(this: any, value?: any) {
        var labels: any = CFG.features && CFG.features.screenRotationDisplayLabels;
        value = String(value == null ? "" : value);
        if (labels && Object.prototype.hasOwnProperty.call(labels, value))
            return labels[value];
        var offset: any = (CFG.features && parseInt(CFG.features.screenRotationDisplayOffset, 10)) || 0;
        var n: any = parseInt(value, 10);
        if (!isFinite(n))
            return value;
        return String((n + offset + 360) % 360);
    }
    function screenRotationSortValue(this: any, value?: any) {
        var displayed: any = parseInt(displayScreenRotation(value), 10);
        if (isFinite(displayed))
            return (displayed + 360) % 360;
        var raw: any = parseInt(value, 10);
        return isFinite(raw) ? (raw + 360) % 360 : 999;
    }
    function sortScreenRotationOptions(this: any, options?: any) {
        return (options || []).slice().sort(function (this: any, a?: any, b?: any) {
            return screenRotationSortValue(a) - screenRotationSortValue(b);
        });
    }
    function appendScreenRotationOption(this: any, select?: any, opt?: any) {
        var o: any = document.createElement("option");
        o.value = opt;
        o.textContent = displayScreenRotation(opt) + " deg";
        select.appendChild(o);
    }
    function screenRotationStartupRequired(this: any) {
        return !!(CFG.features && CFG.features.screenRotation);
    }
    function gridPreviewBlockedByRotationStartup(this: any) {
        return screenRotationStartupRequired() && !state.screenRotationInitialReady;
    }
    function clearInitialScreenRotationTimer(this: any) {
        if (!state.screenRotationInitialTimer)
            return;
        clearTimeout(state.screenRotationInitialTimer);
        state.screenRotationInitialTimer = null;
    }
    function startInitialScreenRotationCheck(this: any) {
        clearInitialScreenRotationTimer();
        state.pendingButtonOrderRaw = null;
        state.screenRotationInitialReady = !screenRotationStartupRequired();
        if (!state.screenRotationInitialReady) {
            state.screenRotationInitialTimer = setTimeout(resolveInitialScreenRotationCheck, SCREEN_ROTATION_STARTUP_FALLBACK_MS);
        }
    }
    function resolveInitialScreenRotationCheck(this: any) {
        if (state.screenRotationInitialReady)
            return;
        clearInitialScreenRotationTimer();
        state.screenRotationInitialReady = true;
        if (state.pendingButtonOrderRaw !== null) {
            applyButtonOrderValue(state.pendingButtonOrderRaw, true);
            state.pendingButtonOrderRaw = null;
        }
        if (els.previewMain)
            renderPreview();
    }
    return {
        "SCREEN_ROTATION_STARTUP_FALLBACK_MS": liveGlobal(() => SCREEN_ROTATION_STARTUP_FALLBACK_MS, (value?: any) => { SCREEN_ROTATION_STARTUP_FALLBACK_MS = value; }),
        "normalizeScreenRotation": staticGlobal(normalizeScreenRotation),
        "activeScreenRotationOptions": staticGlobal(activeScreenRotationOptions),
        "allScreenRotationOptions": staticGlobal(allScreenRotationOptions),
        "syncScreenRotationSelect": staticGlobal(syncScreenRotationSelect),
        "displayScreenRotation": staticGlobal(displayScreenRotation),
        "screenRotationSortValue": staticGlobal(screenRotationSortValue),
        "sortScreenRotationOptions": staticGlobal(sortScreenRotationOptions),
        "appendScreenRotationOption": staticGlobal(appendScreenRotationOption),
        "screenRotationStartupRequired": staticGlobal(screenRotationStartupRequired),
        "gridPreviewBlockedByRotationStartup": staticGlobal(gridPreviewBlockedByRotationStartup),
        "clearInitialScreenRotationTimer": staticGlobal(clearInitialScreenRotationTimer),
        "startInitialScreenRotationCheck": staticGlobal(startInitialScreenRotationCheck),
        "resolveInitialScreenRotationCheck": staticGlobal(resolveInitialScreenRotationCheck),
    };
}
