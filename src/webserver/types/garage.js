// Garage door card: cover toggle or one-tap open/close commands.
function garageCommandMode(mode) {
  return mode === "open" || mode === "close";
}

function garageModeOptionValues() {
  var spec = cardContractOptionSpec("garage", "garage_mode");
  return spec && spec.values ? spec.values.slice() : ["", "open", "close"];
}

function normalizeGarageMode(mode) {
  mode = String(mode || "");
  return garageModeOptionValues().indexOf(mode) >= 0 ? mode : "";
}

function garageModeDefaultIcon(mode) {
  return mode === "open" ? "Garage Open" : "Garage";
}

function garageModeDefaultLabel(mode) {
  if (mode === "open") return "Open";
  if (mode === "close") return "Close";
  return "Garage Door";
}

function garageUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Garage" || icon === "Garage Open";
}

var GARAGE_CARD_METADATA = {
  mode: {
    label: "Interaction",
    idSuffix: "garage-interaction",
    options: [
      ["", "Toggle"],
      ["open", "Open"],
      ["close", "Close"],
    ],
    value: function (b) {
      return normalizeGarageMode(b.sensor);
    },
  },
  display: {
    label: "Display",
    options: [
      ["label", "Label"],
      ["status", "Status"],
    ],
  },
  entity: {
    label: "Entity",
    idSuffix: "entity",
    placeholder: "e.g. cover.garage_door",
    domains: function () { return cardContractDomains("garage"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add an entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "label",
    field: "label",
    rerender: true,
  },
  preview: {
    badge: "garage",
  },
};

registerButtonType("garage", {
  label: function () { return cardContractCardLabel("garage"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("garage"); },
  pickerKey: function () { return cardContractPickerKey("garage"); },
  hidden: function () { return cardContractHidden("garage"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("garage"); },
  cardMetadata: GARAGE_CARD_METADATA,
  onSelect: function (b) {
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Garage";
    b.icon_on = "Garage Open";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var mode = normalizeGarageMode(b.sensor);
    if (b.sensor !== mode) {
      b.sensor = mode;
      helpers.saveField("sensor", mode);
    }
    b.unit = "";
    b.precision = "";
    var normalizedOptions = normalizeGarageOptions(b.options, mode);
    if (b.options !== normalizedOptions) {
      b.options = normalizedOptions;
      helpers.saveField("options", normalizedOptions);
    }
    if (garageCommandMode(mode) && b.icon_on !== "Auto") {
      b.icon_on = "Auto";
      helpers.saveField("icon_on", "Auto");
    } else if (!garageCommandMode(mode) && (!b.icon_on || b.icon_on === "Auto")) {
      b.icon_on = "Garage Open";
      helpers.saveField("icon_on", "Garage Open");
    }

    helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, GARAGE_CARD_METADATA, {
      mode: Object.assign({}, GARAGE_CARD_METADATA.mode, {
        value: function () { return mode; },
        onChange: function () {
          var oldMode = mode;
          var hadDefaultIcon = garageUsesDefaultIcon(b.icon);
          mode = normalizeGarageMode(this.value);
          b.sensor = mode;
          helpers.saveField("sensor", mode);
          b.unit = "";
          b.precision = "";
          helpers.saveField("unit", "");
          helpers.saveField("precision", "");
          b.options = normalizeGarageOptions(b.options, mode);
          helpers.saveField("options", b.options);
          if (hadDefaultIcon || b.icon === garageModeDefaultIcon(oldMode)) {
            b.icon = garageModeDefaultIcon(mode);
            helpers.saveField("icon", b.icon);
          }
          if (garageCommandMode(mode)) {
            b.icon_on = "Auto";
          } else if (!b.icon_on || b.icon_on === "Auto") {
            b.icon_on = "Garage Open";
          }
          helpers.saveField("icon_on", b.icon_on);
          renderButtonSettings();
        },
      }),
    }));

    var labelHost = document.createElement("div");
    var labelControl = helpers.renderCardTextField(labelHost, b, helpers, Object.assign({}, GARAGE_CARD_METADATA.labelField, {
      placeholder: garageCommandMode(mode) ? "e.g. " + garageModeDefaultLabel(mode) + " Garage" : "e.g. Garage Door",
    }));

    function setLabelVisible(value) {
      labelControl.field.style.display = value === "label" ? "" : "none";
    }

    var labelMode = garageLabelDisplayMode(b);
    helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, GARAGE_CARD_METADATA.display, {
        value: function () { return labelMode; },
        onSelect: function (button, cardHelpers, value) {
          labelMode = value;
          setGarageLabelDisplayMode(button, value);
          cardHelpers.saveField("options", button.options);
          setLabelVisible(value);
          scheduleRender();
        },
      }),
    });
    setLabelVisible(labelMode);

    panel.appendChild(labelControl.field);

    helpers.renderCardEntityField(panel, b, helpers, GARAGE_CARD_METADATA);

    var closedIconVal = b.icon && b.icon !== "Auto" ? b.icon : "Garage";
    var iconOnVal = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : "Garage Open";
    if (garageCommandMode(mode)) {
      helpers.renderCardIconPicker(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        value: b.icon && b.icon !== "Auto" ? b.icon : garageModeDefaultIcon(mode),
        fallback: garageModeDefaultIcon(mode),
        label: "Icon",
      });
    } else {
      helpers.renderCardIconPair(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        value: closedIconVal,
        fallback: "Garage",
        label: "Closed Icon",
      }, {
        pickerIdSuffix: "icon-on-picker",
        idSuffix: "icon-on",
        field: "icon_on",
        value: iconOnVal,
        fallback: "Garage Open",
        label: "Open Icon",
      });
    }
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeGarageMode(b.sensor);
    var label = b.label || (garageCommandMode(mode) ? garageModeDefaultLabel(mode) : b.entity || "Garage Door");
    if (garageLabelDisplayMode(b) === "status") label = "Closed";
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: garageModeDefaultIcon(mode),
      badge: GARAGE_CARD_METADATA.preview.badge,
    });
  },
});
