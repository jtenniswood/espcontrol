// Read-only door/window card: shows a binary sensor with subtype-specific icons.
var DOOR_WINDOW_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "door-window-type",
    options: [
      ["door", "Door"],
      ["window", "Window"],
    ],
    value: function (b) {
      return normalizeDoorWindowSubtype(b.precision);
    },
  },
  entity: {
    label: "Sensor Entity",
    idSuffix: "sensor",
    placeholder: "e.g. binary_sensor.patio_door",
    domains: function () { return cardContractDomains("door_window"); },
    bindName: "sensor",
    rerender: true,
    requiredMessage: "Add a door or window sensor before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "label",
    field: "label",
    placeholder: "e.g. Patio Door",
    rerender: true,
  },
  activeColor: {
    label: "Lit When Open",
    idSuffix: "door-window-active-color",
    checked: doorWindowActiveColorEnabled,
  },
};

registerButtonType("door_window", {
  label: function () { return cardContractCardLabel("door_window"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("door_window"); },
  pickerKey: function () { return cardContractPickerKey("door_window"); },
  hidden: function () { return cardContractHidden("door_window"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("door_window"); },
  cardMetadata: DOOR_WINDOW_CARD_METADATA,
  onSelect: function (b) {
    var defaults = cardContractDefaultConfig("door_window");
    Object.keys(defaults).forEach(function (key) { b[key] = defaults[key]; });
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.entity = "";
    b.unit = "";
    b.precision = normalizeDoorWindowSubtype(b.precision);
    b.options = normalizeDoorWindowOptions(b.options);
    if (!b.icon || b.icon === "Auto") b.icon = doorWindowClosedIcon(b.precision);
    if (!b.icon_on || b.icon_on === "Auto") b.icon_on = doorWindowOpenIcon(b.precision);

    var subtypeField = helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, DOOR_WINDOW_CARD_METADATA, {
      mode: Object.assign({}, DOOR_WINDOW_CARD_METADATA.mode, {
        onChange: function () {
          setSubtype(this.value, true);
        },
      }),
    }));
    var subtypeSelect = subtypeField.select;

    helpers.renderBasicCardFields(panel, b, helpers, DOOR_WINDOW_CARD_METADATA);

    var iconPickers = helpers.renderCardIconPair(panel, b, helpers, {
      pickerIdSuffix: "closed-icon-picker",
      idSuffix: "icon",
      field: "icon",
      label: "Closed Icon",
      fallback: function () { return doorWindowClosedIcon(b.precision); },
    }, {
      pickerIdSuffix: "open-icon-picker",
      idSuffix: "icon-on",
      field: "icon_on",
      label: "Open Icon",
      fallback: function () { return doorWindowOpenIcon(b.precision); },
    });
    var closedIconPicker = iconPickers.off;
    var openIconPicker = iconPickers.on;

    helpers.renderCardActiveColorToggle(panel, b, helpers,
      DOOR_WINDOW_CARD_METADATA.activeColor, setDoorWindowActiveColorEnabled);

    function syncIconPicker(picker, value) {
      var preview = picker.querySelector(".sp-icon-picker-preview");
      if (preview) preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(value);
      var input = picker.querySelector(".sp-icon-picker-input");
      if (input) input.value = value;
    }

    function setSubtype(value, persist) {
      var previousClosed = doorWindowClosedIcon(b.precision);
      var previousOpen = doorWindowOpenIcon(b.precision);
      b.precision = normalizeDoorWindowSubtype(value);
      subtypeSelect.value = b.precision;

      if (!b.icon || b.icon === "Auto" || b.icon === previousClosed) {
        b.icon = doorWindowClosedIcon(b.precision);
        syncIconPicker(closedIconPicker, b.icon);
      }
      if (!b.icon_on || b.icon_on === "Auto" || b.icon_on === previousOpen) {
        b.icon_on = doorWindowOpenIcon(b.precision);
        syncIconPicker(openIconPicker, b.icon_on);
      }
      if (!persist) return;
      helpers.saveField("precision", b.precision);
      helpers.saveField("icon", b.icon);
      helpers.saveField("icon_on", b.icon_on);
    }

    setSubtype(b.precision, false);
  },
  renderPreview: function (b, helpers) {
    var subtype = normalizeDoorWindowSubtype(b.precision);
    var label = b.label || b.sensor || (subtype === "window" ? "Window" : "Door");
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: doorWindowClosedIcon(subtype),
      badge: subtype === "window" ? "window-closed" : "door",
    });
  },
});
