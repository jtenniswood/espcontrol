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
      return b.precision === "today" ? "Temperatures Today" : "Temperatures Tomorrow";
    }

    var modeField = document.createElement("div");
    modeField.className = "sp-field";
    modeField.appendChild(helpers.fieldLabel("Display", helpers.idPrefix + "weather-display"));
    var modeSelect = document.createElement("select");
    modeSelect.className = "sp-select";
    modeSelect.id = helpers.idPrefix + "weather-display";
    [
      ["", "Current Conditions"],
      ["today", "Temperatures Today"],
      ["tomorrow", "Temperatures Tomorrow"],
    ].forEach(function (item) {
      var opt = document.createElement("option");
      opt.value = item[0];
      opt.textContent = item[1];
      modeSelect.appendChild(opt);
    });
    modeSelect.value = b.precision === "today" || b.precision === "tomorrow" ? b.precision : "";
    modeField.appendChild(modeSelect);
    panel.appendChild(modeField);

    var labelField = document.createElement("div");
    labelField.className = "sp-field";
    labelField.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
    var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. " + defaultForecastLabel());
    labelField.appendChild(labelInp);
    panel.appendChild(labelField);
    helpers.bindField(labelInp, "label", true);

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

    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Weather Entity", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. weather.forecast_home");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);
    helpers.requireField(entityInp, "Add an entity before saving.");
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "today" || b.precision === "tomorrow") {
      var defaultLabel = b.precision === "today" ? "Temperatures Today" : "Temperatures Tomorrow";
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
