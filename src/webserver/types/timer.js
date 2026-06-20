// Timer card: counts down a Home Assistant timer.* entity.
// Storage: sensor="confirm" enables cancel confirmation; unit stores the timeout in seconds.

function timerParseConfirmTimeout(unit) {
  var value = parseInt(unit, 10);
  if (!isFinite(value) || value < 1) value = 3;
  if (value > 30) value = 30;
  return value;
}

var TIMER_CARD_METADATA = {
  entity: {
    label: "Timer Entity",
    idSuffix: "timer-entity",
    placeholder: "e.g. timer.kitchen",
    domains: function () { return cardContractDomains("timer"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a timer entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "timer-label",
    field: "label",
    placeholder: "e.g. Kitchen",
    rerender: true,
  },
  preview: {
    badge: "timer-outline",
  },
};

registerButtonType("timer", {
  label: function () { return cardContractCardLabel("timer"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("timer"); },
  pickerKey: function () { return cardContractPickerKey("timer"); },
  hidden: function () { return cardContractHidden("timer"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("timer"); },
  cardMetadata: TIMER_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "3";
    b.precision = "";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.precision = "";
    b.options = "";

    helpers.renderCardEntityField(panel, b, helpers, TIMER_CARD_METADATA);
    helpers.renderCardTextField(panel, b, helpers, TIMER_CARD_METADATA.labelField);

    var confirmRow = helpers.toggleRow(
      "Confirm before cancel",
      helpers.idPrefix + "timer-confirm",
      b.sensor === "confirm"
    );

    var timeoutField = document.createElement("div");
    timeoutField.className = "sp-field";
    timeoutField.appendChild(
      helpers.fieldLabel("Confirm Timeout (s)", helpers.idPrefix + "timer-confirm-timeout")
    );
    var timeoutInput = document.createElement("input");
    timeoutInput.type = "number";
    timeoutInput.className = "sp-input";
    timeoutInput.id = helpers.idPrefix + "timer-confirm-timeout";
    timeoutInput.min = "1";
    timeoutInput.max = "30";
    timeoutInput.step = "1";
    timeoutInput.placeholder = "3";
    timeoutInput.value = timerParseConfirmTimeout(b.unit);
    timeoutField.appendChild(timeoutInput);

    panel.appendChild(confirmRow.row);
    panel.appendChild(timeoutField);

    function applyConfirmVisibility() {
      timeoutField.style.display = b.sensor === "confirm" ? "" : "none";
    }

    function saveTimeout() {
      var value = timerParseConfirmTimeout(timeoutInput.value);
      timeoutInput.value = value;
      b.unit = String(value);
      helpers.saveField("unit", b.unit);
    }

    applyConfirmVisibility();
    confirmRow.input.addEventListener("change", function () {
      b.sensor = this.checked ? "confirm" : "";
      helpers.saveField("sensor", b.sensor);
      applyConfirmVisibility();
    });
    timeoutInput.addEventListener("change", saveTimeout);
    timeoutInput.addEventListener("blur", saveTimeout);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Timer";
    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">0:00</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
          helpers.escHtml(label) +
        '</span><span class="sp-type-badge mdi mdi-timer-outline"></span></span>',
    };
  },
});
