// Solar card: energy overview that stores mode + six sensor entities in its
// options string (mode=live,production=sensor.x,...). The main entity field
// stays empty; everything lives in `options`.
var SOLAR_CARD_METADATA = {
  modeSegment: {
    label: "Mode",
    options: [
      ["live", "Live"],
      ["today", "Today"],
    ],
  },
  // Each entry maps an option key to its labelled entity field.
  entityFields: [
    { key: "production", label: "Production" },
    { key: "consumption", label: "Consumption" },
    { key: "net", label: "Net Production" },
    { key: "battery", label: "Battery" },
    { key: "from_grid", label: "From Grid" },
    { key: "to_grid", label: "To Grid" },
  ],
  preview: {
    badge: "solar-power",
  },
};

// ── Option string helpers (k=v,k=v format, shared codec) ────────────────────
function getSolarMode(b) {
  var mode = configOptionValue(b && b.options, "mode");
  return mode === "today" ? "today" : "live";
}

function getSolarOption(b, key) {
  return configOptionValue(b && b.options, key);
}

function setSolarOption(b, key, value) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options, key, value);
  return b.options;
}

registerButtonType("solar", {
  label: function () { return cardContractCardLabel("solar"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("solar"); },
  pickerKey: function () { return cardContractPickerKey("solar"); },
  experimental: function () { return cardContractExperimental("solar"); },
  hidden: function () { return cardContractHidden("solar"); },
  hideLabel: true,
  labelPlaceholder: "e.g. Solar",
  defaultConfig: function () { return cardContractDefaultConfig("solar"); },
  cardMetadata: SOLAR_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "Solar";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    if (!b.icon || b.icon === "Auto") b.icon = "Solar Power";
    b.icon_on = "Auto";
    if (!configOptionValue(b.options, "mode")) setSolarOption(b, "mode", "live");
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";

    // Mode: option-bound segment control.
    helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, SOLAR_CARD_METADATA.modeSegment, {
        value: function () { return getSolarMode(b); },
        onSelect: function (button, cardHelpers, value) {
          setSolarOption(button, "mode", value === "today" ? "today" : "live");
          cardHelpers.saveField("options", button.options);
          scheduleRender();
        },
      }),
    });

    // Six entity autocomplete fields, each bound to an option key.
    var domains = cardContractDomains("solar");
    SOLAR_CARD_METADATA.entityFields.forEach(function (spec) {
      var inputId = helpers.idPrefix + "solar-" + spec.key;
      var input = helpers.entityInput(
        inputId,
        getSolarOption(b, spec.key),
        "e.g. sensor.solar_" + spec.key,
        domains
      );
      var field = helpers.fieldWithControl(spec.label, inputId, input);
      function commit() {
        var current = getSolarOption(b, spec.key);
        if (current === input.value) return;
        setSolarOption(b, spec.key, input.value);
        helpers.saveField("options", b.options);
        scheduleRender();
      }
      input.addEventListener("change", commit);
      input.addEventListener("blur", commit);
      input.addEventListener("keydown", function (e) {
        if (e.key === "Enter") { commit(); this.blur(); }
      });
      panel.appendChild(field);
    });
  },
  renderPreview: function (b, helpers) {
    // Hero Net face: large signed net value, coloured by sign, with a
    // "Net · Solar" badge and an optional battery % corner.
    var netSample = "+2.1";
    var unit = "kW";
    var positive = netSample.charAt(0) !== "-";
    var valueClass = positive ? "sp-solar-net-pos" : "sp-solar-net-neg";
    var iconHtml = cardSensorPreviewHtml(b, helpers, netSample, unit, "sp-solar-hero", valueClass);

    if (getSolarOption(b, "battery")) {
      iconHtml = '<span class="sp-solar-battery-corner">42%</span>' + iconHtml;
    }

    return {
      buttonClass: "sp-solar-card",
      iconHtml: iconHtml,
      labelHtml: cardBadgeLabelHtml(helpers, "Net · Solar", SOLAR_CARD_METADATA.preview.badge),
    };
  },
});
