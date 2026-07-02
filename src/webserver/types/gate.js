// Gate card: cover toggle or one-tap open/close/stop commands.
function gateCommandMode(mode) {
  return mode === "open" || mode === "close" || mode === "stop";
}

function gateModeOptionValues() {
  var spec = cardContractOptionSpec("gate", "gate_mode");
  return spec && spec.values ? spec.values.slice() : ["", "open", "close", "stop"];
}

function normalizeGateMode(mode) {
  mode = String(mode || "");
  return gateModeOptionValues().indexOf(mode) >= 0 ? mode : "";
}

function gateModeDefaultIcon(mode) {
  if (mode === "open") return "Gate Open";
  if (mode === "stop") return "Stop";
  return "Gate";
}

function gateModeDefaultLabel(mode) {
  if (mode === "open") return "Open";
  if (mode === "close") return "Close";
  if (mode === "stop") return "Stop";
  return "Gate";
}

function gateUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Gate" || icon === "Gate Open" || icon === "Stop";
}

var GATE_CARD_METADATA = {
  mode: {
    label: "Interaction",
    idSuffix: "gate-interaction",
    options: [
      ["", "Toggle"],
      ["open", "Open"],
      ["close", "Close"],
      ["stop", "Stop"],
    ],
    value: function (b) {
      return normalizeGateMode(b.sensor);
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
    placeholder: "e.g. cover.driveway_gate",
    domains: function () { return cardContractDomains("gate"); },
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
    badge: "gate",
  },
};

registerButtonType("gate", {
  label: function () { return cardContractCardLabel("gate"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("gate"); },
  pickerKey: function () { return cardContractPickerKey("gate"); },
  hidden: function () { return cardContractHidden("gate"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("gate"); },
  cardMetadata: GATE_CARD_METADATA,
  onSelect: function (b) {
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Gate";
    b.icon_on = "Gate Open";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var mode = normalizeGateMode(b.sensor);
    if (b.sensor !== mode) {
      b.sensor = mode;
      helpers.saveField("sensor", mode);
    }
    b.unit = "";
    b.precision = "";
    var normalizedOptions = normalizeGateOptions(b.options, mode);
    if (b.options !== normalizedOptions) {
      b.options = normalizedOptions;
      helpers.saveField("options", normalizedOptions);
    }
    if (gateCommandMode(mode) && b.icon_on !== "Auto") {
      b.icon_on = "Auto";
      helpers.saveField("icon_on", "Auto");
    } else if (!gateCommandMode(mode) && (!b.icon_on || b.icon_on === "Auto")) {
      b.icon_on = "Gate Open";
      helpers.saveField("icon_on", "Gate Open");
    }

    helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, GATE_CARD_METADATA, {
      mode: Object.assign({}, GATE_CARD_METADATA.mode, {
        value: function () { return mode; },
        onChange: function () {
          var oldMode = mode;
          var hadDefaultIcon = gateUsesDefaultIcon(b.icon);
          mode = normalizeGateMode(this.value);
          b.sensor = mode;
          helpers.saveField("sensor", mode);
          b.unit = "";
          b.precision = "";
          helpers.saveField("unit", "");
          helpers.saveField("precision", "");
          b.options = normalizeGateOptions(b.options, mode);
          helpers.saveField("options", b.options);
          if (hadDefaultIcon || b.icon === gateModeDefaultIcon(oldMode)) {
            b.icon = gateModeDefaultIcon(mode);
            helpers.saveField("icon", b.icon);
          }
          if (gateCommandMode(mode)) {
            b.icon_on = "Auto";
          } else if (!b.icon_on || b.icon_on === "Auto") {
            b.icon_on = "Gate Open";
          }
          helpers.saveField("icon_on", b.icon_on);
          renderButtonSettings();
        },
      }),
    }));

    var labelHost = document.createElement("div");
    var labelControl = helpers.renderCardTextField(labelHost, b, helpers, Object.assign({}, GATE_CARD_METADATA.labelField, {
      placeholder: gateCommandMode(mode) ? "e.g. " + gateModeDefaultLabel(mode) + " Gate" : "e.g. Gate",
    }));

    function setLabelVisible(value) {
      labelControl.field.style.display = value === "label" ? "" : "none";
    }

    var labelMode = gateLabelDisplayMode(b);
    helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, GATE_CARD_METADATA.display, {
        value: function () { return labelMode; },
        onSelect: function (button, cardHelpers, value) {
          labelMode = value;
          setGateLabelDisplayMode(button, value);
          cardHelpers.saveField("options", button.options);
          setLabelVisible(value);
          scheduleRender();
        },
      }),
    });
    setLabelVisible(labelMode);

    panel.appendChild(labelControl.field);

    helpers.renderCardEntityField(panel, b, helpers, GATE_CARD_METADATA);

    var closedIconVal = b.icon && b.icon !== "Auto" ? b.icon : "Gate";
    var iconOnVal = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : "Gate Open";
    if (gateCommandMode(mode)) {
      helpers.renderCardIconPicker(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        value: b.icon && b.icon !== "Auto" ? b.icon : gateModeDefaultIcon(mode),
        fallback: gateModeDefaultIcon(mode),
        label: "Icon",
      });
    } else {
      helpers.renderCardIconPair(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        value: closedIconVal,
        fallback: "Gate",
        label: "Closed Icon",
      }, {
        pickerIdSuffix: "icon-on-picker",
        idSuffix: "icon-on",
        field: "icon_on",
        value: iconOnVal,
        fallback: "Gate Open",
        label: "Open Icon",
      });
    }
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeGateMode(b.sensor);
    var label = b.label || (gateCommandMode(mode) ? gateModeDefaultLabel(mode) : b.entity || "Gate");
    if (gateLabelDisplayMode(b) === "status") label = "Closed";
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: gateModeDefaultIcon(mode),
      badge: GATE_CARD_METADATA.preview.badge,
    });
  },
});
