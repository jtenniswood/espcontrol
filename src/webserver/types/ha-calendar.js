var HA_CALENDAR_CARD_METADATA = {
  entity: {
    label: "Entity",
    idSuffix: "entity",
    placeholder: "e.g. calendar.my_cal (comma-separated for multiple)",
    domains: function () { return cardContractDomains("ha_calendar"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add at least one calendar entity before saving.",
  },
  mode: {
    label: "Type",
    idSuffix: "calendar-display-mode",
    options: [
      ["current", "Current event"],
      ["next_event", "Next event"],
    ],
    value: function (b) {
      return haCalendarDisplayMode(b);
    },
    onChange: function (b, helpers) {
      setHaCalendarDisplayMode(b, this.value);
      if (b.precision === "current" && !b.label) {
        b.label = "Now";
        helpers.saveField("label", b.label);
      }
      helpers.saveField("precision", b.precision);
      helpers.saveField("options", b.options);
      scheduleRender();
    },
  },
  preview: {
    badge: "calendar-clock",
  },
};

var HA_CALENDAR_MINUTE_OPTIONS = [
  ["1", "1m"],
  ["2", "2m"],
  ["3", "3m"],
  ["5", "5m"],
  ["10", "10m"],
];

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
  renderPreview: function (b, helpers) {
    // Event-card modes mirror the device layout: status icon top-left and
    // event title bottom-left. Time-to-next mode keeps the countdown preview.
    var mode = haCalendarDisplayMode(b);
    if (mode === "current" || mode === "next_event") {
      var title = "Event Name";
      var status = mode === "next_event" ? "In 12m" : "Now";
      return {
        buttonClass: "sp-ha-calendar-current",
        iconHtml: "",
        labelHtml:
          '<span class="sp-ha-calendar-title">' + helpers.escHtml(title) + '</span>' +
          '<span class="sp-ha-calendar-status">' + helpers.escHtml(status) + '</span>',
      };
    }
    return {
      iconHtml: cardSensorPreviewHtml(b, helpers, "47", "min"),
      labelHtml: cardBadgeLabelHtml(helpers, b.label || "Calendar", HA_CALENDAR_CARD_METADATA.preview.badge),
    };
  },
  onSelect: function (b) {
    b.entity = "";
    b.label = "Now";
    b.sensor = "";
    b.unit = "";
    b.precision = "current";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    b.precision = haCalendarDisplayMode(b);
    b.options = normalizeHaCalendarOptions(b.options);
    if (b.precision === "current" && !b.label) b.label = "Now";
    var mode = haCalendarDisplayMode(b);

    helpers.renderCardModeSelector(panel, b, helpers, HA_CALENDAR_CARD_METADATA);

    helpers.renderCardEntityField(panel, b, helpers, HA_CALENDAR_CARD_METADATA);

    if (mode === "current") {
      helpers.renderCardOptionToggle(panel, b, helpers, {
        label: "Progress indicator",
        idSuffix: "current-progress",
        checked: function (button) { return haCalendarCurrentProgressEnabled(button); },
        onChange: function (button, cardHelpers, checked) {
          setHaCalendarCurrentProgressEnabled(button, checked);
          cardHelpers.saveField("options", button.options);
          scheduleRender();
        },
      });

      helpers.renderCardTextField(panel, b, helpers, {
        label: "Label",
        idSuffix: "label",
        field: "label",
        placeholder: "Now",
        rerender: true,
      });
    }

    if (mode === "next_event") {
      var urgentToggle = helpers.renderCardOptionToggle(panel, b, helpers, {
        label: "Next event highlight",
        idSuffix: "urgent-color",
        checked: function (button) { return haCalendarUrgentColorEnabled(button); },
        onChange: function (button, cardHelpers, checked) {
          setHaCalendarUrgentColorEnabled(button, checked);
          cardHelpers.saveField("options", button.options);
          scheduleRender();
          renderButtonSettings();
        },
      });

      if (urgentToggle.input.checked) {
        var urgentMinutes = helpers.selectField(
          "Minutes before event",
          helpers.idPrefix + "urgent-minutes",
          HA_CALENDAR_MINUTE_OPTIONS,
          String(haCalendarUrgentMinutes(b)),
          function () {
            setHaCalendarUrgentMinutes(b, this.value);
            helpers.saveField("options", b.options);
          }
        );
        panel.appendChild(urgentMinutes.field);
      }
    }

    if (mode === "next_event") {
      var nextNowToggle = helpers.renderCardOptionToggle(panel, b, helpers, {
        label: "Events starting",
        idSuffix: "next-now-enabled",
        checked: function (button) { return haCalendarNextNowEnabled(button); },
        onChange: function (button, cardHelpers, checked) {
          setHaCalendarNextNowEnabled(button, checked);
          cardHelpers.saveField("options", button.options);
          scheduleRender();
          renderButtonSettings();
        },
      });
      if (nextNowToggle.input.checked) {
        var nextNowMinutes = helpers.selectField(
          "Show Now for",
          helpers.idPrefix + "next-now-minutes",
          HA_CALENDAR_MINUTE_OPTIONS,
          String(haCalendarNextNowMinutes(b)),
          function () {
            setHaCalendarNextNowMinutes(b, this.value);
            helpers.saveField("options", b.options);
          }
        );
        panel.appendChild(nextNowMinutes.field);
      }
    }
  },
});
