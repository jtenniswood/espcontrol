import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installButtonSettingsSelectionModule(): GlobalDescriptors {
    // ── Button Settings Selection ─────────────────────────────────────
    function hideSettingsOverlay(this: any) {
        if (els.settingsOverlay)
            els.settingsOverlay.classList.remove("sp-visible");
    }
    function updatePreviewHint(this: any, c?: any) {
        if (!els.previewHint)
            return;
        c = c || ctx();
        els.previewHint.style.display = "";
        if (isConfigLocked()) {
            els.previewHint.textContent = "Editing is paused while the device reconnects";
        }
        else if (state.clockBarSelectedItem) {
            els.previewHint.textContent = clockBarItemLabel(state.clockBarSelectedItem) + " selected";
        }
        else if (c.selected.length > 1) {
            els.previewHint.textContent = c.selected.length + " buttons selected \u2022 right click to copy, cut, or delete";
        }
        else {
            els.previewHint.textContent = "tap to select \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
        }
    }
    function renderClockBarSelectionBar(this: any) {
        if (!els.selectionBar || !state.clockBarSelectedItem)
            return false;
        els.selectionBar.className = "sp-selection-bar sp-visible";
        var canEditClockBarItem: any = isClockBarTemperatureItem(state.clockBarSelectedItem) ||
            state.clockBarSelectedItem === "voice";
        var label: any = document.createElement("span");
        label.className = "sp-selection-label";
        label.textContent = clockBarItemLabel(state.clockBarSelectedItem) + " selected";
        els.selectionBar.appendChild(label);
        var actions: any = document.createElement("div");
        actions.className = "sp-selection-actions";
        var editBtn: any = createActionButton("sp-selection-btn sp-selection-btn-primary", "Edit", "pencil");
        editBtn.disabled = !canEditClockBarItem;
        editBtn.addEventListener("click", function (this: any, e?: any) {
            e.preventDefault();
            e.stopPropagation();
            if (editBtn.disabled)
                return;
            if (isClockBarTemperatureItem(state.clockBarSelectedItem)) {
                openClockBarTemperatureSettings();
            }
            else if (state.clockBarSelectedItem === "voice") {
                openVoiceServicesSettings();
            }
        });
        actions.appendChild(editBtn);
        var visible: any = clockBarItemActive(state.clockBarSelectedItem);
        var hideBtn: any = createActionButton("sp-selection-btn", visible ? "Hide" : "Show", visible ? "eye-off-outline" : "eye-outline");
        hideBtn.addEventListener("click", function (this: any, e?: any) {
            e.preventDefault();
            e.stopPropagation();
            setClockBarItemVisible(state.clockBarSelectedItem, !visible);
            renderSelectionBar(ctx());
        });
        actions.appendChild(hideBtn);
        els.selectionBar.appendChild(actions);
        return true;
    }
    function renderSelectionBar(this: any, c?: any) {
        if (!els.selectionBar)
            return;
        c = c || ctx();
        els.selectionBar.innerHTML = "";
        if (!isConfigLocked() && renderClockBarSelectionBar())
            return;
        if (isConfigLocked() || !c.selected.length) {
            els.selectionBar.className = "sp-selection-bar";
            return;
        }
        els.selectionBar.className = "sp-selection-bar sp-visible";
        var label: any = document.createElement("span");
        label.className = "sp-selection-label";
        if (c.selected.length === 1 && c.selected[0] === -2) {
            label.textContent = "Back button selected";
        }
        else {
            label.textContent = c.selected.length === 1 ? "1 card selected" : c.selected.length + " cards selected";
        }
        els.selectionBar.appendChild(label);
        var actions: any = document.createElement("div");
        actions.className = "sp-selection-actions";
        if (c.selected.length === 1) {
            var editBtn: any = createActionButton("sp-selection-btn sp-selection-btn-primary", "Edit", "pencil");
            editBtn.addEventListener("click", function (this: any, e?: any) {
                e.preventDefault();
                e.stopPropagation();
                openSelectedCardSettings();
            });
            actions.appendChild(editBtn);
        }
        var menuBtn: any = createActionButton("sp-selection-btn", "", "dots-horizontal", "Card actions");
        menuBtn.addEventListener("click", function (this: any, e?: any) {
            e.preventDefault();
            e.stopPropagation();
            showSelectionMenu(e);
        });
        actions.appendChild(menuBtn);
        els.selectionBar.appendChild(actions);
    }
    function closeSettings(this: any) {
        hideSettingsOverlay();
        _settingsDeferred = false;
        state.settingsDraft = null;
        ctx().setSelected([]);
        state.clockBarSelectedItem = "";
        updateClockBarItemUi();
        renderPreview();
    }
    function clearCardSelection(this: any) {
        var c: any = ctx();
        if (!c.selected.length && c.getLastClicked() < 0 && !state.clockBarSelectedItem)
            return;
        c.setSelected([]);
        c.setLastClicked(-1);
        state.clockBarSelectedItem = "";
        hideSettingsOverlay();
        updateClockBarItemUi();
        renderPreview();
        renderButtonSettings();
    }
    function isSelectionControlTarget(this: any, target?: any) {
        return !!((els.previewMain && els.previewMain.contains(target)) ||
            (els.topbar && els.topbar.contains(target)) ||
            (els.selectionBar && els.selectionBar.contains(target)) ||
            (els.settingsOverlay && els.settingsOverlay.contains(target)) ||
            (ctxMenu && ctxMenu.contains(target)) ||
            // Menus that render on document.body to escape the settings
            // modal's clipping still belong to it: a click inside one must
            // not read as clicking away from the card.
            (target.closest && target.closest(".sp-ctx-menu, .sp-cal-colors, .sp-entity-dropdown")));
    }
    function handleDocumentSelectionMouseDown(this: any, e?: any) {
        if (e.button !== 0)
            return;
        if (isSelectionControlTarget(e.target))
            return;
        clearCardSelection();
    }
    function openSelectedCardSettings(this: any) {
        if (isConfigLocked())
            return;
        if (state.clockBarSelectedItem) {
            if (isClockBarTemperatureItem(state.clockBarSelectedItem))
                openClockBarTemperatureSettings();
            if (state.clockBarSelectedItem === "voice")
                openVoiceServicesSettings();
            return;
        }
        var c: any = ctx();
        if (c.selected.length !== 1)
            return;
        renderButtonSettings(true);
    }
    function selectClockBarItem(this: any, item?: any) {
        if (isConfigLocked() || clockBarItems().indexOf(item) === -1)
            return;
        var c: any = ctx();
        c.setSelected([]);
        c.setLastClicked(-1);
        hideSettingsOverlay();
        state.clockBarSelectedItem = state.clockBarSelectedItem === item ? "" : item;
        updateClockBarItemUi();
        renderPreview();
        renderButtonSettings();
    }
    function openClockBarTemperatureSettings(this: any) {
        if (isConfigLocked())
            return;
        var container: any = els.buttonSettings;
        if (!container)
            return;
        state.clockBarSelectedItem = "temperature";
        container.innerHTML = "";
        if (els.settingsOverlay)
            els.settingsOverlay.classList.add("sp-visible");
        var title: any = document.createElement("div");
        title.className = "sp-section-title";
        title.textContent = "Temperature";
        container.appendChild(title);
        var panel: any = document.createElement("div");
        panel.className = "sp-panel";
        var entityField: any = document.createElement("div");
        entityField.className = "sp-field";
        entityField.appendChild(fieldLabel("Entity", "sp-clockbar-temperature-entity"));
        var entityInp: any = entityInput("sp-clockbar-temperature-entity", primaryClockBarTemperatureEntity(), "sensor.outdoor_temperature", ["sensor"]);
        entityField.appendChild(entityInp);
        panel.appendChild(entityField);
        var degreeToggle: any = toggleRow("Show Degree Symbol", "sp-clockbar-temperature-degree-symbol", state.temperatureDegreeSymbolOn);
        panel.appendChild(degreeToggle.row);
        var saveRow: any = document.createElement("div");
        saveRow.className = "sp-btn-row sp-btn-row--save sp-has-secondary";
        var visible: any = clockBarItemActive("temperature");
        var hideBtn: any = createActionButton("sp-action-btn sp-hide-btn", visible ? "Hide" : "Show", visible ? "eye-off-outline" : "eye-outline");
        hideBtn.addEventListener("click", function (this: any) {
            setClockBarItemVisible("temperature", !visible);
            closeSettings();
        });
        saveRow.appendChild(hideBtn);
        var saveBtn: any = createActionButton("sp-action-btn sp-save-btn", "Save");
        saveBtn.addEventListener("click", function (this: any) {
            saveClockBarTemperatureSettings(entityInp.value, degreeToggle.input.checked);
            closeSettings();
        });
        saveRow.appendChild(saveBtn);
        panel.appendChild(saveRow);
        container.appendChild(panel);
        entityInp.focus();
    }
    return {
        "hideSettingsOverlay": staticGlobal(hideSettingsOverlay),
        "updatePreviewHint": staticGlobal(updatePreviewHint),
        "renderClockBarSelectionBar": staticGlobal(renderClockBarSelectionBar),
        "renderSelectionBar": staticGlobal(renderSelectionBar),
        "closeSettings": staticGlobal(closeSettings),
        "clearCardSelection": staticGlobal(clearCardSelection),
        "isSelectionControlTarget": staticGlobal(isSelectionControlTarget),
        "handleDocumentSelectionMouseDown": staticGlobal(handleDocumentSelectionMouseDown),
        "openSelectedCardSettings": staticGlobal(openSelectedCardSettings),
        "selectClockBarItem": staticGlobal(selectClockBarItem),
        "openClockBarTemperatureSettings": staticGlobal(openClockBarTemperatureSettings),
    };
}
