// Read-only date card: displays either the day/month or local time/date.
var DATE_TIME_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "calendar-mode",
    options: [
      { value: "clock", label: "Clock" },
      { value: "datetime", label: "Time & Date" },
      { value: "", label: "Date" },
      { value: "timezone", label: "World Clock" }
    ],
    value: function (b) {
      return dateTimeCardMode(b);
    },
    onChange: function (b, helpers) {
      setDateTimeCardMode(b, this.value, helpers);
    },
  },
  largeNumbers: {
    label: function (b) {
      return dateTimeLargeNumbersLabel(b);
    },
    idSuffix: "large-date-time-numbers",
    supportedCardSize: function (b, helpers) {
      var cardSize = (helpers && helpers.cardSize) || CARD_SIZE_SINGLE;
      return dateTimeCardMode(b) === "clock"
        ? cardSize === CARD_SIZE_WIDE || cardSize === CARD_SIZE_LARGE
        : cardSize === CARD_SIZE_LARGE;
    },
    hideLabel: function (_b, helpers) {
      return ((helpers && helpers.cardSize) || CARD_SIZE_SINGLE) === CARD_SIZE_WIDE;
    },
  },
  preview: {
    dateBadge: "calendar-month",
    timezoneBadge: "map-clock",
  },
};

function dateTimeCardMode(b) {
  if (b && b.type === "clock") return "clock";
  if (b && b.type === "timezone") return "timezone";
  return b && b.precision === "datetime" ? "datetime" : "";
}

function dateTimeLargeNumbersLabel(b) {
  var mode = dateTimeCardMode(b);
  if (mode === "clock") return "Large Clock";
  if (mode === "datetime") return "Large Time";
  if (mode === "timezone") return "Large World Clock";
  return "Large Date";
}

function defaultTimezoneCardEntity() {
  return (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
}

function dateTimeModeOptionValues() {
  var spec = cardContractOptionSpec("calendar", "date_time_mode");
  return spec && spec.values ? spec.values.slice() : ["clock", "datetime", "", "timezone"];
}

function normalizeDateTimeCardMode(mode) {
  mode = String(mode || "");
  return dateTimeModeOptionValues().indexOf(mode) >= 0 ? mode : "";
}

function setDateTimeCardMode(b, mode, helpers) {
  mode = normalizeDateTimeCardMode(mode);
  if (b.type !== "timezone" && b.type !== "clock" && mode !== "timezone" && mode !== "clock") {
    b.precision = mode === "datetime" ? "datetime" : "";
    helpers.saveField("precision", b.precision);
    return;
  }

  if (mode === "clock") {
    b.type = "clock";
    helpers.applyCardMetadataFields(b, helpers, {
      type: "clock",
      entity: "",
      label: "",
      icon: "Auto",
      icon_on: "Auto",
      sensor: "",
      unit: "",
      precision: "",
      options: b.options,
    });
    renderButtonSettings();
    return;
  }

  if (mode === "timezone") {
    b.type = "timezone";
    helpers.applyCardMetadataFields(b, helpers, {
      type: "timezone",
      entity: defaultTimezoneCardEntity,
      label: "",
      icon: "Auto",
      icon_on: "Auto",
      sensor: "",
      unit: "",
      precision: "",
      options: b.options,
    });
    renderButtonSettings();
    return;
  }

  b.type = "calendar";
  helpers.applyCardMetadataFields(b, helpers, {
    type: "calendar",
    entity: "sensor.date",
    label: "",
    icon: "Auto",
    icon_on: "Auto",
    sensor: "",
    unit: "",
    precision: mode === "datetime" ? "datetime" : "",
    options: b.options,
  });
  if (mode !== "datetime") b.precision = "";
  renderButtonSettings();
}

function dateTimeCardTimeParts() {
  var now = webserverNow();
  var use12h = typeof state !== "undefined" && state.clockFormat === "12h";
  var hour = now.getUTCHours();
  var minute = String(now.getUTCMinutes()).padStart(2, "0");
  var timeValue = "";

  if (use12h) {
    var hour12 = hour % 12;
    if (hour12 === 0) hour12 = 12;
    timeValue = String(hour12) + ":" + minute;
  } else {
    timeValue = String(hour).padStart(2, "0") + ":" + minute;
  }

  return {
    value: timeValue,
    unit: "",
  };
}

registerButtonType("calendar", {
  label: function () { return cardContractCardLabel("calendar"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("calendar"); },
  pickerKey: function () { return cardContractPickerKey("calendar"); },
  hidden: function () { return cardContractHidden("calendar"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("calendar"); },
  cardMetadata: DATE_TIME_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "sensor.date";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.options = "";
    b.precision = b.precision === "datetime" ? "datetime" : "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    if (!b.entity) b.entity = "sensor.date";
    if (b.precision !== "datetime") b.precision = "";

    helpers.renderCardModeSelector(panel, b, helpers, DATE_TIME_CARD_METADATA);
    helpers.renderCardLargeNumbersToggle(panel, b, helpers, DATE_TIME_CARD_METADATA);
  },
  renderPreview: function (b, helpers) {
    var now = webserverNow();
    var isDateTime = b.precision === "datetime";
    var hideLabel = cardLargeNumbersHidePreviewLabel(b, helpers, DATE_TIME_CARD_METADATA);
    var buttonClass = hideLabel
      ? (isDateTime ? "sp-clock-wide-large" : "sp-date-time-wide-large")
      : undefined;
    var day = String(now.getUTCDate());
    var month = typeof monthNameForIndex === "function"
      ? monthNameForIndex(now.getUTCMonth())
      : now.toLocaleString("en", { month: "long", timeZone: "UTC" });

    if (isDateTime) {
      var time = dateTimeCardTimeParts();

      return {
        buttonClass: buttonClass,
        iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
        labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, day + " " + month, DATE_TIME_CARD_METADATA.preview.dateBadge),
      };
    }

    return {
      buttonClass: buttonClass,
      iconHtml: cardSensorPreviewHtml(b, helpers, day, null),
      labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, month, DATE_TIME_CARD_METADATA.preview.dateBadge),
    };
  },
});
