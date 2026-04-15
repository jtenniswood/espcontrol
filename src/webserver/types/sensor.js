// Read-only sensor card: displays a HA sensor value and unit (non-clickable)
registerButtonType("sensor", {
  label: "Sensor",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Living Room",
  onSelect: function (b) {
    b.entity = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var sf = document.createElement("div");
    sf.className = "sp-field";
    sf.appendChild(helpers.fieldLabel("Sensor Entity", helpers.idPrefix + "sensor"));
    var sensorInp = helpers.textInput(helpers.idPrefix + "sensor", b.sensor, "e.g. sensor.living_room_temperature");
    sf.appendChild(sensorInp);
    panel.appendChild(sf);
    helpers.bindField(sensorInp, "sensor", true);

    var uf = document.createElement("div");
    uf.className = "sp-field";
    uf.appendChild(helpers.fieldLabel("Unit", helpers.idPrefix + "unit"));
    var unitInp = helpers.textInput(helpers.idPrefix + "unit", b.unit, "e.g. \u00B0C");
    unitInp.className = "sp-input";
    uf.appendChild(unitInp);
    panel.appendChild(uf);
    helpers.bindField(unitInp, "unit", true);

    var pf = document.createElement("div");
    pf.className = "sp-field";
    pf.appendChild(helpers.fieldLabel("Unit precision", helpers.idPrefix + "precision"));
    var precSeg = document.createElement("div");
    precSeg.className = "sp-segment";
    var precOpts = [["0", "10"], ["1", "10.2"], ["2", "10.21"]];
    for (var i = 0; i < precOpts.length; i++) {
      (function (val, label) {
        var btn = document.createElement("button");
        btn.type = "button";
        btn.textContent = label;
        if ((b.precision || "0") === val) btn.classList.add("active");
        btn.addEventListener("click", function () {
          b.precision = val === "0" ? "" : val;
          helpers.saveField("precision", b.precision);
          var btns = precSeg.querySelectorAll("button");
          for (var j = 0; j < btns.length; j++) btns[j].classList.remove("active");
          btn.classList.add("active");
        });
        precSeg.appendChild(btn);
      })(precOpts[i][0], precOpts[i][1]);
    }
    pf.appendChild(precSeg);
    panel.appendChild(pf);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.sensor || "Sensor";
    var unit = b.unit ? helpers.escHtml(b.unit) : "";
    var prec = parseInt(b.precision || "0", 10) || 0;
    var sampleVal = (0).toFixed(prec);
    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">' + sampleVal + '</span>' +
          '<span class="sp-sensor-unit">' + unit + '</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-gauge"></span></span>',
    };
  },
});
