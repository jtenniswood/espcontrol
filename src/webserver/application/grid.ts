import { state } from "../state/app_instance";
import * as EspControlModel from "../model";
import { domainIcons as DOMAIN_ICONS, iconSlug } from "./ui_primitives";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { ApplicationLayoutState } from "./application_context";
import type { EntityStateFeature } from "./entity_state";
import type { ApplicationApiFeature } from "./api";
import type { ButtonSettingsRenderQueueFeature } from "./button_settings_render_queue";

export interface GridFeature {
    ctx(): any;
    scheduleMainGridSave(): void;
    cancelMainGridSave(): void;
    sizeClass(size?: any): string;
    applyButtonOrderValue(value?: any, skipRender?: any): void;
    serializeGrid(grid?: any): string;
    applyImportedButtonOrder(order?: any, importedSizes?: any): string;
    resolveIcon(button?: any): string;
}

export function createGridFeature(codec: ConfigCodecFeature, runtime: UiRuntimeState, layout: ApplicationLayoutState, entityState: Pick<EntityStateFeature, "entityName">, requestApi: Pick<ApplicationApiFeature, "postText">, renderQueue: ButtonSettingsRenderQueueFeature): GridFeature {
    const { entityName } = entityState;
    const { getSubpage, saveSubpageConfig } = codec;
    // ── Context abstraction ────────────────────────────────────────────────
    var mainGridSaveTimer: any = null;
    function scheduleMainGridSave(this: any) {
        clearTimeout(mainGridSaveTimer);
        mainGridSaveTimer = setTimeout(function () {
            mainGridSaveTimer = null;
            requestApi.postText(entityName("button_order"), serializeGrid(state.grid));
        }, 500);
    }
    function cancelMainGridSave(this: any) {
        clearTimeout(mainGridSaveTimer);
        mainGridSaveTimer = null;
    }
    function ctx(this: any) {
        if (state.editingSubpage) {
            var sp: any = getSubpage(state.editingSubpage);
            return {
                grid: sp.grid, sizes: sp.sizes, buttons: sp.buttons,
                maxSlots: layout.numSlots, selected: state.subpageSelectedSlots,
                isSub: true,
                setSelected: function (this: any, s?: any) { state.subpageSelectedSlots = s; },
                setLastClicked: function (this: any, s?: any) { state.subpageLastClicked = s; },
                getLastClicked: function (this: any) { return state.subpageLastClicked; },
                save: function (this: any) { saveSubpageConfig(state.editingSubpage); },
            };
        }
        return {
            grid: state.grid, sizes: state.sizes, buttons: state.buttons,
            maxSlots: layout.numSlots, selected: state.selectedSlots,
            isSub: false,
            setSelected: function (this: any, s?: any) { state.selectedSlots = s; },
            setLastClicked: function (this: any, s?: any) { state.lastClickedSlot = s; },
            getLastClicked: function (this: any) { return state.lastClickedSlot; },
            save: function (this: any) { scheduleMainGridSave(); },
        };
    }
    // ── Grid helpers ───────────────────────────────────────────────────────
    function sizeClass(this: any, size?: any) {
        var className: any = EspControlModel.cardSizeClass(size);
        return className ? " " + className : "";
    }
    function parseOrder(this: any, str?: any) {
        var parsed: any = EspControlModel.parseGridOrder(str, layout.numSlots, layout.gridCols, state.sizes);
        state.sizes = parsed.sizes;
        return parsed.grid;
    }
    function applyButtonOrderValue(this: any, val?: any, skipRender?: any) {
        runtime.orderReceived = !!(val && val.trim());
        state.sizes = {};
        state.grid = parseOrder(val);
        state.selectedSlots = state.selectedSlots.filter(function (this: any, s?: any) {
            return state.grid.indexOf(s) !== -1;
        });
        if (!skipRender)
            renderQueue.schedule();
    }
    function serializeGrid(this: any, grid?: any) {
        return EspControlModel.serializeGridOrder(grid, state.sizes);
    }
    function applyImportedButtonOrder(this: any, orderStr?: any, importedSizes?: any) {
        state.sizes = importedSizes || {};
        state.grid = parseOrder(orderStr);
        return serializeGrid(state.grid);
    }
    function resolveIcon(this: any, b?: any) {
        var sel: any = b.icon || "Auto";
        if (sel === "Auto" && b.entity) {
            var domain: any = b.entity.split(".")[0];
            return DOMAIN_ICONS[domain] || "cog";
        }
        return iconSlug(sel);
    }
    return {
        ctx,
        scheduleMainGridSave,
        cancelMainGridSave,
        sizeClass,
        applyButtonOrderValue,
        serializeGrid,
        applyImportedButtonOrder,
        resolveIcon,
    };
}
