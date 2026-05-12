// Read-only sensor card: displays either numeric data or a text state.
registerButtonType("sensor", {
  label: "Sensor",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.entity = "";
    b.icon_on = "Auto";
    if (!b.precision) b.precision = "";
    if (b.precision !== "text") b.icon = "Auto";
    if (b.precision === "text") b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var isTextMode = b.precision === "text";
    var isLargeCard = helpers.cardSize === 4;

    var mode = helpers.segmentControl([
      ["numeric", "Numeric"],
      ["text", "Text"],
    ], isTextMode ? "text" : "numeric", function (value) { setMode(value, true); });
    var numericBtn = mode.buttons.numeric;
    var textBtn = mode.buttons.text;
    panel.appendChild(helpers.fieldWithControl("Type", null, mode.segment));

    var sensorField = helpers.entityField(
      "Sensor Entity", helpers.idPrefix + "sensor", b.sensor,
      "e.g. sensor.living_room_temperature",
      ["sensor", "binary_sensor", "text_sensor"], "sensor", true,
      "Add a sensor entity before saving.");
    panel.appendChild(sensorField.field);

    var numericSection = condField();

    var labelField = helpers.textField(
      "Label", helpers.idPrefix + "label", b.label, "e.g. Living Room", "label", true);
    var labelInp = labelField.input;
    numericSection.appendChild(labelField.field);

    var unitField = helpers.textField(
      "Unit", helpers.idPrefix + "unit", b.unit, "e.g. \u00B0C", "unit", true);
    var unitInp = unitField.input;
    unitInp.className = "sp-input";
    numericSection.appendChild(unitField.field);

    var precisionField = helpers.precisionField(helpers.idPrefix + "precision",
      !isTextMode ? (b.precision || "0") : "0", function () {
      b.precision = this.value === "0" ? "" : this.value;
      helpers.saveField("precision", b.precision);
    });
    var precisionSelect = precisionField.select;
    numericSection.appendChild(precisionField.field);

    if (isLargeCard) {
      var largeNumbersToggle = helpers.toggleRow(
        "Large Sensor Numbers", helpers.idPrefix + "large-sensor-numbers",
        sensorLargeNumbersEnabled(b));
      numericSection.appendChild(largeNumbersToggle.row);
      largeNumbersToggle.input.addEventListener("change", function () {
        setSensorLargeNumbersEnabled(b, this.checked);
        helpers.saveField("options", b.options);
      });
    }
    panel.appendChild(numericSection);

    var textSection = condField();
    var textIconPicker = helpers.iconPickerField(
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
        b.options = "";
        labelInp.value = "";
        unitInp.value = "";
        helpers.saveField("precision", "text");
        helpers.saveField("label", "");
        helpers.saveField("unit", "");
        helpers.saveField("icon_on", "Auto");
        helpers.saveField("options", "");
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

    setMode(isTextMode ? "text" : "numeric", false);
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "text") {
      var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">State</span>' +
          '<span class="sp-type-badge mdi mdi-format-text"></span></span>',
      };
    }

    var label = b.label || b.sensor || "Sensor";
    var unit = b.unit ? helpers.escHtml(b.unit) : "";
    var prec = parseInt(b.precision || "0", 10) || 0;
    var sampleVal = (0).toFixed(prec);
    var previewClass = "sp-sensor-preview" +
      (helpers.cardSize === 4 && sensorLargeNumbersEnabled(b) ? " sp-sensor-preview-large" : "");
    return {
      iconHtml:
        '<span class="' + previewClass + '">' +
          '<span class="sp-sensor-value">' + sampleVal + '</span>' +
          '<span class="sp-sensor-unit">' + unit + '</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-gauge"></span></span>',
    };
  },
});
