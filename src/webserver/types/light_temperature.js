// Light temperature slider card: controls color_temp_kelvin on a light entity.
// Slider bottom = min kelvin (warm), top = max kelvin (cool).
// Config fields: unit="min-max" (kelvin range),
// precision="color" (dynamic fill color by current temperature),
// sensor is unused; legacy "kelvin" values are ignored by firmware.

function lightTempSpec() {
  var card = cardContractCard("light_temperature");
  return card && card.behavior && card.behavior.lightTemperature || {};
}

function lightTempDefaultRange() {
  return lightTempSpec().defaultRange || "2000-6500";
}

function lightTempMinLimit() {
  var value = lightTempSpec().min;
  return typeof value === "number" ? value : 1000;
}

function lightTempMaxLimit() {
  var value = lightTempSpec().max;
  return typeof value === "number" ? value : 10000;
}

function lightTempMinMaxLimit() {
  var value = lightTempSpec().minMax;
  return typeof value === "number" ? value : 9900;
}

function lightTempStep() {
  var value = lightTempSpec().step;
  return typeof value === "number" ? value : 100;
}

function lightTempLegacySensorValues() {
  var values = lightTempSpec().legacySensorValues;
  return values ? values.slice() : ["kelvin"];
}

function lightTempSensorNeedsCleanup(value) {
  return lightTempLegacySensorValues().indexOf(value || "") >= 0;
}

function lightTempParseRange(unit) {
  var defaults = lightTempDefaultRange().split("-");
  var defaultMin = parseInt(defaults[0], 10);
  var defaultMax = parseInt(defaults[1], 10);
  if (!isFinite(defaultMin)) defaultMin = 2000;
  if (!isFinite(defaultMax) || defaultMax <= defaultMin) defaultMax = 6500;
  var parts = (unit || lightTempDefaultRange()).split("-");
  var mn = parseInt(parts[0], 10);
  var mx = parseInt(parts[1], 10);
  if (!isFinite(mn) || mn < lightTempMinLimit()) mn = defaultMin;
  if (!isFinite(mx) || mx <= mn) mx = defaultMax;
  return [mn, mx];
}

function lightTempClampMin(v, absMin) {
  var n = parseInt(v, 10);
  if (!isFinite(n)) n = absMin;
  if (n < absMin) n = absMin;
  if (n > lightTempMinMaxLimit()) n = lightTempMinMaxLimit();
  return n;
}

function lightTempClampMax(v, mn) {
  var n = parseInt(v, 10);
  if (!isFinite(n)) n = mn + lightTempStep();
  if (n <= mn) n = mn + lightTempStep();
  if (n > lightTempMaxLimit()) n = lightTempMaxLimit();
  return n;
}

var LIGHT_CONTROL_TYPE_OPTIONS = [
  ["light_control", "All Controls"],
  ["light_switch", "Switch"],
  ["light_brightness", "Brightness"],
  ["light_temperature", "Colour Temperature"],
];

var LIGHT_CONTROL_TYPE_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "light-control-type",
    options: function (b) {
      return LIGHT_CONTROL_TYPE_OPTIONS;
    },
    value: function (b) { return normalizeLightControlType(b.type); },
    onChange: function (b, helpers) {
      setLightControlType(b, this.value, helpers);
    },
  },
};

var LIGHT_TEMPERATURE_CARD_METADATA = {
  mode: LIGHT_CONTROL_TYPE_METADATA.mode,
  entity: {
    label: "Entity",
    placeholder: "e.g. light.living_room",
    domains: function () { return cardContractDomains("light_temperature"); },
  },
  labelField: {
    label: "Label",
    placeholder: "e.g. Living Room",
  },
  icon: {
    field: "icon",
    fallback: "Auto",
  },
  preview: {
    badge: "lightbulb",
  },
};

var LIGHT_FULL_CONTROL_CARD_METADATA = {
  mode: LIGHT_CONTROL_TYPE_METADATA.mode,
  entity: {
    label: "Entity",
    placeholder: "e.g. light.living_room",
    domains: function () { return cardContractDomains("light_control"); },
  },
  labelField: {
    label: "Label",
    placeholder: "e.g. Living Room",
  },
  iconOff: {
    field: "icon",
    fallback: "Lightbulb Outline",
    label: "Off Icon",
  },
  iconOn: {
    field: "icon_on",
    fallback: "Lightbulb",
    label: "On Icon",
  },
  preview: {
    badge: "lightbulb-on",
  },
};

function normalizeLightControlType(type) {
  if (type === "light_switch") return "light_switch";
  if (type === "light_control") return "light_control";
  return type === "light_temperature" ? "light_temperature" : "light_brightness";
}

function setLightControlType(b, type, helpers) {
  var nextType = normalizeLightControlType(type);
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

function renderLightControlTypeField(panel, b, helpers) {
  return helpers.renderCardModeSelector(panel, b, helpers, LIGHT_CONTROL_TYPE_METADATA);
}

function renderLightControlTabSettings(panel, b, helpers) {
  renderModalTabSettings(panel, b, helpers, {
    definitions: lightControlTabDefinitions,
    tabs: lightControlTabs,
    normalizeOptions: normalizeLightControlOptions,
    setTabs: setLightControlTabs,
    idPrefix: "light-tab-",
    groupLabel: "Modal Controls",
    groupIdSuffix: "light-modal-controls",
  });
}

registerButtonType("light_temperature", {
  label: function () { return cardContractCardLabel("light_temperature"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("light_temperature"); },
  hideLabel: true,
  pickerKey: function () { return cardContractPickerKey("light_temperature"); },
  hidden: function () { return cardContractHidden("light_temperature"); },
  defaultConfig: function () { return cardContractDefaultConfig("light_temperature"); },
  isAvailable: function () {
    return false;
  },
  labelPlaceholder: "e.g. Living Room",
  cardMetadata: LIGHT_TEMPERATURE_CARD_METADATA,
  onSelect: function (b) {
    b.sensor = "";
    b.unit = "2000-6500";
    b.precision = "";
    b.icon = "Lightbulb";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    renderLightControlTypeField(panel, b, helpers);

    helpers.renderBasicCardFields(panel, b, helpers, LIGHT_TEMPERATURE_CARD_METADATA, {
      icon: false,
    });

    if (lightTempSensorNeedsCleanup(b.sensor)) {
      b.sensor = "";
      helpers.saveField("sensor", "");
    }

    // Kelvin range
    var range = lightTempParseRange(b.unit);
    var curMin = range[0], curMax = range[1];

    function saveRange(mn, mx) {
      b.unit = mn + "-" + mx;
      helpers.saveField("unit", b.unit);
    }

    var minF = document.createElement("div");
    minF.className = "sp-field";
    minF.appendChild(helpers.fieldLabel("Min Color Temp (K)", helpers.idPrefix + "kmin"));
    var minInp = document.createElement("input");
    minInp.type = "number";
    minInp.className = "sp-input";
    minInp.id = helpers.idPrefix + "kmin";
    minInp.min = "1000";
    minInp.max = "9900";
    minInp.step = "100";
    minInp.placeholder = "2000";
    minInp.value = curMin;
    minF.appendChild(minInp);
    panel.appendChild(minF);

    var maxF = document.createElement("div");
    maxF.className = "sp-field";
    maxF.appendChild(helpers.fieldLabel("Max Color Temp (K)", helpers.idPrefix + "kmax"));
    var maxInp = document.createElement("input");
    maxInp.type = "number";
    maxInp.className = "sp-input";
    maxInp.id = helpers.idPrefix + "kmax";
    maxInp.min = "1100";
    maxInp.max = "10000";
    maxInp.step = "100";
    maxInp.placeholder = "6500";
    maxInp.value = curMax;
    maxF.appendChild(maxInp);
    panel.appendChild(maxF);

    function onRangeChange() {
      var mn = lightTempClampMin(minInp.value, lightTempMinLimit());
      var mx = lightTempClampMax(maxInp.value, mn);
      minInp.value = mn;
      maxInp.value = mx;
      saveRange(mn, mx);
    }
    minInp.addEventListener("change", onRangeChange);
    maxInp.addEventListener("change", onRangeChange);
    minInp.addEventListener("blur", onRangeChange);
    maxInp.addEventListener("blur", onRangeChange);

    helpers.renderCardIconPicker(panel, b, helpers, LIGHT_TEMPERATURE_CARD_METADATA.icon);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Light Temp";
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: "Lightbulb",
      iconExtraHtml:
        '<span class="sp-slider-preview"><span class="sp-slider-track">' +
          '<span class="sp-slider-fill"></span>' +
        '</span></span>',
      badge: LIGHT_TEMPERATURE_CARD_METADATA.preview.badge,
    });
  },
});

registerButtonType("light_control", {
  label: function () { return cardContractCardLabel("light_control"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("light_control"); },
  hideLabel: true,
  pickerKey: function () { return cardContractPickerKey("light_control"); },
  hidden: function () { return cardContractHidden("light_control"); },
  defaultConfig: function () { return cardContractDefaultConfig("light_control"); },
  isAvailable: function () {
    return false;
  },
  labelPlaceholder: "e.g. Living Room",
  cardMetadata: LIGHT_FULL_CONTROL_CARD_METADATA,
  onSelect: function (b) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon = "Lightbulb Outline";
    b.icon_on = "Lightbulb";
  },
  renderSettings: function (panel, b, slot, helpers) {
    renderLightControlTypeField(panel, b, helpers);
    b.options = normalizeLightControlOptions(b.options);

    helpers.renderCardEntityField(panel, b, helpers, LIGHT_FULL_CONTROL_CARD_METADATA);
    renderLightControlTabSettings(panel, b, helpers);
    var cardSettingsDisclosure = helpers.disclosureSection(
      "Card Settings",
      helpers.idPrefix + "light-card-settings",
      false
    );
    var cardSettings = cardSettingsDisclosure.section;
    helpers.renderCardTextField(cardSettings, b, helpers, LIGHT_FULL_CONTROL_CARD_METADATA.labelField);
    helpers.renderCardIconPair(
      cardSettings,
      b,
      helpers,
      LIGHT_FULL_CONTROL_CARD_METADATA.iconOff,
      LIGHT_FULL_CONTROL_CARD_METADATA.iconOn
    );
    panel.appendChild(cardSettingsDisclosure.panel);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Light";
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: "Lightbulb Outline",
      badge: LIGHT_FULL_CONTROL_CARD_METADATA.preview.badge,
    });
  },
});
