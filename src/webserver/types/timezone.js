// Read-only world clock card: displays local time for a selected city.
function timezoneCardCityLabel(tzOption) {
  var tzId = getTzId(tzOption || "");
  if (!tzId) return "World Clock";
  if (tzId === "UTC") return "UTC";
  var city = tzId.substring(tzId.lastIndexOf("/") + 1);
  return city.replace(/_/g, " ");
}

function timezoneCardTimeParts(tzOption) {
  var use12h = typeof state !== "undefined" && state.clockFormat === "12h";
  var tzId = getTzId(tzOption || "UTC");
  try {
    var opts = { timeZone: tzId, hour: "numeric", minute: "2-digit" };
    if (use12h) opts.hour12 = true;
    else opts.hourCycle = "h23";
    var parts = new Intl.DateTimeFormat("en-US", opts).formatToParts(new Date());
    var hour = "";
    var minute = "";
    for (var i = 0; i < parts.length; i++) {
      if (parts[i].type === "hour") hour = parts[i].value;
      else if (parts[i].type === "minute") minute = parts[i].value;
    }
    if (!hour || !minute) return { value: "--:--", unit: "" };
    return {
      value: (use12h ? hour : hour.padStart(2, "0")) + ":" + minute,
      unit: "",
    };
  } catch (e) {
    return { value: "--:--", unit: "" };
  }
}

registerButtonType("timezone", {
  label: function () { return cardContractCardLabel("timezone"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("timezone"); },
  pickerKey: function () { return cardContractPickerKey("timezone"); },
  experimental: function () { return cardContractExperimental("timezone"); },
  hidden: function () { return cardContractHidden("timezone"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("timezone"); },
  isAvailable: function () {
    return false;
  },
  cardMetadata: DATE_TIME_CARD_METADATA,
  onSelect: function (b) {
    b.entity = defaultTimezoneCardEntity();
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    if (!b.entity) b.entity = defaultTimezoneCardEntity();
    if (b.label) {
      b.label = "";
      helpers.saveField("label", "");
    }

    helpers.renderCardModeSelector(panel, b, helpers, DATE_TIME_CARD_METADATA);
    helpers.renderCardLargeNumbersToggle(panel, b, helpers, DATE_TIME_CARD_METADATA);

    var tzSelect = document.createElement("select");
    tzSelect.className = "sp-select";
    tzSelect.id = helpers.idPrefix + "timezone";

    var options = typeof state !== "undefined"
      ? timezoneOptionsWithFallback(state.timezoneOptions, b.entity)
      : [b.entity];

    options.forEach(function (opt) {
      appendTimezoneOption(tzSelect, opt);
    });
    tzSelect.value = b.entity;
    tzSelect.addEventListener("change", function () {
      b.entity = this.value;
      b.label = "";
      helpers.saveField("entity", b.entity);
      helpers.saveField("label", "");
    });

    panel.appendChild(helpers.fieldWithControl("City / Timezone", helpers.idPrefix + "timezone", tzSelect));
  },
  renderPreview: function (b, helpers) {
    var tz = b.entity || (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    var time = timezoneCardTimeParts(tz);
    var hideLabel = cardLargeNumbersHidePreviewLabel(b, helpers, DATE_TIME_CARD_METADATA);
    return {
      buttonClass: hideLabel ? "sp-date-time-wide-large" : undefined,
      iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
      labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, timezoneCardCityLabel(tz), DATE_TIME_CARD_METADATA.preview.timezoneBadge),
    };
  },
});
