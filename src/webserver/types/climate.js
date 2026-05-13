// Climate card: thermostat status plus full-screen climate controls.
registerButtonType("climate", {
  label: "Climate",
  allowInSubpage: true,
  hideLabel: true,
  labelPlaceholder: "e.g. Living Room",
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Thermostat";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    if (!b.icon) b.icon = "Thermostat";
    if (b.icon_on !== "Auto") {
      b.icon_on = "Auto";
      helpers.saveField("icon_on", "Auto");
    }
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

    var entityField = helpers.entityField(
      "Climate Entity", helpers.idPrefix + "entity", b.entity,
      "e.g. climate.living_room", ["climate"], "entity", true,
      "Add a climate entity before saving.");
    panel.appendChild(entityField.field);

    panel.appendChild(helpers.textField(
      "Label", helpers.idPrefix + "label", b.label, "e.g. Living Room", "label", true).field);

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "climate-icon-picker", helpers.idPrefix + "climate-icon",
      b.icon || "Thermostat", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }, "Icon"
    ));

    var precisionField = helpers.selectField("Unit Precision", helpers.idPrefix + "climate-precision", [
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
    panel.appendChild(precisionField.field);

    var hasRange = !!(climateConfig.min || climateConfig.max);
    var advancedToggleSection = helpers.toggleSection(
      "Advanced",
      helpers.idPrefix + "climate-advanced-toggle",
      hasRange
    );
    var advancedToggle = advancedToggleSection.toggle;
    var advanced = advancedToggleSection.section;
    panel.appendChild(advancedToggle.row);
    if (hasRange) advanced.classList.add("sp-visible");

    var minField = helpers.textField(
      "Minimum Temperature", helpers.idPrefix + "climate-min", climateConfig.min, "e.g. 16");
    var minInp = minField.input;
    minInp.inputMode = "decimal";
    advanced.appendChild(minField.field);

    var maxField = helpers.textField(
      "Maximum Temperature", helpers.idPrefix + "climate-max", climateConfig.max, "e.g. 30");
    var maxInp = maxField.input;
    maxInp.inputMode = "decimal";
    advanced.appendChild(maxField.field);

    minInp.addEventListener("change", saveClimateAdvancedSettings);
    maxInp.addEventListener("change", saveClimateAdvancedSettings);
    advancedToggle.input.addEventListener("change", function () {
      if (this.checked) {
        advanced.classList.add("sp-visible");
      } else {
        advanced.classList.remove("sp-visible");
        minInp.value = "";
        maxInp.value = "";
        saveClimateAdvancedSettings();
      }
    });
    panel.appendChild(advanced);
  },
  renderPreview: function (b, helpers) {
    var label = (b.label && b.label.trim()) || (b.entity && b.entity.trim()) || "Climate";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "thermostat";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
        helpers.escHtml(label) + '</span><span class="sp-type-badge mdi mdi-thermostat"></span></span>',
    };
  },
});
