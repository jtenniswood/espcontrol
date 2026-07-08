// ── Preview rendering (unified) ────────────────────────────────────────
// @web-module-requires: state, screen_rotation_state, grid, config_codec, controls, controls_fields

function previewHtmlValue(typePreview, key, fallback) {
  return typePreview && Object.prototype.hasOwnProperty.call(typePreview, key)
    ? typePreview[key]
    : fallback;
}

function buttonTypeRegistryValue(typeDef, key, fallback) {
  if (!typeDef || !Object.prototype.hasOwnProperty.call(typeDef, key)) return fallback;
  var value = typeDef[key];
  if (typeof value === "function") value = value();
  return value == null ? fallback : value;
}

function buttonTypeDisabledForDevice(key) {
  var disabled = CFG.disabledCardTypes || [];
  return disabled.indexOf(key || "") !== -1;
}

function buttonTypeInfoOnlyVisible(key) {
  if (!CFG.infoOnly) return true;
  return [
    "sensor",
    "calendar",
    "clock",
    "door_window",
    "image",
    "local_sensor",
    "presence",
    "timezone",
    "weather",
    "weather_forecast",
  ].indexOf(key || "") !== -1;
}

var CARD_TYPE_PICKER_DETAILS = {
  "": { icon: "toggle-switch", description: "Toggle lights, switches, helpers, or fans." },
  action: { icon: "flash", description: "Run a Home Assistant or local action." },
  alarm: { icon: "shield-home", description: "Control or trigger alarm panel actions." },
  calendar: { icon: "calendar-clock", description: "Show date, time, or world clock values." },
  climate: { icon: "thermostat", description: "Show climate status and temperature controls." },
  cover: { icon: "window-shutter", description: "Control blinds, curtains, or covers." },
  door_window: { icon: "door-open", description: "Show open or closed sensor state." },
  presence: { icon: "account", description: "Show person or presence status." },
  fan_speed: { icon: "fan", description: "Control fan speed, mode, or direction." },
  garage: { icon: "garage", description: "Show and control a garage door." },
  gate: { icon: "gate", description: "Show and control a gate." },
  image: { icon: "image", description: "Display an image card where supported." },
  internal: { icon: "power-plug", description: "Control built-in device relays." },
  light_brightness: { icon: "lightbulb", description: "Configure light switch, brightness, or temperature controls." },
  lawn_mower: { icon: "robot-mower", description: "Show or control a robotic lawn mower." },
  local_sensor: { icon: "gauge", description: "Show a sensor value from this device." },
  lock: { icon: "lock", description: "Show and control a lock." },
  media: { icon: "speaker", description: "Control media playback or volume." },
  media_control: { icon: "music", description: "Open all media controls and volume in a modal." },
  media_cover_art: { icon: "album", description: "Show the current track artwork." },
  push: { icon: "gesture-tap-button", description: "Fire a momentary button event." },
  sensor: { icon: "gauge", description: "Display sensor values or states." },
  slider: { icon: "tune-vertical", description: "Adjust a numeric or brightness value." },
  subpage: { icon: "view-grid-plus", description: "Open a nested page of cards." },
  webhook: { icon: "webhook", description: "Send a direct HTTP request." },
  vacuum: { icon: "robot-vacuum", description: "Show or control a vacuum cleaner." },
  weather: { icon: "weather-partly-cloudy", description: "Show weather or forecast data." },
};

var CARD_TYPE_PICKER_DEFAULTS = {
  climate: "climate_control",
  light_brightness: "light_control",
  media_control: "media",
  media_cover_art: "media",
};

function defaultButtonTypeForPicker(key) {
  return Object.prototype.hasOwnProperty.call(CARD_TYPE_PICKER_DEFAULTS, key)
    ? CARD_TYPE_PICKER_DEFAULTS[key]
    : key;
}

function buttonTypePickerDetails(key, label) {
  var details = CARD_TYPE_PICKER_DETAILS[key || ""] || {};
  return {
    icon: details.icon || "card-outline",
    description: details.description || ("Configure a " + (label || "card") + " card."),
  };
}

function buttonTypePickerOptionList(isSub, selectedTypeKey) {
  var typeOpts = [];
  var selectedUnsupported = null;
  var hasSelectedType = selectedTypeKey !== null && selectedTypeKey !== undefined;
  for (var k in BUTTON_TYPES) {
    var td = BUTTON_TYPES[k];
    var typeKey = k;
    var pickerKey = buttonTypeRegistryValue(td, "pickerKey", "");
    var allowInSubpage = !!buttonTypeRegistryValue(td, "allowInSubpage", false);
    var label = buttonTypeRegistryValue(td, "label", td.key || "Toggle");
    if (buttonTypeDisabledForDevice(typeKey) || buttonTypeDisabledForDevice(pickerKey)) continue;
    if (!buttonTypeInfoOnlyVisible(typeKey) || (pickerKey && !buttonTypeInfoOnlyVisible(pickerKey))) {
      if (hasSelectedType && (selectedTypeKey === typeKey || (pickerKey && selectedTypeKey === pickerKey))) {
        selectedUnsupported = { key: selectedTypeKey, label: label };
      }
      continue;
    }
    if (pickerKey && pickerKey !== typeKey) continue;
    if (isSub && !allowInSubpage) continue;
    if (td.isAvailable && !td.isAvailable({ isSub: isSub }) && selectedTypeKey !== typeKey) continue;
    typeOpts.push(Object.assign({
      key: typeKey,
      label: label,
      disabled: false,
    }, buttonTypePickerDetails(typeKey, label)));
  }
  if (selectedUnsupported) {
    var unsupportedLabel = selectedUnsupported.label + " (not available)";
    typeOpts.push(Object.assign({
      key: selectedUnsupported.key,
      label: unsupportedLabel,
      disabled: true,
    }, buttonTypePickerDetails(selectedUnsupported.key, unsupportedLabel)));
  }
  typeOpts.sort(function (a, b) {
    return a.label.localeCompare(b.label);
  });
  return typeOpts;
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
