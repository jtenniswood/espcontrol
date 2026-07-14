import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installPreviewClipboardModule(): GlobalDescriptors {
    // ── Preview Clipboard ─────────────────────────────────────────────
    // ── Cut / Paste ────────────────────────────────────────────────────────
    function buildClipboardEntry(this: any, slot?: any) {
        if (slot < 1)
            return null;
        var c: any = ctx();
        var src: any = c.buttons[slot - 1];
        var subpageConfig: any = null;
        if (!c.isSub && src.type === "subpage" && state.subpages[slot]) {
            subpageConfig = serializeSubpageConfig(state.subpages[slot]);
        }
        return ClipboardFeature.createClipboardEntry(src, c.sizes[slot] || 1, subpageConfig);
    }
    function copySlot(this: any, slot?: any) {
        var entry: any = buildClipboardEntry(slot);
        if (!entry)
            return;
        state.clipboard = { buttons: [entry] };
    }
    function copyButtons(this: any, slots?: any) {
        var entries: any = [];
        slots.forEach(function (this: any, slot?: any) {
            var entry: any = buildClipboardEntry(slot);
            if (entry)
                entries.push(entry);
        });
        if (!entries.length)
            return;
        state.clipboard = { buttons: entries };
    }
    function cutSlot(this: any, slot?: any) {
        if (isConfigLocked())
            return;
        if (slot < 1)
            return;
        copySlot(slot);
        deleteSlot(slot);
    }
    function cutButtons(this: any, slots?: any) {
        if (isConfigLocked())
            return;
        var cardSlots: any = slots.filter(function (this: any, slot?: any) { return slot > 0; });
        if (!cardSlots.length)
            return;
        copyButtons(cardSlots);
        deleteButtons(cardSlots);
    }
    function pasteButton(this: any, pos?: any) {
        if (isConfigLocked())
            return;
        if (!state.clipboard)
            return;
        var entries: any = state.clipboard.buttons;
        if (!canAddImageCards(imageCardCountInClipboardEntries(entries))) {
            showImageCardLimitBanner();
            return;
        }
        var used: any = {};
        state.grid.forEach(function (this: any, slot?: any) {
            if (slot > 0)
                used[slot] = true;
        });
        var availableSlots: any = [];
        for (var slot: any = 1; slot <= NUM_SLOTS; slot++) {
            if (!used[slot])
                availableSlots.push(slot);
        }
        var plan: any = ClipboardFeature.planClipboardPaste(entries, state.grid, state.sizes, pos, availableSlots, NUM_SLOTS, GRID_COLS);
        state.grid = plan.grid;
        state.sizes = plan.sizes;
        plan.placements.forEach(function (this: any, placement?: any) {
            var newSlot: any = placement.slot;
            state.buttons[newSlot - 1] = placement.button;
            if (placement.subpageConfig) {
                var spCopy: any = parseSubpageConfig(placement.subpageConfig);
                spCopy.sizes = {};
                buildSubpageGrid(spCopy);
                state.subpages[newSlot] = spCopy;
            }
            saveButtonConfig(newSlot);
            saveSubpageEntity(newSlot);
        });
        postText(entityName("button_order"), serializeGrid(state.grid));
        state.clipboard = null;
        state.selectedSlots = [];
        renderPreview();
    }
    function pasteSubpageButton(this: any, pos?: any) {
        if (isConfigLocked())
            return;
        if (!state.clipboard)
            return;
        var homeSlot: any = state.editingSubpage;
        var sp: any = getSubpage(homeSlot);
        var maxPos: any = NUM_SLOTS;
        var entries: any = state.clipboard.buttons;
        if (!canAddImageCards(imageCardCountInClipboardEntries(entries))) {
            showImageCardLimitBanner();
            return;
        }
        var used: any = {};
        sp.grid.forEach(function (this: any, slot?: any) {
            if (slot > 0)
                used[slot] = true;
        });
        var availableSlots: any = [];
        for (var slot: any = 1; slot <= maxPos; slot++) {
            if (!used[slot])
                availableSlots.push(slot);
        }
        var plan: any = ClipboardFeature.planClipboardPaste(entries, sp.grid, sp.sizes, pos, availableSlots, maxPos, GRID_COLS);
        sp.grid = plan.grid;
        sp.sizes = plan.sizes;
        plan.placements.forEach(function (this: any, placement?: any) {
            var newSlot: any = placement.slot;
            while (sp.buttons.length < newSlot) {
                sp.buttons.push(emptyButtonConfig());
            }
            sp.buttons[newSlot - 1] = placement.button;
        });
        sp.order = serializeSubpageGrid(sp);
        state.clipboard = null;
        saveSubpageConfig(homeSlot);
        state.subpageSelectedSlots = [];
        renderPreview();
    }
    return {
        "buildClipboardEntry": staticGlobal(buildClipboardEntry),
        "copySlot": staticGlobal(copySlot),
        "copyButtons": staticGlobal(copyButtons),
        "cutSlot": staticGlobal(cutSlot),
        "cutButtons": staticGlobal(cutButtons),
        "pasteButton": staticGlobal(pasteButton),
        "pasteSubpageButton": staticGlobal(pasteSubpageButton),
    };
}
