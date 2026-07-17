import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installConfigImageOptionsModule(): GlobalDescriptors {
    // ── Image Card Options ─────────────────────────────────────────────
    function imageModalModeValues(this: any) {
        var spec: any = cardContractOptionSpec("image", IMAGE_MODAL_MODE_OPTION);
        return spec && spec.values ? spec.values.slice() : [];
    }
    function normalizeImageModalMode(this: any, value?: any) {
        value = String(value || "").trim();
        var fallback: any = cardContractOptionDefaultValue("image", IMAGE_MODAL_MODE_OPTION, "fill");
        return imageModalModeValues().indexOf(value) >= 0 ? value : fallback;
    }
    function imageSlotCapacity(this: any) {
        return IMAGE_SLOT_CAPACITY;
    }
    function imageSlotCapacityMessage(this: any) {
        if (IMAGE_SLOT_CAPACITY <= 0)
            return "Image cards are not available on this display.";
        var disabled: any = CFG.disabledCardTypes || [];
        var cameraAvailable: any = disabled.indexOf("image") < 0;
        var mediaCoverArtAvailable: any = disabled.indexOf("media_cover_art") < 0;
        if (cameraAvailable && !mediaCoverArtAvailable) {
            return "This display supports up to " + IMAGE_SLOT_CAPACITY +
                (IMAGE_SLOT_CAPACITY === 1 ? " Camera Card" : " Camera Cards") +
                " across the main page and subpages.";
        }
        if (!cameraAvailable && mediaCoverArtAvailable) {
            return "This display supports up to " + IMAGE_SLOT_CAPACITY +
                " Media Cover Art card" + (IMAGE_SLOT_CAPACITY === 1 ? "." : "s.");
        }
        return "Image and Media Cover Art cards use shared image slots. You can save up to " +
            IMAGE_SLOT_CAPACITY + " of these cards across the main page and subpages.";
    }
    function isImageCard(this: any, button?: any) {
        return !!button && (button.type === "image" ||
            (button.type === "media" && mediaEditorMode(button.sensor) === "cover_art"));
    }
    function activeGridSlots(this: any, grid?: any) {
        var slots: any = [];
        var seen: any = {};
        (grid || []).forEach(function (this: any, slot?: any) {
            if (slot <= 0 || seen[slot])
                return;
            seen[slot] = true;
            slots.push(slot);
        });
        return slots;
    }
    function imageCardCountInButtons(this: any, buttons?: any, grid?: any) {
        var count: any = 0;
        var slots: any = activeGridSlots(grid);
        if (!slots.length && buttons && buttons.length) {
            for (var fallbackSlot: any = 1; fallbackSlot <= buttons.length; fallbackSlot++) {
                slots.push(fallbackSlot);
            }
        }
        slots.forEach(function (this: any, slot?: any) {
            if (isImageCard(buttons && buttons[slot - 1]))
                count++;
        });
        return count;
    }
    function imageCardCountInSubpage(this: any, sp?: any) {
        return imageCardCountInButtons(sp && sp.buttons, sp && sp.grid);
    }
    function imageCardCountInClipboardEntry(this: any, entry?: any) {
        var count: any = isImageCard(entry) ? 1 : 0;
        if (entry && entry.subpageConfig) {
            count += imageCardCountInSubpage(parseSubpageConfig(entry.subpageConfig));
        }
        return count;
    }
    function imageCardCountInClipboardEntries(this: any, entries?: any) {
        var count: any = 0;
        (entries || []).forEach(function (this: any, entry?: any) {
            count += imageCardCountInClipboardEntry(entry);
        });
        return count;
    }
    function imageCardCountWithCandidate(this: any, candidate?: any) {
        var count: any = 0;
        var matchedCandidate: any = false;
        activeGridSlots(state.grid).forEach(function (this: any, slot?: any) {
            var button: any = state.buttons[slot - 1];
            if (candidate && !candidate.isSub && candidate.slot === slot) {
                button = candidate.button;
                matchedCandidate = true;
            }
            if (isImageCard(button))
                count++;
        });
        for (var homeSlot in state.subpages) {
            var sp: any = state.subpages[homeSlot];
            activeGridSlots(sp && sp.grid).forEach(function (this: any, slot?: any) {
                var button: any = sp && sp.buttons && sp.buttons[slot - 1];
                if (candidate && candidate.isSub &&
                    String(candidate.homeSlot) === String(homeSlot) &&
                    candidate.slot === slot) {
                    button = candidate.button;
                    matchedCandidate = true;
                }
                if (isImageCard(button))
                    count++;
            });
        }
        if (candidate && !matchedCandidate && isImageCard(candidate.button))
            count++;
        return count;
    }
    function canAddImageCards(this: any, extraCount?: any) {
        extraCount = parseInt(extraCount || 0, 10);
        if (!isFinite(extraCount) || extraCount <= 0)
            return true;
        return imageCardCountWithCandidate() + extraCount <= IMAGE_SLOT_CAPACITY;
    }
    function showImageCardLimitBanner(this: any) {
        showBanner(imageSlotCapacityMessage(), "error");
    }
    function imageModalMode(this: any, b?: any) {
        return normalizeImageModalMode(configOptionValue(b && b.options, IMAGE_MODAL_MODE_OPTION));
    }
    function imageLabelEnabled(this: any, b?: any) {
        return !!(b && configOptionEnabled(b.options, IMAGE_LABEL_OPTION));
    }
    function imageIconEnabled(this: any, b?: any) {
        return !!(b && configOptionEnabled(b.options, IMAGE_ICON_OPTION));
    }
    function normalizeImageOptions(this: any, options?: any) {
        var out: any = "";
        if (configOptionEnabled(options, IMAGE_LABEL_OPTION)) {
            out = setConfigOption(out, IMAGE_LABEL_OPTION, true);
        }
        if (configOptionEnabled(options, IMAGE_ICON_OPTION)) {
            out = setConfigOption(out, IMAGE_ICON_OPTION, true);
        }
        var modalMode: any = normalizeImageModalMode(configOptionValue(options, IMAGE_MODAL_MODE_OPTION));
        if (modalMode !== cardContractOptionDefaultValue("image", IMAGE_MODAL_MODE_OPTION, "fill")) {
            out = setConfigOptionValue(out, IMAGE_MODAL_MODE_OPTION, modalMode);
        }
        return out;
    }
    function setImageLabelEnabled(this: any, b?: any, enabled?: any) {
        if (!b)
            return "";
        b.options = setConfigOption(b.options, IMAGE_LABEL_OPTION, !!enabled);
        if (!enabled)
            b.label = "";
        b.options = normalizeImageOptions(b.options);
        return b.options;
    }
    function setImageIconEnabled(this: any, b?: any, enabled?: any) {
        if (!b)
            return "";
        b.options = setConfigOption(b.options, IMAGE_ICON_OPTION, !!enabled);
        b.options = normalizeImageOptions(b.options);
        return b.options;
    }
    function setImageModalMode(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        var mode: any = normalizeImageModalMode(value);
        b.options = setConfigOptionValue(b.options, IMAGE_MODAL_MODE_OPTION, mode === "fill" ? "" : mode);
        b.options = normalizeImageOptions(b.options);
        return b.options;
    }
    return {
        "imageModalModeValues": staticGlobal(imageModalModeValues),
        "normalizeImageModalMode": staticGlobal(normalizeImageModalMode),
        "imageSlotCapacity": staticGlobal(imageSlotCapacity),
        "imageSlotCapacityMessage": staticGlobal(imageSlotCapacityMessage),
        "isImageCard": staticGlobal(isImageCard),
        "activeGridSlots": staticGlobal(activeGridSlots),
        "imageCardCountInButtons": staticGlobal(imageCardCountInButtons),
        "imageCardCountInSubpage": staticGlobal(imageCardCountInSubpage),
        "imageCardCountInClipboardEntry": staticGlobal(imageCardCountInClipboardEntry),
        "imageCardCountInClipboardEntries": staticGlobal(imageCardCountInClipboardEntries),
        "imageCardCountWithCandidate": staticGlobal(imageCardCountWithCandidate),
        "canAddImageCards": staticGlobal(canAddImageCards),
        "showImageCardLimitBanner": staticGlobal(showImageCardLimitBanner),
        "imageModalMode": staticGlobal(imageModalMode),
        "imageLabelEnabled": staticGlobal(imageLabelEnabled),
        "imageIconEnabled": staticGlobal(imageIconEnabled),
        "normalizeImageOptions": staticGlobal(normalizeImageOptions),
        "setImageLabelEnabled": staticGlobal(setImageLabelEnabled),
        "setImageIconEnabled": staticGlobal(setImageIconEnabled),
        "setImageModalMode": staticGlobal(setImageModalMode),
    };
}
