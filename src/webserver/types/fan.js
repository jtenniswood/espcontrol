// Fan cards: experimental grouped controls for Home Assistant fan entities.

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
  panel.appendChild(helpers.selectField(
    "Type", helpers.idPrefix + "fan-control-type",
    FAN_CONTROL_TYPE_OPTIONS, normalizeFanControlType(b.type), function () {
      setFanControlType(b, this.value, helpers);
    }).field);
}

function fanTypeFactory(opts) {
  return {
    label: "Fans",
    allowInSubpage: true,
    hideLabel: true,
    pickerKey: opts.pickerKey,
    experimental: "developer",
    isAvailable: opts.hidden ? function () { return false; } : null,
    labelPlaceholder: "e.g. Bedroom Fan",
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

      panel.appendChild(helpers.entityField(
        "Fan Entity",
        helpers.idPrefix + "fan-entity",
        b.entity,
        "e.g. fan.bedroom",
        ["fan"],
        "entity",
        true,
        "Add a fan entity before saving."
      ).field);

      panel.appendChild(helpers.textField(
        "Label",
        helpers.idPrefix + "fan-label",
        b.label,
        "e.g. Bedroom Fan",
        "label",
        true
      ).field);

      if (b.type === "fan_switch") {
        panel.appendChild(helpers.iconPickerField(
          helpers.idPrefix + "fan-icon-picker", helpers.idPrefix + "fan-icon",
          b.icon || "Fan Off", function (opt) {
            b.icon = opt || "Fan Off";
            helpers.saveField("icon", b.icon);
          }, "Off Icon"
        ));
        panel.appendChild(helpers.iconPickerField(
          helpers.idPrefix + "fan-icon-on-picker", helpers.idPrefix + "fan-icon-on",
          b.icon_on || "Fan", function (opt) {
            b.icon_on = opt || "Fan";
            helpers.saveField("icon_on", b.icon_on);
          }, "On Icon"
        ));
      } else {
        panel.appendChild(helpers.iconPickerField(
          helpers.idPrefix + "fan-icon-picker", helpers.idPrefix + "fan-icon",
          b.icon || fanControlDefaultIcon(b.type), function (opt) {
            b.icon = opt || fanControlDefaultIcon(b.type);
            helpers.saveField("icon", b.icon);
          }, "Icon"
        ));
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
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
          '<span class="sp-type-badge mdi mdi-' + fanControlBadgeIcon(type) + '"></span></span>',
      };
    },
  };
}

registerButtonType("fan_speed", fanTypeFactory({ type: "fan_speed" }));
registerButtonType("fan_switch", fanTypeFactory({ type: "fan_switch", pickerKey: "fan_speed", hidden: true }));
registerButtonType("fan_oscillate", fanTypeFactory({ type: "fan_oscillate", pickerKey: "fan_speed", hidden: true }));
registerButtonType("fan_direction", fanTypeFactory({ type: "fan_direction", pickerKey: "fan_speed", hidden: true }));
registerButtonType("fan_preset", fanTypeFactory({ type: "fan_preset", pickerKey: "fan_speed", hidden: true }));
