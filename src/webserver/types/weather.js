// Read-only weather card: displays either current conditions or high / low temperatures.
var WEATHER_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "weather-display",
    options: [
      ["", "Current Conditions"],
      ["today", "Temperatures Today"],
      ["tomorrow", "Temperatures Tomorrow"],
      ["3day", "3-Day Forecast"],
    ],
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
  if (b.precision === "today") return "Today";
  if (b.precision === "tomorrow") return "Tomorrow";
  if (b.precision === "3day") return "3-Day Forecast";
  return "Forecast";
}

function weatherModeOptionValues() {
  var spec = cardContractOptionSpec("weather", "weather_mode");
  return spec && spec.values ? spec.values.slice() : ["", "today", "tomorrow", "3day"];
}

function normalizeWeatherCardMode(mode) {
  mode = String(mode || "");
  return weatherModeOptionValues().indexOf(mode) >= 0 ? mode : "";
}

function weatherCardIsForecastMode(b) {
  return !!b && normalizeWeatherCardMode(b.precision) !== "";
}

function weatherCardSupportsLargeNumbers(b) {
  return !!b && cardContractOptionSupportedFor("weather", "large_numbers", { precision: b.precision });
}

function weatherCardIsMultiDayForecastMode(b) {
  return !!b && b.precision === "3day";
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
      helpers.syncCardLargeNumbersToggle(largeNumbersToggle, b, helpers, weatherCardSupportsLargeNumbers(b));
    }

    modeSelect.addEventListener("change", function () {
      syncForecastFields();
    });
    syncForecastFields();
  },
  renderPreview: function (b, helpers) {
    if (weatherCardIsForecastMode(b)) {
      var defaultLabel = weatherCardDefaultForecastLabel(b);
      var label = b.label || defaultLabel;
      if (weatherCardIsMultiDayForecastMode(b)) {
        return {
          iconHtml: '<span class="sp-sensor-preview sp-forecast-preview sp-forecast-multiday">' +
            '<span class="sp-forecast-day">Mon <b>18/10</b></span>' +
            '<span class="sp-forecast-day">Tue <b>19/11</b></span>' +
            '<span class="sp-forecast-day">Wed <b>17/9</b></span>' +
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
