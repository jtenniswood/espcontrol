// Vacuum card: touchscreen-friendly controls for Home Assistant vacuum entities.
var VACUUM_CARD_MODES = [
  ["status", "Status"],
  ["start_stop", "Start / Stop"],
  ["dock", "Dock"],
  ["pause_resume", "Pause / Resume"],
  ["clean_spot", "Spot Clean"],
  ["locate", "Locate"],
  ["clean_area", "Clean Area"],
];

function vacuumModeValues() {
  var spec = cardContractOptionSpec("vacuum", "vacuum_mode");
  return spec && spec.values ? spec.values.slice() : VACUUM_CARD_MODES.map(function (entry) { return entry[0]; });
}

function normalizeVacuumMode(mode) {
  mode = String(mode || "");
  return vacuumModeValues().indexOf(mode) >= 0 ? mode : "start_stop";
}

function vacuumModeNeedsArea(mode) {
  return normalizeVacuumMode(mode) === "clean_area";
}

function vacuumModeDefaultIcon(mode) {
  mode = normalizeVacuumMode(mode);
  if (mode === "dock") return "Robot Vacuum Variant";
  if (mode === "clean_spot") return "Vacuum";
  if (mode === "locate") return "Robot Vacuum Alert";
  if (mode === "clean_area") return "Vacuum Outline";
  return "Robot Vacuum";
}

function vacuumModeBadgeIcon(mode) {
  mode = normalizeVacuumMode(mode);
  if (mode === "dock") return "home-import-outline";
  if (mode === "pause_resume") return "play-pause";
  if (mode === "clean_spot") return "vacuum";
  if (mode === "locate") return "map-marker-question";
  if (mode === "clean_area") return "map-marker-path";
  return "robot-vacuum";
}

function vacuumUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" ||
    icon === "Robot Vacuum" ||
    icon === "Robot Vacuum Alert" ||
    icon === "Robot Vacuum Off" ||
    icon === "Robot Vacuum Variant" ||
    icon === "Robot Vacuum Variant Alert" ||
    icon === "Robot Vacuum Variant Off" ||
    icon === "Vacuum" ||
    icon === "Vacuum Outline";
}

function normalizeVacuumConfig(b) {
  if (!b) return;
  b.sensor = normalizeVacuumMode(b.sensor);
  if (!vacuumModeNeedsArea(b.sensor)) b.unit = "";
  b.precision = "";
  b.options = "";
  b.icon_on = "Auto";
  if (!b.icon || b.icon === "Auto") b.icon = vacuumModeDefaultIcon(b.sensor);
}

var VACUUM_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "vacuum-type",
    options: VACUUM_CARD_MODES,
    value: function (b) {
      return normalizeVacuumMode(b.sensor);
    },
  },
  entity: {
    label: "Vacuum Entity",
    idSuffix: "vacuum-entity",
    placeholder: "e.g. vacuum.kitchen",
    domains: function () { return cardContractDomains("vacuum"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a vacuum entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "vacuum-label",
    field: "label",
    rerender: true,
  },
};

registerButtonType("vacuum", {
  label: function () { return cardContractCardLabel("vacuum"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("vacuum"); },
  pickerKey: function () { return cardContractPickerKey("vacuum"); },
  hidden: function () { return cardContractHidden("vacuum"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("vacuum"); },
  cardMetadata: VACUUM_CARD_METADATA,
  onSelect: function (b) {
    b.label = "";
    b.sensor = "start_stop";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon = "Robot Vacuum";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var mode = normalizeVacuumMode(b.sensor);
    if (b.sensor !== mode) {
      b.sensor = mode;
      helpers.saveField("sensor", mode);
    }
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!vacuumModeNeedsArea(mode) && b.unit) {
      b.unit = "";
      helpers.saveField("unit", "");
    }
    if (!b.icon || b.icon === "Auto") {
      b.icon = vacuumModeDefaultIcon(mode);
      helpers.saveField("icon", b.icon);
    }

    helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, VACUUM_CARD_METADATA, {
      mode: Object.assign({}, VACUUM_CARD_METADATA.mode, {
        value: function () { return mode; },
        onChange: function () {
          var oldMode = mode;
          var hadDefaultIcon = vacuumUsesDefaultIcon(b.icon);
          mode = normalizeVacuumMode(this.value);
          b.sensor = mode;
          helpers.saveField("sensor", mode);
          b.precision = "";
          b.options = "";
          b.icon_on = "Auto";
          helpers.saveField("precision", "");
          helpers.saveField("options", "");
          helpers.saveField("icon_on", "Auto");
          if (!vacuumModeNeedsArea(mode)) {
            b.unit = "";
            helpers.saveField("unit", "");
          }
          if (hadDefaultIcon || b.icon === vacuumModeDefaultIcon(oldMode)) {
            b.icon = vacuumModeDefaultIcon(mode);
            helpers.saveField("icon", b.icon);
          }
          renderButtonSettings();
        },
      }),
    }));

    helpers.renderCardEntityField(panel, b, helpers, VACUUM_CARD_METADATA);

    helpers.renderCardTextField(panel, b, helpers, Object.assign({}, VACUUM_CARD_METADATA.labelField, {
      placeholder: mode === "clean_area" ? "e.g. Clean Kitchen" : "e.g. Kitchen Vacuum",
    }));

    if (vacuumModeNeedsArea(mode)) {
      helpers.renderCardTextField(panel, b, helpers, {
        label: "Area ID",
        idSuffix: "vacuum-area-id",
        field: "unit",
        placeholder: "e.g. kitchen",
        rerender: false,
      });
    }

    helpers.renderCardIconPicker(panel, b, helpers, {
      pickerIdSuffix: "vacuum-icon-picker",
      idSuffix: "vacuum-icon",
      field: "icon",
      fallback: function () { return vacuumModeDefaultIcon(mode); },
      label: "Icon",
    });
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeVacuumMode(b.sensor);
    var label = b.label || b.entity || "Vacuum";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(vacuumModeDefaultIcon(mode));
    var stateBadge = mode === "status" ? '<span class="sp-sensor-badge mdi mdi-format-text"></span>' : "";
    return {
      iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, label, vacuumModeBadgeIcon(mode)),
    };
  },
});
