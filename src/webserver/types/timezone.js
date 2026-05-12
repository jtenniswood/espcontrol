// Read-only world clock card: displays local time for a selected city.
function timezoneCardCityLabel(tzOption) {
  var tzId = getTzId(tzOption || "");
  if (!tzId) return "World Clock";
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
  label: "Date & Time",
  allowInSubpage: true,
  hideLabel: true,
  isAvailable: function () {
    return false;
  },
  onSelect: function (b) {
    b.entity = (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var isLargeCard = helpers.cardSize === 4;
    if (!b.entity) b.entity = (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    if (b.label) {
      b.label = "";
      helpers.saveField("label", "");
    }

    var modeField = helpers.selectField("Type", helpers.idPrefix + "calendar-mode", [
      { value: "datetime", label: "Time & Date" },
      { value: "", label: "Date" },
      { value: "timezone", label: "World Clock" }
    ], "timezone", function () {
      if (this.value !== "timezone") {
        b.type = "calendar";
        b.entity = "sensor.date";
        b.label = "";
        b.icon = "Auto";
        b.icon_on = "Auto";
        b.sensor = "";
        b.unit = "";
        b.precision = this.value === "datetime" ? "datetime" : "";
        helpers.saveField("type", "calendar");
        helpers.saveField("entity", "sensor.date");
        helpers.saveField("label", "");
        helpers.saveField("icon", "Auto");
        helpers.saveField("icon_on", "Auto");
        helpers.saveField("sensor", "");
        helpers.saveField("unit", "");
        helpers.saveField("precision", b.precision);
        helpers.saveField("options", b.options);
        renderButtonSettings();
      }
    });
    panel.appendChild(modeField.field);

    if (isLargeCard) {
      var largeNumbersToggle = helpers.toggleRow(
        "Large Date / Time Numbers", helpers.idPrefix + "large-date-time-numbers",
        cardLargeNumbersEnabled(b));
      panel.appendChild(largeNumbersToggle.row);
      largeNumbersToggle.input.addEventListener("change", function () {
        setSensorLargeNumbersEnabled(b, this.checked);
        helpers.saveField("options", b.options);
      });
    }

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

    panel.appendChild(helpers.fieldWithControl("City / Timezone", helpers.idPrefix + "timezone", tzSelect));
  },
  renderPreview: function (b, helpers) {
    var tz = b.entity || (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    var time = timezoneCardTimeParts(tz);
    return {
      iconHtml:
        '<span class="sp-sensor-preview' +
          (helpers.cardSize === 4 && cardLargeNumbersEnabled(b) ? " sp-sensor-preview-large" : "") + '">' +
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
