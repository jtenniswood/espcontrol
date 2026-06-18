// Fan cards: grouped controls for Home Assistant fan entities.

var FAN_CONTROL_TYPE_OPTIONS = [
  ["fan_switch", "Switch"],
  ["fan_speed", "Speed"],
  ["fan_oscillate", "Oscillation"],
  ["fan_direction", "Direction"],
  ["fan_preset", "Preset"],
];

function normalizeFanControlType(type) {
  if (type === "fan_switch" ||
      type === "fan_oscillate" ||
      type === "fan_direction" ||
      type === "fan_preset") return type;
  return "fan_speed";
}

function fanControlDefaultIcon(type) {
  if (type === "fan_switch") return "Fan Off";
  if (type === "fan_oscillate") return "Fan";
  if (type === "fan_direction") return "Swap Horizontal";
  if (type === "fan_preset") return "Fan Auto";
  return "Fan Speed 2";
}

function fanControlBadgeIcon(type) {
  if (type === "fan_switch") return "fan";
  if (type === "fan_oscillate") return "sync";
  if (type === "fan_direction") return "swap-horizontal";
  if (type === "fan_preset") return "fan-auto";
  return "fan-speed-2";
}

var FAN_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "fan-control-type",
    options: FAN_CONTROL_TYPE_OPTIONS,
    value: function (b) {
      return normalizeFanControlType(b.type);
    },
  },
  entity: {
    label: "Fan Entity",
    idSuffix: "fan-entity",
    placeholder: "e.g. fan.bedroom",
    domains: ["fan"],
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a fan entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "fan-label",
    field: "label",
    placeholder: "e.g. Bedroom Fan",
    rerender: true,
  },
};

function setFanControlType(b, type, helpers) {
  var nextType = normalizeFanControlType(type);
  if (b.type === nextType) return;
  b.type = nextType;
  var td = BUTTON_TYPES[nextType];
  if (td && td.onSelect) td.onSelect(b);
  helpers.saveField("type", nextType);
  helpers.saveField("sensor", b.sensor || "");
  helpers.saveField("unit", b.unit || "");
  helpers.saveField("precision", b.precision || "");
  helpers.saveField("options", b.options || "");
  helpers.saveField("icon", b.icon || "Auto");
  helpers.saveField("icon_on", b.icon_on || "Auto");
  renderButtonSettings();
}

function renderFanControlTypeField(panel, b, helpers) {
  helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, FAN_CARD_METADATA, {
    mode: Object.assign({}, FAN_CARD_METADATA.mode, {
      onChange: function () {
      setFanControlType(b, this.value, helpers);
      },
    }),
  }));
}

function fanTypeFactory(opts) {
  return {
    label: "Fans",
    allowInSubpage: true,
    hideLabel: true,
    pickerKey: opts.pickerKey,
    isAvailable: opts.hidden ? function () { return false; } : null,
    labelPlaceholder: "e.g. Bedroom Fan",
    cardMetadata: FAN_CARD_METADATA,
    onSelect: function (b) {
      b.sensor = "";
      b.unit = "";
      b.precision = "";
      b.options = "";
      b.icon = fanControlDefaultIcon(opts.type);
      b.icon_on = opts.type === "fan_switch" ? "Fan" : "Auto";
    },
    renderSettings: function (panel, b, slot, helpers) {
      b.sensor = "";
      b.unit = "";
      b.precision = "";
      b.options = "";
      if (!b.icon || b.icon === "Auto") b.icon = fanControlDefaultIcon(b.type);
      if (b.type === "fan_switch") {
        if (!b.icon_on || b.icon_on === "Auto") b.icon_on = "Fan";
      } else if (b.icon_on !== "Auto") {
        b.icon_on = "Auto";
        helpers.saveField("icon_on", "Auto");
      }

      renderFanControlTypeField(panel, b, helpers);

      helpers.renderCardEntityField(panel, b, helpers, FAN_CARD_METADATA);

      helpers.renderCardTextField(panel, b, helpers, FAN_CARD_METADATA.labelField);

      if (b.type === "fan_switch") {
        helpers.renderCardIconPicker(panel, b, helpers, {
          pickerIdSuffix: "fan-icon-picker",
          idSuffix: "fan-icon",
          field: "icon",
          fallback: "Fan Off",
          label: "Off Icon",
        });
        helpers.renderCardIconPicker(panel, b, helpers, {
          pickerIdSuffix: "fan-icon-on-picker",
          idSuffix: "fan-icon-on",
          field: "icon_on",
          fallback: "Fan",
          label: "On Icon",
        });
      } else {
        helpers.renderCardIconPicker(panel, b, helpers, {
          pickerIdSuffix: "fan-icon-picker",
          idSuffix: "fan-icon",
          field: "icon",
          fallback: function () { return fanControlDefaultIcon(b.type); },
          label: "Icon",
        });
      }
    },
    renderPreview: function (b, helpers) {
      var type = normalizeFanControlType(b.type);
      var label = b.label || b.entity || "Fan";
      var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(fanControlDefaultIcon(type));
      var iconHtml = '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>';
      if (type === "fan_speed") {
        iconHtml +=
          '<span class="sp-slider-preview"><span class="sp-slider-track">' +
            '<span class="sp-slider-fill"></span>' +
          '</span></span>';
      }
      return {
        iconHtml: iconHtml,
        labelHtml: cardBadgeLabelHtml(helpers, label, fanControlBadgeIcon(type)),
      };
    },
  };
}

registerButtonType("fan_speed", fanTypeFactory({ type: "fan_speed" }));
registerButtonType("fan_switch", fanTypeFactory({ type: "fan_switch", pickerKey: "fan_speed", hidden: true }));
registerButtonType("fan_oscillate", fanTypeFactory({ type: "fan_oscillate", pickerKey: "fan_speed", hidden: true }));
registerButtonType("fan_direction", fanTypeFactory({ type: "fan_direction", pickerKey: "fan_speed", hidden: true }));
registerButtonType("fan_preset", fanTypeFactory({ type: "fan_preset", pickerKey: "fan_speed", hidden: true }));
