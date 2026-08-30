import { state } from "../state/app_instance";
import {
    CARD_SIZE_EXTRA_LARGE,
    CARD_SIZE_EXTRA_TALL,
    CARD_SIZE_EXTRA_WIDE,
    CARD_SIZE_LANDSCAPE_LARGE,
    CARD_SIZE_LARGE,
    CARD_SIZE_MAX_TALL,
    CARD_SIZE_MAX_WIDE,
    CARD_SIZE_PORTRAIT_LARGE,
    CARD_SIZE_SINGLE,
    CARD_SIZE_TALL,
    CARD_SIZE_WIDE,
} from "../model/grid";
import { mdiIcon } from "./ui_primitives";
import { clampMenuPosition } from "../features/preview";
import { resizeGridSlot } from "../features/preview_grid";
import type { ApplicationLayoutState } from "./application_context";
import type { CardRegistry } from "./card_registry";
import type { ConfigCodecFeature } from "./config_codec";
import type { ClockBarFeature } from "./clock_bar_state";
import type { ControlsShellFeature } from "./controls_shell";
import type { AppStatusPreviewFeature } from "./app_status_preview";
import type { GridFeature } from "./grid";
import type { ButtonSettingsSelectionFeature } from "./button_settings_selection";
import type { PreviewRenderFeature } from "./preview_render";
import type { PreviewClipboardFeature } from "./preview_clipboard";
export interface PreviewContextMenuDependencies {
    readonly document: Document;
    readonly window: Window;
    readonly layout: ApplicationLayoutState;
    readonly cards: CardRegistry;
    readonly codec: ConfigCodecFeature;
    readonly clockBar: Pick<ClockBarFeature, "setItemVisible">;
    readonly shell: Pick<ControlsShellFeature, "isConfigLocked">;
    readonly statusPreview: Pick<AppStatusPreviewFeature, "clockBarItemActive" | "clockBarItemLabel" | "clockBarItems" | "isClockBarTemperatureItem" | "updateClockBarItemUi">;
    readonly grid: Pick<GridFeature, "ctx" | "scheduleMainGridSave">;
    readonly selection: Pick<ButtonSettingsSelectionFeature, "hideSettingsOverlay" | "openClockBarTemperatureSettings">;
    readonly preview: Pick<PreviewRenderFeature, "registryValue">;
    readonly clipboard: Pick<PreviewClipboardFeature, "copyButtons" | "copySlot" | "cutButtons" | "cutSlot" | "pasteButton" | "pasteSubpageButton" | "showCopyCode" | "showPasteCode">;
    readonly renderPreview: () => void;
    readonly renderButtonSettings: () => void;
    readonly openCardSettings: (slot: number) => void;
    readonly openVoiceServicesSettings: () => void;
    readonly addSlot: (position: number) => void;
    readonly addSubpageSlot: (position: number) => void;
    readonly duplicateButton: (slot: number) => void;
    readonly duplicateSubpageButton: (slot: number) => void;
    readonly deleteSlot: (slot: number) => void;
    readonly deleteButtons: (slots: number[]) => void;
}
export interface PreviewContextMenuFeature {
    hide(): void;
    contains(target?: any): boolean;
    cardSizeOptions(slot?: any, context?: any): any[];
    showSelection(event?: any): void;
    showClockBar(event?: any, item?: any): void;
    showCard(event?: any, slot?: any): void;
    showBack(event?: any): void;
    showEmpty(event?: any, position?: any): void;
}

export function createPreviewContextMenuFeature(dependencies: PreviewContextMenuDependencies): PreviewContextMenuFeature {
    const document = dependencies.document;
    const window = dependencies.window;
    const { isConfigLocked } = dependencies.shell;
    const { setItemVisible: setClockBarItemVisible } = dependencies.clockBar;
    const { clockBarItemActive, clockBarItemLabel, clockBarItems, isClockBarTemperatureItem, updateClockBarItemUi } = dependencies.statusPreview;
    const { ctx, scheduleMainGridSave } = dependencies.grid;
    const { hideSettingsOverlay, openClockBarTemperatureSettings } = dependencies.selection;
    const { registryValue: buttonTypeRegistryValue } = dependencies.preview;
    const { copyButtons, copySlot, cutButtons, cutSlot, pasteButton, pasteSubpageButton, showCopyCode: showCopyCardCode, showPasteCode: showPasteCardCode } = dependencies.clipboard;
    const { renderPreview, renderButtonSettings, openCardSettings, openVoiceServicesSettings, addSlot, addSubpageSlot, duplicateButton, duplicateSubpageButton, deleteSlot, deleteButtons } = dependencies;
    const {
        cardRequiresSquareSize,
        cardSupportsMaxSize,
        cardSupportsPortraitLargeSize,
        cardSupportsLandscapeLargeSize,
        normalizeCardSizeForConfig,
        getSubpage,
        serializeSubpageGrid,
        exitSubpage,
        saveSubpageConfig,
    } = dependencies.codec;
    // ── Preview Context Menu ──────────────────────────────────────────
    // ── Context menu (unified) ─────────────────────────────────────────────
    var ctxMenu: any = null;
    function positionMenu(this: any, menu?: any, e?: any) {
        var position: any = clampMenuPosition({ x: e.clientX, y: e.clientY }, menu.offsetWidth, menu.offsetHeight, window.innerWidth, window.innerHeight);
        menu.style.left = position.x + "px";
        menu.style.top = position.y + "px";
    }
    function addCtxItem(this: any, icon?: any, text?: any, handler?: any, danger?: any) {
        var item: any = document.createElement("div");
        item.className = "sp-ctx-item" + (danger ? " sp-ctx-danger" : "");
        item.appendChild(mdiIcon(icon));
        item.appendChild(document.createTextNode(text));
        item.addEventListener("mousedown", function (this: any, ev?: any) {
            ev.preventDefault();
            ev.stopPropagation();
            hideContextMenu();
            handler();
        });
        ctxMenu.appendChild(item);
    }
    function addCtxDivider(this: any) {
        var div: any = document.createElement("div");
        div.className = "sp-ctx-divider";
        ctxMenu.appendChild(div);
    }
    function addCtxSubmenu(this: any, icon?: any, text?: any, buildFn?: any) {
        var wrapper: any = document.createElement("div");
        wrapper.className = "sp-ctx-item sp-ctx-sub";
        wrapper.appendChild(mdiIcon(icon));
        wrapper.appendChild(document.createTextNode(text));
        var sub: any = document.createElement("div");
        sub.className = "sp-ctx-submenu";
        buildFn(sub);
        wrapper.appendChild(sub);
        wrapper.addEventListener("mouseenter", function (this: any) {
            sub.style.left = "100%";
            sub.style.right = "auto";
            var r: any = sub.getBoundingClientRect();
            if (r.right > window.innerWidth - 4) {
                sub.style.left = "auto";
                sub.style.right = "100%";
            }
        });
        wrapper.addEventListener("mousedown", function (this: any, ev?: any) { ev.preventDefault(); ev.stopPropagation(); });
        ctxMenu.appendChild(wrapper);
    }
    function addSubItem(this: any, container?: any, icon?: any, text?: any, handler?: any, active?: any) {
        var item: any = document.createElement("div");
        item.className = "sp-ctx-item";
        if (active) {
            item.appendChild(mdiIcon("check", "sp-ctx-check mdi"));
        }
        else {
            var spacer: any = document.createElement("span");
            spacer.style.width = "18px";
            item.appendChild(spacer);
        }
        item.appendChild(document.createTextNode(text));
        item.addEventListener("mousedown", function (this: any, ev?: any) {
            ev.preventDefault();
            ev.stopPropagation();
            hideContextMenu();
            handler();
        });
        container.appendChild(item);
    }
    function resizeSlot(this: any, slot?: any, targetSz?: any) {
        if (isConfigLocked())
            return;
        var c: any = ctx();
        var slotPos: any = slot === -2 ? c.grid.indexOf(-2) : c.grid.indexOf(slot);
        if (slotPos < 0)
            return;
        var button: any = slot === -2 ? null : c.buttons[slot - 1];
        targetSz = normalizeCardSizeForConfig(button, targetSz);
        var curSz: any = c.sizes[slot] || 1;
        if (curSz === targetSz)
            return;
        var resized: any = resizeGridSlot(c.grid, c.sizes, slot, slotPos, targetSz, c.maxSlots, dependencies.layout.gridCols, !c.isSub);
        if (!resized.accepted)
            return;
        c.grid.splice(0, c.grid.length);
        Array.prototype.push.apply(c.grid, resized.grid);
        for (var sizeSlot in c.sizes)
            delete c.sizes[sizeSlot];
        for (var resizedSlot in resized.sizes)
            c.sizes[resizedSlot] = resized.sizes[resizedSlot];
        if (c.isSub) {
            var sp: any = getSubpage(state.editingSubpage);
            sp.order = serializeSubpageGrid(sp);
            saveSubpageConfig(state.editingSubpage);
        }
        else {
            scheduleMainGridSave();
        }
        renderPreview();
        renderButtonSettings();
    }
    function addBulkCardMenuItems(this: any, slots?: any) {
        addCtxItem("clipboard-outline", "Copy " + slots.length + " Cards", function (this: any) { copyButtons(slots); });
        addCtxItem("code-json", "Copy " + slots.length + " Cards as Code", function (this: any) { showCopyCardCode(slots); });
        addCtxItem("content-cut", "Cut " + slots.length + " Cards", function (this: any) { cutButtons(slots); });
        addCtxItem("delete", "Delete " + slots.length + " Cards", function (this: any) { deleteButtons(slots); }, true);
    }
    function cardSizeMenuOptions(this: any, b?: any) {
        var options: any = [
            { size: CARD_SIZE_SINGLE, label: "Single (1x1)" },
        ];
        if (!cardRequiresSquareSize(b)) {
            options.push({ size: CARD_SIZE_TALL, label: "Tall (2x1)" });
            options.push({ size: CARD_SIZE_EXTRA_TALL, label: "Extra Tall (3x1)" });
            options.push({ size: CARD_SIZE_WIDE, label: "Wide (1x2)" });
            options.push({ size: CARD_SIZE_EXTRA_WIDE, label: "Extra Wide (1x3)" });
        }
        options.push({ size: CARD_SIZE_LARGE, label: "Large (2x2)" });
        if (cardRequiresSquareSize(b) && dependencies.layout.gridCols >= 3 && dependencies.layout.gridRows >= 3)
            options.push({ size: CARD_SIZE_EXTRA_LARGE, label: "Extra Large (3x3)" });
        if (cardSupportsMaxSize(b)) {
            options.push({ size: CARD_SIZE_MAX_WIDE, label: "Max Wide (3x2)" });
            options.push({ size: CARD_SIZE_MAX_TALL, label: "Max tall (2x3)" });
        }
        if (cardSupportsLandscapeLargeSize(b))
            options.push({ size: CARD_SIZE_LANDSCAPE_LARGE, label: "Massive Wide (3x4)" });
        if (cardSupportsPortraitLargeSize(b))
            options.push({ size: CARD_SIZE_PORTRAIT_LARGE, label: "Massive (4x3)" });
        return options;
    }
    function addSingleCardMenuItems(this: any, slot?: any) {
        if (slot === -2) {
            addBackButtonMenuItems();
            return;
        }
        var c: any = ctx();
        var b: any = c.buttons[slot - 1];
        addCtxItem("pencil", "Edit Card", function (this: any) { openCardSettings(slot); });
        var ctxTypeDef: any = dependencies.cards.definitions[(b && b.type) || ""];
        if (ctxTypeDef && ctxTypeDef.contextMenuItems &&
            (!c.isSub || buttonTypeRegistryValue(ctxTypeDef, "allowInSubpage", false))) {
            ctxTypeDef.contextMenuItems(slot, b, { addCtxItem: addCtxItem });
        }
        var sz: any = c.sizes[slot] || 1;
        addCtxSubmenu("arrow-expand-all", "Size", function (this: any, sub?: any) {
            cardSizeMenuOptions(b).forEach(function (this: any, option?: any) {
                addSubItem(sub, "", option.label, function (this: any) { resizeSlot(slot, option.size); }, sz === option.size);
            });
        });
        addCtxDivider();
        addCtxItem("content-copy", "Duplicate", function (this: any) {
            if (c.isSub) {
                duplicateSubpageButton(slot);
            }
            else {
                duplicateButton(slot);
            }
        });
        addCtxItem("clipboard-outline", "Copy", function (this: any) { copySlot(slot); });
        addCtxItem("code-json", "Copy Code", function (this: any) { showCopyCardCode([slot]); });
        addCtxItem("content-cut", "Cut", function (this: any) { cutSlot(slot); });
        addCtxItem("delete", "Delete", function (this: any) { deleteSlot(slot); }, true);
    }
    function addClockBarMenuItems(this: any, item?: any) {
        if (isClockBarTemperatureItem(item)) {
            addCtxItem("pencil", "Edit Temperature", function (this: any) { openClockBarTemperatureSettings(); });
            addCtxDivider();
        }
        else if (item === "voice") {
            addCtxItem("pencil", "Edit Voice Services", function (this: any) { openVoiceServicesSettings(); });
            addCtxDivider();
        }
        var visible: any = clockBarItemActive(item);
        var label: any = clockBarItemLabel(item);
        addCtxItem(visible ? "eye-off-outline" : "eye-outline", (visible ? "Hide " : "Show ") + label, function (this: any) {
            setClockBarItemVisible(item, !visible);
        });
    }
    function showSelectionMenu(this: any, e?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        var c: any = ctx();
        if (!c.selected.length)
            return;
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        if (c.selected.length > 1) {
            addBulkCardMenuItems(c.selected.slice());
        }
        else {
            addSingleCardMenuItems(c.selected[0]);
        }
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function showClockBarContextMenu(this: any, e?: any, item?: any) {
        if (isConfigLocked() || clockBarItems().indexOf(item) === -1)
            return;
        hideContextMenu();
        var c: any = ctx();
        if (state.clockBarSelectedItem !== item) {
            c.setSelected([]);
            c.setLastClicked(-1);
            state.clockBarSelectedItem = item;
            hideSettingsOverlay();
            updateClockBarItemUi();
            renderPreview();
            renderButtonSettings();
        }
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        addClockBarMenuItems(item);
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function showContextMenu(this: any, e?: any, slot?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        var c: any = ctx();
        if (c.selected.indexOf(slot) === -1) {
            if (c.selected.length > 1) {
                c.selected.push(slot);
            }
            else {
                c.setSelected([slot]);
                c.setLastClicked(slot);
            }
            renderPreview();
            renderButtonSettings();
            c = ctx();
        }
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        if (c.selected.length > 1 && c.selected.indexOf(slot) !== -1) {
            addBulkCardMenuItems(c.selected.slice());
        }
        else {
            addSingleCardMenuItems(slot);
        }
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function showBackContextMenu(this: any, e?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        addBackButtonMenuItems();
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function addBackButtonMenuItems(this: any) {
        var sp: any = getSubpage(state.editingSubpage);
        var bkSz: any = sp.sizes[-2] || 1;
        addCtxItem("pencil", "Edit Label", function (this: any) { openCardSettings(-2); });
        addCtxItem("keyboard-return", "Exit Subpage", function (this: any) { exitSubpage(); });
        addCtxDivider();
        addCtxSubmenu("arrow-expand-all", "Size", function (this: any, sub?: any) {
            addSubItem(sub, "", "Single (1x1)", function (this: any) { resizeSlot(-2, 1); }, bkSz === 1);
            addSubItem(sub, "", "Tall (2x1)", function (this: any) { resizeSlot(-2, 2); }, bkSz === 2);
            addSubItem(sub, "", "Extra Tall (3x1)", function (this: any) { resizeSlot(-2, 5); }, bkSz === 5);
            addSubItem(sub, "", "Wide (1x2)", function (this: any) { resizeSlot(-2, 3); }, bkSz === 3);
            addSubItem(sub, "", "Extra Wide (1x3)", function (this: any) { resizeSlot(-2, 6); }, bkSz === 6);
            addSubItem(sub, "", "Large (2x2)", function (this: any) { resizeSlot(-2, 4); }, bkSz === 4);
        });
    }
    function showEmptySlotMenu(this: any, e?: any, pos?: any) {
        if (isConfigLocked())
            return;
        hideContextMenu();
        ctxMenu = document.createElement("div");
        ctxMenu.className = "sp-ctx-menu";
        var c: any = ctx();
        addCtxItem("plus", "Create Card", function (this: any) { addSlot(pos); });
        if (!c.isSub) {
            addCtxItem("folder-plus", "Create Subpage", function (this: any) { addSubpageSlot(pos); });
        }
        if (state.clipboard) {
            var count: any = state.clipboard.buttons.length;
            addCtxItem("content-paste", count > 1 ? "Paste " + count + " Cards" : "Paste", function (this: any) {
                if (c.isSub) {
                    pasteSubpageButton(pos);
                }
                else {
                    pasteButton(pos);
                }
            });
        }
        addCtxItem("code-json", "Paste Code", function (this: any) {
            showPasteCardCode(pos, c.isSub);
        });
        document.body.appendChild(ctxMenu);
        positionMenu(ctxMenu, e);
    }
    function hideContextMenu(this: any) {
        if (ctxMenu && ctxMenu.parentNode) {
            ctxMenu.parentNode.removeChild(ctxMenu);
        }
        ctxMenu = null;
    }
    return {
        hide: hideContextMenu,
        contains: (target) => !!(ctxMenu && ctxMenu.contains(target)),
        cardSizeOptions: cardSizeMenuOptions,
        showSelection: showSelectionMenu,
        showClockBar: showClockBarContextMenu,
        showCard: showContextMenu,
        showBack: showBackContextMenu,
        showEmpty: showEmptySlotMenu,
    };
}
