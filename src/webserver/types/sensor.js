// Read-only sensor card: displays either numeric data or a text state.
registerButtonType("sensor", {
  label: "Sensor",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.entity = "";
    b.icon_on = "Auto";
    if (!b.precision) b.precision = "";
    if (b.precision !== "text") b.icon = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var isTextMode = b.precision === "text";

    var modeField = document.createElement("div");
    modeField.className = "sp-field";
    modeField.appendChild(helpers.fieldLabel("Mode"));
    var modeSeg = document.createElement("div");
    modeSeg.className = "sp-segment";
    var numericBtn = document.createElement("button");
    numericBtn.type = "button";
    numericBtn.textContent = "Numeric";
    var textBtn = document.createElement("button");
    textBtn.type = "button";
    textBtn.textContent = "Text";
    modeSeg.appendChild(numericBtn);
    modeSeg.appendChild(textBtn);
    modeField.appendChild(modeSeg);
    panel.appendChild(modeField);

    var sf = document.createElement("div");
    sf.className = "sp-field";
    sf.appendChild(helpers.fieldLabel("Sensor Entity", helpers.idPrefix + "sensor"));
    var sensorInp = helpers.textInput(helpers.idPrefix + "sensor", b.sensor, "e.g. sensor.living_room_temperature");
    sf.appendChild(sensorInp);
    panel.appendChild(sf);
    helpers.bindField(sensorInp, "sensor", true);
    helpers.requireField(sensorInp, "Add a sensor entity before saving.");

    var numericSection = condField();

    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
    var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Living Room");
    lf.appendChild(labelInp);
    numericSection.appendChild(lf);
    helpers.bindField(labelInp, "label", true);

    var uf = document.createElement("div");
    uf.className = "sp-field";
    uf.appendChild(helpers.fieldLabel("Unit", helpers.idPrefix + "unit"));
    var unitInp = helpers.textInput(helpers.idPrefix + "unit", b.unit, "e.g. \u00B0C");
    unitInp.className = "sp-input";
    uf.appendChild(unitInp);
    numericSection.appendChild(uf);
    helpers.bindField(unitInp, "unit", true);

    var pf = document.createElement("div");
    pf.className = "sp-field";
    pf.appendChild(helpers.fieldLabel("Unit Precision", helpers.idPrefix + "precision"));
    var precisionSelect = document.createElement("select");
    precisionSelect.className = "sp-select";
    precisionSelect.id = helpers.idPrefix + "precision";
    var precOpts = [["0", "10"], ["1", "10.2"], ["2", "10.21"]];
    for (var i = 0; i < precOpts.length; i++) {
      var opt = document.createElement("option");
      opt.value = precOpts[i][0];
      opt.textContent = precOpts[i][1];
      precisionSelect.appendChild(opt);
    }
    precisionSelect.value = !isTextMode ? (b.precision || "0") : "0";
    precisionSelect.addEventListener("change", function () {
      b.precision = this.value === "0" ? "" : this.value;
      helpers.saveField("precision", b.precision);
    });
    pf.appendChild(precisionSelect);
    numericSection.appendChild(pf);
    panel.appendChild(numericSection);

    var textSection = condField();
    var textIconPicker = helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Auto", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }
    );
    textSection.appendChild(textIconPicker);
    panel.appendChild(textSection);

    function setMode(mode, persist) {
      isTextMode = mode === "text";
      numericBtn.classList.toggle("active", !isTextMode);
      textBtn.classList.toggle("active", isTextMode);
      numericSection.classList.toggle("sp-visible", !isTextMode);
      textSection.classList.toggle("sp-visible", isTextMode);
      if (!persist) return;
      if (isTextMode) {
        b.precision = "text";
        b.label = "";
        b.unit = "";
        b.icon_on = "Auto";
        labelInp.value = "";
        unitInp.value = "";
        helpers.saveField("precision", "text");
        helpers.saveField("label", "");
        helpers.saveField("unit", "");
        helpers.saveField("icon_on", "Auto");
      } else {
        b.precision = "";
        b.icon = "Auto";
        helpers.saveField("precision", "");
        helpers.saveField("icon", "Auto");
        var iconPreview = textIconPicker.querySelector(".sp-icon-picker-preview");
        if (iconPreview) iconPreview.className = "sp-icon-picker-preview mdi mdi-cog";
        var iconInput = textIconPicker.querySelector(".sp-icon-picker-input");
        if (iconInput) iconInput.value = "Auto";
        precisionSelect.value = "0";
      }
    }

    numericBtn.addEventListener("click", function () { setMode("numeric", true); });
    textBtn.addEventListener("click", function () { setMode("text", true); });
    setMode(isTextMode ? "text" : "numeric", false);
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "text") {
      var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">State</span>' +
          '<span class="sp-type-badge mdi mdi-format-text"></span></span>',
      };
    }

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
