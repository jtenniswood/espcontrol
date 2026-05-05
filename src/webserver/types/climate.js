// Climate card: compact dashboard status that opens a thermostat detail page.
registerButtonType("climate", {
  label: "Climate",
  experimental: "climate",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Living Room",
  onSelect: function (b) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Thermostat";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Climate Entity", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. climate.living_room");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);
    helpers.requireField(entityInp, "Add an entity before saving.");

    var pf = document.createElement("div");
    pf.className = "sp-field";
    pf.appendChild(helpers.fieldLabel("Unit Precision", helpers.idPrefix + "precision"));
    var precisionSelect = document.createElement("select");
    precisionSelect.className = "sp-select";
    precisionSelect.id = helpers.idPrefix + "precision";
    [
      ["0", "Whole numbers"],
      ["1", "1 decimal"],
      ["2", "2 decimals"],
    ].forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt[0];
      o.textContent = opt[1];
      precisionSelect.appendChild(o);
    });
    precisionSelect.value = b.precision || "0";
    precisionSelect.addEventListener("change", function () {
      b.precision = this.value === "0" ? "" : this.value;
      helpers.saveField("precision", b.precision);
    });
    pf.appendChild(precisionSelect);
    panel.appendChild(pf);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Climate";
    var precision = parseInt(b.precision || "0", 10) || 0;
    var sample = (20).toFixed(precision);
    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">' + sample + '</span>' +
          '<span class="sp-sensor-unit">' + temperatureUnitSymbol() + '</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-thermostat"></span></span>',
    };
  },
});
