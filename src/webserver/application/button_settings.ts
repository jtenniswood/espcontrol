import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installButtonSettingsModule(): GlobalDescriptors {
    // ── Button settings panel (unified) ────────────────────────────────────
    function openCardSettings(this: any, slot?: any) {
        if (isConfigLocked())
            return;
        var c: any = ctx();
        if ((slot > 0 || (slot === -2 && c.isSub)) && c.selected.indexOf(slot) === -1) {
            c.setSelected([slot]);
            c.setLastClicked(slot);
            renderPreview();
        }
        renderButtonSettings(true);
    }
    function renderBackButtonSettings(this: any, container?: any, c?: any) {
        if (!c.isSub || c.selected[0] !== -2)
            return false;
        if (els.settingsOverlay)
            els.settingsOverlay.classList.add("sp-visible");
        var sp: any = getSubpage(state.editingSubpage);
        var title: any = document.createElement("div");
        title.className = "sp-section-title";
        title.textContent = "Back Button";
        container.appendChild(title);
        var panel: any = document.createElement("div");
        panel.className = "sp-panel";
        var lf: any = document.createElement("div");
        lf.className = "sp-field";
        lf.appendChild(fieldLabel("Label", "sp-sp-inp-back-label"));
        var labelInp: any = textInput("sp-sp-inp-back-label", sp.backLabel || "Back", "Back");
        lf.appendChild(labelInp);
        panel.appendChild(lf);
        function saveBackLabel(this: any) {
            sp.backLabel = labelInp.value || "Back";
            saveSubpageConfig(state.editingSubpage);
            renderPreview();
        }
        labelInp.addEventListener("input", saveBackLabel);
        labelInp.addEventListener("change", saveBackLabel);
        labelInp.addEventListener("blur", saveBackLabel);
        labelInp.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "Enter") {
                saveBackLabel();
                this.blur();
            }
        });
        var doneRow: any = document.createElement("div");
        doneRow.className = "sp-btn-row sp-btn-row--save";
        var doneBtn: any = createActionButton("sp-action-btn sp-save-btn", "Done");
        doneBtn.addEventListener("click", closeSettings);
        doneRow.appendChild(doneBtn);
        panel.appendChild(doneRow);
        container.appendChild(panel);
        return true;
    }
    function renderButtonSettings(this: any, forceOpen?: any) {
        var container: any = els.buttonSettings;
        container.innerHTML = "";
        var settingsModal: any = els.settingsOverlay ? els.settingsOverlay.querySelector(".sp-settings-modal") : null;
        if (settingsModal)
            settingsModal.classList.remove("sp-card-type-picker-open");
        var c: any = ctx();
        if (isConfigLocked()) {
            hideSettingsOverlay();
            return;
        }
        if (c.selected.length === 0) {
            hideSettingsOverlay();
            return;
        }
        if (c.selected.length > 1) {
            hideSettingsOverlay();
            return;
        }
        if (!forceOpen && !isSettingsOpen()) {
            hideSettingsOverlay();
            return;
        }
        if (els.settingsOverlay)
            els.settingsOverlay.classList.add("sp-visible");
        if (renderBackButtonSettings(container, c))
            return;
        var slot: any = c.selected[0];
        var bIdx: any = slot - 1;
        var pendingNewDraft: any = !!(state.settingsDraft &&
            state.settingsDraft.isNew &&
            state.settingsDraft.slot === slot &&
            state.settingsDraft.isSub === c.isSub &&
            (!c.isSub || state.settingsDraft.homeSlot === state.editingSubpage));
        if (bIdx < 0 || (!pendingNewDraft && bIdx >= c.buttons.length))
            return;
        var liveButton: any = pendingNewDraft ? null : c.buttons[bIdx];
        var draftKey: any = pendingNewDraft
            ? state.settingsDraft!.key
            : (c.isSub ? "sub:" + state.editingSubpage : "main") + ":" + slot;
        function cloneButtonConfig(this: any, src?: any) {
            var cloned: any = EspControlModel.cloneCardConfig(src);
            normalizeButtonConfig(cloned);
            return cloned;
        }
        function copyButtonConfig(this: any, target?: any, src?: any) {
            EspControlModel.copyCardConfig(target, src);
            normalizeButtonConfig(target);
        }
        if (!pendingNewDraft && (!state.settingsDraft || state.settingsDraft.key !== draftKey)) {
            state.settingsDraft = {
                key: draftKey,
                slot: slot,
                homeSlot: state.editingSubpage,
                isSub: c.isSub,
                dirty: false,
                button: cloneButtonConfig(liveButton),
            };
        }
        var b: any = state.settingsDraft!.button;
        var isNewDraft: any = !!state.settingsDraft!.isNew;
        var title: any = document.createElement("div");
        title.className = "sp-section-title";
        title.textContent = "Settings";
        container.appendChild(title);
        var panel: any = document.createElement("div");
        panel.className = "sp-panel";
        var idPrefix: any = c.isSub ? "sp-sp-inp-" : "sp-inp-";
        var requiredFields: any = [];
        function markDraftDirty(this: any) {
            if (state.settingsDraft && state.settingsDraft.key === draftKey) {
                state.settingsDraft.dirty = true;
            }
        }
        function saveField(this: any, field?: any, val?: any) {
            markDraftDirty();
        }
        function fieldContainer(this: any, input?: any) {
            return input && input.closest ? input.closest(".sp-field") : null;
        }
        function clearFieldError(this: any, input?: any) {
            if (!input)
                return;
            input.classList.remove("sp-input-error");
            input.removeAttribute("aria-invalid");
            input.removeAttribute("aria-describedby");
            var field: any = fieldContainer(input);
            if (field)
                field.classList.remove("sp-field-invalid");
            var existing: any = field ? field.querySelector(".sp-field-error") : null;
            if (existing && existing.parentNode)
                existing.parentNode.removeChild(existing);
        }
        function showFieldError(this: any, input?: any, message?: any) {
            if (!input)
                return;
            var field: any = fieldContainer(input);
            input.classList.add("sp-input-error");
            input.setAttribute("aria-invalid", "true");
            if (field)
                field.classList.add("sp-field-invalid");
            var existing: any = field ? field.querySelector(".sp-field-error") : null;
            if (!existing && field) {
                existing = document.createElement("div");
                existing.className = "sp-field-error";
                existing.id = (input.id || "sp-field") + "-error";
                field.appendChild(existing);
            }
            if (existing) {
                existing.textContent = message || "Add an entity before saving.";
                input.setAttribute("aria-describedby", existing.id);
            }
        }
        function requireField(this: any, input?: any, message?: any, isActive?: any) {
            if (!input)
                return;
            requiredFields.push({
                input: input,
                message: message || "Add an entity before saving.",
                isActive: isActive || function (this: any) { return true; },
            });
            function maybeClearError(this: any) {
                if (!isActive || isActive()) {
                    if (String(input.value || "").trim())
                        clearFieldError(input);
                }
                else {
                    clearFieldError(input);
                }
            }
            input.addEventListener("input", maybeClearError);
            input.addEventListener("change", maybeClearError);
        }
        function validateSettingsDraft(this: any) {
            var firstInvalid: any = null;
            for (var i: any = 0; i < requiredFields.length; i++) {
                var rule: any = requiredFields[i];
                if (rule.isActive && !rule.isActive()) {
                    clearFieldError(rule.input);
                    continue;
                }
                if (String(rule.input.value || "").trim()) {
                    clearFieldError(rule.input);
                    continue;
                }
                if (!firstInvalid)
                    firstInvalid = rule.input;
                showFieldError(rule.input, rule.message);
            }
            if (!firstInvalid)
                return true;
            openDisclosureForField(firstInvalid);
            firstInvalid.focus();
            return false;
        }
        function openDisclosureForField(this: any, input?: any) {
            if (!input || !input.closest)
                return;
            var disclosure: any = input.closest(".sp-disclosure");
            if (!disclosure)
                return;
            disclosure.classList.add("sp-open");
            var button: any = disclosure.querySelector(".sp-disclosure-button");
            if (button)
                button.setAttribute("aria-expanded", "true");
        }
        function validateConfigSize(this: any) {
            if (c.isSub)
                return true;
            if (serializeButtonConfig(b).length <= 255)
                return true;
            showBanner("Card settings are too large to save. Shorten confirmation text, labels, or entity IDs.", "error");
            return false;
        }
        function validateImageCardLimit(this: any) {
            var count: any = imageCardCountWithCandidate({
                isSub: c.isSub,
                homeSlot: state.editingSubpage,
                slot: slot,
                button: b,
            });
            if (count <= imageSlotCapacity())
                return true;
            showImageCardLimitBanner();
            return false;
        }
        function validateCardBackgroundImageLimit(this: any) {
            var count: any = cardBackgroundImageCountWithCandidate({
                isSub: c.isSub,
                homeSlot: state.editingSubpage,
                slot: slot,
                button: b,
            });
            if (count <= cardBackgroundImageLimit())
                return true;
            showCardBackgroundImageLimitBanner();
            return false;
        }
        function applyCardSizeConstraint(this: any, savedButton?: any) {
            var currentSize: any = c.sizes[slot] || CARD_SIZE_SINGLE;
            var nextSize: any = normalizeCardSizeForConfig(savedButton, currentSize);
            if (nextSize === currentSize)
                return false;
            if (nextSize === CARD_SIZE_SINGLE)
                delete c.sizes[slot];
            else
                c.sizes[slot] = nextSize;
            clearSpans(c.grid, c.maxSlots);
            applySpans(c.grid, c.sizes, c.maxSlots);
            return true;
        }
        function applySettingsDraft(this: any) {
            if (!state.settingsDraft || state.settingsDraft.key !== draftKey)
                return false;
            var draft: any = state.settingsDraft;
            var savedButton: any = liveButton;
            var sizeChanged: any = false;
            if (draft.isNew) {
                var pos: any = draft.pos;
                if (pos < 0 || pos >= c.maxSlots || c.grid[pos] !== 0) {
                    showBanner("That grid space is no longer available. Close this window and try again.", "error");
                    return false;
                }
                while (c.buttons.length < slot) {
                    c.buttons.push(emptyButtonConfig());
                }
                savedButton = c.buttons[slot - 1];
                copyButtonConfig(savedButton, draft.button);
                c.grid[pos] = slot;
                sizeChanged = applyCardSizeConstraint(savedButton);
                if (c.isSub) {
                    saveSubpageConfig(state.editingSubpage);
                }
                else {
                    postText(entityName("button_order"), serializeGrid(state.grid));
                    saveButtonConfig(slot);
                }
            }
            else {
                copyButtonConfig(liveButton, draft.button);
                sizeChanged = applyCardSizeConstraint(liveButton);
            }
            state.settingsDraft = null;
            if (!draft.isNew && c.isSub) {
                saveSubpageConfig(state.editingSubpage);
            }
            else if (!draft.isNew) {
                if (sizeChanged)
                    postText(entityName("button_order"), serializeGrid(state.grid));
                saveButtonConfig(slot);
            }
            var savedTypeDef: any = BUTTON_TYPES[savedButton.type || ""];
            if (savedTypeDef && savedTypeDef.afterSave) {
                savedTypeDef.afterSave(savedButton, slot, { isSub: c.isSub });
            }
            renderPreview();
            return true;
        }
        function bindField(this: any, input?: any, field?: any, rerender?: any) {
            function syncValue(this: any) {
                if (b[field] === input.value)
                    return;
                b[field] = input.value;
                markDraftDirty();
                if (rerender)
                    renderPreview();
            }
            input.addEventListener("input", function (this: any) {
                syncValue();
            });
            input.addEventListener("change", syncValue);
            input.addEventListener("blur", syncValue);
            input.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key === "Enter") {
                    syncValue();
                    this.blur();
                }
            });
        }
        function makeIconPicker(this: any, pickerId?: any, inputId?: any, currentVal?: any, onSelect?: any, labelText?: any) {
            var icf: any = document.createElement("div");
            icf.className = "sp-field";
            if (labelText === "On Icon")
                icf.classList.add("sp-icon-on-field");
            icf.appendChild(fieldLabel(labelText || "Icon", inputId));
            var picker: any = document.createElement("div");
            picker.className = "sp-icon-picker";
            if (pickerId)
                picker.id = pickerId;
            picker.appendChild(mdiIcon(currentVal, "sp-icon-picker-preview mdi"));
            var input: any = document.createElement("input");
            input.className = "sp-icon-picker-input";
            if (inputId)
                input.id = inputId;
            input.type = "text";
            input.placeholder = "Search icons\u2026";
            input.value = currentVal || "";
            input.autocomplete = "off";
            picker.appendChild(input);
            var dropdown: any = document.createElement("div");
            dropdown.className = "sp-icon-dropdown";
            picker.appendChild(dropdown);
            icf.appendChild(picker);
            initIconPicker(picker, currentVal, onSelect);
            return icf;
        }
        function entityField(this: any, labelText?: any, inputId?: any, value?: any, placeholder?: any, domains?: any, bindName?: any, rerender?: any, requiredMessage?: any) {
            var input: any = entityInput(inputId, value, placeholder, domains);
            var field: any = fieldWithControl(labelText, inputId, input);
            if (bindName)
                bindField(input, bindName, rerender);
            if (requiredMessage)
                requireField(input, requiredMessage);
            return {
                field: field,
                input: input,
            };
        }
        function textField(this: any, labelText?: any, inputId?: any, value?: any, placeholder?: any, bindName?: any, rerender?: any) {
            var input: any = textInput(inputId, value, placeholder);
            var field: any = fieldWithControl(labelText, inputId, input);
            if (bindName)
                bindField(input, bindName, rerender);
            return {
                field: field,
                input: input,
            };
        }
        function toggleSection(this: any, labelText?: any, inputId?: any, checked?: any) {
            return {
                toggle: toggleRow(labelText, inputId, checked),
                section: condField(),
            };
        }
        function precisionField(this: any, inputId?: any, value?: any, onChange?: any) {
            return selectField("Unit Precision", inputId, [
                ["0", "10"],
                ["1", "10.2"],
                ["2", "10.21"],
            ], value || "0", onChange);
        }
        function clearAutomaticTypeDefaults(this: any) {
            var draft: any = state.settingsDraft;
            var automatic: any = draft && draft.key === draftKey && draft.autoSelectedButton;
            if (!isNewDraft || !automatic)
                return;
            var empty: any = emptyButtonConfig();
            ["entity", "label", "icon", "icon_on", "sensor", "unit", "precision", "options"].forEach(function (this: any, field?: any) {
                if (b[field] === automatic[field])
                    b[field] = empty[field];
            });
            draft.autoSelectedButton = null;
        }
        function selectCardType(this: any, newType?: any) {
            var pickerType: any = newType;
            newType = defaultButtonTypeForPicker(newType);
            var wasNewDraftWithoutType: any = isNewDraft && state.settingsDraft &&
                state.settingsDraft.key === draftKey && !state.settingsDraft.typeSelected;
            var keepMediaEntity: any = (pickerType === "media_control" || pickerType === "media_cover_art") && b.type === "media";
            clearAutomaticTypeDefaults();
            if (isNewDraft && b.type === "action" && newType !== "action") {
                b.sensor = "";
                b.unit = "";
                b.precision = "";
                b.options = "";
            }
            b.type = newType;
            if (state.settingsDraft && state.settingsDraft.key === draftKey) {
                state.settingsDraft.typeSelected = true;
            }
            var td: any = BUTTON_TYPES[newType];
            if (td && td.onSelect && !keepMediaEntity)
                td.onSelect(b);
            if (pickerType === "media_control") {
                b.sensor = "control_modal";
                b.label = "All Controls";
                b.icon = "Auto";
                b.icon_on = "Auto";
                b.unit = "";
                b.precision = "";
                b.options = "";
            }
            if (pickerType === "media_cover_art") {
                b.sensor = "cover_art";
                b.label = "Cover Art";
                b.icon = "Auto";
                b.icon_on = "Auto";
                b.unit = "";
                b.precision = "";
                b.options = normalizeMediaOptions(b.options, b.sensor);
            }
            if (wasNewDraftWithoutType && state.settingsDraft && state.settingsDraft.key === draftKey) {
                state.settingsDraft.autoSelectedButton = cloneButtonConfig(b);
            }
            saveField("type", b.type);
            renderButtonSettings();
        }
        function renderCardTypeGrid(this: any, options?: any) {
            var field: any = document.createElement("div");
            field.className = "sp-field sp-card-type-picker-field";
            field.appendChild(fieldLabel("Card", "sp-card-type-picker"));
            var grid: any = document.createElement("div");
            grid.className = "sp-card-type-grid";
            grid.id = "sp-card-type-picker";
            grid.setAttribute("role", "list");
            (options || []).forEach(function (this: any, o?: any) {
                var item: any = document.createElement("button");
                item.type = "button";
                item.className = "sp-card-type-option";
                item.disabled = !!o.disabled;
                item.setAttribute("data-card-type", o.key);
                item.setAttribute("aria-label", o.label + " card type");
                item.appendChild(mdiIcon(o.icon || "card-outline", "sp-card-type-icon mdi"));
                var copy: any = document.createElement("span");
                copy.className = "sp-card-type-copy";
                copy.appendChild(textSpan(o.label, "sp-card-type-title"));
                copy.appendChild(textSpan(o.description || "", "sp-card-type-description"));
                item.appendChild(copy);
                item.addEventListener("click", function (this: any) {
                    if (item.disabled)
                        return;
                    selectCardType(o.key);
                });
                grid.appendChild(item);
            });
            field.appendChild(grid);
            return field;
        }
        function renderActiveDisplaySettings(this: any, panel?: any, button?: any, idPrefix?: any) {
            var hasIconOn: any = button.icon_on && button.icon_on !== "Auto";
            var hasSensor: any = !!button.sensor;
            var activeEnabled: any = hasIconOn || hasSensor || !!button._whenOnActive;
            var activeMode: any = button._whenOnMode || (hasSensor ? "sensor" : "icon");
            var activeToggle: any = toggleRow("Active Display", idPrefix + "whenon-toggle", activeEnabled);
            panel.appendChild(activeToggle.row);
            var activeFields: any = condField();
            if (activeEnabled)
                activeFields.classList.add("sp-visible");
            var modeControl: any = segmentControl([
                ["icon", "On Icon"],
                ["sensor", "Numeric"],
            ], activeMode, function (this: any, mode?: any) {
                setActiveDisplayMode(mode);
            });
            activeFields.appendChild(modeControl.segment);
            var iconSection: any = condField();
            if (activeMode === "icon")
                iconSection.classList.add("sp-visible");
            var iconOnPicker: any = makeIconPicker(idPrefix + "icon-on-picker", idPrefix + "icon-on", hasIconOn ? button.icon_on : "Auto", function (this: any, opt?: any) {
                button.icon_on = opt;
                saveField("icon_on", opt);
            }, "On Icon");
            iconSection.appendChild(iconOnPicker);
            activeFields.appendChild(iconSection);
            var sensorSection: any = condField();
            if (activeMode === "sensor")
                sensorSection.classList.add("sp-visible");
            var sensorField: any = entityField("Sensor Entity", idPrefix + "sensor", button.sensor, "e.g. sensor.printer_percent_complete", ["sensor", "binary_sensor", "text_sensor"], "sensor", true);
            sensorSection.appendChild(sensorField.field);
            var unitField: any = textField("Unit", idPrefix + "unit", button.unit, "e.g. %", "unit", false);
            sensorSection.appendChild(unitField.field);
            var precision: any = precisionField(idPrefix + "precision", button.precision || "0", function (this: any) {
                button.precision = this.value === "0" ? "" : this.value;
                saveField("precision", button.precision);
            });
            sensorSection.appendChild(precision.field);
            activeFields.appendChild(sensorSection);
            panel.appendChild(activeFields);
            function syncIconPicker(this: any, value?: any) {
                if (value === "Auto") {
                    var autoPreview: any = iconOnPicker.querySelector(".sp-icon-picker-preview");
                    if (autoPreview)
                        autoPreview.className = "sp-icon-picker-preview mdi mdi-cog";
                    var autoInput: any = iconOnPicker.querySelector(".sp-icon-picker-input");
                    if (autoInput)
                        autoInput.value = "Auto";
                    return;
                }
                var picker: any = iconOnPicker.querySelector(".sp-icon-picker");
                if (picker && picker._setIcon) {
                    picker._setIcon(value);
                    return;
                }
                var preview: any = iconOnPicker.querySelector(".sp-icon-picker-preview");
                if (preview)
                    preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(value);
                var input: any = iconOnPicker.querySelector(".sp-icon-picker-input");
                if (input)
                    input.value = value;
            }
            function resetSensorFields(this: any) {
                sensorField.input.value = "";
                unitField.input.value = "";
                precision.select.value = "0";
                button.sensor = "";
                button.unit = "";
                button.precision = "";
                saveField("sensor", "");
                saveField("unit", "");
                saveField("precision", "");
            }
            function setActiveDisplayMode(this: any, mode?: any) {
                activeMode = mode;
                button._whenOnActive = true;
                button._whenOnMode = mode;
                modeControl.buttons.icon.classList.toggle("active", mode === "icon");
                modeControl.buttons.sensor.classList.toggle("active", mode === "sensor");
                iconSection.classList.toggle("sp-visible", mode === "icon");
                sensorSection.classList.toggle("sp-visible", mode === "sensor");
                if (mode === "icon") {
                    resetSensorFields();
                }
                else {
                    button.icon_on = "Auto";
                    saveField("icon_on", "Auto");
                    syncIconPicker("Auto");
                }
            }
            activeToggle.input.addEventListener("change", function (this: any) {
                if (this.checked) {
                    button._whenOnActive = true;
                    activeFields.classList.add("sp-visible");
                }
                else {
                    button._whenOnActive = false;
                    button._whenOnMode = null;
                    activeFields.classList.remove("sp-visible");
                    resetSensorFields();
                    button.icon_on = "Auto";
                    saveField("icon_on", "Auto");
                    syncIconPicker("Auto");
                }
            });
        }
        var isNewDraftWithoutType: any = isNewDraft && !state.settingsDraft?.typeSelected;
        var rawTypeDef: any = isNewDraftWithoutType ? null : (BUTTON_TYPES[b.type || ""] || BUTTON_TYPES[""]);
        var typeDef: any = rawTypeDef;
        {
            var selectedTypeKey: any = isNewDraftWithoutType
                ? null
                : buttonTypeRegistryValue(rawTypeDef, "pickerKey", "") || (b.type || "");
            if (!isNewDraftWithoutType && b.type === "media" && mediaEditorMode(b.sensor) === "cover_art")
                selectedTypeKey = "media_cover_art";
            var typeOpts: any = buttonTypePickerOptionList(c.isSub, selectedTypeKey);
            if (isNewDraftWithoutType) {
                if (settingsModal)
                    settingsModal.classList.add("sp-card-type-picker-open");
                panel.appendChild(renderCardTypeGrid(typeOpts));
                container.appendChild(panel);
                return;
            }
            var tf: any = document.createElement("div");
            tf.className = "sp-field";
            tf.appendChild(fieldLabel("Card", "sp-inp-type"));
            var typeSelect: any = document.createElement("select");
            typeSelect.className = "sp-select";
            typeSelect.id = "sp-inp-type";
            typeOpts.forEach(function (this: any, o?: any) {
                var opt: any = document.createElement("option");
                opt.value = o.key;
                opt.textContent = o.label;
                opt.disabled = !!o.disabled;
                if (selectedTypeKey === o.key)
                    opt.selected = true;
                typeSelect.appendChild(opt);
            });
            typeSelect.addEventListener("change", function (this: any) {
                selectCardType(this.value);
            });
            tf.appendChild(typeSelect);
            panel.appendChild(tf);
        }
        var typeHelpers: any = {
            makeIconPicker: makeIconPicker,
            iconPickerField: makeIconPicker,
            fieldWithControl: fieldWithControl,
            selectField: selectField,
            entityField: entityField,
            textField: textField,
            segmentControl: segmentControl,
            toggleSection: toggleSection,
            disclosureSection: disclosureSection,
            precisionField: precisionField,
            fieldLabel: fieldLabel,
            textInput: textInput,
            entityInput: entityInput,
            bindField: bindField,
            saveField: saveField,
            applyCardMetadataFields: applyCardMetadataFields,
            renderCardModeSelector: renderCardModeSelector,
            renderCardLargeNumbersToggle: renderCardLargeNumbersToggle,
            syncCardLargeNumbersToggle: syncCardLargeNumbersToggle,
            renderCardEntityField: renderCardEntityField,
            renderCardTextField: renderCardTextField,
            renderCardNumberField: renderCardNumberField,
            renderCardIconPicker: renderCardIconPicker,
            renderCardIconPair: renderCardIconPair,
            renderCardOptionToggle: renderCardOptionToggle,
            renderCardActiveColorToggle: renderCardActiveColorToggle,
            renderBasicCardFields: renderBasicCardFields,
            renderCardSegmentControl: renderCardSegmentControl,
            renderCardBackgroundControl: renderCardBackgroundControl,
            requireField: requireField,
            clearFieldError: clearFieldError,
            toggleRow: toggleRow,
            cardSize: c.sizes[slot] || 1,
            idPrefix: idPrefix,
            isSub: c.isSub,
        };
        if (typeDef && typeDef.renderSettingsBeforeLabel &&
            (!c.isSub || buttonTypeRegistryValue(typeDef, "allowInSubpage", false))) {
            typeDef.renderSettingsBeforeLabel(panel, b, slot, typeHelpers);
        }
        if (!typeDef || !typeDef.hideLabel) {
            var lf: any = document.createElement("div");
            lf.className = "sp-field";
            lf.appendChild(fieldLabel("Label", idPrefix + "label"));
            var labelPlaceholder: any = (typeDef && typeDef.labelPlaceholder) || "e.g. Kitchen";
            var labelInp: any = textInput(idPrefix + "label", b.label, labelPlaceholder);
            lf.appendChild(labelInp);
            panel.appendChild(lf);
            bindField(labelInp, "label", true);
        }
        if (typeDef && typeDef.renderSettings &&
            (!c.isSub || buttonTypeRegistryValue(typeDef, "allowInSubpage", false))) {
            typeDef.renderSettings(panel, b, slot, typeHelpers);
        }
        else {
            // Toggle fallback: entity, icons, sensor data
            var ef: any = document.createElement("div");
            ef.className = "sp-field";
            ef.appendChild(fieldLabel("Entity", idPrefix + "entity"));
            var entityInp: any = entityInput(idPrefix + "entity", b.entity, "e.g. light.kitchen", [
                "light", "switch", "input_boolean", "fan"
            ]);
            ef.appendChild(entityInp);
            panel.appendChild(ef);
            bindField(entityInp, "entity", true);
            requireField(entityInp, "Add an entity before saving.");
            panel.appendChild(makeIconPicker(idPrefix + "icon-picker", idPrefix + "icon", b.icon || "Auto", function (this: any, opt?: any) {
                b.icon = opt;
                saveField("icon", opt);
            }, "Off Icon"));
            renderActiveDisplaySettings(panel, b, idPrefix);
            var patternField: any = selectField("On State Pattern", idPrefix + "on-pattern", [
                ["", "Solid"],
                ["stripes", "Stripes"],
            ], cardOnPattern(b), function (this: any) {
                setCardOnPattern(b, this.value);
                saveField("options", b.options);
                renderPreview();
            });
            panel.appendChild(patternField.field);
        }
        renderCardBackgroundControl(panel, b, typeHelpers);
        var saveRow: any = document.createElement("div");
        saveRow.className = "sp-btn-row sp-btn-row--save";
        if (!isNewDraft) {
            var delBtn: any = createActionButton("sp-action-btn sp-delete-btn", "", "trash-can-outline");
            delBtn.addEventListener("click", function (this: any) {
                state.settingsDraft = null;
                deleteSlot(slot);
            });
            saveRow.appendChild(delBtn);
            saveRow.classList.add("sp-has-delete");
        }
        var rightGroup: any = document.createElement("div");
        rightGroup.className = "sp-btn-group-right";
        var editSubBtn: any = panel.querySelector(".sp-edit-subpage-btn");
        if (editSubBtn && isNewDraft && editSubBtn.parentNode) {
            editSubBtn.parentNode.removeChild(editSubBtn);
        }
        else if (editSubBtn) {
            rightGroup.appendChild(editSubBtn);
        }
        var saveBtn: any = createActionButton("sp-action-btn sp-save-btn", "Save");
        saveBtn.addEventListener("click", function (this: any) {
            if (!validateSettingsDraft())
                return;
            if (!validateImageCardLimit())
                return;
            if (!validateCardBackgroundImageLimit())
                return;
            if (!validateConfigSize())
                return;
            if (!applySettingsDraft())
                return;
            closeSettings();
        });
        rightGroup.appendChild(saveBtn);
        saveRow.appendChild(rightGroup);
        panel.appendChild(saveRow);
        container.appendChild(panel);
    }
    return {
        "openCardSettings": staticGlobal(openCardSettings),
        "renderBackButtonSettings": staticGlobal(renderBackButtonSettings),
        "renderButtonSettings": staticGlobal(renderButtonSettings),
    };
}
