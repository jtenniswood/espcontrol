// Read-only sensor card: displays either numeric data or a text state.
var SENSOR_CARD_LOCAL_SENSOR = "local";

function sensorCardIsLocal(b) {
  if (!b) return false;
  return b.type === "local_sensor" || (b.type === "sensor" && b.sensor === SENSOR_CARD_LOCAL_SENSOR);
}

var SENSOR_CARD_METADATA = {
  source: {
    label: "Source",
    options: [
      ["ha", "Home Assistant"],
      [SENSOR_CARD_LOCAL_SENSOR, "Local Sensor"],
    ],
    value: function (b) {
      return sensorCardIsLocal(b) ? SENSOR_CARD_LOCAL_SENSOR : "ha";
    },
  },
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
      return !sensorCardIsLocal(b) && b.precision !== "icon" && b.precision !== "text";
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
    var sourceControl = helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, SENSOR_CARD_METADATA.source, {
        onSelect: function (button, cardHelpers, value) {
          setSource(value);
        },
      }),
    });
    var sourceButtons = sourceControl.buttons;
    sourceButtons.ha.classList.toggle("active", !sensorCardIsLocal(b));
    sourceButtons[SENSOR_CARD_LOCAL_SENSOR].classList.toggle("active", sensorCardIsLocal(b));

    function setSource(value) {
      var local = value === SENSOR_CARD_LOCAL_SENSOR;
      if (local === sensorCardIsLocal(b)) return;
      b.type = "sensor";
      b.entity = "";
      b.label = "";
      b.sensor = local ? SENSOR_CARD_LOCAL_SENSOR : "";
      b.unit = "";
      b.icon = "Auto";
      b.icon_on = "Auto";
      b.precision = "";
      b.options = "";
      helpers.saveField("type", "sensor");
      helpers.saveField("entity", "");
      helpers.saveField("label", "");
      helpers.saveField("sensor", b.sensor);
      helpers.saveField("unit", "");
      helpers.saveField("icon", "Auto");
      helpers.saveField("icon_on", "Auto");
      helpers.saveField("precision", "");
      helpers.saveField("options", "");
      renderButtonSettings();
    }

    if (sensorCardIsLocal(b)) {
      renderSensorLocalSettings(panel, b, slot, helpers);
      return;
    }

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
    if (sensorCardIsLocal(b)) return sensorLocalPreview(b, helpers);

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

function renderSensorLocalSettings(panel, b, slot, helpers) {
  b.type = "sensor";
  b.sensor = SENSOR_CARD_LOCAL_SENSOR;
  var isTextMode = b.precision === "text";
  var showAll = false;
  var fetchedSensors = null;

  var modeField = document.createElement("div");
  modeField.className = "sp-field";
  modeField.appendChild(helpers.fieldLabel("Display"));
  var modeSeg = document.createElement("div");
  modeSeg.className = "sp-segment";
  var numericBtn = document.createElement("button");
  numericBtn.type = "button";
  numericBtn.textContent = "Numeric";
  var textBtn = document.createElement("button");
  textBtn.type = "button";
  textBtn.textContent = "Text";
  modeSeg.appendChild(numericBtn);
  modeSeg.appendChild(textBtn);
  modeField.appendChild(modeSeg);
  panel.appendChild(modeField);

  var pickerSection = document.createElement("div");
  panel.appendChild(pickerSection);

  var numericSection = condField();

  var lf = document.createElement("div");
  lf.className = "sp-field";
  lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
  var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Living Room");
  lf.appendChild(labelInp);
  numericSection.appendChild(lf);
  helpers.bindField(labelInp, "label", true);

  var uf = document.createElement("div");
  uf.className = "sp-field";
  uf.appendChild(helpers.fieldLabel("Unit", helpers.idPrefix + "unit"));
  var unitInp = helpers.textInput(helpers.idPrefix + "unit", b.unit, "e.g. \u00B0C");
  unitInp.className = "sp-input";
  uf.appendChild(unitInp);
  numericSection.appendChild(uf);
  helpers.bindField(unitInp, "unit", true);

  var pf = document.createElement("div");
  pf.className = "sp-field";
  pf.appendChild(helpers.fieldLabel("Unit Precision", helpers.idPrefix + "precision"));
  var precisionSelect = document.createElement("select");
  precisionSelect.className = "sp-select";
  precisionSelect.id = helpers.idPrefix + "precision";
  var precOpts = [["0", "10"], ["1", "10.2"], ["2", "10.21"]];
  for (var i = 0; i < precOpts.length; i++) {
    var opt = document.createElement("option");
    opt.value = precOpts[i][0];
    opt.textContent = precOpts[i][1];
    precisionSelect.appendChild(opt);
  }
  precisionSelect.value = !isTextMode ? (b.precision || "0") : "0";
  precisionSelect.addEventListener("change", function () {
    b.precision = this.value === "0" ? "" : this.value;
    helpers.saveField("precision", b.precision);
  });
  pf.appendChild(precisionSelect);
  numericSection.appendChild(pf);
  panel.appendChild(numericSection);

  var textSection = condField();
  var textIconPicker = helpers.makeIconPicker(
    helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
    b.icon || "Auto", function (opt) {
      b.icon = opt;
      helpers.saveField("icon", opt);
    }
  );
  textSection.appendChild(textIconPicker);
  panel.appendChild(textSection);

  function setMode(mode, persist) {
    isTextMode = mode === "text";
    numericBtn.classList.toggle("active", !isTextMode);
    textBtn.classList.toggle("active", isTextMode);
    numericSection.classList.toggle("sp-visible", !isTextMode);
    textSection.classList.toggle("sp-visible", isTextMode);
    if (!persist) return;
    if (isTextMode) {
      b.precision = "text";
      b.label = "";
      b.unit = "";
      b.icon_on = "Auto";
      labelInp.value = "";
      unitInp.value = "";
      helpers.saveField("precision", "text");
      helpers.saveField("label", "");
      helpers.saveField("unit", "");
      helpers.saveField("icon_on", "Auto");
    } else {
      b.precision = "";
      b.icon = "Auto";
      helpers.saveField("precision", "");
      helpers.saveField("icon", "Auto");
      var iconPreview = textIconPicker.querySelector(".sp-icon-picker-preview");
      if (iconPreview) iconPreview.className = "sp-icon-picker-preview mdi mdi-cog";
      var iconInput = textIconPicker.querySelector(".sp-icon-picker-input");
      if (iconInput) iconInput.value = "Auto";
      precisionSelect.value = "0";
    }
  }

  numericBtn.addEventListener("click", function () {
    setMode("numeric", true);
    if (fetchedSensors) buildDropdown(fetchedSensors);
  });
  textBtn.addEventListener("click", function () {
    setMode("text", true);
    if (fetchedSensors) buildDropdown(fetchedSensors);
  });
  setMode(isTextMode ? "text" : "numeric", false);

  function buildDropdown(sensors) {
    pickerSection.innerHTML = "";
    pickerSection.className = "";
    var wantType = isTextMode ? "text" : "numeric";
    var filtered = sensors.filter(function (s) {
      return s.type === wantType && (showAll || !s.internal);
    });

    var sf = document.createElement("div");
    sf.className = "sp-field";
    sf.appendChild(helpers.fieldLabel("Local Sensor", helpers.idPrefix + "sensor-sel"));
    var sel = document.createElement("select");
    sel.className = "sp-select";
    sel.id = helpers.idPrefix + "sensor-sel";

    var placeholder = document.createElement("option");
    placeholder.value = "";
    placeholder.textContent = "Choose a sensor…";
    sel.appendChild(placeholder);

    filtered.forEach(function (s) {
      var opt = document.createElement("option");
      opt.value = s.key;
      opt.textContent = s.name + (s.type === "text" ? " (text)" : "");
      if (s.key === b.entity) opt.selected = true;
      sel.appendChild(opt);
    });

    if (b.entity && !filtered.some(function (s) { return s.key === b.entity; })) {
      var curOpt = document.createElement("option");
      curOpt.value = b.entity;
      curOpt.textContent = b.entity + " (current)";
      curOpt.selected = true;
      sel.appendChild(curOpt);
    }

    sel.addEventListener("change", function () {
      var key = this.value;
      if (!key) return;
      b.entity = key;
      helpers.saveField("entity", key);
      var sensor = sensors.find(function (s) { return s.key === key; });
      if (!sensor) return;
      if (!b.label) {
        b.label = sensor.name;
        labelInp.value = sensor.name;
        helpers.saveField("label", sensor.name);
      }
      if (!b.unit && sensor.unit) {
        b.unit = sensor.unit;
        unitInp.value = sensor.unit;
        helpers.saveField("unit", sensor.unit);
      }
      setMode(sensor.type === "text" ? "text" : "numeric", true);
    });
    sf.appendChild(sel);
    pickerSection.appendChild(sf);

    var tog = toggleRow("Show internal sensors", helpers.idPrefix + "show-all", showAll);
    tog.input.addEventListener("change", function () {
      showAll = this.checked;
      buildDropdown(sensors);
    });
    pickerSection.appendChild(tog.row);
  }

  function buildManualInput() {
    pickerSection.innerHTML = "";
    pickerSection.className = "sp-local-picker-fallback";

    var errDiv = document.createElement("div");
    errDiv.className = "sp-banner sp-error";
    errDiv.textContent = "Could not reach device. Enter sensor key manually.";
    pickerSection.appendChild(errDiv);

    var kf = document.createElement("div");
    kf.className = "sp-field";
    kf.appendChild(helpers.fieldLabel("Sensor Key", helpers.idPrefix + "local-sensor-key"));
    var keyInp = helpers.textInput(helpers.idPrefix + "local-sensor-key", b.entity, "e.g. room_temp");
    kf.appendChild(keyInp);
    pickerSection.appendChild(kf);
    helpers.bindField(keyInp, "entity", true);
    helpers.requireField(keyInp, "Add a sensor key before saving.");
  }

  var loadingDiv = document.createElement("div");
  loadingDiv.className = "sp-field";
  loadingDiv.textContent = "Loading sensors…";
  pickerSection.appendChild(loadingDiv);

  fetch("/local_sensors")
    .then(function (resp) {
      if (!resp.ok) throw new Error("HTTP " + resp.status);
      return resp.json();
    })
    .then(function (data) {
      fetchedSensors = data;
      buildDropdown(data);
    })
    .catch(function () {
      buildManualInput();
    });
}

function sensorLocalPreview(b, helpers) {
  if (b.precision === "text") {
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, "State", SENSOR_CARD_METADATA.preview.numericBadge),
    };
  }

  var label = b.label || b.entity || "Local Sensor";
  var unit = b.unit ? helpers.escHtml(b.unit) : "";
  var prec = parseInt(b.precision || "0", 10) || 0;
  var sampleVal = (0).toFixed(prec);
  return {
    iconHtml: cardSensorPreviewHtml(b, helpers, sampleVal, unit),
    labelHtml: cardBadgeLabelHtml(helpers, label, SENSOR_CARD_METADATA.preview.numericBadge),
  };
}
