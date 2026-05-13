// Default button type: HA entity toggle (on/off switch)
registerButtonType("", {
  label: "Switch",
  allowInSubpage: true,
  renderSettings: function (panel, b, slot, helpers) {
    var showSensor = !!b.sensor;
    var sensorMode = b.precision === "text" ? "text" : "numeric";

    var entityField = helpers.entityField(
      "Entity", helpers.idPrefix + "entity", b.entity, "e.g. light.kitchen",
      ["light", "switch", "input_boolean", "fan"], "entity", true,
      "Add an entity before saving.");
    panel.appendChild(entityField.field);

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Auto", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }, "Off Icon"
    ));

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "icon-on-picker", helpers.idPrefix + "icon-on",
      b.icon_on || "Auto", function (opt) {
        b.icon_on = opt;
        helpers.saveField("icon_on", opt);
      }, "On Icon"
    ));

    var sensorToggleSection = helpers.toggleSection(
      "Active Display",
      helpers.idPrefix + "sensor-when-on-toggle",
      showSensor
    );
    var sensorToggle = sensorToggleSection.toggle;
    var sensorSection = sensorToggleSection.section;
    panel.appendChild(sensorToggle.row);
    if (showSensor) sensorSection.classList.add("sp-visible");

    var mode = helpers.segmentControl([
      ["numeric", "Numeric"],
      ["text", "Text"],
    ], sensorMode, function (value) { setSensorMode(value, true); });
    var numericBtn = mode.buttons.numeric;
    var textBtn = mode.buttons.text;
    sensorSection.appendChild(helpers.fieldWithControl("Type", null, mode.segment));

    var sensorField = helpers.entityField(
      "Sensor Entity", helpers.idPrefix + "sensor", b.sensor,
      "e.g. sensor.printer_percent_complete",
      ["sensor", "binary_sensor", "text_sensor"], "sensor", true);
    var sensorInp = sensorField.input;
    sensorSection.appendChild(sensorField.field);

    var numericSection = condField();

    var unitField = helpers.textField("Unit", helpers.idPrefix + "unit", b.unit, "e.g. %", "unit", false);
    var unitInp = unitField.input;
    unitInp.className = "sp-input";
    numericSection.appendChild(unitField.field);

    var precisionField = helpers.precisionField(helpers.idPrefix + "precision",
      sensorMode === "numeric" ? (b.precision || "0") : "0", function () {
      b.precision = this.value === "0" ? "" : this.value;
      helpers.saveField("precision", b.precision);
    });
    var precisionSelect = precisionField.select;
    numericSection.appendChild(precisionField.field);
    sensorSection.appendChild(numericSection);

    panel.appendChild(sensorSection);

    function setSensorMode(mode, persist) {
      sensorMode = mode;
      numericBtn.classList.toggle("active", mode === "numeric");
      textBtn.classList.toggle("active", mode === "text");
      numericSection.classList.toggle("sp-visible", mode === "numeric");
      if (!persist) return;
      if (mode === "text") {
        b.precision = "text";
        b.unit = "";
        unitInp.value = "";
        helpers.saveField("precision", "text");
        helpers.saveField("unit", "");
      } else {
        b.precision = "";
        helpers.saveField("precision", "");
        precisionSelect.value = "0";
      }
    }

    setSensorMode(sensorMode, false);

    sensorToggle.input.addEventListener("change", function () {
      showSensor = this.checked;
      sensorSection.classList.toggle("sp-visible", showSensor);
      helpers.saveField("sensor", b.sensor || "");
      if (showSensor) {
        setSensorMode(sensorMode, true);
        return;
      }
      b.sensor = "";
      b.unit = "";
      b.precision = "";
      sensorInp.value = "";
      unitInp.value = "";
      helpers.saveField("sensor", "");
      helpers.saveField("unit", "");
      helpers.saveField("precision", "");
      setSensorMode("numeric", false);
    });

    var confirmOn = switchConfirmationEnabled(b);
    var confirmToggleSection = helpers.toggleSection(
      "Confirmation Required",
      helpers.idPrefix + "confirm-toggle",
      confirmOn
    );
    var confirmToggle = confirmToggleSection.toggle;
    var confirmSection = confirmToggleSection.section;
    panel.appendChild(confirmToggle.row);
    if (confirmOn) confirmSection.classList.add("sp-visible");

    var messageField = helpers.textField(
      "Message",
      helpers.idPrefix + "confirm-message",
      switchConfirmationMessage(b),
      SWITCH_CONFIRM_DEFAULT_MESSAGE
    );
    var messageInput = messageField.input;
    messageInput.maxLength = 72;
    confirmSection.appendChild(messageField.field);

    var yesField = helpers.textField(
      "Confirm Button",
      helpers.idPrefix + "confirm-yes",
      switchConfirmationYesText(b),
      SWITCH_CONFIRM_DEFAULT_YES
    );
    var yesInput = yesField.input;
    yesInput.maxLength = 20;
    confirmSection.appendChild(yesField.field);

    var noField = helpers.textField(
      "Cancel Button",
      helpers.idPrefix + "confirm-no",
      switchConfirmationNoText(b),
      SWITCH_CONFIRM_DEFAULT_NO
    );
    var noInput = noField.input;
    noInput.maxLength = 20;
    confirmSection.appendChild(noField.field);

    panel.appendChild(confirmSection);

    function saveConfirmationOptions() {
      setSwitchConfirmationOptions(
        b,
        confirmToggle.input.checked,
        messageInput.value || SWITCH_CONFIRM_DEFAULT_MESSAGE,
        yesInput.value || SWITCH_CONFIRM_DEFAULT_YES,
        noInput.value || SWITCH_CONFIRM_DEFAULT_NO
      );
      helpers.saveField("options", b.options);
    }

    confirmToggle.input.addEventListener("change", function () {
      confirmSection.classList.toggle("sp-visible", this.checked);
      if (this.checked) {
        if (!messageInput.value) messageInput.value = SWITCH_CONFIRM_DEFAULT_MESSAGE;
        if (!yesInput.value) yesInput.value = SWITCH_CONFIRM_DEFAULT_YES;
        if (!noInput.value) noInput.value = SWITCH_CONFIRM_DEFAULT_NO;
      }
      saveConfirmationOptions();
    });

    [messageInput, yesInput, noInput].forEach(function (input) {
      input.addEventListener("input", saveConfirmationOptions);
      input.addEventListener("change", saveConfirmationOptions);
      input.addEventListener("blur", saveConfirmationOptions);
    });
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Configure";
    var badgeIcon = b.sensor
      ? (b.precision === "text" ? "format-text" : "gauge")
      : "toggle-switch-variant-off";
    return {
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-' + badgeIcon + '"></span></span>',
    };
  },
});

registerButtonType("light_switch", {
  label: "Lights",
  allowInSubpage: true,
  hideLabel: true,
  pickerKey: "light_brightness",
  isAvailable: function () {
    return false;
  },
  labelPlaceholder: "e.g. Living Room",
  onSelect: function (b) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    renderLightControlTypeField(panel, b, helpers);

    panel.appendChild(helpers.entityField(
      "Entity",
      helpers.idPrefix + "entity",
      b.entity,
      "e.g. light.living_room",
      ["light"],
      "entity",
      true,
      "Add a light entity before saving."
    ).field);

    panel.appendChild(helpers.textField(
      "Label",
      helpers.idPrefix + "label",
      b.label,
      "e.g. Living Room",
      "label",
      true
    ).field);

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Auto", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }, "Off Icon"
    ));

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "icon-on-picker", helpers.idPrefix + "icon-on",
      b.icon_on || "Auto", function (opt) {
        b.icon_on = opt;
        helpers.saveField("icon_on", opt);
      }, "On Icon"
    ));
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Configure";
    return {
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-lightbulb"></span></span>',
    };
  },
});
