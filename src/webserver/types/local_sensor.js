// Local sensor card: displays a value from any ESPHome sensor component on the device.
// The ESP32 auto-subscribes to the sensor's on_value callbacks — no firmware changes needed.
// For computed/non-entity values, use send_local_sensor_update() as a fallback.
registerButtonType("local_sensor", {
  label: "Local Sensor",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "";
    b.icon_on = "Auto";
    if (!b.precision) b.precision = "";
    if (b.precision !== "text") b.icon = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var isTextMode = b.precision === "text";
    var showAll = false;
    var fetchedSensors = null;

    // ── Mode toggle ──────────────────────────────────────────────────
    var modeField = document.createElement("div");
    modeField.className = "sp-field";
    modeField.appendChild(helpers.fieldLabel("Display"));
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

    // ── Sensor picker section (async) ────────────────────────────────
    var pickerSection = document.createElement("div");
    panel.appendChild(pickerSection);

    // ── Numeric fields (label / unit / precision) ────────────────────
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
    var unitInp = helpers.textInput(helpers.idPrefix + "unit", b.unit, "e.g. °C");
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

    // ── Text fields (icon picker) ────────────────────────────────────
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

    // ── Mode switch ──────────────────────────────────────────────────
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

    numericBtn.addEventListener("click", function () {
      setMode("numeric", true);
      if (fetchedSensors) buildDropdown(fetchedSensors);
    });
    textBtn.addEventListener("click", function () {
      setMode("text", true);
      if (fetchedSensors) buildDropdown(fetchedSensors);
    });
    setMode(isTextMode ? "text" : "numeric", false);

    // ── Picker builder ───────────────────────────────────────────────
    function buildDropdown(sensors) {
      pickerSection.innerHTML = "";
      var wantType = isTextMode ? "text" : "numeric";
      var filtered = sensors.filter(function (s) {
        return s.type === wantType && (showAll || !s.internal);
      });

      var sf = document.createElement("div");
      sf.className = "sp-field";
      sf.appendChild(helpers.fieldLabel("Sensor", helpers.idPrefix + "sensor-sel"));
      var sel = document.createElement("select");
      sel.className = "sp-select";
      sel.id = helpers.idPrefix + "sensor-sel";

      var placeholder = document.createElement("option");
      placeholder.value = "";
      placeholder.textContent = "Choose a sensor…";
      sel.appendChild(placeholder);

      filtered.forEach(function (s) {
        var opt = document.createElement("option");
        opt.value = s.key;
        opt.textContent = s.name + (s.type === "text" ? " (text)" : "");
        if (s.key === b.entity) opt.selected = true;
        sel.appendChild(opt);
      });

      // If current key isn't in the filtered list, add it so selection is preserved
      if (b.entity && !filtered.some(function (s) { return s.key === b.entity; })) {
        var curOpt = document.createElement("option");
        curOpt.value = b.entity;
        curOpt.textContent = b.entity + " (current)";
        curOpt.selected = true;
        sel.appendChild(curOpt);
      }

      sel.addEventListener("change", function () {
        var key = this.value;
        if (!key) return;
        b.entity = key;
        helpers.saveField("entity", key);
        var sensor = sensors.find(function (s) { return s.key === key; });
        if (!sensor) return;
        // Auto-fill label and unit only when empty
        if (!b.label) {
          b.label = sensor.name;
          labelInp.value = sensor.name;
          helpers.saveField("label", sensor.name);
        }
        if (!b.unit && sensor.unit) {
          b.unit = sensor.unit;
          unitInp.value = sensor.unit;
          helpers.saveField("unit", sensor.unit);
        }
        setMode(sensor.type === "text" ? "text" : "numeric", true);
      });
      sf.appendChild(sel);
      pickerSection.appendChild(sf);

      var tog = toggleRow("Show internal sensors", helpers.idPrefix + "show-all", showAll);
      tog.input.addEventListener("change", function () {
        showAll = this.checked;
        buildDropdown(sensors);
      });
      pickerSection.appendChild(tog.row);
    }

    function buildManualInput() {
      pickerSection.innerHTML = "";

      var errDiv = document.createElement("div");
      errDiv.className = "sp-banner sp-error";
      errDiv.textContent = "Could not reach device. Enter sensor key manually.";
      pickerSection.appendChild(errDiv);

      var kf = document.createElement("div");
      kf.className = "sp-field";
      kf.appendChild(helpers.fieldLabel("Sensor Key", helpers.idPrefix + "local-sensor-key"));
      var keyInp = helpers.textInput(helpers.idPrefix + "local-sensor-key", b.entity, "e.g. room_temp");
      kf.appendChild(keyInp);
      pickerSection.appendChild(kf);
      helpers.bindField(keyInp, "entity", true);
      helpers.requireField(keyInp, "Add a sensor key before saving.");
    }

    // Show loading state then fetch
    var loadingDiv = document.createElement("div");
    loadingDiv.className = "sp-field";
    loadingDiv.textContent = "Loading sensors…";
    pickerSection.appendChild(loadingDiv);

    fetch("/local_sensors")
      .then(function (resp) {
        if (!resp.ok) throw new Error("HTTP " + resp.status);
        return resp.json();
      })
      .then(function (data) {
        fetchedSensors = data;
        buildDropdown(data);
      })
      .catch(function () {
        buildManualInput();
      });
  },
  renderPreview: function (b, helpers) {
    if (b.precision === "text") {
      var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">State</span>' +
          '<span class="sp-type-badge mdi mdi-gauge"></span></span>',
      };
    }

    var label = b.label || b.entity || "Local Sensor";
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
