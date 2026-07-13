// ── Preview rendering (unified) ────────────────────────────────────────
// @web-module-requires: state, screen_rotation_state, grid, config_codec, controls, controls_fields

function previewHtmlValue(typePreview, key, fallback) {
  return PreviewFeature.previewValue(typePreview, key, fallback);
}

function buttonTypeRegistryValue(typeDef, key, fallback) {
  return PreviewFeature.registryValue(typeDef, key, fallback);
}

function buttonTypeDisabledForDevice(key) {
  var disabled = CFG.disabledCardTypes || [];
  return disabled.indexOf(key || "") !== -1;
}

function buttonTypeInfoOnlyVisible(key) {
  return PreviewFeature.infoOnlyCardVisible(key || "", !!CFG.infoOnly);
}

function defaultButtonTypeForPicker(key) {
  return PreviewFeature.defaultCardTypeForPicker(key);
}

function buttonTypePickerDetails(key, label) {
  return PreviewFeature.cardTypePickerDetails(key || "", label || "");
}

function buttonTypePickerOptionList(isSub, selectedTypeKey) {
  return PreviewFeature.cardTypePickerOptions(
    BUTTON_TYPES,
    CFG.disabledCardTypes || [],
    !!CFG.infoOnly,
    !!isSub,
    selectedTypeKey
  );
}

function buttonTypePickerKeys(isSub, selectedTypeKey) {
  return buttonTypePickerOptionList(!!isSub, selectedTypeKey).map(function (opt) {
    return opt.key;
  });
}

function buttonTypeVisibleInPicker(key, isSub) {
  return buttonTypePickerKeys(!!isSub, null).indexOf(key) >= 0;
}

function renderPreview() {
  var main = els.previewMain;
  main.innerHTML = "";
  main.className = "sp-main" + (state.subpageChevronsOn ? "" : " sp-hide-subpage-chevrons");
  if (gridPreviewBlockedByRotationStartup()) {
    main.className += " sp-grid-loading";
    main.setAttribute("aria-busy", "true");
    return;
  }
  main.removeAttribute("aria-busy");
  var c = ctx();

  updatePreviewHint(c);

  for (var pos = 0; pos < c.maxSlots; pos++) {
    var slot = c.grid[pos];
    if (slot === -1) continue;

    if (slot === -2) {
      var backBtn = document.createElement("div");
      var bkSz = c.sizes[-2];
      var backLabel = c.isSub ? (getSubpage(state.editingSubpage).backLabel || "Back") : "Back";
      backBtn.className = "sp-btn sp-back-btn" + sizeClass(bkSz) +
        (c.selected.indexOf(-2) !== -1 ? " sp-selected" : "");
      backBtn.innerHTML =
        '<span class="sp-btn-icon sp-back-hit mdi mdi-chevron-left"></span>' +
        '<span class="sp-btn-label">' + escHtml(backLabel) + '</span>';
      backBtn.style.backgroundColor = "#" + WEB_UI_COLORS.secondary;
      backBtn.style.cursor = "pointer";
      backBtn.setAttribute("data-pos", pos);
      backBtn.draggable = !isConfigLocked();
      main.appendChild(backBtn);
    } else if (slot > 0) {
      var bIdx = slot - 1;
      if (c.isSub && bIdx >= c.buttons.length) continue;
      var b = c.buttons[bIdx];
      if (state.settingsDraft &&
          state.settingsDraft.slot === slot &&
          state.settingsDraft.isSub === c.isSub &&
          (!c.isSub || state.settingsDraft.homeSlot === state.editingSubpage)) {
        b = state.settingsDraft.button;
      }
      if (!buttonTypeInfoOnlyVisible(b.type || "")) {
        var hidden = document.createElement("div");
        hidden.className = "sp-empty-cell sp-info-only-hidden";
        hidden.setAttribute("data-pos", pos);
        main.appendChild(hidden);
        continue;
      }
      var iconName = resolveIcon(b);
      var label = b.label || b.entity || "Configure";
      var color = (b.type === "sensor" || b.type === "local_sensor" || b.type === "door_window" || b.type === "presence" || b.type === "weather" || b.type === "weather_forecast" || b.type === "calendar" || b.type === "clock" || b.type === "timezone")
        ? WEB_UI_COLORS.tertiary : WEB_UI_COLORS.secondary;
      var previewTypeDef = BUTTON_TYPES[b.type || ""] || null;
      if (previewTypeDef && c.isSub && !buttonTypeRegistryValue(previewTypeDef, "allowInSubpage", false)) {
        previewTypeDef = null;
      }
      var slotSz = c.sizes[slot];
      var typePreview = previewTypeDef && previewTypeDef.renderPreview
        ? previewTypeDef.renderPreview(b, { escHtml: escHtml, cardSize: slotSz || 1 })
        : null;

      var btn = document.createElement("div");
      btn.className = "sp-btn" +
        (typePreview && typePreview.buttonClass ? " " + typePreview.buttonClass : "") +
        sizeClass(slotSz) +
        (c.selected.indexOf(slot) !== -1 ? " sp-selected" : "");
      btn.style.backgroundColor = "#" + color;
      btn.draggable = !isConfigLocked();
      btn.setAttribute("data-pos", pos);
      btn.setAttribute("data-slot", slot);
      var hasWhenOn = !typePreview && (b.sensor || (b.icon_on && b.icon_on !== "Auto"));
      if (!typePreview && hasWhenOn && typeof cardOnPattern === "function" && cardOnPattern(b) === "stripes") {
        var onColor = state.onColor && state.onColor.length === 6 ? state.onColor : WEB_UI_COLORS.primary;
        btn.style.backgroundImage =
          "repeating-linear-gradient(135deg,#" + onColor + " 0,#" + onColor +
          " 12px,rgba(255,255,255,.22) 12px,rgba(255,255,255,.22) 20px)";
      }
      var badgeIcon = b.sensor ? "gauge" : "swap-horizontal";
      var sensorBadge = hasWhenOn
        ? '<span class="sp-sensor-badge mdi mdi-' + badgeIcon + '"></span>'
        : '';
      var labelHtml = previewHtmlValue(typePreview, "labelHtml",
        '<span class="sp-btn-label">' + escHtml(label) + '</span>');
      var iconHtml = previewHtmlValue(typePreview, "iconHtml",
        '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>');
      btn.innerHTML =
        sensorBadge +
        iconHtml +
        labelHtml;
      main.appendChild(btn);
    } else {
      var empty = document.createElement("div");
      empty.className = "sp-empty-cell";
      empty.setAttribute("data-pos", pos);
      empty.innerHTML = '<span class="sp-add-pill"><span class="sp-add-icon mdi mdi-plus"></span></span>';
      main.appendChild(empty);
    }
  }
  renderSelectionBar(c);
}
