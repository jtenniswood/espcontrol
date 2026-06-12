// ── Button settings panel (unified) ────────────────────────────────────

function hideSettingsOverlay() {
  if (els.settingsOverlay) els.settingsOverlay.classList.remove("sp-visible");
}

function updatePreviewHint(c) {
  if (!els.previewHint) return;
  c = c || ctx();
  els.previewHint.style.display = "";
  if (isConfigLocked()) {
    els.previewHint.textContent = "Editing is paused while the device reconnects";
  } else if (state.clockBarSelectedItem) {
    els.previewHint.textContent = clockBarItemLabel(state.clockBarSelectedItem) + " selected";
  } else if (c.selected.length > 1) {
    els.previewHint.textContent = c.selected.length + " buttons selected \u2022 right click to copy, cut, or delete";
  } else {
    els.previewHint.textContent = "tap to select \u2022 shift/ctrl+tap to multi-select \u2022 right click to manage";
  }
}

function renderClockBarSelectionBar() {
  if (!els.selectionBar || !state.clockBarSelectedItem) return false;
  els.selectionBar.className = "sp-selection-bar sp-visible";

  var label = document.createElement("span");
  label.className = "sp-selection-label";
  label.textContent = clockBarItemLabel(state.clockBarSelectedItem) + " selected";
  els.selectionBar.appendChild(label);

  var actions = document.createElement("div");
  actions.className = "sp-selection-actions";

  var editBtn = document.createElement("button");
  editBtn.type = "button";
  editBtn.className = "sp-selection-btn sp-selection-btn-primary";
  editBtn.innerHTML = '<span class="mdi mdi-pencil"></span>Edit';
  editBtn.disabled = !isClockBarTemperatureItem(state.clockBarSelectedItem);
  editBtn.addEventListener("click", function (e) {
    e.preventDefault();
    e.stopPropagation();
    if (!editBtn.disabled) openClockBarTemperatureSettings();
  });
  actions.appendChild(editBtn);

  var visible = clockBarItemActive(state.clockBarSelectedItem);
  var hideBtn = document.createElement("button");
  hideBtn.type = "button";
  hideBtn.className = "sp-selection-btn";
  hideBtn.innerHTML = '<span class="mdi mdi-' + (visible ? "eye-off-outline" : "eye-outline") + '"></span>' +
    (visible ? "Hide" : "Show");
  hideBtn.addEventListener("click", function (e) {
    e.preventDefault();
    e.stopPropagation();
    setClockBarItemVisible(state.clockBarSelectedItem, !visible);
    renderSelectionBar(ctx());
  });
  actions.appendChild(hideBtn);

  var menuBtn = document.createElement("button");
  menuBtn.type = "button";
  menuBtn.className = "sp-selection-btn";
  menuBtn.setAttribute("aria-label", "Clock bar actions");
  menuBtn.innerHTML = '<span class="mdi mdi-dots-horizontal"></span>';
  menuBtn.addEventListener("click", function (e) {
    e.preventDefault();
    e.stopPropagation();
    showClockBarContextMenu(e, state.clockBarSelectedItem);
  });
  actions.appendChild(menuBtn);

  els.selectionBar.appendChild(actions);
  return true;
}

function renderSelectionBar(c) {
  if (!els.selectionBar) return;
  c = c || ctx();
  els.selectionBar.innerHTML = "";
  if (!isConfigLocked() && renderClockBarSelectionBar()) return;
  if (isConfigLocked() || !c.selected.length) {
    els.selectionBar.className = "sp-selection-bar";
    return;
  }

  els.selectionBar.className = "sp-selection-bar sp-visible";

  var label = document.createElement("span");
  label.className = "sp-selection-label";
  if (c.selected.length === 1 && c.selected[0] === -2) {
    label.textContent = "Back button selected";
  } else {
    label.textContent = c.selected.length === 1 ? "1 card selected" : c.selected.length + " cards selected";
  }
  els.selectionBar.appendChild(label);

  var actions = document.createElement("div");
  actions.className = "sp-selection-actions";

  if (c.selected.length === 1) {
    var editBtn = document.createElement("button");
    editBtn.type = "button";
    editBtn.className = "sp-selection-btn sp-selection-btn-primary";
    editBtn.innerHTML = '<span class="mdi mdi-pencil"></span>Edit';
    editBtn.addEventListener("click", function (e) {
      e.preventDefault();
      e.stopPropagation();
      openSelectedCardSettings();
    });
    actions.appendChild(editBtn);
  }

  var menuBtn = document.createElement("button");
  menuBtn.type = "button";
  menuBtn.className = "sp-selection-btn";
  menuBtn.setAttribute("aria-label", "Card actions");
  menuBtn.innerHTML = '<span class="mdi mdi-dots-horizontal"></span>';
  menuBtn.addEventListener("click", function (e) {
    e.preventDefault();
    e.stopPropagation();
    showSelectionMenu(e);
  });
  actions.appendChild(menuBtn);
  els.selectionBar.appendChild(actions);
}

function closeSettings() {
  hideSettingsOverlay();
  _settingsDeferred = false;
  state.settingsDraft = null;
  ctx().setSelected([]);
  state.clockBarSelectedItem = "";
  updateClockBarItemUi();
  renderPreview();
}

function clearCardSelection() {
  var c = ctx();
  if (!c.selected.length && c.getLastClicked() < 0 && !state.clockBarSelectedItem) return;
  c.setSelected([]);
  c.setLastClicked(-1);
  state.clockBarSelectedItem = "";
  hideSettingsOverlay();
  updateClockBarItemUi();
  renderPreview();
  renderButtonSettings();
}

function isSelectionControlTarget(target) {
  return !!(
    (els.previewMain && els.previewMain.contains(target)) ||
    (els.topbar && els.topbar.contains(target)) ||
    (els.selectionBar && els.selectionBar.contains(target)) ||
    (els.settingsOverlay && els.settingsOverlay.contains(target)) ||
    (ctxMenu && ctxMenu.contains(target)) ||
    (target.closest && target.closest(".sp-ctx-menu"))
  );
}

function handleDocumentSelectionMouseDown(e) {
  if (e.button !== 0) return;
  if (isSelectionControlTarget(e.target)) return;
  clearCardSelection();
}

function openSelectedCardSettings() {
  if (isConfigLocked()) return;
  if (state.clockBarSelectedItem) {
    if (isClockBarTemperatureItem(state.clockBarSelectedItem)) openClockBarTemperatureSettings();
    return;
  }
  var c = ctx();
  if (c.selected.length !== 1) return;
  renderButtonSettings(true);
}

function selectClockBarItem(item) {
  if (isConfigLocked() || clockBarItems().indexOf(item) === -1) return;
  var c = ctx();
  c.setSelected([]);
  c.setLastClicked(-1);
  hideSettingsOverlay();
  state.clockBarSelectedItem = state.clockBarSelectedItem === item ? "" : item;
  updateClockBarItemUi();
  renderPreview();
  renderButtonSettings();
}

function openClockBarTemperatureSettings() {
  if (isConfigLocked()) return;
  var container = els.buttonSettings;
  if (!container) return;
  state.clockBarSelectedItem = "temperature";
  container.innerHTML = "";
  if (els.settingsOverlay) els.settingsOverlay.classList.add("sp-visible");

  var title = document.createElement("div");
  title.className = "sp-section-title";
  title.textContent = "Temperature";
  container.appendChild(title);

  var panel = document.createElement("div");
  panel.className = "sp-panel";

  var entityField = document.createElement("div");
  entityField.className = "sp-field";
  entityField.appendChild(fieldLabel("Entity", "sp-clockbar-temperature-entity"));
  var entityInp = entityInput(
    "sp-clockbar-temperature-entity",
    primaryClockBarTemperatureEntity(),
    "sensor.outdoor_temperature",
    ["sensor"]
  );
  entityField.appendChild(entityInp);
  panel.appendChild(entityField);

  var degreeToggle = toggleRow(
    "Show Degree Symbol",
    "sp-clockbar-temperature-degree-symbol",
    state.temperatureDegreeSymbolOn
  );
  panel.appendChild(degreeToggle.row);

  var saveRow = document.createElement("div");
  saveRow.className = "sp-btn-row sp-btn-row--save sp-has-secondary";

  var visible = clockBarItemActive("temperature");
  var hideBtn = document.createElement("button");
  hideBtn.type = "button";
  hideBtn.className = "sp-action-btn sp-hide-btn";
  hideBtn.innerHTML = '<span class="mdi mdi-' + (visible ? "eye-off-outline" : "eye-outline") + '"></span>' +
    (visible ? "Hide" : "Show");
  hideBtn.addEventListener("click", function () {
    setClockBarItemVisible("temperature", !visible);
    closeSettings();
  });
  saveRow.appendChild(hideBtn);

  var saveBtn = document.createElement("button");
  saveBtn.type = "button";
  saveBtn.className = "sp-action-btn sp-save-btn";
  saveBtn.textContent = "Save";
  saveBtn.addEventListener("click", function () {
    saveClockBarTemperatureSettings(entityInp.value, degreeToggle.input.checked);
    closeSettings();
  });
  saveRow.appendChild(saveBtn);
  panel.appendChild(saveRow);

  container.appendChild(panel);
  entityInp.focus();
}

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
  var doneBtn = document.createElement("button");
  doneBtn.type = "button";
  doneBtn.className = "sp-action-btn sp-save-btn";
  doneBtn.textContent = "Done";
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
    firstInvalid.focus();
    return false;
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

  function applySettingsDraft() {
    if (!state.settingsDraft || state.settingsDraft.key !== draftKey) return false;
    var draft = state.settingsDraft;
    var savedButton = liveButton;
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
      if (c.isSub) {
        saveSubpageConfig(state.editingSubpage);
      } else {
        postText(entityName("button_order"), serializeGrid(state.grid));
        saveButtonConfig(slot);
      }
    } else {
      copyButtonConfig(liveButton, draft.button);
    }
    state.settingsDraft = null;
    if (!draft.isNew && c.isSub) {
      saveSubpageConfig(state.editingSubpage);
    } else if (!draft.isNew) {
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
    icf.appendChild(fieldLabel(labelText || "Icon", inputId));
    var picker = document.createElement("div");
    picker.className = "sp-icon-picker";
    if (pickerId) picker.id = pickerId;
    picker.innerHTML =
      '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(currentVal) + '"></span>' +
      '<input class="sp-icon-picker-input"' + (inputId ? ' id="' + inputId + '"' : '') +
      ' type="text" placeholder="Search icons\u2026" value="' + escAttr(currentVal) + '" autocomplete="off">' +
      '<div class="sp-icon-dropdown"></div>';
    icf.appendChild(picker);
    initIconPicker(picker, currentVal, onSelect);
    return icf;
  }

  function fieldWithControl(labelText, inputId, control) {
    var field = document.createElement("div");
    field.className = "sp-field";
    field.appendChild(fieldLabel(labelText, inputId));
    if (control) field.appendChild(control);
    return field;
  }

  function optionValue(option) {
    if (Array.isArray(option)) return option[0];
    if (option && typeof option === "object") return option.value;
    return option;
  }

  function optionLabel(option) {
    if (Array.isArray(option)) return option[1];
    if (option && typeof option === "object") return option.label;
    return option;
  }

  function selectField(labelText, inputId, options, value, onChange) {
    var select = document.createElement("select");
    select.className = "sp-select";
    if (inputId) select.id = inputId;
    (options || []).forEach(function (entry) {
      var option = document.createElement("option");
      option.value = optionValue(entry);
      option.textContent = optionLabel(entry);
      select.appendChild(option);
    });
    select.value = value || "";
    if (onChange) select.addEventListener("change", onChange);
    return {
      field: fieldWithControl(labelText, inputId, select),
      select: select,
    };
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

  function segmentControl(options, value, onSelect) {
    var segment = document.createElement("div");
    segment.className = "sp-segment";
    var buttons = {};
    (options || []).forEach(function (entry) {
      var optValue = optionValue(entry);
      var button = document.createElement("button");
      button.type = "button";
      button.textContent = optionLabel(entry);
      button.classList.toggle("active", optValue === value);
      button.addEventListener("click", function () {
        for (var key in buttons) buttons[key].classList.toggle("active", key === optValue);
        if (onSelect) onSelect(optValue, button);
      });
      segment.appendChild(button);
      buttons[optValue] = button;
    });
    return {
      segment: segment,
      buttons: buttons,
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
    b.type = newType;
    if (state.settingsDraft && state.settingsDraft.key === draftKey) {
      state.settingsDraft.typeSelected = true;
    }
    var td = BUTTON_TYPES[newType];
    if (td && td.onSelect) td.onSelect(b);
    saveField("type", newType);
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
      item.innerHTML =
        '<span class="sp-card-type-icon mdi mdi-' + escAttr(o.icon || "card-outline") + '"></span>' +
        '<span class="sp-card-type-copy">' +
        '<span class="sp-card-type-title">' + escHtml(o.label) + '</span>' +
        '<span class="sp-card-type-description">' + escHtml(o.description || "") + '</span>' +
        '</span>';
      item.addEventListener("click", function () {
        if (item.disabled) return;
        selectCardType(o.key);
      });
      grid.appendChild(item);
    });
    field.appendChild(grid);
    return field;
  }

  var isNewDraftWithoutType = isNewDraft && !state.settingsDraft.typeSelected;
  var rawTypeDef = isNewDraftWithoutType ? null : (BUTTON_TYPES[b.type || ""] || BUTTON_TYPES[""]);
  var typeDef = rawTypeDef;
  var rawExperimental = buttonTypeRegistryValue(rawTypeDef, "experimental", "");
  if (rawExperimental && !isExperimentalEnabled(rawExperimental)) {
    typeDef = hiddenExperimentalButtonTypeDef(rawTypeDef);
  }
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

    // When-on section
    var hasIconOn = b.icon_on && b.icon_on !== "Auto";
    var hasSensor = !!b.sensor;
    var whenOnEnabled = hasIconOn || hasSensor || !!b._whenOnActive;
    var whenOnMode = b._whenOnMode || (hasSensor ? "sensor" : "icon");

    var whenOnToggle = toggleRow("Active Display", idPrefix + "whenon-toggle", whenOnEnabled);
    panel.appendChild(whenOnToggle.row);

    var whenOnCond = condField();
    if (whenOnEnabled) whenOnCond.classList.add("sp-visible");

    var seg = document.createElement("div");
    seg.className = "sp-segment";
    var btnIcon = document.createElement("button");
    btnIcon.type = "button";
    btnIcon.textContent = "On Icon";
    if (whenOnMode === "icon") btnIcon.classList.add("active");
    var btnSensor = document.createElement("button");
    btnSensor.type = "button";
    btnSensor.textContent = "Numeric";
    if (whenOnMode === "sensor") btnSensor.classList.add("active");
    seg.appendChild(btnIcon);
    seg.appendChild(btnSensor);
    whenOnCond.appendChild(seg);

    // Icon-on section
    var iconOnSection = condField();
    if (whenOnMode === "icon") iconOnSection.classList.add("sp-visible");
    var ionLabel = fieldLabel("On Icon", idPrefix + "icon-on");
    iconOnSection.appendChild(ionLabel);
    var iconOnVal = hasIconOn ? b.icon_on : "Auto";
    var iconOnPicker = document.createElement("div");
    iconOnPicker.className = "sp-icon-picker";
    iconOnPicker.id = idPrefix + "icon-on-picker";
    iconOnPicker.innerHTML =
      '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(iconOnVal) + '"></span>' +
      '<input class="sp-icon-picker-input" id="' + idPrefix + 'icon-on" type="text" ' +
      'placeholder="Search icons\u2026" value="' + escAttr(iconOnVal) + '" autocomplete="off">' +
      '<div class="sp-icon-dropdown"></div>';
    iconOnSection.appendChild(iconOnPicker);
    whenOnCond.appendChild(iconOnSection);

    initIconPicker(iconOnPicker, iconOnVal, function (opt) {
      b.icon_on = opt;
      saveField("icon_on", opt);
    });

    // Sensor section
    var sensorSection = condField();
    if (whenOnMode === "sensor") sensorSection.classList.add("sp-visible");

    var sf = document.createElement("div");
    sf.className = "sp-field";
    sf.appendChild(fieldLabel("Sensor Entity", idPrefix + "sensor"));
    var sensorInp = entityInput(idPrefix + "sensor", b.sensor, "e.g. sensor.printer_percent_complete", [
      "sensor", "binary_sensor", "text_sensor"
    ]);
    sf.appendChild(sensorInp);
    sensorSection.appendChild(sf);

    var uf = document.createElement("div");
    uf.className = "sp-field";
    uf.appendChild(fieldLabel("Unit", idPrefix + "unit"));
    var unitInp = textInput(idPrefix + "unit", b.unit, "e.g. %");
    unitInp.className = "sp-input";
    uf.appendChild(unitInp);
    sensorSection.appendChild(uf);

    var pf = document.createElement("div");
    pf.className = "sp-field";
    pf.appendChild(fieldLabel("Unit Precision", idPrefix + "precision"));
    var precisionSelect = document.createElement("select");
    precisionSelect.className = "sp-select";
    precisionSelect.id = idPrefix + "precision";
    var precOpts = [["0", "10"], ["1", "10.2"], ["2", "10.21"]];
    for (var pi = 0; pi < precOpts.length; pi++) {
      var opt = document.createElement("option");
      opt.value = precOpts[pi][0];
      opt.textContent = precOpts[pi][1];
      precisionSelect.appendChild(opt);
    }
    precisionSelect.value = b.precision || "0";
    precisionSelect.addEventListener("change", function () {
      b.precision = this.value === "0" ? "" : this.value;
      saveField("precision", b.precision);
    });
    pf.appendChild(precisionSelect);
    sensorSection.appendChild(pf);
    whenOnCond.appendChild(sensorSection);

    panel.appendChild(whenOnCond);

    bindField(sensorInp, "sensor", true);
    bindField(unitInp, "unit", false);

    function setWhenOnMode(mode) {
      whenOnMode = mode;
      b._whenOnActive = true;
      b._whenOnMode = mode;
      btnIcon.classList.toggle("active", mode === "icon");
      btnSensor.classList.toggle("active", mode === "sensor");
      iconOnSection.classList.toggle("sp-visible", mode === "icon");
      sensorSection.classList.toggle("sp-visible", mode === "sensor");
      if (mode === "icon") {
        sensorInp.value = "";
        unitInp.value = "";
        precisionSelect.value = "0";
        b.sensor = "";
        b.unit = "";
        b.precision = "";
        saveField("sensor", "");
        saveField("unit", "");
        saveField("precision", "");
      } else {
        b.icon_on = "Auto";
        saveField("icon_on", "Auto");
        var ionPreview = iconOnPicker.querySelector(".sp-icon-picker-preview");
        if (ionPreview) ionPreview.className = "sp-icon-picker-preview mdi mdi-cog";
        var ionInput = iconOnPicker.querySelector(".sp-icon-picker-input");
        if (ionInput) ionInput.value = "Auto";
      }
    }

    btnIcon.addEventListener("click", function () { setWhenOnMode("icon"); });
    btnSensor.addEventListener("click", function () { setWhenOnMode("sensor"); });

    whenOnToggle.input.addEventListener("change", function () {
      if (this.checked) {
        b._whenOnActive = true;
        whenOnCond.classList.add("sp-visible");
      } else {
        b._whenOnActive = false;
        b._whenOnMode = null;
        whenOnCond.classList.remove("sp-visible");
        sensorInp.value = "";
        unitInp.value = "";
        precisionSelect.value = "0";
        b.sensor = "";
        b.unit = "";
        b.precision = "";
        b.icon_on = "Auto";
        saveField("sensor", "");
        saveField("unit", "");
        saveField("precision", "");
        saveField("icon_on", "Auto");
      }
    });

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
    var delBtn = document.createElement("button");
    delBtn.className = "sp-action-btn sp-delete-btn";
    delBtn.innerHTML = '<span class="mdi mdi-trash-can-outline"></span>';
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
  var saveBtn = document.createElement("button");
  saveBtn.className = "sp-action-btn sp-save-btn";
  saveBtn.textContent = "Save";
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

// ── Render debouncing ──────────────────────────────────────────────────

var _renderPending = false;
function scheduleRender() {
  if (_renderPending) return;
  _renderPending = true;
  requestAnimationFrame(function () {
    _renderPending = false;
    renderPreview();
    if (isSettingsOpen() || isSettingsFocused()) {
      _settingsDeferred = true;
    } else {
      renderButtonSettings();
    }
  });
}

var _settingsDeferred = false;
document.addEventListener("focusout", function (e) {
  if (!_settingsDeferred) return;
  if (e.relatedTarget && els.buttonSettings && els.buttonSettings.contains(e.relatedTarget)) return;
  requestAnimationFrame(function () {
    if (isSettingsOpen()) return;
    if (!isSettingsFocused()) {
      _settingsDeferred = false;
      renderButtonSettings();
    }
  });
});

document.addEventListener("keydown", function (e) {
  if (e.key === "Escape" && els.settingsOverlay &&
      els.settingsOverlay.classList.contains("sp-visible")) {
    closeSettings();
  }
});

// ── Icon picker (optimized) ────────────────────────────────────────────

function initIconPicker(picker, currentIcon, onSelect) {
  var input = picker.querySelector(".sp-icon-picker-input");
  var dropdown = picker.querySelector(".sp-icon-dropdown");
  var preview = picker.querySelector(".sp-icon-picker-preview");
  var highlighted = -1;
  var optionEls = null;
  var emptyEl = null;

  function ensureBuilt() {
    if (optionEls) return;
    optionEls = [];
    var frag = document.createDocumentFragment();
    ICON_OPTIONS.forEach(function (opt) {
      var row = document.createElement("div");
      row.className = "sp-icon-option" + (opt === currentIcon ? " sp-active" : "");
      row.innerHTML =
        '<span class="sp-icon-option-icon mdi mdi-' + iconSlug(opt) + '"></span>' +
        '<span class="sp-icon-option-label">' + escHtml(opt) + '</span>';
      row._lcName = opt.toLowerCase();
      row._optName = opt;
      row.addEventListener("mousedown", function (e) {
        e.preventDefault();
        selectOpt(opt);
      });
      frag.appendChild(row);
      optionEls.push(row);
    });
    emptyEl = document.createElement("div");
    emptyEl.className = "sp-icon-option sp-icon-option--empty";
    emptyEl.textContent = "No matches";
    emptyEl.style.display = "none";
    frag.appendChild(emptyEl);
    dropdown.appendChild(frag);
  }

  function filterOpts(filter) {
    ensureBuilt();
    highlighted = -1;
    var lc = (filter || "").toLowerCase();
    var hasMatch = false;
    for (var i = 0; i < optionEls.length; i++) {
      var match = !lc || optionEls[i]._lcName.indexOf(lc) !== -1;
      optionEls[i].style.display = match ? "" : "none";
      optionEls[i].classList.remove("sp-highlighted");
      if (match) hasMatch = true;
    }
    emptyEl.style.display = hasMatch ? "none" : "";
  }

  function setPickerIcon(opt) {
    currentIcon = opt;
    input.value = opt;
    preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(opt);
    if (optionEls) {
      for (var i = 0; i < optionEls.length; i++) {
        optionEls[i].classList.toggle("sp-active", optionEls[i]._optName === opt);
      }
    }
  }

  function selectOpt(opt) {
    setPickerIcon(opt);
    closePicker();
    onSelect(opt);
    renderPreview();
  }
  picker._setIcon = setPickerIcon;

  function openPicker() {
    if (currentIcon === "Auto") {
      input.value = "";
      filterOpts("");
    } else {
      filterOpts(currentIcon);
      setTimeout(function () { input.select(); }, 0);
    }
    picker.classList.add("sp-open");
  }

  function closePicker() {
    picker.classList.remove("sp-open");
    input.value = currentIcon;
    highlighted = -1;
  }

  function getVisible() {
    var vis = [];
    if (optionEls) {
      for (var i = 0; i < optionEls.length; i++) {
        if (optionEls[i].style.display !== "none") vis.push(optionEls[i]);
      }
    }
    return vis;
  }

  function highlightAt(idx) {
    var visible = getVisible();
    if (visible.length === 0) return;
    if (optionEls) optionEls.forEach(function (el) { el.classList.remove("sp-highlighted"); });
    if (idx < 0) idx = visible.length - 1;
    if (idx >= visible.length) idx = 0;
    highlighted = idx;
    visible[highlighted].classList.add("sp-highlighted");
    visible[highlighted].scrollIntoView({ block: "nearest" });
  }

  input.addEventListener("focus", openPicker);
  input.addEventListener("blur", closePicker);

  input.addEventListener("input", function () {
    filterOpts(this.value);
    var vis = getVisible();
    if (vis.length > 0) highlightAt(0);
  });

  input.addEventListener("keydown", function (e) {
    if (e.key === "ArrowDown") {
      e.preventDefault();
      if (!picker.classList.contains("sp-open")) { openPicker(); return; }
      highlightAt(highlighted + 1);
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      highlightAt(highlighted - 1);
    } else if (e.key === "Enter") {
      e.preventDefault();
      var visible = getVisible();
      if (highlighted >= 0 && highlighted < visible.length) {
        selectOpt(visible[highlighted]._optName);
      }
    } else if (e.key === "Tab") {
      var visible = getVisible();
      if (picker.classList.contains("sp-open") && highlighted >= 0 && highlighted < visible.length) {
        selectOpt(visible[highlighted]._optName);
      }
    } else if (e.key === "Escape") {
      e.preventDefault();
      closePicker();
      input.blur();
    }
  });
}
