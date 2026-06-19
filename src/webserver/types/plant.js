// Read-only plant card: displays Home Assistant plant status or one plant metric.
var PLANT_CARD_MODES = [
  ["status", "Status"],
  ["moisture", "Moisture"],
  ["battery", "Battery"],
  ["temperature", "Temperature"],
  ["conductivity", "Conductivity"],
  ["brightness", "Brightness"],
];

function plantModeValues() {
  var spec = cardContractOptionSpec("plant", "plant_mode");
  return spec && spec.values ? spec.values.slice() : PLANT_CARD_MODES.map(function (entry) { return entry[0]; });
}

function normalizePlantMode(mode) {
  mode = String(mode || "");
  return plantModeValues().indexOf(mode) >= 0 ? mode : "status";
}

function plantCardIsMetricMode(b) {
  return !!b && cardContractOptionSupportedFor("plant", "large_numbers", { precision: b.precision });
}

function plantModeBadgeIcon(mode) {
  mode = normalizePlantMode(mode);
  if (mode === "moisture") return "water-percent";
  if (mode === "battery") return "battery";
  if (mode === "temperature") return "thermometer";
  if (mode === "conductivity") return "flash";
  if (mode === "brightness") return "weather-sunny";
  return "leaf";
}

function plantModeLabel(mode) {
  mode = normalizePlantMode(mode);
  for (var i = 0; i < PLANT_CARD_MODES.length; i++) {
    if (PLANT_CARD_MODES[i][0] === mode) return PLANT_CARD_MODES[i][1];
  }
  return "Status";
}

function plantMetricPreview(mode) {
  mode = normalizePlantMode(mode);
  if (mode === "moisture") return ["45", "%"];
  if (mode === "battery") return ["78", "%"];
  if (mode === "temperature") return ["21", temperatureUnitSymbol()];
  if (mode === "conductivity") return ["350", "\u00B5S/cm"];
  if (mode === "brightness") return ["1200", "lx"];
  return ["OK", ""];
}

var PLANT_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "plant-type",
    options: PLANT_CARD_MODES,
    value: function (b) {
      return normalizePlantMode(b.precision);
    },
    onChange: function (b, helpers) {
      b.precision = normalizePlantMode(this.value);
      helpers.saveField("precision", b.precision);
    },
  },
  entity: {
    label: "Plant Entity",
    idSuffix: "plant-entity",
    placeholder: "e.g. plant.monstera",
    domains: function () { return cardContractDomains("plant"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a plant entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "plant-label",
    field: "label",
    placeholder: "e.g. Monstera",
    rerender: true,
  },
  largeNumbers: {
    label: "Large Metric Numbers",
    idSuffix: "large-plant-numbers",
    supported: plantCardIsMetricMode,
  },
};

registerButtonType("plant", {
  label: function () { return cardContractCardLabel("plant"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("plant"); },
  pickerKey: function () { return cardContractPickerKey("plant"); },
  hidden: function () { return cardContractHidden("plant"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("plant"); },
  cardMetadata: PLANT_CARD_METADATA,
  onSelect: function (b) {
    b.icon = "Leaf";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.options = "";
    b.precision = normalizePlantMode(b.precision);
  },
  renderSettings: function (panel, b, slot, helpers) {
    var modeField = helpers.renderCardModeSelector(panel, b, helpers, PLANT_CARD_METADATA);
    var modeSelect = modeField.select;

    helpers.renderCardEntityField(panel, b, helpers, PLANT_CARD_METADATA);
    helpers.renderCardTextField(panel, b, helpers, PLANT_CARD_METADATA.labelField);
    var largeNumbersToggle = helpers.renderCardLargeNumbersToggle(panel, b, helpers, PLANT_CARD_METADATA);

    function syncMetricFields() {
      helpers.syncCardLargeNumbersToggle(largeNumbersToggle, b, helpers, plantCardIsMetricMode(b));
    }

    modeSelect.addEventListener("change", syncMetricFields);
    syncMetricFields();
  },
  renderPreview: function (b, helpers) {
    var mode = normalizePlantMode(b.precision);
    var label = b.label || b.entity || "Plant";
    var badge = plantModeBadgeIcon(mode);
    if (plantCardIsMetricMode({ precision: mode })) {
      var metric = plantMetricPreview(mode);
      return {
        iconHtml: cardSensorPreviewHtml(b, helpers, metric[0], metric[1]),
        labelHtml: cardBadgeLabelHtml(helpers, label || plantModeLabel(mode), badge),
      };
    }
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-leaf"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, label, badge),
    };
  },
});
