// Read-only weather card: displays either current conditions or high / low temperatures.
registerButtonType("weather", {
  label: "Weather",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.options = "";
    if (b.precision !== "today" && b.precision !== "tomorrow") b.precision = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var isLargeCard = helpers.cardSize === 4;

    function defaultForecastLabel() {
      return b.precision === "today" ? "Today" : "Tomorrow";
    }

    function isForecastMode() {
      return b.precision === "today" || b.precision === "tomorrow";
    }

    var modeField = helpers.selectField("Type", helpers.idPrefix + "weather-display", [
      ["", "Current Conditions"],
      ["today", "Temperatures Today"],
      ["tomorrow", "Temperatures Tomorrow"],
    ], b.precision === "today" || b.precision === "tomorrow" ? b.precision : "");
    var modeSelect = modeField.select;
    panel.appendChild(modeField.field);

    var entityField = helpers.entityField(
      "Weather Entity", helpers.idPrefix + "entity", b.entity,
      "e.g. weather.forecast_home", ["weather"], "entity", true,
      "Add an entity before saving.");
    panel.appendChild(entityField.field);

    var labelControl = helpers.textField(
      "Label", helpers.idPrefix + "label", b.label, "e.g. " + defaultForecastLabel(),
      "label", true);
    var labelField = labelControl.field;
    var labelInp = labelControl.input;
    panel.appendChild(labelField);

    var largeNumbersToggle = null;
    if (isLargeCard) {
      largeNumbersToggle = helpers.toggleRow(
        "Large Temperature Numbers", helpers.idPrefix + "large-weather-numbers",
        cardLargeNumbersEnabled(b));
      panel.appendChild(largeNumbersToggle.row);
      largeNumbersToggle.input.addEventListener("change", function () {
        setSensorLargeNumbersEnabled(b, this.checked);
        helpers.saveField("options", b.options);
      });
    }

    function syncForecastFields() {
      var forecast = isForecastMode();
      labelField.style.display = forecast ? "" : "none";
      labelInp.placeholder = "e.g. " + defaultForecastLabel();
      if (largeNumbersToggle) {
        largeNumbersToggle.row.style.display = forecast ? "" : "none";
        if (!forecast && cardLargeNumbersEnabled(b)) {
          setSensorLargeNumbersEnabled(b, false);
          largeNumbersToggle.input.checked = false;
          helpers.saveField("options", b.options);
        }
      }
    }

    modeSelect.addEventListener("change", function () {
      b.precision = this.value;
      helpers.saveField("precision", b.precision);
      syncForecastFields();
    });
    syncForecastFields();
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "today" || b.precision === "tomorrow") {
      var label = b.label || "";
      var previewClass = "sp-sensor-preview sp-forecast-preview" +
        (helpers.cardSize === 4 && cardLargeNumbersEnabled(b) ? " sp-sensor-preview-large" : "");
      return {
        iconHtml:
          '<span class="' + previewClass + '">' +
            '<span class="sp-sensor-value sp-forecast-value">18/10</span>' +
            '<span class="sp-sensor-unit">' + temperatureUnitSymbol() + '</span>' +
          '</span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
          '<span class="sp-type-badge mdi mdi-weather-partly-cloudy"></span></span>',
      };
    }
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-weather-cloudy"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label"></span>' +
        '<span class="sp-type-badge mdi mdi-weather-cloudy"></span></span>',
    };
  },
});
