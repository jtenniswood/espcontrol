// Light temperature slider card: controls color_temp_kelvin on a light entity.
// Slider bottom = min kelvin (warm), top = max kelvin (cool).
// Config fields: unit="min-max" (kelvin range),
// precision="color" (dynamic fill color by current temperature),
// sensor="kelvin" (show live kelvin value as label when light is on).

function lightTempParseRange(unit) {
  var parts = (unit || "2000-6500").split("-");
  var mn = parseInt(parts[0], 10);
  var mx = parseInt(parts[1], 10);
  if (!isFinite(mn) || mn < 1000) mn = 2000;
  if (!isFinite(mx) || mx <= mn) mx = 6500;
  return [mn, mx];
}

function lightTempClampMin(v, absMin) {
  var n = parseInt(v, 10);
  if (!isFinite(n)) n = absMin;
  if (n < absMin) n = absMin;
  if (n > 9900) n = 9900;
  return n;
}

function lightTempClampMax(v, mn) {
  var n = parseInt(v, 10);
  if (!isFinite(n)) n = mn + 100;
  if (n <= mn) n = mn + 100;
  if (n > 10000) n = 10000;
  return n;
}

registerButtonType("light_temperature", {
  label: "Light Temperature Slider",
  experimental: "light_temperature",
  allowInSubpage: true,
  hideLabel: true,
  labelPlaceholder: "e.g. Living Room",
  onSelect: function (b) {
    b.sensor = "";
    b.unit = "2000-6500";
    b.precision = "";
    b.icon = "Lightbulb";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    // Entity ID
    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Entity ID", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. light.living_room");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);

    // Kelvin range
    var range = lightTempParseRange(b.unit);
    var curMin = range[0], curMax = range[1];

    function saveRange(mn, mx) {
      b.unit = mn + "-" + mx;
      helpers.saveField("unit", b.unit);
    }

    var minF = document.createElement("div");
    minF.className = "sp-field";
    minF.appendChild(helpers.fieldLabel("Min Color Temp (K)", helpers.idPrefix + "kmin"));
    var minInp = document.createElement("input");
    minInp.type = "number";
    minInp.className = "sp-input";
    minInp.id = helpers.idPrefix + "kmin";
    minInp.min = "1000";
    minInp.max = "9900";
    minInp.step = "100";
    minInp.placeholder = "2000";
    minInp.value = curMin;
    minF.appendChild(minInp);
    panel.appendChild(minF);

    var maxF = document.createElement("div");
    maxF.className = "sp-field";
    maxF.appendChild(helpers.fieldLabel("Max Color Temp (K)", helpers.idPrefix + "kmax"));
    var maxInp = document.createElement("input");
    maxInp.type = "number";
    maxInp.className = "sp-input";
    maxInp.id = helpers.idPrefix + "kmax";
    maxInp.min = "1100";
    maxInp.max = "10000";
    maxInp.step = "100";
    maxInp.placeholder = "6500";
    maxInp.value = curMax;
    maxF.appendChild(maxInp);
    panel.appendChild(maxF);

    function onRangeChange() {
      var mn = lightTempClampMin(minInp.value, 1000);
      var mx = lightTempClampMax(maxInp.value, mn);
      minInp.value = mn;
      maxInp.value = mx;
      saveRange(mn, mx);
    }
    minInp.addEventListener("change", onRangeChange);
    maxInp.addEventListener("change", onRangeChange);
    minInp.addEventListener("blur", onRangeChange);
    maxInp.addEventListener("blur", onRangeChange);

    // Icon
    panel.appendChild(helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Auto", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }
    ));

    // Label mode
    var labelMode = b.sensor === "kelvin" ? "setting" : "label";
    var labelModeField = document.createElement("div");
    labelModeField.className = "sp-field";
    labelModeField.appendChild(helpers.fieldLabel("Label"));
    var labelModeSeg = document.createElement("div");
    labelModeSeg.className = "sp-segment";
    var labelBtn = document.createElement("button");
    labelBtn.type = "button";
    labelBtn.textContent = "Label";
    var settingBtn = document.createElement("button");
    settingBtn.type = "button";
    settingBtn.textContent = "Setting";
    labelModeSeg.appendChild(labelBtn);
    labelModeSeg.appendChild(settingBtn);
    labelModeField.appendChild(labelModeSeg);
    panel.appendChild(labelModeField);

    var labelSection = condField();
    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
    var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Living Room");
    lf.appendChild(labelInp);
    labelSection.appendChild(lf);
    panel.appendChild(labelSection);
    helpers.bindField(labelInp, "label", true);

    function setLabelMode(mode, persist) {
      labelMode = mode === "setting" ? "setting" : "label";
      labelBtn.classList.toggle("active", labelMode === "label");
      settingBtn.classList.toggle("active", labelMode === "setting");
      labelSection.classList.toggle("sp-visible", labelMode === "label");
      if (!persist) return;
      b.sensor = labelMode === "setting" ? "kelvin" : "";
      helpers.saveField("sensor", b.sensor);
    }
    labelBtn.addEventListener("click", function () { setLabelMode("label", true); });
    settingBtn.addEventListener("click", function () { setLabelMode("setting", true); });
    setLabelMode(labelMode, false);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Light Temp";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "lightbulb";
    return {
      iconHtml:
        '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
        '<span class="sp-slider-preview"><span class="sp-slider-track">' +
          '<span class="sp-slider-fill"></span>' +
        '</span></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-lightbulb"></span></span>',
    };
  },
});
