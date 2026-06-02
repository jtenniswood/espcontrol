var HA_CALENDAR_CARD_METADATA = {
  entity: {
    label: "Calendar Entities",
    idSuffix: "entity",
    placeholder: "e.g. calendar.my_cal (comma-separated for multiple)",
    domains: function () { return cardContractDomains("ha_calendar"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add at least one calendar entity before saving.",
  },
  displayMode: {
    label: "Display Mode",
    options: [
      ["", "Next"],
      ["current", "Current"],
    ],
  },
  modalLayout: {
    label: "Modal Layout",
    options: [
      ["", "Compact"],
      ["column", "Time Column"],
    ],
  },
  preview: {
    badge: "calendar-clock",
  },
};

registerButtonType("ha_calendar", {
  label: function () { return cardContractCardLabel("ha_calendar"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("ha_calendar"); },
  pickerKey: function () { return cardContractPickerKey("ha_calendar"); },
  experimental: function () { return cardContractExperimental("ha_calendar"); },
  hidden: function () { return cardContractHidden("ha_calendar"); },
  hideLabel: true,
  labelPlaceholder: "e.g. Calendar",
  defaultConfig: function () { return cardContractDefaultConfig("ha_calendar"); },
  cardMetadata: HA_CALENDAR_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";

    helpers.renderCardEntityField(panel, b, helpers, HA_CALENDAR_CARD_METADATA);

    helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, HA_CALENDAR_CARD_METADATA.displayMode, {
        value: function () { return haCalendarDisplayMode(b); },
        onSelect: function (button, cardHelpers, value) {
          setHaCalendarDisplayMode(button, value);
          cardHelpers.saveField("options", button.options);
          scheduleRender();
        },
      }),
    });

    helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, HA_CALENDAR_CARD_METADATA.modalLayout, {
        value: function () { return haCalendarModalLayout(b); },
        onSelect: function (button, cardHelpers, value) {
          setHaCalendarModalLayout(button, value);
          cardHelpers.saveField("options", button.options);
          scheduleRender();
        },
      }),
    });

    helpers.renderCardTextField(panel, b, helpers, {
      label: "Label (optional)",
      idSuffix: "label",
      field: "label",
      placeholder: "Calendar",
      rerender: true,
    });
  },
});
