import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installPreviewInteractionsModule(): GlobalDescriptors {
    // ── Preview event delegation & drag ────────────────────────────────────
    function clearPlaceholder(this: any) {
        if (previewPlaceholder) {
            previewPlaceholder.classList.remove("sp-drop-placeholder");
            previewPlaceholder = null;
        }
    }
    function clearTextSelection(this: any) {
        var selection: any = window.getSelection && window.getSelection();
        if (selection && selection.removeAllRanges)
            selection.removeAllRanges();
    }
    function setupPreviewEvents(this: any) {
        var container: any = els.previewMain;
        var pendingCellIdx: any = -1;
        state.clockBarDragItem = "";
        if (els.topbar) {
            els.topbar.addEventListener("click", function (this: any, e?: any) {
                if (isConfigLocked()) {
                    e.preventDefault();
                    e.stopPropagation();
                    return;
                }
                var target: any = e.target.closest("[data-clockbar-item]");
                if (!target)
                    return;
                e.preventDefault();
                e.stopPropagation();
                selectClockBarItem(target.getAttribute("data-clockbar-item"));
            });
            els.topbar.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key !== "Enter" && e.key !== " ")
                    return;
                var target: any = e.target.closest("[data-clockbar-item]");
                if (!target)
                    return;
                e.preventDefault();
                selectClockBarItem(target.getAttribute("data-clockbar-item"));
            });
            els.topbar.addEventListener("contextmenu", function (this: any, e?: any) {
                if (isConfigLocked()) {
                    e.preventDefault();
                    return;
                }
                var target: any = e.target.closest("[data-clockbar-item]");
                if (!target)
                    return;
                e.preventDefault();
                e.stopPropagation();
                showClockBarContextMenu(e, target.getAttribute("data-clockbar-item"));
            });
        }
        function isBackExitTarget(this: any, e?: any, target?: any) {
            var icon: any = target.querySelector(".sp-back-hit");
            if (!icon)
                return false;
            var rect: any = icon.getBoundingClientRect();
            var pad: any = 12;
            return e.clientX >= rect.left - pad &&
                e.clientX <= rect.right + pad &&
                e.clientY >= rect.top - pad &&
                e.clientY <= rect.bottom + pad;
        }
        container.addEventListener("mousedown", function (this: any, e?: any) {
            if (isConfigLocked())
                return;
            if (!e.target.closest("[data-pos]"))
                return;
            if (e.shiftKey || e.ctrlKey || e.metaKey)
                e.preventDefault();
        });
        // Click delegation
        container.addEventListener("click", function (this: any, e?: any) {
            if (isConfigLocked()) {
                e.preventDefault();
                e.stopPropagation();
                return;
            }
            if (e.target.closest(".sp-subpage-badge")) {
                var btnEl: any = e.target.closest("[data-slot]");
                if (btnEl) {
                    var badgeSlot: any = parseInt(btnEl.getAttribute("data-slot"), 10);
                    enterSubpage(badgeSlot);
                    return;
                }
            }
            var target: any = e.target.closest("[data-pos]");
            if (!target)
                return;
            var pos: any = parseInt(target.getAttribute("data-pos"), 10);
            var c: any = ctx();
            var slot: any = c.grid[pos];
            if (slot > 0) {
                handleBtnClick(e, slot, pos);
            }
            else if (slot === -2) {
                if (didDrag) {
                    didDrag = false;
                    return;
                }
                if (isBackExitTarget(e, target)) {
                    exitSubpage();
                }
                else {
                    handleBtnClick(e, slot, pos);
                }
            }
            else if (slot === 0) {
                if (state.clipboard) {
                    e.preventDefault();
                    e.stopPropagation();
                    showEmptySlotMenu(e, pos);
                }
                else {
                    addSlot(pos);
                }
            }
        });
        // Context menu delegation
        container.addEventListener("contextmenu", function (this: any, e?: any) {
            if (isConfigLocked()) {
                e.preventDefault();
                return;
            }
            var target: any = e.target.closest("[data-pos]");
            if (!target)
                return;
            e.preventDefault();
            var pos: any = parseInt(target.getAttribute("data-pos"), 10);
            var c: any = ctx();
            var slot: any = c.grid[pos];
            if (slot > 0) {
                showContextMenu(e, slot);
            }
            else if (slot === -2) {
                showBackContextMenu(e);
            }
            else if (slot === 0) {
                showEmptySlotMenu(e, pos);
            }
        });
        // Drag delegation
        container.addEventListener("dragstart", function (this: any, e?: any) {
            if (isConfigLocked()) {
                e.preventDefault();
                return;
            }
            var target: any = e.target.closest(".sp-btn") || e.target.closest(".sp-back-btn");
            if (!target)
                return;
            var pos: any = parseInt(target.getAttribute("data-pos"), 10);
            dragSrcPos = pos;
            if (CFG.dragAnimation)
                dragSrcEl = target;
            dragIsSubpage = !!state.editingSubpage;
            didDrag = true;
            dragEnterCount = 0;
            container.classList.add("sp-drag-active");
            e.dataTransfer.effectAllowed = "move";
            e.dataTransfer.setData("text/plain", String(pos));
            if (CFG.dragAnimation) {
                requestAnimationFrame(function (this: any) { target.classList.add("sp-dragging"); });
            }
        });
        container.addEventListener("dragend", function (this: any) {
            dragSrcPos = -1;
            previewDropIdx = -1;
            dragIsSubpage = false;
            dragEnterCount = 0;
            clearPlaceholder();
            if (dragSrcEl) {
                dragSrcEl.classList.remove("sp-dragging");
                dragSrcEl = null;
            }
            setTimeout(function (this: any) { container.classList.remove("sp-drag-active"); }, 50);
        });
        // Drop zone
        function updatePlaceholder(this: any, cellIdx?: any) {
            if (cellIdx === previewDropIdx)
                return;
            previewDropIdx = cellIdx;
            clearPlaceholder();
            var target: any = container.querySelector('[data-pos="' + cellIdx + '"]');
            if (target) {
                previewPlaceholder = target;
                previewPlaceholder.classList.add("sp-drop-placeholder");
            }
        }
        container.addEventListener("dragenter", function (this: any, e?: any) {
            if (isConfigLocked())
                return;
            if (dragSrcPos < 0)
                return;
            e.preventDefault();
            dragEnterCount++;
        });
        container.addEventListener("dragover", function (this: any, e?: any) {
            if (isConfigLocked())
                return;
            if (dragSrcPos < 0)
                return;
            e.preventDefault();
            e.dataTransfer.dropEffect = "move";
            if (CFG.dragAnimation) {
                pendingCellIdx = getCellFromEvent(e, container);
                if (dragRafPending)
                    return;
                dragRafPending = true;
                requestAnimationFrame(function (this: any) {
                    dragRafPending = false;
                    if (dragSrcPos < 0)
                        return;
                    updatePlaceholder(pendingCellIdx);
                });
            }
            else {
                updatePlaceholder(getCellFromEvent(e, container));
            }
        });
        container.addEventListener("dragleave", function (this: any) {
            dragEnterCount--;
            if (dragEnterCount <= 0) {
                dragEnterCount = 0;
                previewDropIdx = -1;
                clearPlaceholder();
            }
        });
        container.addEventListener("drop", function (this: any, e?: any) {
            if (isConfigLocked()) {
                e.preventDefault();
                return;
            }
            e.preventDefault();
            dragEnterCount = 0;
            var toPos: any = previewDropIdx;
            previewDropIdx = -1;
            clearPlaceholder();
            if (dragSrcEl) {
                dragSrcEl.classList.remove("sp-dragging");
                dragSrcEl = null;
            }
            var c: any = ctx();
            if (dragSrcPos < 0 || toPos < 0 || toPos >= c.maxSlots) {
                dragSrcPos = -1;
                dragIsSubpage = false;
                return;
            }
            if (dragSrcPos === toPos) {
                dragSrcPos = -1;
                dragIsSubpage = false;
                return;
            }
            if (!moveSelectedToCell(dragSrcPos, toPos))
                moveToCell(dragSrcPos, toPos);
            renderPreview();
            renderButtonSettings();
            c.save();
            dragSrcPos = -1;
            dragIsSubpage = false;
        });
    }
    function handleBtnClick(this: any, e?: any, slot?: any, pos?: any) {
        if (isConfigLocked())
            return;
        if (didDrag) {
            didDrag = false;
            return;
        }
        state.clockBarSelectedItem = "";
        var c: any = ctx();
        if (e.shiftKey || e.ctrlKey || e.metaKey)
            e.preventDefault();
        if (slot === -2) {
            if (c.selected.length === 1 && c.selected[0] === -2) {
                c.setSelected([]);
            }
            else {
                c.setSelected([-2]);
            }
            c.setLastClicked(-1);
            renderPreview();
            renderButtonSettings();
            clearTextSelection();
            return;
        }
        if (e.shiftKey && c.getLastClicked() > 0) {
            var anchorPos: any = c.grid.indexOf(c.getLastClicked());
            if (anchorPos !== -1) {
                var from: any = Math.min(anchorPos, pos);
                var to: any = Math.max(anchorPos, pos);
                var newSel: any = [];
                for (var i: any = from; i <= to; i++) {
                    if (c.grid[i] > 0)
                        newSel.push(c.grid[i]);
                }
                c.setSelected(newSel);
                renderPreview();
                hideSettingsOverlay();
                clearTextSelection();
                return;
            }
        }
        if (e.ctrlKey || e.metaKey) {
            var idx: any = c.selected.indexOf(slot);
            if (idx !== -1) {
                c.selected.splice(idx, 1);
            }
            else {
                c.selected.push(slot);
                c.setLastClicked(slot);
            }
            renderPreview();
            hideSettingsOverlay();
            clearTextSelection();
            return;
        }
        if (c.selected.length === 1 && c.selected[0] === slot) {
            c.setSelected([]);
            c.setLastClicked(-1);
        }
        else {
            c.setSelected([slot]);
            c.setLastClicked(slot);
        }
        renderPreview();
        renderButtonSettings();
    }
    function selectButton(this: any, slot?: any) {
        if (isConfigLocked())
            return;
        if (slot < 1) {
            state.selectedSlots = [];
        }
        else {
            state.selectedSlots = [slot];
            state.lastClickedSlot = slot;
        }
        renderPreview();
        renderButtonSettings();
    }
    // ── Button management (unified) ────────────────────────────────────────
    function firstFreeSlot(this: any) {
        var used: any = {};
        state.grid.forEach(function (this: any, s?: any) {
            if (s > 0)
                used[s] = true;
        });
        for (var i: any = 1; i <= NUM_SLOTS; i++) {
            if (!used[i])
                return i;
        }
        return -1;
    }
    function firstFreeCell(this: any, afterPos?: any) {
        var start: any = afterPos != null ? afterPos : 0;
        for (var i: any = 0; i < NUM_SLOTS; i++) {
            var candidate: any = (start + i) % NUM_SLOTS;
            if (state.grid[candidate] === 0)
                return candidate;
        }
        return -1;
    }
    function emptyButtonConfig(this: any, type?: any) {
        return EspControlModel.emptyCardConfig(type);
    }
    function newCardDraftKey(this: any, isSub?: any, homeSlot?: any, pos?: any, slot?: any) {
        return (isSub ? "sub:" + homeSlot : "main") + ":new:" + pos + ":" + slot;
    }
    function beginNewCardDraft(this: any, pos?: any, slot?: any, isSub?: any) {
        state.settingsDraft = {
            key: newCardDraftKey(isSub, state.editingSubpage, pos, slot),
            slot: slot,
            homeSlot: state.editingSubpage,
            isSub: isSub,
            isNew: true,
            pos: pos,
            dirty: false,
            typeSelected: false,
            button: emptyButtonConfig(),
        };
        if (isSub) {
            state.subpageSelectedSlots = [slot];
            state.subpageLastClicked = slot;
        }
        else {
            state.selectedSlots = [slot];
            state.lastClickedSlot = slot;
        }
        renderPreview();
        renderButtonSettings(true);
    }
    function addSlot(this: any, pos?: any) {
        if (isConfigLocked())
            return;
        var c: any = ctx();
        if (pos < 0 || pos >= c.maxSlots || c.grid[pos] !== 0)
            return;
        if (c.isSub) {
            var sp: any = getSubpage(state.editingSubpage);
            var newSlot: any = subpageFirstFreeSlot(sp);
            beginNewCardDraft(pos, newSlot, true);
        }
        else {
            var slot: any = firstFreeSlot();
            if (slot < 0)
                return;
            beginNewCardDraft(pos, slot, false);
        }
    }
    function addSubpageSlot(this: any, pos?: any) {
        if (isConfigLocked())
            return;
        var c: any = ctx();
        if (c.isSub)
            return;
        var slot: any = firstFreeSlot();
        if (slot < 0)
            return;
        state.buttons[slot - 1] = emptyButtonConfig("subpage");
        state.grid[pos] = slot;
        state.subpages[slot] = { order: [], buttons: [], grid: [], sizes: {} };
        buildSubpageGrid(state.subpages[slot]);
        postText(entityName("button_order"), serializeGrid(state.grid));
        saveButtonConfig(slot);
        saveSubpageEntity(slot);
        selectButton(slot);
    }
    function duplicateButton(this: any, srcSlot?: any) {
        if (isConfigLocked())
            return;
        var newSlot: any = firstFreeSlot();
        if (newSlot < 0)
            return;
        var srcSz: any = state.sizes[srcSlot] || 1;
        var srcPos: any = state.grid.indexOf(srcSlot);
        var placement: any = findDuplicatePlacement(state.grid, srcPos + 1, srcSz, NUM_SLOTS);
        if (placement.pos < 0)
            return;
        var src: any = state.buttons[srcSlot - 1];
        var extraImageCards: any = isImageCard(src) ? 1 : 0;
        if (state.subpages[srcSlot])
            extraImageCards += imageCardCountInSubpage(state.subpages[srcSlot]);
        if (!canAddImageCards(extraImageCards)) {
            showImageCardLimitBanner();
            return;
        }
        state.buttons[newSlot - 1] = {
            entity: src.entity, label: src.label, icon: src.icon,
            icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
            type: src.type || "", precision: src.precision || "",
            options: src.options || "",
        };
        if (placement.size === 1)
            delete state.sizes[newSlot];
        else
            state.sizes[newSlot] = placement.size;
        placeSlotAt(state.grid, newSlot, placement.pos, placement.size);
        if (state.subpages[srcSlot]) {
            var spJson: any = serializeSubpageConfig(state.subpages[srcSlot]);
            var spCopy: any = parseSubpageConfig(spJson);
            spCopy.sizes = {};
            buildSubpageGrid(spCopy);
            state.subpages[newSlot] = spCopy;
        }
        postText(entityName("button_order"), serializeGrid(state.grid));
        saveButtonConfig(newSlot);
        saveSubpageEntity(newSlot);
        state.selectedSlots = [newSlot];
        state.lastClickedSlot = newSlot;
        renderPreview();
    }
    function duplicateSubpageButton(this: any, srcSlot?: any) {
        if (isConfigLocked())
            return;
        var homeSlot: any = state.editingSubpage;
        var sp: any = getSubpage(homeSlot);
        var newSlot: any = subpageFirstFreeSlot(sp);
        while (sp.buttons.length < newSlot) {
            sp.buttons.push(emptyButtonConfig());
        }
        var srcSz: any = sp.sizes[srcSlot] || 1;
        var srcPos: any = sp.grid.indexOf(srcSlot);
        var placement: any = findDuplicatePlacement(sp.grid, srcPos + 1, srcSz, NUM_SLOTS);
        if (placement.pos < 0)
            return;
        var src: any = sp.buttons[srcSlot - 1];
        if (!canAddImageCards(isImageCard(src) ? 1 : 0)) {
            showImageCardLimitBanner();
            return;
        }
        sp.buttons[newSlot - 1] = {
            entity: src.entity, label: src.label, icon: src.icon,
            icon_on: src.icon_on, sensor: src.sensor, unit: src.unit,
            type: src.type || "", precision: src.precision || "",
            options: src.options || "",
        };
        if (placement.size === 1)
            delete sp.sizes[newSlot];
        else
            sp.sizes[newSlot] = placement.size;
        placeSlotAt(sp.grid, newSlot, placement.pos, placement.size);
        sp.order = serializeSubpageGrid(sp);
        saveSubpageConfig(homeSlot);
        state.subpageSelectedSlots = [newSlot];
        state.subpageLastClicked = newSlot;
        renderPreview();
    }
    function deleteSlot(this: any, slot?: any) {
        if (isConfigLocked())
            return;
        var c: any = ctx();
        for (var i: any = 0; i < c.maxSlots; i++) {
            if (c.grid[i] === slot) {
                c.grid[i] = 0;
                var cells: any = coveredCells(i, c.sizes[slot] || 1, c.maxSlots, false);
                for (var ci: any = 0; ci < cells.length; ci++) {
                    if (c.grid[cells[ci]] === -1)
                        c.grid[cells[ci]] = 0;
                }
                break;
            }
        }
        delete c.sizes[slot];
        var selIdx: any = c.selected.indexOf(slot);
        if (selIdx !== -1)
            c.selected.splice(selIdx, 1);
        if (c.isSub) {
            var sp: any = getSubpage(state.editingSubpage);
            if (slot >= 1 && slot <= sp.buttons.length) {
                sp.buttons[slot - 1] = emptyButtonConfig();
            }
            sp.order = serializeSubpageGrid(sp);
            state.subpageLastClicked = -1;
            saveSubpageConfig(state.editingSubpage);
        }
        else {
            postText(entityName("button_order"), serializeGrid(state.grid));
            state.buttons[slot - 1] = emptyButtonConfig();
            delete state.subpages[slot];
            saveButtonConfig(slot);
            saveSubpageEntity(slot);
        }
        renderPreview();
        renderButtonSettings();
    }
    function deleteButtons(this: any, slots?: any) {
        if (isConfigLocked())
            return;
        var c: any = ctx();
        for (var i: any = 0; i < c.maxSlots; i++) {
            if (slots.indexOf(c.grid[i]) !== -1) {
                var cells: any = coveredCells(i, c.sizes[c.grid[i]] || 1, c.maxSlots, false);
                for (var ci: any = 0; ci < cells.length; ci++) {
                    if (c.grid[cells[ci]] === -1)
                        c.grid[cells[ci]] = 0;
                }
                c.grid[i] = 0;
            }
        }
        slots.forEach(function (this: any, slot?: any) { delete c.sizes[slot]; });
        c.setSelected([]);
        c.setLastClicked(-1);
        if (c.isSub) {
            var sp: any = getSubpage(state.editingSubpage);
            slots.forEach(function (this: any, slot?: any) {
                if (slot >= 1 && slot <= sp.buttons.length) {
                    sp.buttons[slot - 1] = emptyButtonConfig();
                }
            });
            sp.order = serializeSubpageGrid(sp);
            saveSubpageConfig(state.editingSubpage);
        }
        else {
            slots.forEach(function (this: any, slot?: any) {
                state.buttons[slot - 1] = emptyButtonConfig();
                delete state.subpages[slot];
                saveButtonConfig(slot);
                saveSubpageEntity(slot);
            });
            postText(entityName("button_order"), serializeGrid(state.grid));
        }
        renderPreview();
        renderButtonSettings();
    }
    return {
        "clearPlaceholder": staticGlobal(clearPlaceholder),
        "clearTextSelection": staticGlobal(clearTextSelection),
        "setupPreviewEvents": staticGlobal(setupPreviewEvents),
        "handleBtnClick": staticGlobal(handleBtnClick),
        "selectButton": staticGlobal(selectButton),
        "firstFreeSlot": staticGlobal(firstFreeSlot),
        "firstFreeCell": staticGlobal(firstFreeCell),
        "emptyButtonConfig": staticGlobal(emptyButtonConfig),
        "newCardDraftKey": staticGlobal(newCardDraftKey),
        "beginNewCardDraft": staticGlobal(beginNewCardDraft),
        "addSlot": staticGlobal(addSlot),
        "addSubpageSlot": staticGlobal(addSubpageSlot),
        "duplicateButton": staticGlobal(duplicateButton),
        "duplicateSubpageButton": staticGlobal(duplicateSubpageButton),
        "deleteSlot": staticGlobal(deleteSlot),
        "deleteButtons": staticGlobal(deleteButtons),
    };
}
