// Climate card: thermostat status plus full-screen climate controls.
var CLIMATE_CARD_METADATA = {
  entity: {
    label: "Climate Entity",
    idSuffix: "entity",
    placeholder: "e.g. climate.living_room",
    domains: function () { return cardContractDomains("climate"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a climate entity before saving.",
  },
  labelDisplay: {
    label: "Label Display",
    options: [
      ["label", "Label"],
      ["status", "Status"],
      ["actual", "Actual"],
      ["target", "Target"],
    ],
  },
  numberDisplay: {
    label: "Icon & Temperatures",
    options: [
      ["icon", "Icon"],
      ["actual", "Actual"],
      ["target", "Target"],
    ],
  },
  temperatureStep: {
    label: "Temperature Step",
    options: [
      ["1", "1 degree"],
      ["0.5", "0.5 degree"],
    ],
  },
  largeNumbers: {
    label: "Large Temperature Numbers",
    idSuffix: "large-temperature-numbers",
    supported: function (b) {
      return climateNumberDisplayMode(b) !== "icon";
    },
  },
  preview: {
    badge: "thermostat",
  },
};

registerButtonType("climate", {
  label: function () { return cardContractCardLabel("climate"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("climate"); },
  pickerKey: function () { return cardContractPickerKey("climate"); },
  hidden: function () { return cardContractHidden("climate"); },
  hideLabel: true,
  labelPlaceholder: "e.g. Living Room",
  defaultConfig: function () { return cardContractDefaultConfig("climate"); },
  cardMetadata: CLIMATE_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "Climate";
    b.sensor = "";
    b.unit = "";
    b.type = "climate_control";
    b.precision = "";
    b.icon = "Thermostat";
    b.icon_on = "Auto";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    if (b.type !== "climate_control") {
      b.type = "climate_control";
      helpers.saveField("type", b.type);
    }
    b.sensor = "";
    b.unit = "";
    if (!b.icon) b.icon = "Thermostat";
    if (!b.icon_on) b.icon_on = "Auto";

    var climateConfig = parseClimatePrecisionConfig(b.precision);
    var normalizedPrecision = climatePrecisionConfig(
      climateConfig.precision,
      climateConfig.min,
      climateConfig.max
    );
    if (b.precision !== normalizedPrecision) {
      b.precision = normalizedPrecision;
      helpers.saveField("precision", normalizedPrecision);
    }

    helpers.renderCardEntityField(panel, b, helpers, CLIMATE_CARD_METADATA);
    var modalTabsDisclosure = helpers.disclosureSection(
      "Modal Settings",
      helpers.idPrefix + "climate-modal-tabs",
      b._modalSettingsOpen === true
    );
    renderModalTabSettings(modalTabsDisclosure.section, b, helpers, {
      definitions: climateControlTabDefinitions,
      tabs: climateControlTabs,
      normalizeOptions: function (options) { return normalizeClimateOptions(options, true); },
      setTabs: setClimateControlTabs,
      idPrefix: "climate-tab-",
      hideHeading: true,
    });
    panel.appendChild(modalTabsDisclosure.panel);

    var labelField = condField();
    labelField.classList.add("sp-climate-settings-gap");
    helpers.renderCardTextField(labelField, b, helpers, {
      label: "Label",
      idSuffix: "label",
      field: "label",
      placeholder: "Climate",
      rerender: true,
    });
    function syncLabelField() {
      labelField.classList.toggle("sp-visible", climateLabelDisplayMode(b) === "label");
    }

    var cardSettingsDisclosure = helpers.disclosureSection(
      "Card Settings",
      helpers.idPrefix + "climate-card-settings",
      false
    );
    var cardSettings = cardSettingsDisclosure.section;
    helpers.renderCardSegmentControl(cardSettings, b, helpers, {
      segment: Object.assign({}, CLIMATE_CARD_METADATA.labelDisplay, {
        value: function () { return climateLabelDisplayMode(b); },
        onSelect: function (button, cardHelpers, value) {
          setClimateLabelDisplayMode(button, value);
          cardHelpers.saveField("options", button.options);
          syncLabelField();
          scheduleRender();
        },
      }),
    });
    syncLabelField();
    cardSettings.appendChild(labelField);

    helpers.renderCardSegmentControl(cardSettings, b, helpers, {
      segment: Object.assign({}, CLIMATE_CARD_METADATA.numberDisplay, {
        value: function () { return climateNumberDisplayMode(b); },
        onSelect: function (button, cardHelpers, value) {
          setClimateNumberDisplayMode(button, value);
          cardHelpers.saveField("options", button.options);
          syncIconFields();
          scheduleRender();
        },
      }),
    });
    var iconFields = condField();
    iconFields.classList.add("sp-climate-settings-gap");
    helpers.renderCardIconPicker(iconFields, b, helpers, {
      pickerIdSuffix: "climate-icon-picker",
      idSuffix: "climate-icon",
      field: "icon",
      fallback: "Thermostat",
      label: "Off Icon",
      onChange: function () { scheduleRender(); },
    });
    helpers.renderCardIconPicker(iconFields, b, helpers, {
      pickerIdSuffix: "climate-icon-on-picker",
      idSuffix: "climate-icon-on",
      field: "icon_on",
      fallback: "Auto",
      label: "On Icon",
      onChange: function () { scheduleRender(); },
    });
    function syncIconFields() {
      iconFields.classList.toggle("sp-visible", climateNumberDisplayMode(b) === "icon");
    }
    syncIconFields();
    cardSettings.appendChild(iconFields);

    var precisionField = helpers.selectField("Temperature Settings", helpers.idPrefix + "climate-precision", [
      ["", "10"],
      ["1", "10.2"],
    ], climateConfig.precision);
    var precision = precisionField.select;
    function saveClimateAdvancedSettings() {
      b.precision = climatePrecisionConfig(precision.value, minInp.value, maxInp.value);
      helpers.saveField("precision", b.precision);
      scheduleRender();
    }
    precision.addEventListener("change", saveClimateAdvancedSettings);
    var stepField = helpers.selectField(
      CLIMATE_CARD_METADATA.temperatureStep.label,
      helpers.idPrefix + "climate-temperature-step",
      CLIMATE_CARD_METADATA.temperatureStep.options,
      climateTemperatureStep(b)
    );
    stepField.select.addEventListener("change", function () {
      setClimateTemperatureStep(b, stepField.select.value);
      helpers.saveField("options", b.options);
      scheduleRender();
    });
    helpers.renderCardLargeNumbersToggle(cardSettings, b, helpers, CLIMATE_CARD_METADATA);
    panel.appendChild(cardSettingsDisclosure.panel);

    var advancedDisclosure = helpers.disclosureSection(
      "Advanced",
      helpers.idPrefix + "climate-advanced",
      false
    );
    var advanced = advancedDisclosure.section;
    advanced.appendChild(precisionField.field);
    advanced.appendChild(stepField.field);

    var minField = helpers.textField(
      "Minimum Temperature", helpers.idPrefix + "climate-min", climateConfig.min, "e.g. -25");
    var minInp = minField.input;
    minInp.inputMode = "decimal";
    advanced.appendChild(minField.field);

    var maxField = helpers.textField(
      "Maximum Temperature", helpers.idPrefix + "climate-max", climateConfig.max, "e.g. 5");
    var maxInp = maxField.input;
    maxInp.inputMode = "decimal";
    advanced.appendChild(maxField.field);

    minInp.addEventListener("change", saveClimateAdvancedSettings);
    maxInp.addEventListener("change", saveClimateAdvancedSettings);
    panel.appendChild(advancedDisclosure.panel);
  },
  renderPreview: function (b, helpers) {
    var climateConfig = parseClimatePrecisionConfig(b.precision);
    var prec = parseInt(climateConfig.precision || "0", 10) || 0;
    var unit = temperatureUnitSymbol();
    var actualVal = (21).toFixed(prec);
    var targetVal = (20).toFixed(prec);
    var numberMode = climateNumberDisplayMode(b);
    var numberVal = numberMode === "actual" ? actualVal : targetVal;
    var labelMode = climateLabelDisplayMode(b);
    var label = (b.label && b.label.trim()) || "Climate";
    if (labelMode === "status") {
      label = "Idle";
    } else if (labelMode === "actual") {
      label = actualVal + unit;
    } else if (labelMode === "target") {
      label = targetVal + unit;
    }
    function climateLabelHtml() {
      return cardBadgeLabelHtml(helpers, label, CLIMATE_CARD_METADATA.preview.badge);
    }
    if (numberMode === "icon") {
      var iconName = b.icon && b.icon !== "Auto" ? b.icon : "Thermostat";
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconSlug(iconName) + '"></span>',
        labelHtml: climateLabelHtml(),
      };
    }
    return {
      buttonClass: "sp-climate-temp-card",
      iconHtml: cardSensorPreviewHtml(b, helpers, numberVal, unit),
      labelHtml: climateLabelHtml(),
    };
  },
});

registerButtonType("climate_control", Object.assign({}, BUTTON_TYPES.climate, {
  label: function () { return cardContractCardLabel("climate_control"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("climate_control"); },
  pickerKey: function () { return cardContractPickerKey("climate_control"); },
  hidden: function () { return cardContractHidden("climate_control"); },
  defaultConfig: function () { return cardContractDefaultConfig("climate_control"); },
}));
