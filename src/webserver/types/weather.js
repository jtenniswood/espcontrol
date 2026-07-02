// Read-only weather card: displays either current conditions or high / low temperatures.
var WEATHER_CARD_MODE_LABELS = {
  "": "Current Conditions",
  today: "Temperatures Today",
  tomorrow: "Temperatures Tomorrow",
  hero: "Hero (current + today)",
  daily_strip: "Daily Strip (5 days)",
  hourly_strip: "Hourly Strip (12 hours)",
};

var WEATHER_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "weather-display",
    options: [
      ["", "Current Conditions"],
      ["today", "Temperatures Today"],
      ["tomorrow", "Temperatures Tomorrow"],
      ["hero", "Hero (current + today)"],
      ["daily_strip", "Daily Strip (5 days)"],
      ["hourly_strip", "Hourly Strip (12 hours)"],
    ],
    value: function (b) {
      if (!b) return "";
      if (b.precision === "daily_strip") return "daily_strip";
      if (b.precision === "hourly_strip") return "hourly_strip";
      if (b.precision === "hero") return "hero";
      return weatherCardIsForecastMode(b) ? b.precision : "";
    },
    onChange: function (b, helpers) {
      b.precision = this.value;
      helpers.saveField("precision", b.precision);
    },
  },
  entity: {
    label: "Weather Entity",
    idSuffix: "entity",
    placeholder: "e.g. weather.forecast_home",
    domains: function () { return cardContractDomains("weather"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add an entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "label",
    placeholder: function (b) {
      return "e.g. " + weatherCardDefaultForecastLabel(b);
    },
  },
  largeNumbers: {
    label: "Large Temperature Numbers",
    idSuffix: "large-weather-numbers",
    supported: weatherCardIsForecastMode,
  },
  preview: {
    forecastBadge: "weather-partly-cloudy",
    currentBadge: "weather-cloudy",
  },
};

function weatherCardDefaultForecastLabel(b) {
  if (b.precision === "today") return "Today";
  if (b.precision === "tomorrow") return "Tomorrow";
  if (b.precision === "daily_strip") return "Forecast";
  if (b.precision === "hourly_strip") return "Hourly";
  return "Tomorrow";
}

function weatherModeOptionValues() {
  var spec = cardContractOptionSpec("weather", "weather_mode");
  return spec && spec.values ? spec.values.slice() : ["", "today", "tomorrow"];
}

function normalizeWeatherCardMode(mode) {
  mode = String(mode || "");
  return weatherModeOptionValues().indexOf(mode) >= 0 ? mode : "";
}

function weatherCardShowsLabelField(b) {
  return weatherCardIsForecastMode(b) ||
    (!!b && (b.precision === "daily_strip" || b.precision === "hourly_strip"));
}

function weatherCardSupportsLargeNumbers(b) {
  return weatherCardIsForecastMode(b) || (!!b && b.precision === "hero");
}

function weatherCardIsForecastMode(b) {
  return !!b && cardContractOptionSupportedFor("weather", "large_numbers", { precision: b.precision });
}

function weatherCardHasWideSlot(cardSize) {
  cardSize = cardSize || CARD_SIZE_SINGLE;
  return cardSize === CARD_SIZE_WIDE || cardSize === CARD_SIZE_EXTRA_WIDE;
}

function weatherCardHasExtraWideSlot(cardSize) {
  cardSize = cardSize || CARD_SIZE_SINGLE;
  return cardSize === CARD_SIZE_EXTRA_WIDE;
}

function weatherCardCurrentSize(helpers) {
  if (helpers && typeof helpers.getCardSize === "function") return helpers.getCardSize();
  return (helpers && helpers.cardSize) || CARD_SIZE_SINGLE;
}

function weatherCardModeOptions() {
  return weatherModeOptionValues().map(function (value) {
    return [value, WEATHER_CARD_MODE_LABELS[value] || value];
  });
}

function weatherCardRefreshPreview() {
  if (typeof renderPreview === "function") renderPreview();
}

function weatherCardHourlyStripPreviewHtml() {
  return '<div class="sp-forecast-hourly-preview">' +
    "<span>3p</span><span>4p</span><span>5p</span><span>6p</span><span>7p</span><span>8p</span>" +
    "<span>9p</span><span>10p</span><span>11p</span><span>12a</span><span>1a</span><span>2a</span>" +
    "</div>";
}

registerButtonType("weather", {
  label: function () { return cardContractCardLabel("weather"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("weather"); },
  pickerKey: function () { return cardContractPickerKey("weather"); },
  hidden: function () { return cardContractHidden("weather"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("weather"); },
  cardMetadata: WEATHER_CARD_METADATA,
  validateSave: function (b, slot, ctx) {
    if (!b) return true;
    var cardSize = ctx && typeof ctx.getCardSize === "function"
      ? ctx.getCardSize()
      : (ctx && ctx.cardSize);
    if (b.precision === "daily_strip") {
      if (weatherCardHasWideSlot(cardSize)) return true;
      if (ctx && ctx.showBanner) {
        ctx.showBanner(
          "Daily Strip needs a wide grid slot. Resize this card to wide (add w in Grid Order, e.g. 3w) before saving.",
          "error"
        );
      }
      return false;
    }
    if (b.precision === "hourly_strip") {
      if (weatherCardHasExtraWideSlot(cardSize)) return true;
      if (ctx && ctx.showBanner) {
        ctx.showBanner(
          "Hourly Strip needs an extra-wide grid slot. Resize this card to extra-wide (add x in Grid Order, e.g. 3x) before saving.",
          "error"
        );
      }
      return false;
    }
    return true;
  },
  onSelect: function (b) {
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.options = "";
    b.precision = normalizeWeatherCardMode(b.precision);
  },
  renderSettings: function (panel, b, slot, helpers) {
    var modeField = helpers.renderCardModeSelector(panel, b, helpers, WEATHER_CARD_METADATA);
    var modeSelect = modeField.select;

    helpers.renderCardEntityField(panel, b, helpers, WEATHER_CARD_METADATA);

    var labelControl = helpers.renderCardTextField(panel, b, helpers, WEATHER_CARD_METADATA.labelField);
    var labelField = labelControl.field;
    var labelInp = labelControl.input;

    var largeNumbersToggle = helpers.renderCardLargeNumbersToggle(panel, b, helpers, WEATHER_CARD_METADATA);

    var stripHint = document.createElement("div");
    stripHint.className = "sp-hint";
    panel.appendChild(stripHint);

    function syncStripHint() {
      var cardSize = weatherCardCurrentSize(helpers);
      var daily = b.precision === "daily_strip";
      var hourly = b.precision === "hourly_strip";
      stripHint.style.display = daily || hourly ? "" : "none";
      if (daily) {
        stripHint.textContent = weatherCardHasWideSlot(cardSize)
          ? "Shows five days with weekday, icon, and high/low."
          : "Daily Strip requires a wide grid slot. Resize this card to wide (add w in Grid Order, e.g. 3w) before saving.";
        return;
      }
      if (hourly) {
        stripHint.textContent = weatherCardHasExtraWideSlot(cardSize)
          ? "Shows the next twelve hours with hour, icon, and temperature."
          : "Hourly Strip requires an extra-wide grid slot. Resize this card to extra-wide (add x in Grid Order, e.g. 3x) before saving.";
      }
    }

    function syncForecastFields() {
      var forecast = weatherCardShowsLabelField(b);
      var heroOrForecast = weatherCardSupportsLargeNumbers(b);
      labelField.style.display = forecast ? "" : "none";
      labelInp.placeholder = "e.g. " + weatherCardDefaultForecastLabel(b);
      helpers.syncCardLargeNumbersToggle(largeNumbersToggle, b, helpers, heroOrForecast);
      syncStripHint();
    }

    modeSelect.addEventListener("change", function () {
      syncForecastFields();
      weatherCardRefreshPreview();
    });
    syncForecastFields();
  },
  renderPreview: function (b, helpers) {
    var cardSize = weatherCardCurrentSize(helpers);
    if (b.precision === "hero") {
      return {
        iconHtml:
          '<div class="sp-forecast-hero-preview">' +
          '<span class="sp-btn-icon mdi mdi-weather-partly-cloudy"></span>' +
          '<span class="sp-forecast-hero-condition">Cloudy</span>' +
          cardSensorPreviewHtml(b, helpers, "12", temperatureUnitSymbol(), "sp-forecast-preview sp-forecast-hero-temp", "sp-forecast-value") +
          "</div>",
        labelHtml: cardSensorPreviewHtml(b, helpers, "18/10", temperatureUnitSymbol(), "sp-forecast-hero-range", "sp-forecast-value"),
      };
    }
    if (b.precision === "daily_strip") {
      if (!weatherCardHasWideSlot(cardSize)) {
        return {
          iconHtml: cardSensorPreviewHtml(b, helpers, "18/10", temperatureUnitSymbol(), "sp-forecast-preview", "sp-forecast-value"),
          labelHtml: cardBadgeLabelHtml(helpers, "Today", WEATHER_CARD_METADATA.preview.forecastBadge),
        };
      }
      return {
        buttonClass: "sp-weather-strip-card",
        iconHtml: '<div class="sp-forecast-strip-preview"><span>Sun</span><span>Mon</span><span>Tue</span><span>Wed</span><span>Thu</span></div>',
        labelHtml: cardBadgeLabelHtml(helpers, b.label || "Forecast", WEATHER_CARD_METADATA.preview.forecastBadge),
      };
    }
    if (b.precision === "hourly_strip") {
      if (!weatherCardHasExtraWideSlot(cardSize)) {
        return {
          iconHtml: cardSensorPreviewHtml(b, helpers, "18", temperatureUnitSymbol(), "sp-forecast-preview", "sp-forecast-value"),
          labelHtml: cardBadgeLabelHtml(helpers, "Hourly", WEATHER_CARD_METADATA.preview.forecastBadge),
        };
      }
      return {
        buttonClass: "sp-weather-strip-card",
        iconHtml: weatherCardHourlyStripPreviewHtml(),
        labelHtml: cardBadgeLabelHtml(helpers, b.label || "Hourly", WEATHER_CARD_METADATA.preview.forecastBadge),
      };
    }
    if (weatherCardIsForecastMode(b)) {
      var defaultLabel = weatherCardDefaultForecastLabel(b);
      var label = b.label || defaultLabel;
      return {
        iconHtml: cardSensorPreviewHtml(b, helpers, "18/10", temperatureUnitSymbol(), "sp-forecast-preview", "sp-forecast-value"),
        labelHtml: cardBadgeLabelHtml(helpers, label, WEATHER_CARD_METADATA.preview.forecastBadge),
      };
    }
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-weather-cloudy"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, "Cloudy", WEATHER_CARD_METADATA.preview.currentBadge),
    };
  },
});
