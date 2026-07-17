import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installGridModule(): GlobalDescriptors {
    // ── Context abstraction ────────────────────────────────────────────────
    function ctx(this: any) {
        if (state.editingSubpage) {
            var sp: any = getSubpage(state.editingSubpage);
            return {
                grid: sp.grid, sizes: sp.sizes, buttons: sp.buttons,
                maxSlots: NUM_SLOTS, selected: state.subpageSelectedSlots,
                isSub: true,
                setSelected: function (this: any, s?: any) { state.subpageSelectedSlots = s; },
                setLastClicked: function (this: any, s?: any) { state.subpageLastClicked = s; },
                getLastClicked: function (this: any) { return state.subpageLastClicked; },
                save: function (this: any) { saveSubpageConfig(state.editingSubpage); },
            };
        }
        return {
            grid: state.grid, sizes: state.sizes, buttons: state.buttons,
            maxSlots: NUM_SLOTS, selected: state.selectedSlots,
            isSub: false,
            setSelected: function (this: any, s?: any) { state.selectedSlots = s; },
            setLastClicked: function (this: any, s?: any) { state.lastClickedSlot = s; },
            getLastClicked: function (this: any) { return state.lastClickedSlot; },
            save: function (this: any) { postText(entityName("button_order"), serializeGrid(state.grid)); },
        };
    }
    // ── Grid helpers ───────────────────────────────────────────────────────
    var CARD_SIZE_SINGLE: any = EspControlModel.CARD_SIZE_SINGLE;
    var CARD_SIZE_TALL: any = EspControlModel.CARD_SIZE_TALL;
    var CARD_SIZE_WIDE: any = EspControlModel.CARD_SIZE_WIDE;
    var CARD_SIZE_LARGE: any = EspControlModel.CARD_SIZE_LARGE;
    var CARD_SIZE_EXTRA_TALL: any = EspControlModel.CARD_SIZE_EXTRA_TALL;
    var CARD_SIZE_EXTRA_WIDE: any = EspControlModel.CARD_SIZE_EXTRA_WIDE;
    var CARD_SIZE_EXTRA_LARGE: any = EspControlModel.CARD_SIZE_EXTRA_LARGE;
    var CARD_SIZE_MAX_WIDE: any = EspControlModel.CARD_SIZE_MAX_WIDE;
    var CARD_SIZE_MAX_TALL: any = EspControlModel.CARD_SIZE_MAX_TALL;
    var CARD_SIZE_PORTRAIT_LARGE: any = EspControlModel.CARD_SIZE_PORTRAIT_LARGE;
    function sizeFromToken(this: any, token?: any) {
        return EspControlModel.sizeFromToken(token);
    }
    function sizeToken(this: any, size?: any) {
        return EspControlModel.sizeToken(size);
    }
    function sizeRowSpan(this: any, size?: any) {
        return EspControlModel.sizeRowSpan(size);
    }
    function sizeColSpan(this: any, size?: any) {
        return EspControlModel.sizeColSpan(size);
    }
    function cardSizeClass(this: any, size?: any) {
        return EspControlModel.cardSizeClass(size);
    }
    function sizeClass(this: any, size?: any) {
        var className: any = cardSizeClass(size);
        return className ? " " + className : "";
    }
    function coveredCells(this: any, pos?: any, size?: any, maxSlots?: any, includeOrigin?: any) {
        return EspControlModel.coveredCells(pos, size, maxSlots, GRID_COLS, includeOrigin);
    }
    function sizeFitsAt(this: any, pos?: any, size?: any, maxSlots?: any) {
        return EspControlModel.sizeFitsAt(pos, size, maxSlots, GRID_COLS);
    }
    function markSpannedCells(this: any, grid?: any, pos?: any, size?: any, maxSlots?: any) {
        EspControlModel.markSpannedCells(grid, pos, size, maxSlots, GRID_COLS);
    }
    function parseOrder(this: any, str?: any) {
        var parsed: any = EspControlModel.parseGridOrder(str, NUM_SLOTS, GRID_COLS, state.sizes);
        state.sizes = parsed.sizes;
        return parsed.grid;
    }
    function applyButtonOrderValue(this: any, val?: any, skipRender?: any) {
        orderReceived = !!(val && val.trim());
        state.sizes = {};
        state.grid = parseOrder(val);
        state.selectedSlots = state.selectedSlots.filter(function (this: any, s?: any) {
            return state.grid.indexOf(s) !== -1;
        });
        if (!skipRender)
            scheduleRender();
    }
    function applySpans(this: any, grid?: any, sizes?: any, maxSlots?: any) {
        EspControlModel.applySpans(grid, sizes, maxSlots, GRID_COLS);
    }
    function serializeGrid(this: any, grid?: any) {
        return EspControlModel.serializeGridOrder(grid, state.sizes);
    }
    function applyImportedButtonOrder(this: any, orderStr?: any, importedSizes?: any) {
        state.sizes = importedSizes || {};
        state.grid = parseOrder(orderStr);
    }
    function clearSpans(this: any, grid?: any, maxSlots?: any) {
        EspControlModel.clearSpans(grid, maxSlots);
    }
    function resolveIcon(this: any, b?: any) {
        var sel: any = b.icon || "Auto";
        if (sel === "Auto" && b.entity) {
            var domain: any = b.entity.split(".")[0];
            return DOMAIN_ICONS[domain] || "cog";
        }
        return iconSlug(sel);
    }
    function btnDisplayName(this: any, b?: any) {
        return b.label || b.entity || "Configure";
    }
    return {
        "ctx": staticGlobal(ctx),
        "CARD_SIZE_SINGLE": liveGlobal(() => CARD_SIZE_SINGLE, (value?: any) => { CARD_SIZE_SINGLE = value; }),
        "CARD_SIZE_TALL": liveGlobal(() => CARD_SIZE_TALL, (value?: any) => { CARD_SIZE_TALL = value; }),
        "CARD_SIZE_WIDE": liveGlobal(() => CARD_SIZE_WIDE, (value?: any) => { CARD_SIZE_WIDE = value; }),
        "CARD_SIZE_LARGE": liveGlobal(() => CARD_SIZE_LARGE, (value?: any) => { CARD_SIZE_LARGE = value; }),
        "CARD_SIZE_EXTRA_TALL": liveGlobal(() => CARD_SIZE_EXTRA_TALL, (value?: any) => { CARD_SIZE_EXTRA_TALL = value; }),
        "CARD_SIZE_EXTRA_WIDE": liveGlobal(() => CARD_SIZE_EXTRA_WIDE, (value?: any) => { CARD_SIZE_EXTRA_WIDE = value; }),
        "CARD_SIZE_EXTRA_LARGE": liveGlobal(() => CARD_SIZE_EXTRA_LARGE, (value?: any) => { CARD_SIZE_EXTRA_LARGE = value; }),
        "CARD_SIZE_MAX_WIDE": liveGlobal(() => CARD_SIZE_MAX_WIDE, (value?: any) => { CARD_SIZE_MAX_WIDE = value; }),
        "CARD_SIZE_MAX_TALL": liveGlobal(() => CARD_SIZE_MAX_TALL, (value?: any) => { CARD_SIZE_MAX_TALL = value; }),
        "CARD_SIZE_PORTRAIT_LARGE": liveGlobal(() => CARD_SIZE_PORTRAIT_LARGE, (value?: any) => { CARD_SIZE_PORTRAIT_LARGE = value; }),
        "sizeFromToken": staticGlobal(sizeFromToken),
        "sizeToken": staticGlobal(sizeToken),
        "sizeRowSpan": staticGlobal(sizeRowSpan),
        "sizeColSpan": staticGlobal(sizeColSpan),
        "cardSizeClass": staticGlobal(cardSizeClass),
        "sizeClass": staticGlobal(sizeClass),
        "coveredCells": staticGlobal(coveredCells),
        "sizeFitsAt": staticGlobal(sizeFitsAt),
        "markSpannedCells": staticGlobal(markSpannedCells),
        "parseOrder": staticGlobal(parseOrder),
        "applyButtonOrderValue": staticGlobal(applyButtonOrderValue),
        "applySpans": staticGlobal(applySpans),
        "serializeGrid": staticGlobal(serializeGrid),
        "applyImportedButtonOrder": staticGlobal(applyImportedButtonOrder),
        "clearSpans": staticGlobal(clearSpans),
        "resolveIcon": staticGlobal(resolveIcon),
        "btnDisplayName": staticGlobal(btnDisplayName),
    };
}
