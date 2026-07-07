// ── Button Settings Selection ─────────────────────────────────────
// @web-module-requires: state, clock_bar_state, config_codec, config_post_api, controls, controls_fields, preview_render, settings_page

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
  var canEditClockBarItem = isClockBarTemperatureItem(state.clockBarSelectedItem) ||
    state.clockBarSelectedItem === "voice";

  var label = document.createElement("span");
  label.className = "sp-selection-label";
  label.textContent = clockBarItemLabel(state.clockBarSelectedItem) + " selected";
  els.selectionBar.appendChild(label);

  var actions = document.createElement("div");
  actions.className = "sp-selection-actions";

  var editBtn = createActionButton("sp-selection-btn sp-selection-btn-primary", "Edit", "pencil");
  editBtn.disabled = !canEditClockBarItem;
  editBtn.addEventListener("click", function (e) {
    e.preventDefault();
    e.stopPropagation();
    if (editBtn.disabled) return;
    if (isClockBarTemperatureItem(state.clockBarSelectedItem)) {
      openClockBarTemperatureSettings();
    } else if (state.clockBarSelectedItem === "voice") {
      openVoiceServicesSettings();
    }
  });
  actions.appendChild(editBtn);

  var visible = clockBarItemActive(state.clockBarSelectedItem);
  var hideBtn = createActionButton(
    "sp-selection-btn",
    visible ? "Hide" : "Show",
    visible ? "eye-off-outline" : "eye-outline"
  );
  hideBtn.addEventListener("click", function (e) {
    e.preventDefault();
    e.stopPropagation();
    setClockBarItemVisible(state.clockBarSelectedItem, !visible);
    renderSelectionBar(ctx());
  });
  actions.appendChild(hideBtn);

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
    var editBtn = createActionButton("sp-selection-btn sp-selection-btn-primary", "Edit", "pencil");
    editBtn.addEventListener("click", function (e) {
      e.preventDefault();
      e.stopPropagation();
      openSelectedCardSettings();
    });
    actions.appendChild(editBtn);
  }

  var menuBtn = createActionButton("sp-selection-btn", "", "dots-horizontal", "Card actions");
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
    if (state.clockBarSelectedItem === "voice") openVoiceServicesSettings();
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
  var hideBtn = createActionButton(
    "sp-action-btn sp-hide-btn",
    visible ? "Hide" : "Show",
    visible ? "eye-off-outline" : "eye-outline"
  );
  hideBtn.addEventListener("click", function () {
    setClockBarItemVisible("temperature", !visible);
    closeSettings();
  });
  saveRow.appendChild(hideBtn);

  var saveBtn = createActionButton("sp-action-btn sp-save-btn", "Save");
  saveBtn.addEventListener("click", function () {
    saveClockBarTemperatureSettings(entityInp.value, degreeToggle.input.checked);
    closeSettings();
  });
  saveRow.appendChild(saveBtn);
  panel.appendChild(saveRow);

  container.appendChild(panel);
  entityInp.focus();
}
