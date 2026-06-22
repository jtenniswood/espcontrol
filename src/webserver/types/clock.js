// Read-only local clock card: displays the panel's local time only.
registerButtonType("clock", {
  label: function () { return cardContractCardLabel("clock"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("clock"); },
  pickerKey: function () { return cardContractPickerKey("clock"); },
  hidden: function () { return cardContractHidden("clock"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("clock"); },
  isAvailable: function () {
    return false;
  },
  cardMetadata: DATE_TIME_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.entity = "";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";

    helpers.renderCardModeSelector(panel, b, helpers, DATE_TIME_CARD_METADATA);
    helpers.renderCardLargeNumbersToggle(panel, b, helpers, DATE_TIME_CARD_METADATA);
  },
  renderPreview: function (b, helpers) {
    var time = dateTimeCardTimeParts();
    return {
      buttonClass: cardLargeNumbersHidePreviewLabel(b, helpers, DATE_TIME_CARD_METADATA)
        ? "sp-clock-wide-large"
        : undefined,
      iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
      labelHtml: "",
    };
  },
});
