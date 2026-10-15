import { createColorSwatches } from "../components/color_swatches";
import { configOptionValue, setCardColor } from "../model/config_primitives";
import { splitSubpageConfigChunks } from "../model/subpage";
import { defaultCardBackgroundColor } from "../state/preview_theme";
import type { ConfigPersistenceFeature } from "./config_post_api";
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
    CARD_SIZE_ULTRA_WIDE,
    CARD_SIZE_WIDE,
    sizeColSpan,
    sizeRowSpan,
} from "../model/grid";
import { mdiIcon } from "./ui_primitives";
import { clampMenuPosition } from "../features/preview";
import { resizeGridSlot } from "../features/preview_grid";
import { installPreviewResize, resizeTargetDimensions } from "./preview_resize";
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
    readonly configPersistence: Pick<ConfigPersistenceFeature, "saveButtonConfig" | "subpageEntityKeys">;
    readonly clockBar: Pick<ClockBarFeature, "setItemVisible">;
    readonly shell: Pick<ControlsShellFeature, "isConfigLocked" | "showBanner">;
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
    const { isConfigLocked, showBanner } = dependencies.shell;
    const { setItemVisible: setClockBarItemVisible } = dependencies.clockBar;
    const { clockBarItemActive, clockBarItemLabel, clockBarItems, isClockBarTemperatureItem, updateClockBarItemUi } = dependencies.statusPreview;
    const { ctx, scheduleMainGridSave } = dependencies.grid;
    const { hideSettingsOverlay, openClockBarTemperatureSettings } = dependencies.selection;
    const { registryValue: buttonTypeRegistryValue } = dependencies.preview;
    const { copyButtons, copySlot, cutButtons, cutSlot, pasteButton, pasteSubpageButton, showCopyCode: showCopyCardCode, showPasteCode: showPasteCardCode } = dependencies.clipboard;
    const { renderPreview, renderButtonSettings, openCardSettings, openVoiceServicesSettings, addSlot, addSubpageSlot, duplicateButton, duplicateSubpageButton, deleteSlot, deleteButtons } = dependencies;
    const {
        cardIsWifiSharing,
        cardSupportsWifiPortraitSizes,
        cardSupportsExtraLargeSize,
        cardSupportsMaxSize,
        cardSupportsSquareMaxSizes,
        cardSupportsPortraitLargeSize,
        cardSupportsLandscapeLargeSize,
        cardSupportsUltraWideSize,
        normalizeCardSizeForConfig,
        serializeButtonConfig,
        serializeSubpageConfig,
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
    function addCtxItem(this: any, icon?: any, text?: any, handler?: any, danger?: any, container: HTMLElement = ctxMenu) {
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
        container.appendChild(item);
    }
    function addCtxDivider(container: HTMLElement = ctxMenu) {
        var div: any = document.createElement("div");
        div.className = "sp-ctx-divider";
        container.appendChild(div);
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
    function supportsCardColour(slot: number) {
        const c = ctx();
        if (slot === -2) return c.isSub;
        const button = c.buttons[slot - 1];
        return !!button && button.type !== "image" && button.type !== "wifi_qr_card";
    }
    function addColourMenuItem(slots: number[]) {
        const colourSlots = slots.filter(supportsCardColour);
        if (!colourSlots.length) return;
        const item = document.createElement("div");
        item.className = "sp-ctx-sub";
        const trigger = document.createElement("div");
        trigger.className = "sp-ctx-item sp-ctx-sub-trigger";
        trigger.tabIndex = 0;
        trigger.setAttribute("role", "button");
        trigger.setAttribute("aria-label", "Change Colour");
        trigger.setAttribute("aria-haspopup", "true");
        trigger.setAttribute("aria-expanded", "false");
        trigger.appendChild(mdiIcon("palette"));
        trigger.appendChild(document.createTextNode("Change Colour"));
        const submenu = createColourPicker(colourSlots);
        item.append(trigger, submenu);
        ctxMenu.appendChild(item);
        let closeTimer: number | undefined;
        function open() {
            window.clearTimeout(closeTimer);
            if (isConfigLocked()) return;
            submenu.style.display = "block";
            const bounds = trigger.getBoundingClientRect();
            const width = submenu.offsetWidth;
            const x = bounds.right + width <= window.innerWidth - 4 ? bounds.right : bounds.left - width;
            positionMenu(submenu, { clientX: x, clientY: bounds.top });
            trigger.setAttribute("aria-expanded", "true");
        }
        function close() {
            window.clearTimeout(closeTimer);
            submenu.style.display = "none";
            trigger.setAttribute("aria-expanded", "false");
        }
        item.addEventListener("mouseenter", open);
        item.addEventListener("mouseleave", () => {
            closeTimer = window.setTimeout(close, 150);
        });
        trigger.addEventListener("mousedown", e => e.preventDefault());
        trigger.addEventListener("click", e => {
            e.stopPropagation();
            open();
        });
        item.addEventListener("keydown", e => {
            if (e.target === trigger && ["ArrowRight", "Enter", " "].includes(e.key)) {
                e.preventDefault();
                open();
                submenu.querySelector("button")?.focus();
            } else if (e.key === "ArrowLeft") {
                e.preventDefault();
                close();
                trigger.focus();
            }
        });
    }
    function createColourPicker(slots: number[]) {
        const c = ctx();
        const homeSlot = state.editingSubpage;
        function colour(slot: number): string {
            if (slot === -2) return getSubpage(homeSlot).backColor || defaultCardBackgroundColor("", state);
            const button = c.buttons[slot - 1];
            return configOptionValue(button.options, "card_off_color") || defaultCardBackgroundColor(button.type || "", state);
        }
        function saveColour(hex: string) {
            if (isConfigLocked() || state.editingSubpage !== homeSlot) return;
            const current = ctx();
            if (slots.some(slot => !current.grid.includes(slot) || !supportsCardColour(slot))) return;
            if (!current.isSub && slots.some(slot => {
                const button = current.buttons[slot - 1];
                return serializeButtonConfig({ ...button, options: setCardColor(button.options, hex) }).length > 255;
            })) {
                hideContextMenu();
                showBanner("Card settings are too large to save. Shorten confirmation text, labels, or entity IDs.", "error");
                return;
            }
            if (current.isSub) {
                const subpage = getSubpage(homeSlot);
                const candidate = { ...subpage, buttons: subpage.buttons.map((button: any) => ({ ...button })) };
                for (const slot of slots) {
                    if (slot === -2) candidate.backColor = hex;
                    else candidate.buttons[slot - 1].options = setCardColor(candidate.buttons[slot - 1].options, hex);
                }
                candidate.order = serializeSubpageGrid(candidate);
                if (!splitSubpageConfigChunks(serializeSubpageConfig(candidate), dependencies.configPersistence.subpageEntityKeys().length, 255)) {
                    hideContextMenu();
                    showBanner("Subpage is too large to save. Shorten labels or entity IDs.", "error");
                    return;
                }
            }
            for (const slot of slots) {
                if (slot === -2) getSubpage(homeSlot).backColor = hex;
                else {
                    const button = current.buttons[slot - 1];
                    button.options = setCardColor(button.options, hex);
                    if (!c.isSub) dependencies.configPersistence.saveButtonConfig(slot);
                }
            }
            if (c.isSub) saveSubpageConfig(homeSlot);
            renderPreview();
            renderButtonSettings();
            hideContextMenu();
        }
        const submenu = document.createElement("div");
        submenu.className = "sp-ctx-submenu sp-ctx-colours";
        const firstColour = colour(slots[0]!);
        const selectedColour = slots.every(slot => colour(slot) === firstColour) ? firstColour : "";
        submenu.appendChild(createColorSwatches(selectedColour, saveColour, "Card colour presets"));
        // Keep the native picker attached until its change event commits the colour.
        submenu.addEventListener("click", e => e.stopPropagation());
        addCtxDivider(submenu);
        addCtxItem("restore", "Reset Colour", () => saveColour(""), false, submenu);
        return submenu;
    }
    function addBulkCardMenuItems(this: any, slots?: any) {
        addColourMenuItem(slots);
        addCtxItem("clipboard-outline", "Copy " + slots.length + " Cards", function (this: any) { copyButtons(slots); });
        addCtxItem("code-json", "Copy " + slots.length + " Cards as Code", function (this: any) { showCopyCardCode(slots); });
        addCtxItem("content-cut", "Cut " + slots.length + " Cards", function (this: any) { cutButtons(slots); });
        addCtxItem("delete", "Delete " + slots.length + " Cards", function (this: any) { deleteButtons(slots); }, true);
    }
    function cardSizeMenuOptions(this: any, b?: any) {
        var options: any = [
            { size: CARD_SIZE_SINGLE, label: "Single (1x1)" },
        ];
        function fitsScreen(size: number) {
            return sizeColSpan(size) <= dependencies.layout.gridCols &&
                sizeRowSpan(size) <= dependencies.layout.gridRows;
        }
        if (!cardIsWifiSharing(b)) {
            if (fitsScreen(CARD_SIZE_TALL)) options.push({ size: CARD_SIZE_TALL, label: "Tall (2x1)" });
            if (fitsScreen(CARD_SIZE_EXTRA_TALL)) options.push({ size: CARD_SIZE_EXTRA_TALL, label: "Extra Tall (3x1)" });
            if (fitsScreen(CARD_SIZE_WIDE)) options.push({ size: CARD_SIZE_WIDE, label: "Wide (1x2)" });
            if (fitsScreen(CARD_SIZE_EXTRA_WIDE)) options.push({ size: CARD_SIZE_EXTRA_WIDE, label: "Extra Wide (1x3)" });
            if (cardSupportsUltraWideSize(b))
                options.push({ size: CARD_SIZE_ULTRA_WIDE, label: "Ultra Wide (1x5)" });
        }
        if (fitsScreen(CARD_SIZE_LARGE)) options.push({ size: CARD_SIZE_LARGE, label: "Large (2x2)" });
        if (cardSupportsExtraLargeSize(b) && fitsScreen(CARD_SIZE_EXTRA_LARGE))
            options.push({ size: CARD_SIZE_EXTRA_LARGE, label: "Extra Large (3x3)" });
        if (cardSupportsWifiPortraitSizes(b)) {
            if (fitsScreen(CARD_SIZE_MAX_TALL)) options.push({ size: CARD_SIZE_MAX_TALL, label: "Max Tall (2x3)" });
            if (fitsScreen(CARD_SIZE_PORTRAIT_LARGE)) options.push({ size: CARD_SIZE_PORTRAIT_LARGE, label: "Massive (3x4)" });
        }
        if (cardSupportsMaxSize(b) || cardSupportsSquareMaxSizes(b)) {
            if (fitsScreen(CARD_SIZE_MAX_WIDE)) options.push({ size: CARD_SIZE_MAX_WIDE, label: "Max Wide (3x2)" });
            if (fitsScreen(CARD_SIZE_MAX_TALL)) options.push({ size: CARD_SIZE_MAX_TALL, label: "Max tall (2x3)" });
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
        addColourMenuItem([slot]);
        var ctxTypeDef: any = dependencies.cards.definitions[(b && b.type) || ""];
        if (ctxTypeDef && ctxTypeDef.contextMenuItems &&
            (!c.isSub || buttonTypeRegistryValue(ctxTypeDef, "allowInSubpage", false))) {
            ctxTypeDef.contextMenuItems(slot, b, { addCtxItem: addCtxItem });
        }
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
        addCtxItem("pencil", "Edit Label", function (this: any) { openCardSettings(-2); });
        addColourMenuItem([-2]);
        addCtxItem("keyboard-return", "Exit Subpage", function (this: any) { exitSubpage(); });
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
        addCtxDivider();
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
    installPreviewResize({
        document,
        isConfigLocked,
        commit: resizeSlot,
        targets(slot, card) {
            const c = ctx();
            const pos = c.grid.indexOf(slot);
            const currentSize = c.sizes[slot] || 1;
            const rect = card.getBoundingClientRect();
            const gap = parseFloat(window.getComputedStyle(card.parentElement!).gap) || 0;
            const cellWidth = (rect.width - (sizeColSpan(currentSize) - 1) * gap) / sizeColSpan(currentSize);
            const cellHeight = (rect.height - (sizeRowSpan(currentSize) - 1) * gap) / sizeRowSpan(currentSize);
            return cardSizeMenuOptions(slot === -2 ? null : c.buttons[slot - 1])
                .filter((option: { size: number }) => slot !== -2 || [CARD_SIZE_SINGLE, CARD_SIZE_TALL, CARD_SIZE_EXTRA_TALL, CARD_SIZE_WIDE, CARD_SIZE_EXTRA_WIDE, CARD_SIZE_ULTRA_WIDE, CARD_SIZE_LARGE].includes(option.size))
                .filter((option: { size: number }) => resizeGridSlot(c.grid, c.sizes, slot, pos, option.size, c.maxSlots, dependencies.layout.gridCols, !c.isSub).accepted)
                .map((option: { size: number }) => resizeTargetDimensions(option.size, cellWidth, cellHeight, gap));
        },
    });
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
