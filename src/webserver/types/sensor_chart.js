// Sensor chart card: displays a live numeric value with a sparkline history chart.
// The chart accumulates samples over a configurable interval (seconds) and plots
// one aggregated point (avg / max / min / last) per interval.
registerButtonType("sensor_chart", {
  label: "Sensor + Chart",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.entity = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    if (!b.precision) b.precision = "";
    if (!b.options) b.options = "";
    b.options = sensorChartNormalizeOptions(b.options);
  },
  renderSettings: function (panel, b, slot, helpers) {
    var sensorField = helpers.entityField(
      "Sensor Entity", helpers.idPrefix + "sensor", b.sensor,
      "e.g. sensor.living_room_temperature",
      ["sensor", "binary_sensor", "text_sensor"], "sensor", true,
      "Add a sensor entity before saving.");
    panel.appendChild(sensorField.field);

    var labelField = helpers.textField(
      "Label", helpers.idPrefix + "label", b.label, "e.g. Température", "label", true);
    panel.appendChild(labelField.field);

    var unitField = helpers.textField(
      "Unit", helpers.idPrefix + "unit", b.unit, "e.g. °C", "unit", true);
    panel.appendChild(unitField.field);

    var precisionField = helpers.precisionField(helpers.idPrefix + "precision",
      b.precision || "0", function () {
        b.precision = this.value === "0" ? "" : this.value;
        helpers.saveField("precision", b.precision);
      });
    panel.appendChild(precisionField.field);

    // Interval select
    var intervalSel = document.createElement("select");
    intervalSel.className = "sp-select";
    var intervals = [
      ["1", "1 s (test)"], ["300", "5 min"], ["900", "15 min"], ["1800", "30 min"],
      ["3600", "1 h"], ["7200", "2 h"], ["21600", "6 h"],
      ["43200", "12 h"], ["86400", "24 h"]
    ];
    var curInterval = configOptionValue(b.options, "interval") || "3600";
    intervals.forEach(function (iv) {
      var opt = document.createElement("option");
      opt.value = iv[0];
      opt.textContent = iv[1];
      if (iv[0] === curInterval) opt.selected = true;
      intervalSel.appendChild(opt);
    });
    intervalSel.addEventListener("change", function () {
      b.options = setConfigOptionValue(b.options, "interval", this.value);
      helpers.saveField("options", b.options);
    });
    panel.appendChild(helpers.fieldWithControl("Interval", null, intervalSel));

    // Mode segment
    var curMode = configOptionValue(b.options, "mode") || "avg";
    var mode = helpers.segmentControl([
      ["avg", "Avg"], ["max", "Max"], ["min", "Min"], ["last", "Last"]
    ], curMode, function (value) {
      b.options = setConfigOptionValue(b.options, "mode", value);
      helpers.saveField("options", b.options);
    });
    panel.appendChild(helpers.fieldWithControl("Mode", null, mode.segment));

    // Points field
    var curPoints = configOptionValue(b.options, "points") || "24";
    var pointsField = helpers.textField(
      "Points", helpers.idPrefix + "points", curPoints, "2–60", null, true);
    pointsField.input.addEventListener("change", function () {
      var v = parseInt(this.value, 10);
      if (isNaN(v) || v < 2) v = 2;
      if (v > 60) v = 60;
      this.value = String(v);
      b.options = setConfigOptionValue(b.options, "points", String(v));
      helpers.saveField("options", b.options);
    });
    panel.appendChild(pointsField.field);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.sensor || "Sensor";
    var unit = b.unit ? helpers.escHtml(b.unit) : "";
    var prec = parseInt(b.precision || "0", 10) || 0;
    var sampleVal = (0).toFixed(prec);
    var sparkline =
      '<svg class="sp-chart-sparkline" viewBox="0 0 60 20" preserveAspectRatio="none"' +
      ' xmlns="http://www.w3.org/2000/svg">' +
      '<defs><linearGradient id="scg" x1="0" y1="0" x2="0" y2="1">' +
      '<stop offset="0%" stop-color="#42A5F5" stop-opacity=".35"/>' +
      '<stop offset="100%" stop-color="#42A5F5" stop-opacity=".05"/>' +
      '</linearGradient></defs>' +
      '<polygon points="0,20 0,16 10,13 20,11 30,7 40,9 50,5 60,3 60,20"' +
      ' fill="url(#scg)"/>' +
      '<polyline points="0,16 10,13 20,11 30,7 40,9 50,5 60,3"' +
      ' fill="none" stroke="#42A5F5" stroke-width="1.5" stroke-linejoin="round"/>' +
      '</svg>';
    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">' + sampleVal + '</span>' +
          '<span class="sp-sensor-unit">' + unit + '</span>' +
        '</span>' + sparkline,
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
        helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-chart-line"></span></span>',
    };
  },
});

function sensorChartNormalizeOptions(options) {
  var interval = configOptionValue(options, "interval") || "3600";
  var mode = configOptionValue(options, "mode") || "avg";
  var points = configOptionValue(options, "points") || "24";
  var out = "";
  out = setConfigOptionValue(out, "interval", interval);
  out = setConfigOptionValue(out, "mode", mode);
  out = setConfigOptionValue(out, "points", points);
  return out;
}
