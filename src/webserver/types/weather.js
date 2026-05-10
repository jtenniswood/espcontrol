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
    if (b.precision !== "today" && b.precision !== "tomorrow") b.precision = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    function defaultForecastLabel() {
      return b.precision === "today" ? "Today" : "Tomorrow";
    }

    var modeField = helpers.selectField("Display", helpers.idPrefix + "weather-display", [
      ["", "Current Conditions"],
      ["today", "Temperatures Today"],
      ["tomorrow", "Temperatures Tomorrow"],
    ], b.precision === "today" || b.precision === "tomorrow" ? b.precision : "");
    var modeSelect = modeField.select;
    panel.appendChild(modeField.field);

    var labelControl = helpers.textField(
      "Label", helpers.idPrefix + "label", b.label, "e.g. " + defaultForecastLabel(),
      "label", true);
    var labelField = labelControl.field;
    var labelInp = labelControl.input;
    panel.appendChild(labelField);

    function syncLabelField() {
      var forecast = b.precision === "today" || b.precision === "tomorrow";
      labelField.style.display = forecast ? "" : "none";
      labelInp.placeholder = "e.g. " + defaultForecastLabel();
    }

    modeSelect.addEventListener("change", function () {
      b.precision = this.value;
      helpers.saveField("precision", b.precision);
      syncLabelField();
    });
    syncLabelField();

    var entityField = helpers.entityField(
      "Weather Entity", helpers.idPrefix + "entity", b.entity,
      "e.g. weather.forecast_home", ["weather"], "entity", true,
      "Add an entity before saving.");
    panel.appendChild(entityField.field);
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "today" || b.precision === "tomorrow") {
      var defaultLabel = b.precision === "today" ? "Today" : "Tomorrow";
      var label = b.label || defaultLabel;
      return {
        iconHtml:
          '<span class="sp-sensor-preview sp-forecast-preview">' +
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
        '<span class="sp-btn-label-row"><span class="sp-btn-label">Cloudy</span>' +
        '<span class="sp-type-badge mdi mdi-weather-cloudy"></span></span>',
    };
  },
});
