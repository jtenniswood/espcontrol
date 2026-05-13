// Legacy read-only forecast card: displays tomorrow's high / low temperature.
registerButtonType("weather_forecast", {
  label: "Weather Forecast",
  allowInSubpage: true,
  hideLabel: true,
  isAvailable: function () {
    return false;
  },
  onSelect: function (b) {
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "tomorrow";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var entityField = helpers.entityField(
      "Weather Entity", helpers.idPrefix + "entity", b.entity,
      "e.g. weather.forecast_home", ["weather"], "entity", true,
      "Add an entity before saving.");
    panel.appendChild(entityField.field);
  },
  renderPreview: function (b, helpers) {
    return {
      iconHtml:
        '<span class="sp-sensor-preview sp-forecast-preview">' +
          '<span class="sp-sensor-value sp-forecast-value">18/10</span>' +
          '<span class="sp-sensor-unit">' + temperatureUnitSymbol() + '</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label"></span>' +
        '<span class="sp-type-badge mdi mdi-weather-partly-cloudy"></span></span>',
    };
  },
});
