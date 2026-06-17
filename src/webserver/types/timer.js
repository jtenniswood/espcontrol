// Timer card: live countdown and modal controls for Home Assistant timer helpers.
var TIMER_DEFAULT_MINUTES_OPTION = "default_minutes";
var TIMER_DEFAULT_MINUTES = 10;
var TIMER_MAX_MINUTES = 99 * 60 + 59;

function normalizeTimerDefaultMinutes(value) {
  var parsed = parseInt(String(value || ""), 10);
  if (!Number.isFinite(parsed) || parsed < 1) return TIMER_DEFAULT_MINUTES;
  if (parsed > TIMER_MAX_MINUTES) return TIMER_MAX_MINUTES;
  return parsed;
}

function timerDefaultMinutes(options) {
  return normalizeTimerDefaultMinutes(configOptionValue(options, TIMER_DEFAULT_MINUTES_OPTION));
}

function timerOptions(defaultMinutes) {
  return TIMER_DEFAULT_MINUTES_OPTION + "=" + normalizeTimerDefaultMinutes(defaultMinutes);
}

function normalizeTimerConfig(b) {
  if (!b) return;
  b.sensor = "";
  b.unit = "";
  b.precision = "";
  b.icon_on = "Auto";
  if (!b.icon || b.icon === "Auto") b.icon = "Timer";
  b.options = timerOptions(timerDefaultMinutes(b.options));
}

function timerHoursFromMinutes(totalMinutes) {
  return Math.floor(normalizeTimerDefaultMinutes(totalMinutes) / 60);
}

function timerMinutesRemainder(totalMinutes) {
  return normalizeTimerDefaultMinutes(totalMinutes) % 60;
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
    placeholder: "e.g. Kitchen Timer",
    rerender: true,
  },
  icon: {
    pickerIdSuffix: "timer-icon-picker",
    idSuffix: "timer-icon",
    field: "icon",
    fallback: "Timer",
    label: "Icon",
  },
};

registerButtonType("timer", {
  label: function () { return cardContractCardLabel("timer"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("timer"); },
  pickerKey: function () { return cardContractPickerKey("timer"); },
  experimental: function () { return cardContractExperimental("timer"); },
  hidden: function () { return cardContractHidden("timer"); },
  defaultConfig: function () { return cardContractDefaultConfig("timer"); },
  cardMetadata: TIMER_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "Timer";
    b.icon = "Timer";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = timerOptions(TIMER_DEFAULT_MINUTES);
  },
  renderSettings: function (panel, b, slot, helpers) {
    normalizeTimerConfig(b);
    helpers.saveField("sensor", "");
    helpers.saveField("unit", "");
    helpers.saveField("precision", "");
    helpers.saveField("icon_on", "Auto");
    helpers.saveField("options", b.options);

    helpers.renderCardEntityField(panel, b, helpers, TIMER_CARD_METADATA);
    helpers.renderCardTextField(panel, b, helpers, TIMER_CARD_METADATA.labelField);

    var defaultMinutes = timerDefaultMinutes(b.options);
    var hoursControl = helpers.renderCardNumberField(panel, b, helpers, {
      label: "Default Hours",
      idSuffix: "timer-default-hours",
      min: 0,
      max: 99,
      step: 1,
      value: function () { return String(timerHoursFromMinutes(defaultMinutes)); },
      placeholder: "0",
    });
    var minutesControl = helpers.renderCardNumberField(panel, b, helpers, {
      label: "Default Minutes",
      idSuffix: "timer-default-minutes",
      min: 0,
      max: 59,
      step: 1,
      value: function () { return String(timerMinutesRemainder(defaultMinutes)); },
      placeholder: "10",
    });

    function saveDefaultDuration() {
      var hours = parseInt(hoursControl.input.value || "0", 10);
      var minutes = parseInt(minutesControl.input.value || "0", 10);
      if (!Number.isFinite(hours) || hours < 0) hours = 0;
      if (!Number.isFinite(minutes) || minutes < 0) minutes = 0;
      if (hours > 99) hours = 99;
      if (minutes > 59) minutes = 59;
      defaultMinutes = normalizeTimerDefaultMinutes(hours * 60 + minutes);
      hoursControl.input.value = String(timerHoursFromMinutes(defaultMinutes));
      minutesControl.input.value = String(timerMinutesRemainder(defaultMinutes));
      b.options = timerOptions(defaultMinutes);
      helpers.saveField("options", b.options);
      renderPreview();
    }

    hoursControl.input.addEventListener("change", saveDefaultDuration);
    hoursControl.input.addEventListener("blur", saveDefaultDuration);
    minutesControl.input.addEventListener("change", saveDefaultDuration);
    minutesControl.input.addEventListener("blur", saveDefaultDuration);

    helpers.renderCardIconPicker(panel, b, helpers, TIMER_CARD_METADATA.icon);
  },
  renderPreview: function (b, helpers) {
    var defaultMinutes = timerDefaultMinutes(b.options);
    var hours = timerHoursFromMinutes(defaultMinutes);
    var minutes = timerMinutesRemainder(defaultMinutes);
    var value = hours > 0
      ? String(hours) + ":" + String(minutes).padStart(2, "0") + ":00"
      : String(minutes) + ":00";
    var label = b.label || b.entity || "Timer";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "timer-outline";
    return {
      iconHtml:
        '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
        '<span class="sp-sensor-preview"><span class="sp-sensor-value">' +
        helpers.escHtml(value) + "</span></span>",
      labelHtml: cardBadgeLabelHtml(helpers, label, "timer-outline"),
    };
  },
});
