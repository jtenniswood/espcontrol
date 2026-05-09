// Climate card: thermostat status plus full-screen climate controls.
registerButtonType("climate", {
  label: "Climate",
  allowInSubpage: true,
  hideLabel: true,
  labelPlaceholder: "e.g. Living Room",
  experimental: "climate",
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    if (String(b.precision || "") !== "1") {
      b.precision = "";
      helpers.saveField("precision", "");
    }

    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Climate Entity", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. climate.living_room");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);
    helpers.requireField(entityInp, "Add a climate entity before saving.");

    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
    var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Living Room");
    lf.appendChild(labelInp);
    panel.appendChild(lf);
    helpers.bindField(labelInp, "label", true);

    var pf = document.createElement("div");
    pf.className = "sp-field";
    pf.appendChild(helpers.fieldLabel("Unit Precision", helpers.idPrefix + "climate-precision"));
    var precision = document.createElement("select");
    precision.className = "sp-select";
    precision.id = helpers.idPrefix + "climate-precision";
    [
      ["", "10"],
      ["1", "10.2"],
    ].forEach(function (entry) {
      var opt = document.createElement("option");
      opt.value = entry[0];
      opt.textContent = entry[1];
      precision.appendChild(opt);
    });
    precision.value = b.precision === "1" ? b.precision : "";
    precision.addEventListener("change", function () {
      b.precision = this.value;
      helpers.saveField("precision", b.precision);
      scheduleRender();
    });
    pf.appendChild(precision);
    panel.appendChild(pf);
  },
  renderPreview: function (b, helpers) {
    var label = (b.label && b.label.trim()) || (b.entity && b.entity.trim()) || "Climate";
    var value = b.precision === "1" ? "20.0" : (b.precision === "2" ? "20.00" : "20");
    var unit = (typeof state !== "undefined" && state.temperatureUnit && state.temperatureUnit !== "Auto")
      ? state.temperatureUnit
      : "°C";
    return {
      iconHtml:
        '<span class="sp-sensor-preview"><span class="sp-sensor-value">' +
        helpers.escHtml(value) + '</span><span class="sp-sensor-unit">' +
        helpers.escHtml(unit) + '</span></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
        helpers.escHtml(label) + '</span><span class="sp-type-badge mdi mdi-thermostat"></span></span>',
    };
  },
});
