// ── Button settings panel (unified) ────────────────────────────────────
// @web-module-requires: state, clock_bar_state, entity_state, grid, config_codec, config_post_api, controls, controls_fields, preview_render


function openCardSettings(slot) {
  if (isConfigLocked()) return;
  var c = ctx();
  if ((slot > 0 || (slot === -2 && c.isSub)) && c.selected.indexOf(slot) === -1) {
    c.setSelected([slot]);
    c.setLastClicked(slot);
    renderPreview();
  }
  renderButtonSettings(true);
}

function renderBackButtonSettings(container, c) {
  if (!c.isSub || c.selected[0] !== -2) return false;
  if (els.settingsOverlay) els.settingsOverlay.classList.add("sp-visible");
  var sp = getSubpage(state.editingSubpage);

  var title = document.createElement("div");
  title.className = "sp-section-title";
  title.textContent = "Back Button";
  container.appendChild(title);

  var panel = document.createElement("div");
  panel.className = "sp-panel";
  var lf = document.createElement("div");
  lf.className = "sp-field";
  lf.appendChild(fieldLabel("Label", "sp-sp-inp-back-label"));
  var labelInp = textInput("sp-sp-inp-back-label", sp.backLabel || "Back", "Back");
  lf.appendChild(labelInp);
  panel.appendChild(lf);

  function saveBackLabel() {
    sp.backLabel = labelInp.value || "Back";
    saveSubpageConfig(state.editingSubpage);
    renderPreview();
  }
  labelInp.addEventListener("input", saveBackLabel);
  labelInp.addEventListener("change", saveBackLabel);
  labelInp.addEventListener("blur", saveBackLabel);
  labelInp.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      saveBackLabel();
      this.blur();
    }
  });

  var doneRow = document.createElement("div");
  doneRow.className = "sp-btn-row sp-btn-row--save";
  var doneBtn = createActionButton("sp-action-btn sp-save-btn", "Done");
  doneBtn.addEventListener("click", closeSettings);
  doneRow.appendChild(doneBtn);
  panel.appendChild(doneRow);

  container.appendChild(panel);
  return true;
}

function renderButtonSettings(forceOpen) {
  var container = els.buttonSettings;
  container.innerHTML = "";
  var settingsModal = els.settingsOverlay ? els.settingsOverlay.querySelector(".sp-settings-modal") : null;
  if (settingsModal) settingsModal.classList.remove("sp-card-type-picker-open");
  var c = ctx();

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

  if (els.settingsOverlay) els.settingsOverlay.classList.add("sp-visible");

  if (renderBackButtonSettings(container, c)) return;

  var slot = c.selected[0];
  var bIdx = slot - 1;
  var pendingNewDraft = !!(
    state.settingsDraft &&
    state.settingsDraft.isNew &&
    state.settingsDraft.slot === slot &&
    state.settingsDraft.isSub === c.isSub &&
    (!c.isSub || state.settingsDraft.homeSlot === state.editingSubpage)
  );
  if (bIdx < 0 || (!pendingNewDraft && bIdx >= c.buttons.length)) return;
  var liveButton = pendingNewDraft ? null : c.buttons[bIdx];
  var draftKey = pendingNewDraft
    ? state.settingsDraft.key
    : (c.isSub ? "sub:" + state.editingSubpage : "main") + ":" + slot;

  function cloneButtonConfig(src) {
    return EspControlModel.cloneCardConfig(src);
  }

  function copyButtonConfig(target, src) {
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
  var b = state.settingsDraft.button;
  var isNewDraft = !!state.settingsDraft.isNew;

  var title = document.createElement("div");
  title.className = "sp-section-title";
  title.textContent = "Settings";
  container.appendChild(title);

  var panel = document.createElement("div");
  panel.className = "sp-panel";

  var idPrefix = c.isSub ? "sp-sp-inp-" : "sp-inp-";
  var requiredFields = [];

  function markDraftDirty() {
    if (state.settingsDraft && state.settingsDraft.key === draftKey) {
      state.settingsDraft.dirty = true;
    }
  }

  function saveField(field, val) {
    markDraftDirty();
  }

  function fieldContainer(input) {
    return input && input.closest ? input.closest(".sp-field") : null;
  }

  function clearFieldError(input) {
    if (!input) return;
    input.classList.remove("sp-input-error");
    input.removeAttribute("aria-invalid");
    input.removeAttribute("aria-describedby");
    var field = fieldContainer(input);
    if (field) field.classList.remove("sp-field-invalid");
    var existing = field ? field.querySelector(".sp-field-error") : null;
    if (existing && existing.parentNode) existing.parentNode.removeChild(existing);
  }

  function showFieldError(input, message) {
    if (!input) return;
    var field = fieldContainer(input);
    input.classList.add("sp-input-error");
    input.setAttribute("aria-invalid", "true");
    if (field) field.classList.add("sp-field-invalid");
    var existing = field ? field.querySelector(".sp-field-error") : null;
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

  function requireField(input, message, isActive) {
    if (!input) return;
    requiredFields.push({
      input: input,
      message: message || "Add an entity before saving.",
      isActive: isActive || function () { return true; },
    });
    function maybeClearError() {
      if (!isActive || isActive()) {
        if (String(input.value || "").trim()) clearFieldError(input);
      } else {
        clearFieldError(input);
      }
    }
    input.addEventListener("input", maybeClearError);
    input.addEventListener("change", maybeClearError);
  }

  function validateSettingsDraft() {
    var firstInvalid = null;
    for (var i = 0; i < requiredFields.length; i++) {
      var rule = requiredFields[i];
      if (rule.isActive && !rule.isActive()) {
        clearFieldError(rule.input);
        continue;
      }
      if (String(rule.input.value || "").trim()) {
        clearFieldError(rule.input);
        continue;
      }
      if (!firstInvalid) firstInvalid = rule.input;
      showFieldError(rule.input, rule.message);
    }
    if (!firstInvalid) return true;
    openDisclosureForField(firstInvalid);
    firstInvalid.focus();
    return false;
  }

  function openDisclosureForField(input) {
    if (!input || !input.closest) return;
    var disclosure = input.closest(".sp-disclosure");
    if (!disclosure) return;
    disclosure.classList.add("sp-open");
    var button = disclosure.querySelector(".sp-disclosure-button");
    if (button) button.setAttribute("aria-expanded", "true");
  }

  function validateConfigSize() {
    if (c.isSub) return true;
    if (serializeButtonConfig(b).length <= 255) return true;
    showBanner("Card settings are too large to save. Shorten confirmation text, labels, or entity IDs.", "error");
    return false;
  }

  function validateImageCardLimit() {
    var count = imageCardCountWithCandidate({
      isSub: c.isSub,
      homeSlot: state.editingSubpage,
      slot: slot,
      button: b,
    });
    if (count <= imageCardLimit()) return true;
    showImageCardLimitBanner();
    return false;
  }

  function applyCardSizeConstraint(savedButton) {
    var currentSize = c.sizes[slot] || CARD_SIZE_SINGLE;
    var nextSize = normalizeCardSizeForConfig(savedButton, currentSize);
    if (nextSize === currentSize) return false;
    if (nextSize === CARD_SIZE_SINGLE) delete c.sizes[slot];
    else c.sizes[slot] = nextSize;
    clearSpans(c.grid, c.maxSlots);
    applySpans(c.grid, c.sizes, c.maxSlots);
    return true;
  }

  function applySettingsDraft() {
    if (!state.settingsDraft || state.settingsDraft.key !== draftKey) return false;
    var draft = state.settingsDraft;
    var savedButton = liveButton;
    var sizeChanged = false;
    if (draft.isNew) {
      var pos = draft.pos;
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
      } else {
        postText(entityName("button_order"), serializeGrid(state.grid));
        saveButtonConfig(slot);
      }
    } else {
      copyButtonConfig(liveButton, draft.button);
      sizeChanged = applyCardSizeConstraint(liveButton);
    }
    state.settingsDraft = null;
    if (!draft.isNew && c.isSub) {
      saveSubpageConfig(state.editingSubpage);
    } else if (!draft.isNew) {
      if (sizeChanged) postText(entityName("button_order"), serializeGrid(state.grid));
      saveButtonConfig(slot);
    }
    var savedTypeDef = BUTTON_TYPES[savedButton.type || ""];
    if (savedTypeDef && savedTypeDef.afterSave) {
      savedTypeDef.afterSave(savedButton, slot, { isSub: c.isSub });
    }
    renderPreview();
    return true;
  }

  function bindField(input, field, rerender) {
    function syncValue() {
      if (b[field] === input.value) return;
      b[field] = input.value;
      markDraftDirty();
      if (rerender) renderPreview();
    }
    input.addEventListener("input", function () {
      syncValue();
    });
    input.addEventListener("change", syncValue);
    input.addEventListener("blur", syncValue);
    input.addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        syncValue();
        this.blur();
      }
    });
  }

  function makeIconPicker(pickerId, inputId, currentVal, onSelect, labelText) {
    var icf = document.createElement("div");
    icf.className = "sp-field";
    if (labelText === "On Icon") icf.classList.add("sp-icon-on-field");
    icf.appendChild(fieldLabel(labelText || "Icon", inputId));
    var picker = document.createElement("div");
    picker.className = "sp-icon-picker";
    if (pickerId) picker.id = pickerId;
    picker.appendChild(mdiIcon(currentVal, "sp-icon-picker-preview mdi"));
    var input = document.createElement("input");
    input.className = "sp-icon-picker-input";
    if (inputId) input.id = inputId;
    input.type = "text";
    input.placeholder = "Search icons\u2026";
    input.value = currentVal || "";
    input.autocomplete = "off";
    picker.appendChild(input);
    var dropdown = document.createElement("div");
    dropdown.className = "sp-icon-dropdown";
    picker.appendChild(dropdown);
    icf.appendChild(picker);
    initIconPicker(picker, currentVal, onSelect);
    return icf;
  }

  function entityField(labelText, inputId, value, placeholder, domains, bindName, rerender, requiredMessage) {
    var input = entityInput(inputId, value, placeholder, domains);
    var field = fieldWithControl(labelText, inputId, input);
    if (bindName) bindField(input, bindName, rerender);
    if (requiredMessage) requireField(input, requiredMessage);
    return {
      field: field,
      input: input,
    };
  }

  function textField(labelText, inputId, value, placeholder, bindName, rerender) {
    var input = textInput(inputId, value, placeholder);
    var field = fieldWithControl(labelText, inputId, input);
    if (bindName) bindField(input, bindName, rerender);
    return {
      field: field,
      input: input,
    };
  }

  function toggleSection(labelText, inputId, checked) {
    return {
      toggle: toggleRow(labelText, inputId, checked),
      section: condField(),
    };
  }

  function precisionField(inputId, value, onChange) {
    return selectField("Unit Precision", inputId, [
      ["0", "10"],
      ["1", "10.2"],
      ["2", "10.21"],
    ], value || "0", onChange);
  }

  function selectCardType(newType) {
    if (newType === "__choose-card-type__") return;
    var pickerType = newType;
    newType = defaultButtonTypeForPicker(newType);
    var keepMediaEntity = pickerType === "media_control" && b.type === "media";
    b.type = newType;
    if (state.settingsDraft && state.settingsDraft.key === draftKey) {
      state.settingsDraft.typeSelected = true;
    }
    var td = BUTTON_TYPES[newType];
    if (td && td.onSelect && !keepMediaEntity) td.onSelect(b);
    if (pickerType === "media_control") {
      b.sensor = "control_modal";
      b.label = "All Controls";
      b.icon = "Auto";
      b.icon_on = "Auto";
      b.unit = "";
      b.precision = "";
      b.options = "";
    }
    saveField("type", b.type);
    renderButtonSettings();
  }

  function renderCardTypeGrid(options) {
    var field = document.createElement("div");
    field.className = "sp-field sp-card-type-picker-field";
    field.appendChild(fieldLabel("Card", "sp-card-type-picker"));
    var grid = document.createElement("div");
    grid.className = "sp-card-type-grid";
    grid.id = "sp-card-type-picker";
    grid.setAttribute("role", "list");
    (options || []).forEach(function (o) {
      var item = document.createElement("button");
      item.type = "button";
      item.className = "sp-card-type-option";
      item.disabled = !!o.disabled;
      item.setAttribute("data-card-type", o.key);
      item.setAttribute("aria-label", o.label + " card type");
      item.appendChild(mdiIcon(o.icon || "card-outline", "sp-card-type-icon mdi"));
      var copy = document.createElement("span");
      copy.className = "sp-card-type-copy";
      copy.appendChild(textSpan(o.label, "sp-card-type-title"));
      copy.appendChild(textSpan(o.description || "", "sp-card-type-description"));
      item.appendChild(copy);
      item.addEventListener("click", function () {
        if (item.disabled) return;
        selectCardType(o.key);
      });
      grid.appendChild(item);
    });
    field.appendChild(grid);
    return field;
  }

  function renderActiveDisplaySettings(panel, button, idPrefix) {
    var hasIconOn = button.icon_on && button.icon_on !== "Auto";
    var hasSensor = !!button.sensor;
    var activeEnabled = hasIconOn || hasSensor || !!button._whenOnActive;
    var activeMode = button._whenOnMode || (hasSensor ? "sensor" : "icon");

    var activeToggle = toggleRow("Active Display", idPrefix + "whenon-toggle", activeEnabled);
    panel.appendChild(activeToggle.row);

    var activeFields = condField();
    if (activeEnabled) activeFields.classList.add("sp-visible");

    var modeControl = segmentControl([
      ["icon", "On Icon"],
      ["sensor", "Numeric"],
    ], activeMode, function (mode) {
      setActiveDisplayMode(mode);
    });
    activeFields.appendChild(modeControl.segment);

    var iconSection = condField();
    if (activeMode === "icon") iconSection.classList.add("sp-visible");
    var iconOnPicker = makeIconPicker(
      idPrefix + "icon-on-picker",
      idPrefix + "icon-on",
      hasIconOn ? button.icon_on : "Auto",
      function (opt) {
        button.icon_on = opt;
        saveField("icon_on", opt);
      },
      "On Icon"
    );
    iconSection.appendChild(iconOnPicker);
    activeFields.appendChild(iconSection);

    var sensorSection = condField();
    if (activeMode === "sensor") sensorSection.classList.add("sp-visible");

    var sensorField = entityField(
      "Sensor Entity",
      idPrefix + "sensor",
      button.sensor,
      "e.g. sensor.printer_percent_complete",
      ["sensor", "binary_sensor", "text_sensor"],
      "sensor",
      true
    );
    sensorSection.appendChild(sensorField.field);

    var unitField = textField("Unit", idPrefix + "unit", button.unit, "e.g. %", "unit", false);
    sensorSection.appendChild(unitField.field);

    var precision = precisionField(idPrefix + "precision", button.precision || "0", function () {
      button.precision = this.value === "0" ? "" : this.value;
      saveField("precision", button.precision);
    });
    sensorSection.appendChild(precision.field);
    activeFields.appendChild(sensorSection);

    panel.appendChild(activeFields);

    function syncIconPicker(value) {
      if (value === "Auto") {
        var autoPreview = iconOnPicker.querySelector(".sp-icon-picker-preview");
        if (autoPreview) autoPreview.className = "sp-icon-picker-preview mdi mdi-cog";
        var autoInput = iconOnPicker.querySelector(".sp-icon-picker-input");
        if (autoInput) autoInput.value = "Auto";
        return;
      }
      var picker = iconOnPicker.querySelector(".sp-icon-picker");
      if (picker && picker._setIcon) {
        picker._setIcon(value);
        return;
      }
      var preview = iconOnPicker.querySelector(".sp-icon-picker-preview");
      if (preview) preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(value);
      var input = iconOnPicker.querySelector(".sp-icon-picker-input");
      if (input) input.value = value;
    }

    function resetSensorFields() {
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

    function setActiveDisplayMode(mode) {
      activeMode = mode;
      button._whenOnActive = true;
      button._whenOnMode = mode;
      modeControl.buttons.icon.classList.toggle("active", mode === "icon");
      modeControl.buttons.sensor.classList.toggle("active", mode === "sensor");
      iconSection.classList.toggle("sp-visible", mode === "icon");
      sensorSection.classList.toggle("sp-visible", mode === "sensor");
      if (mode === "icon") {
        resetSensorFields();
      } else {
        button.icon_on = "Auto";
        saveField("icon_on", "Auto");
        syncIconPicker("Auto");
      }
    }

    activeToggle.input.addEventListener("change", function () {
      if (this.checked) {
        button._whenOnActive = true;
        activeFields.classList.add("sp-visible");
      } else {
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

  var isNewDraftWithoutType = isNewDraft && !state.settingsDraft.typeSelected;
  var rawTypeDef = isNewDraftWithoutType ? null : (BUTTON_TYPES[b.type || ""] || BUTTON_TYPES[""]);
  var typeDef = rawTypeDef;
  {
    var chooseTypeValue = "__choose-card-type__";
    var selectedTypeKey = isNewDraftWithoutType
      ? null
      : buttonTypeRegistryValue(rawTypeDef, "pickerKey", "") || (b.type || "");
    var typeOpts = buttonTypePickerOptionList(c.isSub, selectedTypeKey);
    var tf = document.createElement("div");
    tf.className = "sp-field";
    tf.appendChild(fieldLabel("Card", "sp-inp-type"));
    var typeSelect = document.createElement("select");
    typeSelect.className = "sp-select";
    typeSelect.id = "sp-inp-type";
    if (isNewDraftWithoutType) {
      var chooseOpt = document.createElement("option");
      chooseOpt.value = chooseTypeValue;
      chooseOpt.textContent = "Select card type";
      chooseOpt.disabled = true;
      chooseOpt.selected = true;
      typeSelect.appendChild(chooseOpt);
    }
    typeOpts.forEach(function (o) {
      var opt = document.createElement("option");
      opt.value = o.key;
      opt.textContent = o.label;
      opt.disabled = !!o.disabled;
      if (!isNewDraftWithoutType && selectedTypeKey === o.key) opt.selected = true;
      typeSelect.appendChild(opt);
    });
    typeSelect.addEventListener("change", function () {
      selectCardType(this.value);
    });
    if (isNewDraftWithoutType) {
      if (settingsModal) settingsModal.classList.add("sp-card-type-picker-open");
      panel.appendChild(renderCardTypeGrid(typeOpts));
      container.appendChild(panel);
      return;
    }
    tf.appendChild(typeSelect);
    panel.appendChild(tf);
  }

  var typeHelpers = {
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
    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(fieldLabel("Label", idPrefix + "label"));
    var labelPlaceholder = (typeDef && typeDef.labelPlaceholder) || "e.g. Kitchen";
    var labelInp = textInput(idPrefix + "label", b.label, labelPlaceholder);
    lf.appendChild(labelInp);
    panel.appendChild(lf);
    bindField(labelInp, "label", true);
  }

  if (typeDef && typeDef.renderSettings &&
      (!c.isSub || buttonTypeRegistryValue(typeDef, "allowInSubpage", false))) {
    typeDef.renderSettings(panel, b, slot, typeHelpers);
  } else {
    // Toggle fallback: entity, icons, sensor data
    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(fieldLabel("Entity", idPrefix + "entity"));
    var entityInp = entityInput(idPrefix + "entity", b.entity, "e.g. light.kitchen", [
      "light", "switch", "input_boolean", "fan"
    ]);
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    bindField(entityInp, "entity", true);
    requireField(entityInp, "Add an entity before saving.");

    panel.appendChild(makeIconPicker(idPrefix + "icon-picker", idPrefix + "icon", b.icon || "Auto", function (opt) {
      b.icon = opt;
      saveField("icon", opt);
    }, "Off Icon"));

    renderActiveDisplaySettings(panel, b, idPrefix);

    var patternField = selectField("On State Pattern", idPrefix + "on-pattern", [
      ["", "Solid"],
      ["stripes", "Stripes"],
    ], cardOnPattern(b), function () {
      setCardOnPattern(b, this.value);
      saveField("options", b.options);
      renderPreview();
    });
    panel.appendChild(patternField.field);
  }

  var saveRow = document.createElement("div");
  saveRow.className = "sp-btn-row sp-btn-row--save";

  if (!isNewDraft) {
    var delBtn = createActionButton("sp-action-btn sp-delete-btn", "", "trash-can-outline");
    delBtn.addEventListener("click", function () {
      state.settingsDraft = null;
      deleteSlot(slot);
    });
    saveRow.appendChild(delBtn);
    saveRow.classList.add("sp-has-delete");
  }

  var rightGroup = document.createElement("div");
  rightGroup.className = "sp-btn-group-right";
  var editSubBtn = panel.querySelector(".sp-edit-subpage-btn");
  if (editSubBtn && isNewDraft && editSubBtn.parentNode) {
    editSubBtn.parentNode.removeChild(editSubBtn);
  } else if (editSubBtn) {
    rightGroup.appendChild(editSubBtn);
  }
  var saveBtn = createActionButton("sp-action-btn sp-save-btn", "Save");
  saveBtn.addEventListener("click", function () {
    if (!validateSettingsDraft()) return;
    if (!validateImageCardLimit()) return;
    if (!validateConfigSize()) return;
    if (!applySettingsDraft()) return;
    closeSettings();
  });
  rightGroup.appendChild(saveBtn);
  saveRow.appendChild(rightGroup);
  panel.appendChild(saveRow);

  container.appendChild(panel);
}
