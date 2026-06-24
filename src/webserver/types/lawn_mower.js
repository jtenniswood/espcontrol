// Lawn Mower card: touchscreen-friendly controls for Home Assistant mower entities.
var LAWN_MOWER_CARD_MODES = [
  ["status", "Status"],
  ["control_panel", "Control Panel"],
  ["start_mowing", "Start Mowing"],
  ["dock", "Dock"],
  ["pause_resume", "Pause / Resume"],
];

function lawnMowerModeValues() {
  return entityModeValues("lawn_mower", "lawn_mower_mode", LAWN_MOWER_CARD_MODES);
}

function normalizeLawnMowerMode(mode) {
  return normalizeEntityMode(mode, lawnMowerModeValues(), "start_mowing");
}

function lawnMowerModeDefaultIcon(mode) {
  mode = normalizeLawnMowerMode(mode);
  if (mode === "dock") return "Robot Mower Outline";
  return "Robot Mower";
}

function lawnMowerModeBadgeIcon(mode) {
  mode = normalizeLawnMowerMode(mode);
  if (mode === "status") return "format-text";
  if (mode === "control_panel") return "tune-vertical";
  if (mode === "dock") return "home-import-outline";
  if (mode === "pause_resume") return "play-pause";
  return "robot-mower";
}

function lawnMowerUsesDefaultIcon(icon) {
  return entityModeCardUsesDefaultIcon(icon, [
    "Lawnmower",
    "Robot Mower",
    "Robot Mower Outline",
  ]);
}

function normalizeLawnMowerConfig(b) {
  normalizeEntityModeCardConfig(b, {
    normalizeMode: normalizeLawnMowerMode,
    defaultIcon: lawnMowerModeDefaultIcon,
  });
}

var LAWN_MOWER_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "lawn-mower-type",
    options: LAWN_MOWER_CARD_MODES,
    value: function (b) {
      return normalizeLawnMowerMode(b.sensor);
    },
  },
  entity: {
    label: "Lawn Mower Entity",
    idSuffix: "lawn-mower-entity",
    placeholder: "e.g. lawn_mower.backyard",
    domains: function () { return cardContractDomains("lawn_mower"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a lawn mower entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "lawn-mower-label",
    field: "label",
    placeholder: "e.g. Backyard Mower",
    rerender: true,
  },
};

registerButtonType("lawn_mower", {
  label: function () { return cardContractCardLabel("lawn_mower"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("lawn_mower"); },
  pickerKey: function () { return cardContractPickerKey("lawn_mower"); },
  hidden: function () { return cardContractHidden("lawn_mower"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("lawn_mower"); },
  cardMetadata: LAWN_MOWER_CARD_METADATA,
  normalizeConfig: normalizeLawnMowerConfig,
  onSelect: function (b) {
    var defaults = cardContractDefaultConfig("lawn_mower");
    Object.keys(defaults).forEach(function (key) {
      if (key !== "entity") b[key] = defaults[key];
    });
  },
  renderSettings: function (panel, b, slot, helpers) {
    var mode = normalizeLawnMowerMode(b.sensor);
    if (b.sensor !== mode) {
      b.sensor = mode;
      helpers.saveField("sensor", mode);
    }
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") {
      b.icon = lawnMowerModeDefaultIcon(mode);
      helpers.saveField("icon", b.icon);
    }

    helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, LAWN_MOWER_CARD_METADATA, {
      mode: Object.assign({}, LAWN_MOWER_CARD_METADATA.mode, {
        value: function () { return mode; },
        onChange: function () {
          var oldMode = mode;
          mode = normalizeLawnMowerMode(this.value);
          applyEntityModeCardModeChange(b, helpers, oldMode, mode, {
            defaultIcon: lawnMowerModeDefaultIcon,
            usesDefaultIcon: lawnMowerUsesDefaultIcon,
          });
          renderButtonSettings();
        },
      }),
    }));

    helpers.renderCardEntityField(panel, b, helpers, LAWN_MOWER_CARD_METADATA);
    helpers.renderCardTextField(panel, b, helpers, LAWN_MOWER_CARD_METADATA.labelField);
    helpers.renderCardIconPicker(panel, b, helpers, {
      pickerIdSuffix: "lawn-mower-icon-picker",
      idSuffix: "lawn-mower-icon",
      field: "icon",
      fallback: function () { return lawnMowerModeDefaultIcon(mode); },
      label: "Icon",
    });
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeLawnMowerMode(b.sensor);
    var label = b.label || b.entity || "Lawn Mower";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(lawnMowerModeDefaultIcon(mode));
    var stateBadge = mode === "status" ? '<span class="sp-sensor-badge mdi mdi-format-text"></span>' : "";
    return {
      iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, label, lawnMowerModeBadgeIcon(mode)),
    };
  },
});
