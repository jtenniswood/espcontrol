import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installStateModule(): GlobalDescriptors {
    // ── State ──────────────────────────────────────────────────────────────
    function uniqueOptions(this: any, options?: any) {
        var out: any = [];
        (options || []).forEach(function (this: any, opt?: any) {
            opt = String(opt);
            if (out.indexOf(opt) < 0)
                out.push(opt);
        });
        return out;
    }
    function setSelectValue(this: any, select?: any, value?: any, label?: any) {
        if (!select)
            return;
        value = String(value);
        var found: any = false;
        for (var i: any = 0; i < select.options.length; i++) {
            if (select.options[i].value === value) {
                found = true;
                break;
            }
        }
        if (!found) {
            var o: any = document.createElement("option");
            o.value = value;
            o.textContent = label || value;
            select.appendChild(o);
        }
        select.value = value;
    }
    var els: any = {};
    var dragSrcPos: any = -1;
    var didDrag: any = false;
    var previewPlaceholder: any = null;
    var previewDropIdx: any = -1;
    var dragRafPending: any = CFG.dragAnimation ? false : null;
    var dragSrcEl: any = CFG.dragAnimation ? null : null;
    var dragIsSubpage: any = false;
    var dragEnterCount: any = 0;
    var orderReceived: any = false;
    var migrationTimer: any = null;
    var sliderMigrationTimer: any = null;
    var pendingSliderSubpageMigrations: any = {};
    var _eventSource: any = null;
    // ── Utilities ──────────────────────────────────────────────────────────
    function escHtml(this: any, s?: any) {
        return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;")
            .replace(/>/g, "&gt;").replace(/"/g, "&quot;");
    }
    function mdiIcon(this: any, icon?: any, className?: any) {
        var iconName: any = String(icon || "cog").trim();
        var span: any = document.createElement("span");
        span.className = className || "mdi";
        span.classList.add("mdi-" + (/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(iconName) ? iconName : iconSlug(iconName)));
        return span;
    }
    function textSpan(this: any, text?: any, className?: any) {
        var span: any = document.createElement("span");
        if (className)
            span.className = className;
        span.textContent = text == null ? "" : String(text);
        return span;
    }
    function escAttr(this: any, s?: any) {
        return String(s == null ? "" : s).replace(/&/g, "&amp;").replace(/"/g, "&quot;")
            .replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }
    function isSettingsFocused(this: any) {
        var ae: any = document.activeElement;
        return ae && els.buttonSettings && els.buttonSettings.contains(ae);
    }
    function isSettingsOpen(this: any) {
        return !!(els.settingsOverlay && els.settingsOverlay.classList.contains("sp-visible"));
    }
    return {
        "uniqueOptions": staticGlobal(uniqueOptions),
        "setSelectValue": staticGlobal(setSelectValue),
        "els": liveGlobal(() => els, (value?: any) => { els = value; }),
        "dragSrcPos": liveGlobal(() => dragSrcPos, (value?: any) => { dragSrcPos = value; }),
        "didDrag": liveGlobal(() => didDrag, (value?: any) => { didDrag = value; }),
        "previewPlaceholder": liveGlobal(() => previewPlaceholder, (value?: any) => { previewPlaceholder = value; }),
        "previewDropIdx": liveGlobal(() => previewDropIdx, (value?: any) => { previewDropIdx = value; }),
        "dragRafPending": liveGlobal(() => dragRafPending, (value?: any) => { dragRafPending = value; }),
        "dragSrcEl": liveGlobal(() => dragSrcEl, (value?: any) => { dragSrcEl = value; }),
        "dragIsSubpage": liveGlobal(() => dragIsSubpage, (value?: any) => { dragIsSubpage = value; }),
        "dragEnterCount": liveGlobal(() => dragEnterCount, (value?: any) => { dragEnterCount = value; }),
        "orderReceived": liveGlobal(() => orderReceived, (value?: any) => { orderReceived = value; }),
        "migrationTimer": liveGlobal(() => migrationTimer, (value?: any) => { migrationTimer = value; }),
        "sliderMigrationTimer": liveGlobal(() => sliderMigrationTimer, (value?: any) => { sliderMigrationTimer = value; }),
        "pendingSliderSubpageMigrations": liveGlobal(() => pendingSliderSubpageMigrations, (value?: any) => { pendingSliderSubpageMigrations = value; }),
        "_eventSource": liveGlobal(() => _eventSource, (value?: any) => { _eventSource = value; }),
        "escHtml": staticGlobal(escHtml),
        "mdiIcon": staticGlobal(mdiIcon),
        "textSpan": staticGlobal(textSpan),
        "escAttr": staticGlobal(escAttr),
        "isSettingsFocused": staticGlobal(isSettingsFocused),
        "isSettingsOpen": staticGlobal(isSettingsOpen),
    };
}
