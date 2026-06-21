// Read-only weather card: displays either current conditions or high / low temperatures.
var WEATHER_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "weather-display",
    options: [
      ["", "Current Conditions"],
      ["today", "Temperatures Today"],
      ["tomorrow", "Temperatures Tomorrow"],
      ["daily_strip", "Daily Strip (5 days)"],
    ],
    value: function (b) {
      if (!b) return "";
      if (b.precision === "daily_strip") return "daily_strip";
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
  return weatherCardIsForecastMode(b) || (!!b && b.precision === "daily_strip");
}

function weatherCardIsForecastMode(b) {
  return !!b && cardContractOptionSupportedFor("weather", "large_numbers", { precision: b.precision });
}

function weatherCardHasWideSlot(cardSize) {
  cardSize = cardSize || CARD_SIZE_SINGLE;
  return cardSize === CARD_SIZE_WIDE || cardSize === CARD_SIZE_EXTRA_WIDE;
}

function weatherCardModeOptions(cardSize, currentMode) {
  var options = WEATHER_CARD_METADATA.mode.options.slice();
  if (weatherCardHasWideSlot(cardSize)) return options;
  return options.filter(function (entry) {
    return entry[0] !== "daily_strip" || currentMode === "daily_strip";
  });
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
    if (!b || b.precision !== "daily_strip") return true;
    if (weatherCardHasWideSlot(ctx && ctx.cardSize)) return true;
    if (ctx && ctx.showBanner) {
      ctx.showBanner(
        "Daily Strip needs a wide grid slot. Resize this card to wide (add w in Grid Order, e.g. 3w) before saving.",
        "error"
      );
    }
    return false;
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

    function syncModeOptions() {
      var options = weatherCardModeOptions(helpers.cardSize, b.precision);
      var selected = modeSelect.value;
      modeSelect.innerHTML = "";
      options.forEach(function (entry) {
        var option = document.createElement("option");
        option.value = entry[0];
        option.textContent = entry[1];
        modeSelect.appendChild(option);
      });
      if (options.some(function (entry) { return entry[0] === selected; })) {
        modeSelect.value = selected;
      } else if (b.precision === "daily_strip") {
        modeSelect.value = "daily_strip";
      } else {
        modeSelect.value = "";
        if (b.precision !== "") {
          b.precision = "";
          helpers.saveField("precision", "");
        }
      }
    }

    function syncDailyStripHint() {
      var show = b.precision === "daily_strip";
      stripHint.style.display = show ? "" : "none";
      if (!show) return;
      stripHint.textContent = weatherCardHasWideSlot(helpers.cardSize)
        ? "Shows five days with weekday, icon, and high/low."
        : "Daily Strip requires a wide grid slot. Resize this card to wide (add w in Grid Order, e.g. 3w) before saving.";
    }

    function syncForecastFields() {
      syncModeOptions();
      var forecast = weatherCardShowsLabelField(b);
      labelField.style.display = forecast ? "" : "none";
      labelInp.placeholder = "e.g. " + weatherCardDefaultForecastLabel(b);
      helpers.syncCardLargeNumbersToggle(largeNumbersToggle, b, helpers, forecast);
      syncDailyStripHint();
    }

    modeSelect.addEventListener("change", function () {
      WEATHER_CARD_METADATA.mode.onChange.call(this, b, helpers);
      syncForecastFields();
    });
    syncForecastFields();
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "daily_strip") {
      if (!weatherCardHasWideSlot(helpers && helpers.cardSize)) {
        return {
          iconHtml: cardSensorPreviewHtml(b, helpers, "18/10", temperatureUnitSymbol(), "sp-forecast-preview", "sp-forecast-value"),
          labelHtml: cardBadgeLabelHtml(helpers, "Today", WEATHER_CARD_METADATA.preview.forecastBadge),
        };
      }
      return {
        iconHtml: '<div class="sp-forecast-strip-preview"><span>Sun</span><span>Mon</span><span>Tue</span><span>Wed</span><span>Thu</span></div>',
        labelHtml: cardBadgeLabelHtml(helpers, b.label || "Forecast", WEATHER_CARD_METADATA.preview.forecastBadge),
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
