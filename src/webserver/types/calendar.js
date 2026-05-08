// Read-only date card: displays either the day/month or local time/date.
registerButtonType("calendar", {
  label: "Date & Time",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.entity = "sensor.date";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = b.precision === "datetime" ? "datetime" : "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    if (!b.entity) b.entity = "sensor.date";
    if (b.precision !== "datetime") b.precision = "";

    var displayField = document.createElement("div");
    displayField.className = "sp-field";
    displayField.appendChild(helpers.fieldLabel("Display", helpers.idPrefix + "calendar-mode"));
    var modeSelect = document.createElement("select");
    modeSelect.className = "sp-select";
    modeSelect.id = helpers.idPrefix + "calendar-mode";

    [
      { value: "datetime", label: "Time & Date" },
      { value: "", label: "Date" },
      { value: "timezone", label: "World Clock" }
    ].forEach(function (optCfg) {
      var opt = document.createElement("option");
      opt.value = optCfg.value;
      opt.textContent = optCfg.label;
      modeSelect.appendChild(opt);
    });

    modeSelect.value = b.precision;
    modeSelect.addEventListener("change", function () {
      if (this.value === "timezone") {
        b.type = "timezone";
        b.entity = (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
        b.label = "";
        b.icon = "Auto";
        b.icon_on = "Auto";
        b.sensor = "";
        b.unit = "";
        b.precision = "";
        helpers.saveField("type", "timezone");
        helpers.saveField("entity", b.entity);
        helpers.saveField("label", "");
        helpers.saveField("icon", "Auto");
        helpers.saveField("icon_on", "Auto");
        helpers.saveField("sensor", "");
        helpers.saveField("unit", "");
        helpers.saveField("precision", "");
        renderButtonSettings();
      } else {
        b.precision = this.value === "datetime" ? "datetime" : "";
        helpers.saveField("precision", b.precision);
      }
    });
    displayField.appendChild(modeSelect);
    panel.appendChild(displayField);
  },
  renderPreview: function (b, helpers) {
    var now = new Date();
    var isDateTime = b.precision === "datetime";
    var day = String(now.getDate());
    var month = now.toLocaleString("en", { month: "long" });

    if (isDateTime) {
      var use12h = typeof state !== "undefined" && state.clockFormat === "12h";
      var hour = now.getHours();
      var minute = String(now.getMinutes()).padStart(2, "0");
      var timeValue = "";
      var timeUnit = "";

      if (use12h) {
        var hour12 = hour % 12;
        if (hour12 === 0) hour12 = 12;
        timeValue = String(hour12) + ":" + minute;
        timeUnit = hour < 12 ? "am" : "pm";
      } else {
        timeValue = String(hour).padStart(2, "0") + ":" + minute;
      }

      return {
        iconHtml:
          '<span class="sp-sensor-preview">' +
            '<span class="sp-sensor-value">' + helpers.escHtml(timeValue) + '</span>' +
            '<span class="sp-sensor-unit">' + helpers.escHtml(timeUnit) + '</span>' +
          '</span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
            helpers.escHtml(day + " " + month) +
          '</span><span class="sp-type-badge mdi mdi-calendar-month"></span></span>',
      };
    }

    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">' + day + '</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(month) + '</span>' +
        '<span class="sp-type-badge mdi mdi-calendar-month"></span></span>',
    };
  },
});
