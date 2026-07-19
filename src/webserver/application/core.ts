import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { CARD_RUNTIME_SPECS } from "../generated/card_contract";
export function installCore(): GlobalDescriptors {
    var DEVICE_ID: any = deviceId;
    var CFG: any = deviceConfig;
    var NUM_SLOTS: any = CFG.slots;
    var TOTAL_SLOTS: any = NUM_SLOTS;
    var GRID_COLS: any = CFG.cols;
    var GRID_ROWS: any = CFG.rows;
    function isPortraitRotation(this: any, value?: any) {
        value = String(value == null ? "0" : value);
        return value === "90" || value === "270";
    }
    function activeLayout(this: any) {
        if (isPortraitRotation(state.screenRotation) && CFG.portrait)
            return CFG.portrait;
        return CFG;
    }
    function screenWidthPercent(this: any, screen?: any) {
        var width: any = screen && screen.width;
        if (typeof width !== "string")
            return null;
        var match: any = width.trim().match(/^([0-9]+(?:\.[0-9]+)?)%$/);
        if (!match)
            return null;
        var pct: any = parseFloat(match[1]);
        return isFinite(pct) && pct > 0 ? pct : null;
    }
    function previewLayoutScale(this: any, layout?: any) {
        var baseWidth: any = screenWidthPercent(CFG.screen);
        var activeWidth: any = screenWidthPercent((layout && layout.screen) || CFG.screen);
        if (!baseWidth || !activeWidth)
            return 1;
        return baseWidth / activeWidth;
    }
    function layoutSection(this: any, layout?: any, key?: any) {
        return (layout && layout[key]) || CFG[key] || {};
    }
    function scaledCqw(this: any, value?: any, scale?: any) {
        value = parseFloat(value);
        if (!isFinite(value))
            value = 0;
        return (value * scale) + "cqw";
    }
    function scaledCqwText(this: any, value?: any, scale?: any) {
        return String(value || "").replace(/(-?[0-9]+(?:\.[0-9]+)?)cqw/g, function (this: any, _?: any, num?: any) {
            return scaledCqw(num, scale);
        });
    }
    function syncPreviewGridTop(this: any, layout?: any, scale?: any) {
        var grid: any = layoutSection(layout || activeLayout(), "grid");
        scale = scale || previewLayoutScale(layout || activeLayout());
        var compactTop: any = grid.compactTop != null ? grid.compactTop : grid.bottom;
        var gridTop: any = clockBarVisibleInPreview() ? grid.top : compactTop;
        document.documentElement.style.setProperty("--grid-top", scaledCqw(gridTop, scale));
    }
    function syncPreviewStyleVars(this: any, layout?: any, scale?: any) {
        var r: any = document.documentElement.style;
        var topbar: any = layoutSection(layout, "topbar");
        var grid: any = layoutSection(layout, "grid");
        var btn: any = layoutSection(layout, "btn");
        var sensorBadge: any = layoutSection(layout, "sensorBadge");
        var emptyCell: any = layoutSection(layout, "emptyCell");
        var subpageBadge: any = layoutSection(layout, "subpageBadge");
        r.setProperty("--topbar-h", scaledCqw(topbar.height, scale));
        r.setProperty("--topbar-pad", scaledCqwText(topbar.padding, scale));
        r.setProperty("--topbar-fs", scaledCqw(topbar.fontSize, scale));
        if (topbar.clockFontSize)
            r.setProperty("--clock-fs", scaledCqw(topbar.clockFontSize, scale));
        else
            r.removeProperty("--clock-fs");
        syncPreviewGridTop(layout, scale);
        r.setProperty("--grid-left", scaledCqw(grid.left, scale));
        r.setProperty("--grid-right", scaledCqw(grid.right, scale));
        r.setProperty("--grid-bottom", scaledCqw(grid.bottom, scale));
        r.setProperty("--grid-gap", scaledCqw(grid.gap, scale));
        r.setProperty("--btn-r", scaledCqw(btn.radius, scale));
        r.setProperty("--btn-pad", scaledCqw(btn.padding, scale));
        if (btn.borderWidth != null)
            r.setProperty("--btn-border", scaledCqw(btn.borderWidth, scale));
        else
            r.removeProperty("--btn-border");
        r.setProperty("--btn-icon", scaledCqw(btn.iconSize, scale));
        r.setProperty("--btn-label", scaledCqw(btn.labelSize, scale));
        r.setProperty("--media-cover-title", scaledCqw(btn.coverArtTitleSize, scale));
        r.setProperty("--media-cover-artist", scaledCqw(btn.coverArtArtistSize, scale));
        r.setProperty("--btn-label-weight", String(btn.labelWeight || 400));
        r.setProperty("--btn-lines", String(btn.labelLines || 1));
        r.setProperty("--btn-lines-dbl", String(btn.labelLinesDouble || btn.labelLines || 1));
        r.setProperty("--sensor-top", scaledCqw(sensorBadge.top, scale));
        r.setProperty("--sensor-right", scaledCqw(sensorBadge.right, scale));
        r.setProperty("--sensor-fs", scaledCqw(sensorBadge.fontSize, scale));
        r.setProperty("--empty-r", scaledCqw(emptyCell.radius, scale));
        r.setProperty("--subpage-bottom", scaledCqw(subpageBadge.bottom, scale));
        r.setProperty("--subpage-right", scaledCqw(subpageBadge.right, scale));
        r.setProperty("--subpage-fs", scaledCqw(subpageBadge.fontSize, scale));
    }
    function syncPreviewOrientation(this: any) {
        var layout: any = activeLayout();
        var screen: any = layout.screen || CFG.screen;
        var scale: any = previewLayoutScale(layout);
        GRID_COLS = layout.cols || CFG.cols;
        GRID_ROWS = layout.rows || Math.ceil(NUM_SLOTS / GRID_COLS);
        var r: any = document.documentElement.style;
        r.setProperty("--screen-w", screen.width || CFG.screen.width);
        r.setProperty("--screen-aspect", screen.aspect || CFG.screen.aspect);
        r.setProperty("--grid-cols", "repeat(" + GRID_COLS + "," + CFG.grid.fr + ")");
        r.setProperty("--grid-rows", "repeat(" + GRID_ROWS + "," + CFG.grid.fr + ")");
        syncPreviewStyleVars(layout, scale);
        var largeSensorUnitOffsetPercent: any = typeof CFG.largeSensorUnitOffsetPercent === "number"
            ? CFG.largeSensorUnitOffsetPercent : -10;
        r.setProperty("--large-sensor-unit-offset-y", "calc(var(--btn-icon) * 2.5 * " + (largeSensorUnitOffsetPercent / 100) + ")");
        if (state.grid && state.grid.length) {
            clearSpans(state.grid, NUM_SLOTS);
            applySpans(state.grid, state.sizes, NUM_SLOTS);
        }
        if (state.editingSubpage) {
            var sp: any = getSubpage(state.editingSubpage);
            clearSpans(sp.grid, NUM_SLOTS);
            applySpans(sp.grid, sp.sizes, NUM_SLOTS);
        }
    }
    var ICON_EXCEPTIONS: any = GENERATED_ICON_EXCEPTIONS;
    var ICON_NAMES: any = GENERATED_ICON_NAMES.slice();
    // Convert an icon display name to its MDI CSS class slug (e.g. "Lightbulb" → "lightbulb")
    function iconSlug(this: any, name?: any) {
        return ICON_EXCEPTIONS[name] || name.toLowerCase().replace(/[^a-z0-9]/g, function (this: any, ch?: any) {
            return ch === " " ? "-" : "";
        }) || "cog";
    }
    var ICON_OPTIONS: any = ["Auto"].concat(ICON_NAMES).sort();
    var DOMAIN_ICONS: any = GENERATED_DOMAIN_ICONS;
    // ── Button type plugin registry ──────────────────────────────────────
    var BUTTON_TYPES: any = {};
    function registerButtonType(this: any, key?: any, def?: any) {
        // New button types should define cardMetadata for shared settings and preview plumbing.
        BUTTON_TYPES[key] = Object.assign({
            key: key,
            label: key || "Toggle",
            allowInSubpage: false,
            hideLabel: false,
            labelPlaceholder: null,
            pickerKey: null,
            isAvailable: null,
            onSelect: null,
            renderSettingsBeforeLabel: null,
            renderSettings: null,
            renderPreview: null,
            contextMenuItems: null,
            cardMetadata: null,
            runtimeSpec: null,
            defaultConfig: null,
            normalizeConfig: null,
        }, def, {
            runtimeSpec: CARD_RUNTIME_SPECS[key] || null,
        });
    }
    function subpageStateDisplayMode(this: any, b?: any) {
        if (!b || !b.sensor)
            return "off";
        if (b.sensor === "indicator")
            return "icon";
        return b.precision === "text" ? "text" : "numeric";
    }
    var WEBSERVER_MOCK_NOW_ISO: any = "2026-01-01T09:00:00Z";
    var webserverUseMockNowForTest: any = false;
    function webserverMockNow(this: any) {
        return new Date(WEBSERVER_MOCK_NOW_ISO);
    }
    function webserverNow(this: any) {
        return webserverUseMockNowForTest ? webserverMockNow() : new Date();
    }
    function withWebserverMockNow(this: any, callback?: any) {
        var previous: any = webserverUseMockNowForTest;
        webserverUseMockNowForTest = true;
        try {
            return callback();
        }
        finally {
            webserverUseMockNowForTest = previous;
        }
    }
    return {
        "DEVICE_ID": liveGlobal(() => DEVICE_ID, (value?: any) => { DEVICE_ID = value; }),
        "CFG": liveGlobal(() => CFG, (value?: any) => { CFG = value; }),
        "NUM_SLOTS": liveGlobal(() => NUM_SLOTS, (value?: any) => { NUM_SLOTS = value; }),
        "TOTAL_SLOTS": liveGlobal(() => TOTAL_SLOTS, (value?: any) => { TOTAL_SLOTS = value; }),
        "GRID_COLS": liveGlobal(() => GRID_COLS, (value?: any) => { GRID_COLS = value; }),
        "GRID_ROWS": liveGlobal(() => GRID_ROWS, (value?: any) => { GRID_ROWS = value; }),
        "isPortraitRotation": staticGlobal(isPortraitRotation),
        "activeLayout": staticGlobal(activeLayout),
        "screenWidthPercent": staticGlobal(screenWidthPercent),
        "previewLayoutScale": staticGlobal(previewLayoutScale),
        "layoutSection": staticGlobal(layoutSection),
        "scaledCqw": staticGlobal(scaledCqw),
        "scaledCqwText": staticGlobal(scaledCqwText),
        "syncPreviewGridTop": staticGlobal(syncPreviewGridTop),
        "syncPreviewStyleVars": staticGlobal(syncPreviewStyleVars),
        "syncPreviewOrientation": staticGlobal(syncPreviewOrientation),
        "ICON_EXCEPTIONS": liveGlobal(() => ICON_EXCEPTIONS, (value?: any) => { ICON_EXCEPTIONS = value; }),
        "ICON_NAMES": liveGlobal(() => ICON_NAMES, (value?: any) => { ICON_NAMES = value; }),
        "iconSlug": staticGlobal(iconSlug),
        "ICON_OPTIONS": liveGlobal(() => ICON_OPTIONS, (value?: any) => { ICON_OPTIONS = value; }),
        "DOMAIN_ICONS": liveGlobal(() => DOMAIN_ICONS, (value?: any) => { DOMAIN_ICONS = value; }),
        "BUTTON_TYPES": liveGlobal(() => BUTTON_TYPES, (value?: any) => { BUTTON_TYPES = value; }),
        "registerButtonType": staticGlobal(registerButtonType),
        "subpageStateDisplayMode": staticGlobal(subpageStateDisplayMode),
        "WEBSERVER_MOCK_NOW_ISO": liveGlobal(() => WEBSERVER_MOCK_NOW_ISO, (value?: any) => { WEBSERVER_MOCK_NOW_ISO = value; }),
        "webserverUseMockNowForTest": liveGlobal(() => webserverUseMockNowForTest, (value?: any) => { webserverUseMockNowForTest = value; }),
        "webserverMockNow": staticGlobal(webserverMockNow),
        "webserverNow": staticGlobal(webserverNow),
        "withWebserverMockNow": staticGlobal(withWebserverMockNow),
    };
}
