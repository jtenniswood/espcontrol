// Read-only weather card: displays either current conditions or high / low temperatures.
var WEATHER_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "weather-display",
    options: weatherModeOptions,
    value: function (b) {
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
    supported: weatherCardSupportsLargeNumbers,
  },
  preview: {
    forecastBadge: "weather-partly-cloudy",
    currentBadge: "weather-cloudy",
  },
};

function weatherCardDefaultForecastLabel(b) {
  if (b.precision === "3day") return "3-Day Forecast";
  return b.precision === "today" ? "Today" : "Tomorrow";
}

function weatherForecastCardsSupported() {
  var disabled = CFG.disabledCardTypes || [];
  return disabled.indexOf("weather_forecast") === -1;
}

function weatherModeOptions() {
  var options = [
    ["", "Current Conditions"],
    ["today", "Temperatures Today"],
    ["tomorrow", "Temperatures Tomorrow"],
    ["3day", "3-Day Forecast"],
  ];
  return weatherForecastCardsSupported() ? options : [options[0]];
}

function weatherModeOptionValues() {
  var spec = cardContractOptionSpec("weather", "weather_mode");
  var values = spec && spec.values ? spec.values.slice() : ["", "today", "tomorrow"];
  return weatherForecastCardsSupported() ? values : values.filter(function (value) {
    return value === "";
  });
}

function normalizeWeatherCardMode(mode) {
  mode = String(mode || "");
  return weatherModeOptionValues().indexOf(mode) >= 0 ? mode : "";
}

function weatherCardIsForecastMode(b) {
  return weatherForecastCardsSupported() &&
    !!b &&
    normalizeWeatherCardMode(b.precision) !== "";
}

function weatherCardSupportsLargeNumbers(b) {
  return weatherForecastCardsSupported() &&
    !!b &&
    b.precision !== "3day" &&
    cardContractOptionSupportedFor("weather", "large_numbers", { precision: b.precision });
}

registerButtonType("weather", {
  label: function () { return cardContractCardLabel("weather"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("weather"); },
  pickerKey: function () { return cardContractPickerKey("weather"); },
  hidden: function () { return cardContractHidden("weather"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("weather"); },
  cardMetadata: WEATHER_CARD_METADATA,
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

    function syncForecastFields() {
      var forecast = weatherCardIsForecastMode(b);
      labelField.style.display = forecast ? "" : "none";
      labelInp.placeholder = "e.g. " + weatherCardDefaultForecastLabel(b);
      helpers.syncCardLargeNumbersToggle(largeNumbersToggle, b, helpers, forecast);
    }

    modeSelect.addEventListener("change", function () {
      syncForecastFields();
    });
    syncForecastFields();
  },
  renderPreview: function (b, helpers) {
    if (weatherCardIsForecastMode(b)) {
      var escHtml = helpers.escHtml;
      var defaultLabel = weatherCardDefaultForecastLabel(b);
      var label = b.label || defaultLabel;
      if (b.precision === "3day") {
        return {
          iconHtml: '<span class="sp-forecast-multiday">' +
            '<span><b>Mon</b><em>18' + escHtml(temperatureUnitSymbol()) + ' / 10' + escHtml(temperatureUnitSymbol()) + '</em></span>' +
            '<span><b>Tue</b><em>20' + escHtml(temperatureUnitSymbol()) + ' / 11' + escHtml(temperatureUnitSymbol()) + '</em></span>' +
            '<span><b>Wed</b><em>17' + escHtml(temperatureUnitSymbol()) + ' / 9' + escHtml(temperatureUnitSymbol()) + '</em></span>' +
            '</span>',
          labelHtml: cardBadgeLabelHtml(helpers, label, WEATHER_CARD_METADATA.preview.forecastBadge),
        };
      }
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
