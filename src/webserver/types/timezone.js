// Read-only timezone card: displays local time for a selected city.
function timezoneCardCityLabel(tzOption) {
  var tzId = getTzId(tzOption || "");
  if (!tzId) return "Timezone";
  if (tzId === "UTC") return "UTC";
  var city = tzId.substring(tzId.lastIndexOf("/") + 1);
  return city.replace(/_/g, " ");
}

function timezoneCardTimeParts(tzOption) {
  var use12h = typeof state !== "undefined" && state.clockFormat === "12h";
  var tzId = getTzId(tzOption || "UTC");
  try {
    var opts = { timeZone: tzId, hour: "numeric", minute: "2-digit" };
    if (use12h) opts.hour12 = true;
    else opts.hourCycle = "h23";
    var parts = new Intl.DateTimeFormat("en-US", opts).formatToParts(new Date());
    var hour = "";
    var minute = "";
    var dayPeriod = "";
    for (var i = 0; i < parts.length; i++) {
      if (parts[i].type === "hour") hour = parts[i].value;
      else if (parts[i].type === "minute") minute = parts[i].value;
      else if (parts[i].type === "dayPeriod") dayPeriod = parts[i].value;
    }
    if (!hour || !minute) return { value: "--:--", unit: "" };
    return {
      value: (use12h ? hour : hour.padStart(2, "0")) + ":" + minute,
      unit: use12h ? dayPeriod.toLowerCase().replace(/\./g, "") : "",
    };
  } catch (e) {
    return { value: "--:--", unit: "" };
  }
}

registerButtonType("timezone", {
  label: "Timezone",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.entity = (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    if (!b.entity) b.entity = (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    if (b.label) {
      b.label = "";
      helpers.saveField("label", "");
    }

    var tzField = document.createElement("div");
    tzField.className = "sp-field";
    tzField.appendChild(helpers.fieldLabel("Timezone", helpers.idPrefix + "timezone"));

    var tzSelect = document.createElement("select");
    tzSelect.className = "sp-select";
    tzSelect.id = helpers.idPrefix + "timezone";

    var options = [];
    if (typeof state !== "undefined" && state.timezoneOptions.length) {
      options = state.timezoneOptions.slice();
    }
    if (options.indexOf(b.entity) === -1) options.unshift(b.entity);

    options.forEach(function (opt) {
      appendTimezoneOption(tzSelect, opt);
    });
    tzSelect.value = b.entity;
    tzSelect.addEventListener("change", function () {
      b.entity = this.value;
      b.label = "";
      helpers.saveField("entity", b.entity);
      helpers.saveField("label", "");
    });

    tzField.appendChild(tzSelect);
    panel.appendChild(tzField);
  },
  renderPreview: function (b, helpers) {
    var tz = b.entity || (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    var time = timezoneCardTimeParts(tz);
    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">' + helpers.escHtml(time.value) + '</span>' +
          '<span class="sp-sensor-unit">' + helpers.escHtml(time.unit) + '</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
          helpers.escHtml(timezoneCardCityLabel(tz)) +
        '</span><span class="sp-type-badge mdi mdi-map-clock"></span></span>',
    };
  },
});
