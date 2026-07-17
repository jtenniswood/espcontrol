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
    function cardTransferError(message: any) {
        var err: any = new Error(message);
        err.cardTransferMessage = message;
        return err;
    }
    function cardTransferEntryFromClipboard(entry: any) {
        var transfer: any = {
            entity: entry.entity || "",
            label: entry.label || "",
            icon: entry.icon || "Auto",
            icon_on: entry.icon_on || "Auto",
            sensor: entry.sensor || "",
            unit: entry.unit || "",
            type: entry.type || "",
            precision: entry.precision || "",
            options: entry.options || "",
            size: entry.size || 1,
        };
        if (entry.subpageConfig) {
            transfer.subpage = EspControlModel.structuredSubpageFromParsed(parseSubpageConfig(entry.subpageConfig));
        }
        return transfer;
    }
    function cardTransferCodeForSlots(slots: any) {
        var entries: any = [];
        slots.forEach(function (slot: any) {
            var entry: any = buildClipboardEntry(slot);
            if (entry)
                entries.push(cardTransferEntryFromClipboard(entry));
        });
        return EspControlModel.createCardTransferCode({
            device: DEVICE_ID,
            firmware: String(state.firmwareVersion || ""),
        }, entries);
    }
    function cardTransferTypeLabel(type: any) {
        return type ? type.replace(/_/g, " ") : "switch";
    }
    function validateCardTransferButton(button: any, inSubpage: any, warnings: any) {
        var normalized: any = normalizeButtonConfig(EspControlModel.cloneCardConfig(button));
        var type: any = normalized.type || "";
        var typeDef: any = BUTTON_TYPES[type];
        if (!typeDef) {
            throw cardTransferError("This controller does not support the " +
                cardTransferTypeLabel(type) + " card type.");
        }
        if (buttonConfigDisabledForDevice(normalized)) {
            throw cardTransferError("This controller does not support the " +
                cardTransferTypeLabel(type) + " card type.");
        }
        if (inSubpage && type === "subpage") {
            throw cardTransferError("Subpage cards cannot be placed inside another subpage.");
        }
        if (inSubpage && !buttonTypeRegistryValue(typeDef, "allowInSubpage", false)) {
            throw cardTransferError("The " + cardTransferTypeLabel(type) +
                " card type cannot be placed inside a subpage.");
        }
        if (type === "internal" ||
            (type === "action" && normalized.sensor === ACTION_CARD_LOCAL_ACTION) ||
            (type === "sensor" && normalized.sensor === SENSOR_CARD_LOCAL_SENSOR)) {
            warnings.local = true;
        }
        return normalized;
    }
    function prepareTransferredSubpage(parsed: any) {
        var orderedSlots: any = [];
        var layoutSlots: any = [];
        var requestedSizes: any = {};
        var seen: any = {};
        var hasBack: any = false;
        (parsed.order || []).forEach(function (item: any) {
            var token: any = parseBackOrderToken(item).token;
            if (!token) {
                layoutSlots.push(0);
                return;
            }
            if (EspControlModel.isBackOrderToken(token)) {
                if (!hasBack) {
                    orderedSlots.push(-2);
                    layoutSlots.push(-2);
                }
                else {
                    layoutSlots.push(0);
                }
                hasBack = true;
                requestedSizes[-2] = sizeFromToken(token.charAt(1));
                return;
            }
            var slot: any = parseInt(token, 10);
            if (!slot || slot > parsed.buttons.length || seen[slot]) {
                layoutSlots.push(0);
                return;
            }
            seen[slot] = true;
            orderedSlots.push(slot);
            layoutSlots.push(slot);
            requestedSizes[slot] = sizeFromToken(token.charAt(token.length - 1));
        });
        if (!hasBack) {
            orderedSlots.unshift(-2);
            layoutSlots.unshift(-2);
            requestedSizes[-2] = 1;
        }
        if (orderedSlots.length > NUM_SLOTS) {
            throw cardTransferError("A copied subpage has more cards than this controller can display.");
        }
        var targetSizes: any = {};
        for (var requestedSlot in requestedSizes) {
            var requestedSlotNumber: any = parseInt(requestedSlot, 10);
            var requestedButton: any = requestedSlotNumber > 0
                ? parsed.buttons[requestedSlotNumber - 1]
                : {};
            targetSizes[requestedSlot] = normalizeCardSizeForConfig(requestedButton, requestedSizes[requestedSlot]);
        }
        var placementOrder: any = layoutSlots.length <= NUM_SLOTS ? layoutSlots : orderedSlots;
        var targetGrid: any = placeOrderedGridEntries(placementOrder, targetSizes, NUM_SLOTS);
        var placed: any = {};
        targetGrid.forEach(function (slot: any) {
            if (slot > 0 || slot === -2)
                placed[slot] = true;
        });
        for (var orderedIndex: any = 0; orderedIndex < orderedSlots.length; orderedIndex++) {
            if (!placed[orderedSlots[orderedIndex]]) {
                throw cardTransferError("A copied subpage does not fit on this controller.");
            }
        }
        var slotMap: any = {};
        var buttons: any = [];
        var nextSlot: any = 1;
        orderedSlots.forEach(function (oldSlot: any) {
            if (oldSlot < 1)
                return;
            slotMap[oldSlot] = nextSlot++;
            buttons.push(parsed.buttons[oldSlot - 1]);
        });
        var grid: any = targetGrid.map(function (oldSlot: any) {
            return oldSlot > 0 ? slotMap[oldSlot] : oldSlot;
        });
        var sizes: any = {};
        if (targetSizes[-2] > 1)
            sizes[-2] = targetSizes[-2];
        for (var oldKey in slotMap) {
            if (targetSizes[oldKey] > 1)
                sizes[slotMap[oldKey]] = targetSizes[oldKey];
        }
        var resized: any = false;
        for (var requestedKey in requestedSizes) {
            if ((requestedSizes[requestedKey] || 1) !== (targetSizes[requestedKey] || 1))
                resized = true;
        }
        var subpage: any = {
            order: [],
            buttons: buttons,
            grid: grid,
            sizes: sizes,
            backLabel: parsed.backLabel || "Back",
        };
        subpage.order = serializeSubpageGrid(subpage);
        return { subpage: subpage, resized: resized };
    }
    function clipboardEntriesFromCardTransfer(envelope: any, targetIsSubpage: any) {
        var entries: any = [];
        var warnings: any = { local: false, cardResized: false, subpageResized: false };
        envelope.cards.forEach(function (transfer: any) {
            var button: any = validateCardTransferButton(transfer, targetIsSubpage, warnings);
            var requestedSize: any = transfer.size || 1;
            var normalizedSize: any = normalizeCardSizeForConfig(button, requestedSize);
            if (normalizedSize !== requestedSize)
                warnings.cardResized = true;
            var entry: any = {
                entity: button.entity,
                label: button.label,
                icon: button.icon,
                icon_on: button.icon_on,
                sensor: button.sensor,
                unit: button.unit,
                type: button.type || "",
                precision: button.precision || "",
                options: button.options || "",
                size: normalizedSize,
                subpageConfig: null,
            };
            if (transfer.subpage) {
                if (targetIsSubpage) {
                    throw cardTransferError("Subpage cards can only be pasted onto the home screen.");
                }
                var parsed: any = EspControlModel.parseStructuredSubpageConfig(transfer.subpage);
                parsed.buttons = parsed.buttons.map(function (subpageButton: any) {
                    return validateCardTransferButton(subpageButton, true, warnings);
                });
                var prepared: any = prepareTransferredSubpage(parsed);
                entry.subpageConfig = serializeSubpageConfig(prepared.subpage);
                if (prepared.resized)
                    warnings.subpageResized = true;
            }
            entries.push(entry);
        });
        return { entries: entries, warnings: warnings };
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
    function cloneSizeMap(sizes: any) {
        var out: any = {};
        for (var key in sizes || {})
            out[key] = sizes[key];
        return out;
    }
    function clipboardButtonConfig(entry: any) {
        return normalizeButtonConfig(EspControlModel.cloneCardConfig(entry));
    }
    function firstUnusedClipboardSlot(grid: any, maxSlots: any) {
        var used: any = {};
        grid.forEach(function (slot: any) {
            if (slot > 0)
                used[slot] = true;
        });
        for (var slot: any = 1; slot <= maxSlots; slot++) {
            if (!used[slot])
                return slot;
        }
        return -1;
    }
    function clipboardSubpageFits(sp: any) {
        var serialized: any = serializeSubpageConfig(sp);
        return !!EspControlModel.splitSubpageConfigChunks(serialized, subpageEntityKeys().length, 255);
    }
    function planMainClipboardPaste(entries: any, pos: any) {
        var nextGrid: any = state.grid.slice();
        var nextSizes: any = cloneSizeMap(state.sizes);
        var nextButtons: any = state.buttons.map(function (button: any) {
            return EspControlModel.cloneCardConfig(button);
        });
        var nextSubpages: any = {};
        for (var existingKey in state.subpages)
            nextSubpages[existingKey] = state.subpages[existingKey];
        var slots: any = [];
        var resized: any = 0;
        for (var i: any = 0; i < entries.length; i++) {
            var newSlot: any = firstUnusedClipboardSlot(nextGrid, NUM_SLOTS);
            if (newSlot < 0)
                return { error: "There is not enough room to paste every card." };
            var entry: any = entries[i];
            var requestedSize: any = entry.size || 1;
            var placement: any = findDuplicatePlacement(nextGrid, pos, requestedSize, NUM_SLOTS);
            if (placement.pos < 0)
                return { error: "There is not enough room to paste every card." };
            if (placement.size !== requestedSize)
                resized++;
            var buttonConfig: any = clipboardButtonConfig(entry);
            if (serializeButtonConfig(buttonConfig).length > 255) {
                return { error: "A copied card's settings are too large for this controller." };
            }
            nextButtons[newSlot - 1] = buttonConfig;
            if (placement.size === 1)
                delete nextSizes[newSlot];
            else
                nextSizes[newSlot] = placement.size;
            placeSlotAt(nextGrid, newSlot, placement.pos, placement.size);
            if (entry.subpageConfig) {
                var subpage: any = parseSubpageConfig(entry.subpageConfig);
                subpage.sizes = {};
                buildSubpageGrid(subpage);
                if (!clipboardSubpageFits(subpage)) {
                    return { error: "A copied subpage is too large for this controller." };
                }
                nextSubpages[newSlot] = subpage;
            }
            else {
                delete nextSubpages[newSlot];
            }
            slots.push(newSlot);
        }
        return {
            grid: nextGrid,
            sizes: nextSizes,
            buttons: nextButtons,
            subpages: nextSubpages,
            slots: slots,
            resized: resized,
        };
    }
    function cloneSubpageForClipboard(sp: any) {
        return {
            order: (sp.order || []).slice(),
            buttons: (sp.buttons || []).map(function (button: any) {
                return EspControlModel.cloneCardConfig(button);
            }),
            grid: (sp.grid || []).slice(),
            sizes: cloneSizeMap(sp.sizes),
            backLabel: sp.backLabel || "Back",
        };
    }
    function planSubpageClipboardPaste(entries: any, pos: any) {
        var homeSlot: any = state.editingSubpage;
        var subpage: any = cloneSubpageForClipboard(getSubpage(homeSlot));
        var slots: any = [];
        var resized: any = 0;
        for (var i: any = 0; i < entries.length; i++) {
            var entry: any = entries[i];
            var typeDef: any = BUTTON_TYPES[entry.type || ""];
            if (entry.subpageConfig || entry.type === "subpage") {
                return { error: "Subpage cards can only be pasted onto the home screen." };
            }
            if (!typeDef || !buttonTypeRegistryValue(typeDef, "allowInSubpage", false)) {
                return { error: "The " + cardTransferTypeLabel(entry.type || "") +
                        " card type cannot be placed inside a subpage." };
            }
            var newSlot: any = firstUnusedClipboardSlot(subpage.grid, NUM_SLOTS);
            if (newSlot < 0)
                return { error: "There is not enough room to paste every card." };
            var requestedSize: any = entry.size || 1;
            var placement: any = findDuplicatePlacement(subpage.grid, pos, requestedSize, NUM_SLOTS);
            if (placement.pos < 0)
                return { error: "There is not enough room to paste every card." };
            if (placement.size !== requestedSize)
                resized++;
            while (subpage.buttons.length < newSlot)
                subpage.buttons.push(emptyButtonConfig());
            subpage.buttons[newSlot - 1] = clipboardButtonConfig(entry);
            if (placement.size === 1)
                delete subpage.sizes[newSlot];
            else
                subpage.sizes[newSlot] = placement.size;
            placeSlotAt(subpage.grid, newSlot, placement.pos, placement.size);
            slots.push(newSlot);
        }
        subpage.order = serializeSubpageGrid(subpage);
        if (!clipboardSubpageFits(subpage)) {
            return { error: "The updated subpage is too large to save on this controller." };
        }
        return { subpage: subpage, slots: slots, resized: resized };
    }
    function performClipboardPaste(entries: any, pos: any, targetIsSubpage: any) {
        if (isConfigLocked())
            return { ok: false, error: "Configuration is locked." };
        if (!entries || !entries.length)
            return { ok: false, error: "No copied cards are available." };
        if (!canAddImageCards(imageCardCountInClipboardEntries(entries))) {
            showImageCardLimitBanner();
            return { ok: false, error: imageSlotCapacityMessage() };
        }
        var plan: any = targetIsSubpage
            ? planSubpageClipboardPaste(entries, pos)
            : planMainClipboardPaste(entries, pos);
        if (plan.error) {
            showBanner(plan.error, "error");
            return { ok: false, error: plan.error };
        }
        if (targetIsSubpage) {
            var homeSlot: any = state.editingSubpage;
            state.subpages[homeSlot] = plan.subpage;
            saveSubpageConfig(homeSlot);
            state.subpageSelectedSlots = [];
        }
        else {
            state.grid = plan.grid;
            state.sizes = plan.sizes;
            state.buttons = plan.buttons;
            state.subpages = plan.subpages;
            for (var i: any = 0; i < plan.slots.length; i++) {
                saveButtonConfig(plan.slots[i]);
                saveSubpageEntity(plan.slots[i]);
            }
            postText(entityName("button_order"), serializeGrid(state.grid));
            state.selectedSlots = [];
        }
        renderPreview();
        return { ok: true, count: plan.slots.length, resized: plan.resized };
    }
    function pasteButton(this: any, pos?: any) {
        if (!state.clipboard)
            return;
        var result: any = performClipboardPaste(state.clipboard.buttons, pos, false);
        if (!result.ok)
            return result;
        state.clipboard = null;
        if (result.resized)
            showBanner("Cards pasted. Some were resized to fit.", "warning");
        return result;
    }
    function pasteSubpageButton(this: any, pos?: any) {
        if (!state.clipboard)
            return;
        var result: any = performClipboardPaste(state.clipboard.buttons, pos, true);
        if (!result.ok)
            return result;
        state.clipboard = null;
        if (result.resized)
            showBanner("Cards pasted. Some were resized to fit.", "warning");
        return result;
    }
    var cardTransferOverlay: any = null;
    var cardTransferPreviousFocus: any = null;
    var cardTransferKeyHandler: any = null;
    var cardTransferDialogCounter: any = 0;
    function closeCardTransferDialog() {
        if (!cardTransferOverlay)
            return;
        if (cardTransferKeyHandler)
            document.removeEventListener("keydown", cardTransferKeyHandler);
        cardTransferOverlay.remove();
        cardTransferOverlay = null;
        cardTransferKeyHandler = null;
        if (cardTransferPreviousFocus && cardTransferPreviousFocus.focus)
            cardTransferPreviousFocus.focus();
        cardTransferPreviousFocus = null;
    }
    function createCardTransferDialog(title: any) {
        closeCardTransferDialog();
        cardTransferPreviousFocus = document.activeElement;
        var overlay: any = document.createElement("div");
        overlay.className = "sp-transfer-overlay";
        var dialog: any = document.createElement("div");
        dialog.className = "sp-transfer-dialog";
        dialog.setAttribute("role", "dialog");
        dialog.setAttribute("aria-modal", "true");
        var titleId: any = "sp-transfer-title-" + (++cardTransferDialogCounter);
        dialog.setAttribute("aria-labelledby", titleId);
        var heading: any = document.createElement("h2");
        heading.id = titleId;
        heading.textContent = title;
        dialog.appendChild(heading);
        var close: any = document.createElement("button");
        close.type = "button";
        close.className = "sp-transfer-close";
        close.innerHTML = '<svg class="sp-transfer-close-icon" viewBox="0 0 24 24" aria-hidden="true" focusable="false">' +
            '<path d="M18.3 5.71 12 12l6.3 6.29-1.41 1.41L10.59 13.41 4.29 19.7 2.88 18.29 9.17 12 2.88 5.71 4.29 4.3l6.3 6.29 6.3-6.29z"></path>' +
            '</svg>';
        close.setAttribute("aria-label", "Close");
        close.addEventListener("click", closeCardTransferDialog);
        dialog.appendChild(close);
        overlay.appendChild(dialog);
        overlay.addEventListener("mousedown", function (event: any) {
            if (event.target === overlay)
                closeCardTransferDialog();
        });
        cardTransferKeyHandler = function (event: any) {
            if (event.key === "Escape")
                closeCardTransferDialog();
        };
        document.addEventListener("keydown", cardTransferKeyHandler);
        document.body.appendChild(overlay);
        cardTransferOverlay = overlay;
        return dialog;
    }
    function showCopyCardCode(slots: any) {
        var code: any;
        try {
            code = cardTransferCodeForSlots(slots);
        }
        catch (error: any) {
            showBanner(error.cardTransferMessage || error.message || "Could not create card code.", "error");
            return;
        }
        var dialog: any = createCardTransferDialog("Copy Code");
        var intro: any = document.createElement("p");
        intro.textContent = "Copy this code to another controller.";
        dialog.appendChild(intro);
        var textarea: any = document.createElement("textarea");
        textarea.className = "sp-input sp-textarea sp-transfer-code";
        textarea.readOnly = true;
        textarea.value = code;
        textarea.setAttribute("aria-label", "Card transfer code");
        dialog.appendChild(textarea);
        var privacy: any = document.createElement("p");
        privacy.className = "sp-transfer-note";
        privacy.textContent = "Keep this code private. It can include webhook URLs, headers, or other sensitive configuration.";
        dialog.appendChild(privacy);
        textarea.focus();
        textarea.select();
    }
    function showPasteCardCode(pos: any, targetIsSubpage: any) {
        var dialog: any = createCardTransferDialog("Paste Code");
        var intro: any = document.createElement("p");
        intro.textContent = "Paste the code below. Nothing is changed until you choose Paste.";
        dialog.appendChild(intro);
        var textarea: any = document.createElement("textarea");
        textarea.className = "sp-input sp-textarea sp-transfer-code";
        textarea.placeholder = "Paste the code here";
        textarea.setAttribute("aria-label", "Card transfer code");
        dialog.appendChild(textarea);
        var errorText: any = document.createElement("div");
        errorText.className = "sp-transfer-error";
        errorText.setAttribute("aria-live", "assertive");
        dialog.appendChild(errorText);
        var actions: any = document.createElement("div");
        actions.className = "sp-transfer-actions sp-btn-row";
        var cancel: any = createActionButton("sp-action-btn sp-cancel-btn", "Cancel");
        cancel.addEventListener("click", closeCardTransferDialog);
        actions.appendChild(cancel);
        var paste: any = createActionButton("sp-action-btn sp-save-btn", "Paste");
        paste.addEventListener("click", function () {
            errorText.textContent = "";
            try {
                var envelope: any = EspControlModel.parseCardTransferCode(textarea.value);
                var converted: any = clipboardEntriesFromCardTransfer(envelope, targetIsSubpage);
                var result: any = performClipboardPaste(converted.entries, pos, targetIsSubpage);
                if (!result.ok) {
                    errorText.textContent = result.error || "The cards could not be pasted.";
                    return;
                }
                closeCardTransferDialog();
                var message: any = result.count === 1 ? "Card pasted." : result.count + " cards pasted.";
                var warning: any = false;
                if (result.resized || converted.warnings.cardResized) {
                    message += " Some were resized to fit.";
                    warning = true;
                }
                if (converted.warnings.local) {
                    message += " Review local device references on this controller.";
                    warning = true;
                }
                if (converted.warnings.subpageResized) {
                    message += " A subpage layout was resized to fit.";
                    warning = true;
                }
                showBanner(message, warning ? "warning" : "success");
            }
            catch (error: any) {
                errorText.textContent = error.cardTransferMessage || error.message || "Invalid card code.";
            }
        });
        actions.appendChild(paste);
        dialog.appendChild(actions);
        textarea.addEventListener("input", function () { errorText.textContent = ""; });
        textarea.focus();
    }
    return {
        "buildClipboardEntry": staticGlobal(buildClipboardEntry),
        "copySlot": staticGlobal(copySlot),
        "copyButtons": staticGlobal(copyButtons),
        "clipboardEntriesFromCardTransfer": staticGlobal(clipboardEntriesFromCardTransfer),
        "cutSlot": staticGlobal(cutSlot),
        "cutButtons": staticGlobal(cutButtons),
        "showCopyCardCode": staticGlobal(showCopyCardCode),
        "showPasteCardCode": staticGlobal(showPasteCardCode),
        "pasteButton": staticGlobal(pasteButton),
        "pasteSubpageButton": staticGlobal(pasteSubpageButton),
    };
}
