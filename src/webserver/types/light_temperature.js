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
  labelPlaceholder: "e.g. Living Room",
  onSelect: function (b) {
    b.sensor = "";
    b.unit = "2000-6500";
    b.precision = "";
    b.icon = "Auto";
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

    // Color fill by temperature
    var colorRow = helpers.toggleRow("Use light color", helpers.idPrefix + "kelvin-color", b.precision === "color");
    panel.appendChild(colorRow.row);
    colorRow.input.addEventListener("change", function () {
      b.precision = this.checked ? "color" : "";
      helpers.saveField("precision", b.precision);
    });

    // Show kelvin value as label when light is on
    var kRow = helpers.toggleRow("Show kelvin value when on", helpers.idPrefix + "show-kelvin", b.sensor === "kelvin");
    panel.appendChild(kRow.row);
    kRow.input.addEventListener("change", function () {
      b.sensor = this.checked ? "kelvin" : "";
      helpers.saveField("sensor", b.sensor);
    });
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Light Temp";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "thermometer";
    return {
      iconHtml:
        '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
        '<span class="sp-slider-preview"><span class="sp-slider-track">' +
          '<span class="sp-slider-fill"></span>' +
        '</span></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-thermometer"></span></span>',
    };
  },
});
