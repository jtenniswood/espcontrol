// Solar card: energy overview that stores mode + six sensor entities in its
// options string (mode=live,production=sensor.x,...). The main entity field
// stays empty; everything lives in `options`.
var SOLAR_CARD_METADATA = {
  modeSegment: {
    label: "Mode",
    options: [
      ["live", "Live"],
      ["today", "Today"],
      ["flow", "Flow"],
    ],
  },
  invertProduction: {
    label: "Invert Production",
    idSuffix: "solar-invert-production",
    checked: function (b) { return configOptionEnabled(b.options, "invert_production"); },
    onChange: function (b, helpers, checked) {
      b.options = setConfigOption(b.options, "invert_production", checked);
      helpers.saveField("options", b.options);
      scheduleRender();
    },
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

// Abbreviated option keys to stay within the 255-char config limit.
// Long keys are accepted on read for migration of existing configs.
var SOLAR_KEY_SHORT = {
  mode: "m", production: "p", consumption: "c", net: "n",
  battery: "b", from_grid: "fg", to_grid: "tg", invert_production: "inv"
};

// Solar cards store entity IDs without the "sensor." domain prefix.
function solarEntityToStorage(v) {
  return (v || "").replace(/^sensor\./, "");
}
function solarEntityFromStorage(v) {
  if (!v) return "";
  return v.indexOf(".") === -1 ? "sensor." + v : v;
}

// Read a solar option, trying short key first then long key (migration).
function solarRead(opts, key) {
  var sk = SOLAR_KEY_SHORT[key] || key;
  var v = configOptionValue(opts, sk);
  if (!v) v = configOptionValue(opts, key);  // fallback for old configs
  return v;
}

function getSolarMode(b) {
  var mode = solarRead(b && b.options, "mode");
  if (mode === "today") return "today";
  if (mode === "flow")  return "flow";
  return "live";
}

function getSolarEntityOption(b, key) {
  return solarEntityFromStorage(solarRead(b && b.options, key));
}

function setSolarEntityOption(b, key, value) {
  if (!b) return "";
  var sk = SOLAR_KEY_SHORT[key] || key;
  // Remove any legacy long-key entry, then write short key
  b.options = deleteConfigOptionValue(b.options, key);
  b.options = setConfigOptionValue(b.options, sk, solarEntityToStorage(value));
  return b.options;
}

function getSolarOption(b, key) {
  return solarRead(b && b.options, key);
}

function setSolarOption(b, key, value) {
  if (!b) return "";
  var sk = SOLAR_KEY_SHORT[key] || key;
  b.options = deleteConfigOptionValue(b.options, key);  // remove legacy long key
  b.options = setConfigOptionValue(b.options, sk, value);
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
          var newMode = (value === "today") ? "today" : (value === "flow") ? "flow" : "live";
          setSolarOption(button, "mode", newMode);
          cardHelpers.saveField("options", button.options);
          if (newMode === "flow") cardHelpers.requestCardSize(4);
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
        getSolarEntityOption(b, spec.key),
        "e.g. sensor.solar_" + spec.key,
        domains
      );
      var field = helpers.fieldWithControl(spec.label, inputId, input);
      function commit() {
        var current = getSolarEntityOption(b, spec.key);
        if (current === input.value) return;
        setSolarEntityOption(b, spec.key, input.value);
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

    // Invert Production toggle — shown only when a production entity is set.
    if (getSolarEntityOption(b, "production")) {
      helpers.renderCardOptionToggle(panel, b, helpers, SOLAR_CARD_METADATA.invertProduction);
    }
  },
  renderPreview: function (b, helpers) {
    if (getSolarMode(b) === "flow") {
      return {
        buttonClass: "sp-solar-card",
        iconHtml: (
          '<svg viewBox="0 0 60 60" width="36" height="36" style="overflow:visible">' +
          '<circle cx="30" cy="10" r="9" fill="none" stroke="#f59e0b" stroke-width="2.5"/>' +
          '<circle cx="50" cy="30" r="9" fill="none" stroke="#3b82f6" stroke-width="2.5"/>' +
          '<circle cx="30" cy="50" r="9" fill="none" stroke="#22c55e" stroke-width="2.5"/>' +
          '<circle cx="10" cy="30" r="9" fill="none" stroke="#8b5cf6" stroke-width="2.5"/>' +
          '<line x1="30" y1="19" x2="30" y2="41" stroke="#f59e0b" stroke-width="1.5" opacity="0.5"/>' +
          '<line x1="19" y1="30" x2="41" y2="30" stroke="#f59e0b" stroke-width="1.5" opacity="0.5"/>' +
          '<circle cx="30" cy="30" r="3" fill="#444"/>' +
          '</svg>'
        ),
        labelHtml: cardBadgeLabelHtml(helpers, "Flow", SOLAR_CARD_METADATA.preview.badge),
      };
    }
    var isToday = getSolarMode(b) === "today";
    var netSample = isToday ? "18.4" : "+2.1";
    var unit = isToday ? "kWh" : "kW";
    var label = isToday ? "Today" : "Live";
    var positive = netSample.charAt(0) !== "-";
    var valueClass = positive ? "sp-solar-net-pos" : "sp-solar-net-neg";
    var iconHtml = cardSensorPreviewHtml(b, helpers, netSample, unit, "sp-solar-hero", valueClass);

    if (getSolarOption(b, "battery")) {
      iconHtml = '<span class="sp-solar-battery-corner">42%</span>' + iconHtml;
    }

    return {
      buttonClass: "sp-solar-card",
      iconHtml: iconHtml,
      labelHtml: cardBadgeLabelHtml(helpers, label, SOLAR_CARD_METADATA.preview.badge),
    };
  },
});
