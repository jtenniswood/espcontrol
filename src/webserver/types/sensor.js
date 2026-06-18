// Read-only sensor card: displays either numeric data or a text state.
var SENSOR_CARD_METADATA = {
  entity: {
    label: "Sensor Entity",
    idSuffix: "sensor",
    placeholder: "e.g. sensor.living_room_temperature",
    domains: function () { return cardContractDomains("sensor"); },
    bindName: "sensor",
    rerender: true,
    requiredMessage: "Add a sensor entity before saving.",
  },
  segment: {
    label: "Type",
    options: [
      ["icon", "Icon"],
      ["numeric", "Numeric"],
      ["text", "Text"],
    ],
    value: function (b) {
      if (b.precision === "icon") return "icon";
      return b.precision === "text" ? "text" : "numeric";
    },
  },
  largeNumbers: {
    label: "Large Sensor Numbers",
    idSuffix: "large-sensor-numbers",
    supported: function (b) {
      return b.precision !== "icon" && b.precision !== "text";
    },
  },
  preview: {
    iconBadge: "toggle-switch",
    numericBadge: "gauge",
    textBadge: "format-text",
  },
};

registerButtonType("sensor", {
  label: function () { return cardContractCardLabel("sensor"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("sensor"); },
  pickerKey: function () { return cardContractPickerKey("sensor"); },
  hidden: function () { return cardContractHidden("sensor"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("sensor"); },
  cardMetadata: SENSOR_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.icon_on = "Auto";
    if (!b.precision) b.precision = "";
    if (b.precision !== "icon" && b.precision !== "text") b.icon = "Auto";
    b.options = normalizeSensorOptions(b.options, b.precision);
  },
  renderSettings: function (panel, b, slot, helpers) {
    var displayMode = b.precision === "icon" || b.precision === "text" ? b.precision : "numeric";
    var isTextMode = displayMode === "text";

    helpers.renderCardEntityField(panel, b, helpers, SENSOR_CARD_METADATA);

    var mode = helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, SENSOR_CARD_METADATA.segment, {
        onSelect: function (button, cardHelpers, value) {
          setMode(value, true);
        },
      }),
    });
    var iconBtn = mode.buttons.icon;
    var numericBtn = mode.buttons.numeric;
    var textBtn = mode.buttons.text;

    var numericSection = condField();

    var labelField = helpers.renderCardTextField(numericSection, b, helpers, {
      label: "Label",
      idSuffix: "label",
      field: "label",
      placeholder: "e.g. Living Room",
      rerender: true,
    });
    var labelInp = labelField.input;

    var unitField = helpers.renderCardTextField(numericSection, b, helpers, {
      label: "Unit",
      idSuffix: "unit",
      field: "unit",
      placeholder: "e.g. \u00B0C",
      rerender: true,
    });
    var unitInp = unitField.input;
    unitInp.className = "sp-input";

    var precisionField = helpers.precisionField(helpers.idPrefix + "precision",
      !isTextMode ? (b.precision || "0") : "0", function () {
      b.precision = this.value === "0" ? "" : this.value;
      helpers.saveField("precision", b.precision);
    });
    var precisionSelect = precisionField.select;
    numericSection.appendChild(precisionField.field);

    helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, SENSOR_CARD_METADATA);
    panel.appendChild(numericSection);

    var textSection = condField();
    var textIconPicker = helpers.renderCardIconPicker(textSection, b, helpers, {
      pickerIdSuffix: "icon-picker",
      idSuffix: "icon",
      field: "icon",
      fallback: "Auto",
    });
    panel.appendChild(textSection);

    var iconSection = condField();
    var offIconPicker = helpers.renderCardIconPicker(iconSection, b, helpers, {
      pickerIdSuffix: "icon-off-picker",
      idSuffix: "icon-off",
      field: "icon",
      fallback: "Auto",
      label: "Icon",
    });
    var onIconPicker = helpers.renderCardIconPicker(iconSection, b, helpers, {
      pickerIdSuffix: "icon-on-picker",
      idSuffix: "icon-on",
      field: "icon_on",
      fallback: "Auto",
      label: "On Icon",
    });
    panel.appendChild(iconSection);

    var hasStateLabels = sensorStateLabelsEnabled(b);
    var advancedToggleSection = helpers.toggleSection(
      "Advanced",
      helpers.idPrefix + "sensor-advanced-toggle",
      hasStateLabels
    );
    var advancedToggle = advancedToggleSection.toggle;
    var advanced = advancedToggleSection.section;
    panel.appendChild(advancedToggle.row);
    if (hasStateLabels && isTextMode) advanced.classList.add("sp-visible");

    var stateTextGrid = document.createElement("div");
    stateTextGrid.className = "sp-state-translation-grid";
    advanced.appendChild(stateTextGrid);

    var inputTextField = helpers.textField(
      "Input Status",
      helpers.idPrefix + "sensor-state-input",
      sensorStateInput(b),
      "e.g. high"
    );
    var inputTextInp = inputTextField.input;
    stateTextGrid.appendChild(inputTextField.field);

    var outputTextField = helpers.textField(
      "Display Text",
      helpers.idPrefix + "sensor-state-output",
      sensorStateOutput(b),
      "e.g. Please empty"
    );
    var outputTextInp = outputTextField.input;
    stateTextGrid.appendChild(outputTextField.field);

    var inputText2Field = helpers.textField(
      "Input Status 2",
      helpers.idPrefix + "sensor-state-input-2",
      sensorStateInput2(b),
      "e.g. low"
    );
    var inputText2Inp = inputText2Field.input;
    stateTextGrid.appendChild(inputText2Field.field);

    var outputText2Field = helpers.textField(
      "Display Text 2",
      helpers.idPrefix + "sensor-state-output-2",
      sensorStateOutput2(b),
      "e.g. Full"
    );
    var outputText2Inp = outputText2Field.input;
    stateTextGrid.appendChild(outputText2Field.field);

    function saveStateTranslation() {
      setSensorStateTranslations(
        b,
        advancedToggle.input.checked,
        inputTextInp.value,
        outputTextInp.value,
        inputText2Inp.value,
        outputText2Inp.value
      );
      helpers.saveField("options", b.options);
    }

    inputTextInp.addEventListener("change", saveStateTranslation);
    outputTextInp.addEventListener("change", saveStateTranslation);
    inputText2Inp.addEventListener("change", saveStateTranslation);
    outputText2Inp.addEventListener("change", saveStateTranslation);
    advancedToggle.input.addEventListener("change", function () {
      if (this.checked) {
        if (!isTextMode) setMode("text", true);
        advanced.classList.add("sp-visible");
      } else {
        advanced.classList.remove("sp-visible");
        inputTextInp.value = "";
        outputTextInp.value = "";
        inputText2Inp.value = "";
        outputText2Inp.value = "";
      }
      saveStateTranslation();
    });
    panel.appendChild(advanced);

    function resetIconPicker(picker, value, slug) {
      var iconPreview = picker.querySelector(".sp-icon-picker-preview");
      if (iconPreview) iconPreview.className = "sp-icon-picker-preview mdi mdi-" + slug;
      var iconInput = picker.querySelector(".sp-icon-picker-input");
      if (iconInput) iconInput.value = value;
    }

    function syncAdvancedVisibility() {
      advancedToggle.row.style.display = isTextMode ? "" : "none";
      if (!isTextMode) advanced.classList.remove("sp-visible");
    }

    function setMode(mode, persist) {
      displayMode = mode === "icon" || mode === "text" ? mode : "numeric";
      isTextMode = displayMode === "text";
      iconBtn.classList.toggle("active", displayMode === "icon");
      numericBtn.classList.toggle("active", displayMode === "numeric");
      textBtn.classList.toggle("active", isTextMode);
      numericSection.classList.toggle("sp-visible", displayMode === "numeric");
      textSection.classList.toggle("sp-visible", isTextMode);
      iconSection.classList.toggle("sp-visible", displayMode === "icon");
      syncAdvancedVisibility();
      if (!persist) return;
      if (isTextMode) {
        b.precision = "text";
        b.label = "";
        b.unit = "";
        b.icon_on = "Auto";
        b.options = normalizeSensorOptions(b.options, "text");
        labelInp.value = "";
        unitInp.value = "";
        helpers.saveField("precision", "text");
        helpers.saveField("label", "");
        helpers.saveField("unit", "");
        helpers.saveField("icon_on", "Auto");
        helpers.saveField("options", b.options);
        resetIconPicker(onIconPicker, "Auto", "cog");
      } else if (displayMode === "icon") {
        b.precision = "icon";
        b.unit = "";
        b.options = normalizeSensorOptions(b.options, "icon");
        unitInp.value = "";
        helpers.saveField("precision", "icon");
        helpers.saveField("unit", "");
        helpers.saveField("options", b.options);
        advancedToggle.input.checked = false;
        advanced.classList.remove("sp-visible");
        inputTextInp.value = "";
        outputTextInp.value = "";
        inputText2Inp.value = "";
        outputText2Inp.value = "";
      } else {
        b.precision = "";
        b.icon = "Auto";
        b.icon_on = "Auto";
        b.options = normalizeSensorOptions(b.options, "");
        helpers.saveField("precision", "");
        helpers.saveField("icon", "Auto");
        helpers.saveField("icon_on", "Auto");
        helpers.saveField("options", b.options);
        advancedToggle.input.checked = false;
        advanced.classList.remove("sp-visible");
        inputTextInp.value = "";
        outputTextInp.value = "";
        inputText2Inp.value = "";
        outputText2Inp.value = "";
        resetIconPicker(textIconPicker, "Auto", "cog");
        resetIconPicker(offIconPicker, "Auto", "cog");
        resetIconPicker(onIconPicker, "Auto", "cog");
        precisionSelect.value = "0";
      }
    }

    setMode(displayMode, false);
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "icon") {
      var stateIconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + stateIconName + '"></span>',
        labelHtml: cardBadgeLabelHtml(helpers, b.label || b.sensor || "Sensor", SENSOR_CARD_METADATA.preview.iconBadge),
      };
    }

    if (b.precision === "text") {
      var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
        labelHtml: cardBadgeLabelHtml(helpers, "State", SENSOR_CARD_METADATA.preview.textBadge),
      };
    }

    var label = b.label || b.sensor || "Sensor";
    var unit = b.unit || "";
    var prec = parseInt(b.precision || "0", 10) || 0;
    var sampleVal = (0).toFixed(prec);
    return {
      iconHtml: cardSensorPreviewHtml(b, helpers, sampleVal, unit),
      labelHtml: cardBadgeLabelHtml(helpers, label, SENSOR_CARD_METADATA.preview.numericBadge),
    };
  },
});
