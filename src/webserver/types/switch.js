// Default button type: HA entity toggle (on/off switch)
var SWITCH_CARD_METADATA = {
  entity: {
    label: "Entity",
    placeholder: "e.g. light.kitchen",
    domains: function () { return cardContractDomains(""); },
    requiredMessage: "Add an entity before saving.",
  },
  iconOff: {
    field: "icon",
    fallback: "Auto",
    label: "Off Icon",
  },
  iconOn: {
    field: "icon_on",
    fallback: "Auto",
    label: "On Icon",
  },
  activeDisplay: {
    label: "Active Display",
    idSuffix: "sensor-when-on-toggle",
    checked: function (b) { return !!b.sensor; },
  },
  largeNumbers: {
    label: "Large Active Display Numbers",
    idSuffix: "large-active-display-numbers",
    supported: function (b) {
      return !!(b && b.sensor && b.precision !== "text");
    },
  },
  sensorMode: {
    label: "Type",
    options: [
      ["numeric", "Numeric"],
      ["text", "Text"],
    ],
  },
  sensorEntity: {
    label: "Sensor Entity",
    idSuffix: "sensor",
    placeholder: "e.g. sensor.printer_percent_complete",
    domains: ["sensor", "binary_sensor", "text_sensor"],
    bindName: "sensor",
  },
  unitField: {
    label: "Unit",
    idSuffix: "unit",
    placeholder: "e.g. %",
    bindName: "unit",
    rerender: false,
  },
  confirmationToggle: {
    label: "Confirmation Required",
    idSuffix: "confirm-toggle",
    checked: function (b) { return switchConfirmationEnabled(b); },
  },
  confirmationMode: {
    label: "When",
    options: [
      ["off", "Off"],
      ["on", "On"],
      ["both", "Both"],
    ],
  },
  confirmationMessage: {
    label: "Message",
    idSuffix: "confirm-message",
    placeholder: SWITCH_CONFIRM_DEFAULT_MESSAGE,
    bindName: null,
    value: function (b) { return switchConfirmationMessage(b); },
  },
  confirmationYes: {
    label: "Confirm Button",
    idSuffix: "confirm-yes",
    placeholder: SWITCH_CONFIRM_DEFAULT_YES,
    bindName: null,
    value: function (b) { return switchConfirmationYesText(b); },
  },
  confirmationNo: {
    label: "Cancel Button",
    idSuffix: "confirm-no",
    placeholder: SWITCH_CONFIRM_DEFAULT_NO,
    bindName: null,
    value: function (b) { return switchConfirmationNoText(b); },
  },
  preview: {
    switchBadge: "toggle-switch-variant-off",
    numericBadge: "gauge",
    textBadge: "format-text",
  },
};

var LIGHT_SWITCH_CARD_METADATA = {
  mode: LIGHT_CONTROL_TYPE_METADATA.mode,
  entity: {
    label: "Entity",
    placeholder: "e.g. light.living_room",
    domains: function () { return cardContractDomains("light_switch"); },
    requiredMessage: "Add a light entity before saving.",
  },
  labelField: {
    label: "Label",
    placeholder: "e.g. Living Room",
  },
  iconOff: {
    field: "icon",
    fallback: "Auto",
    label: "Off Icon",
  },
  iconOn: {
    field: "icon_on",
    fallback: "Auto",
    label: "On Icon",
  },
  preview: {
    badge: "lightbulb",
  },
};

registerButtonType("", {
  label: function () { return cardContractCardLabel(""); },
  allowInSubpage: function () { return cardContractAllowInSubpage(""); },
  pickerKey: function () { return cardContractPickerKey(""); },
  hidden: function () { return cardContractHidden(""); },
  defaultConfig: function () { return cardContractDefaultConfig(""); },
  cardMetadata: SWITCH_CARD_METADATA,
  renderSettings: function (panel, b, slot, helpers) {
    var showSensor = !!b.sensor;
    var sensorMode = b.precision === "text" ? "text" : "numeric";

    helpers.renderBasicCardFields(panel, b, helpers, SWITCH_CARD_METADATA);

    var sensorToggle = helpers.renderCardOptionToggle(panel, b, helpers, SWITCH_CARD_METADATA.activeDisplay);
    var sensorSection = condField();
    if (showSensor) sensorSection.classList.add("sp-visible");

    var mode = helpers.renderCardSegmentControl(sensorSection, b, helpers, Object.assign({}, SWITCH_CARD_METADATA.sensorMode, {
      value: function () { return sensorMode; },
      onSelect: function (b, helpers, value) { setSensorMode(value, true); },
    }));
    var numericBtn = mode.buttons.numeric;
    var textBtn = mode.buttons.text;

    var sensorField = helpers.renderCardEntityField(sensorSection, b, helpers, {
      entity: SWITCH_CARD_METADATA.sensorEntity,
    });
    var sensorInp = sensorField.input;

    var numericSection = condField();

    var unitField = helpers.renderCardTextField(numericSection, b, helpers, SWITCH_CARD_METADATA.unitField);
    var unitInp = unitField.input;

    var precisionField = helpers.precisionField(helpers.idPrefix + "precision",
      sensorMode === "numeric" ? (b.precision || "0") : "0", function () {
      b.precision = this.value === "0" ? "" : this.value;
      helpers.saveField("precision", b.precision);
    });
    var precisionSelect = precisionField.select;
    numericSection.appendChild(precisionField.field);
    helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, SWITCH_CARD_METADATA);
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
    var confirmMode = switchConfirmationMode(b) || "off";
    var confirmToggle = helpers.renderCardOptionToggle(panel, b, helpers, SWITCH_CARD_METADATA.confirmationToggle);
    var confirmSection = condField();
    if (confirmOn) confirmSection.classList.add("sp-visible");

    helpers.renderCardSegmentControl(confirmSection, b, helpers, Object.assign({}, SWITCH_CARD_METADATA.confirmationMode, {
      value: function () { return confirmMode; },
      onSelect: function (b, helpers, value) {
        var previousDefault = switchConfirmationDefaultMessageForMode(confirmMode);
        confirmMode = value;
        if (!messageInput.value || messageInput.value === previousDefault) {
          messageInput.value = switchConfirmationDefaultMessageForMode(confirmMode);
        }
        saveConfirmationOptions();
      },
    }));

    var messageField = helpers.renderCardTextField(confirmSection, b, helpers, SWITCH_CARD_METADATA.confirmationMessage);
    var messageInput = messageField.input;
    messageInput.maxLength = 72;

    var yesField = helpers.renderCardTextField(confirmSection, b, helpers, SWITCH_CARD_METADATA.confirmationYes);
    var yesInput = yesField.input;
    yesInput.maxLength = 20;

    var noField = helpers.renderCardTextField(confirmSection, b, helpers, SWITCH_CARD_METADATA.confirmationNo);
    var noInput = noField.input;
    noInput.maxLength = 20;

    panel.appendChild(confirmSection);

    function saveConfirmationOptions() {
      setSwitchConfirmationOptions(
        b,
        confirmToggle.input.checked ? confirmMode : "",
        messageInput.value || switchConfirmationDefaultMessageForMode(confirmMode),
        yesInput.value || SWITCH_CONFIRM_DEFAULT_YES,
        noInput.value || SWITCH_CONFIRM_DEFAULT_NO
      );
      helpers.saveField("options", b.options);
    }

    confirmToggle.input.addEventListener("change", function () {
      confirmSection.classList.toggle("sp-visible", this.checked);
      if (this.checked) {
        if (!messageInput.value) messageInput.value = switchConfirmationDefaultMessageForMode(confirmMode);
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
      ? (b.precision === "text" ? SWITCH_CARD_METADATA.preview.textBadge : SWITCH_CARD_METADATA.preview.numericBadge)
      : SWITCH_CARD_METADATA.preview.switchBadge;
    var preview = {
      labelHtml: cardBadgeLabelHtml(helpers, label, badgeIcon),
    };
    if (b.sensor && b.precision !== "text" &&
        cardLargeNumbersActiveForCardSize(b, helpers, SWITCH_CARD_METADATA)) {
      preview.iconHtml = cardSensorPreviewHtml(b, helpers, "42", b.unit || "");
    }
    return preview;
  },
});

registerButtonType("light_switch", {
  label: function () { return cardContractCardLabel("light_switch"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("light_switch"); },
  hideLabel: true,
  pickerKey: function () { return cardContractPickerKey("light_switch"); },
  hidden: function () { return cardContractHidden("light_switch"); },
  defaultConfig: function () { return cardContractDefaultConfig("light_switch"); },
  isAvailable: function () {
    return false;
  },
  labelPlaceholder: "e.g. Living Room",
  cardMetadata: LIGHT_SWITCH_CARD_METADATA,
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

    helpers.renderBasicCardFields(panel, b, helpers, LIGHT_SWITCH_CARD_METADATA);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Configure";
    return {
      labelHtml: cardBadgeLabelHtml(helpers, label, LIGHT_SWITCH_CARD_METADATA.preview.badge),
    };
  },
});
