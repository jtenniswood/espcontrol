// Camera snapshot card: tapping opens a fullscreen popup with periodic JPEG
// snapshots from Home Assistant's camera_proxy REST endpoint.
registerButtonType("camera", {
  label: "Camera",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Front Door",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "5";
    b.unit = "";
    b.icon = "Camera";
    b.icon_on = "Auto";
    b.precision = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var entityField = helpers.entityField(
      "Camera Entity", helpers.idPrefix + "entity", b.entity,
      "e.g. camera.front_door", ["camera"], "entity", true,
      "Add a camera entity before saving."
    );
    panel.appendChild(entityField.field);

    var rawSecs = parseInt(b.sensor, 10);
    if (!isFinite(rawSecs) || rawSecs < 2) rawSecs = 5;
    if (rawSecs > 120) rawSecs = 120;
    b.sensor = String(rawSecs);

    var intervalInput = helpers.textInput(
      helpers.idPrefix + "interval", b.sensor, "5"
    );
    intervalInput.type = "number";
    intervalInput.min = "2";
    intervalInput.max = "120";
    intervalInput.addEventListener("change", function () {
      var secs = parseInt(this.value, 10);
      if (!isFinite(secs) || secs < 2) secs = 2;
      if (secs > 120) secs = 120;
      b.sensor = String(secs);
      helpers.saveField("sensor", b.sensor);
    });
    var intervalLabel = helpers.fieldLabel(
      "Refresh Interval (seconds)", helpers.idPrefix + "interval"
    );
    var intervalField = document.createElement("div");
    intervalField.className = "sp-field";
    intervalField.appendChild(intervalLabel);
    intervalField.appendChild(intervalInput);
    panel.appendChild(intervalField);

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Camera", function (opt) {
        b.icon = opt || "Camera";
        helpers.saveField("icon", b.icon);
      }
    ));

    var haUrl = (typeof state !== "undefined" && state.haBaseUrl) || "http://homeassistant.local:8123";
    var haToken = (typeof state !== "undefined" && state.haRestToken) || "";
    var haNote = document.createElement("div");
    haNote.className = "sp-apply-note";
    var tokenStatus = haToken ? "token set" : "no token";
    haNote.textContent = "HA: " + haUrl + " — " + tokenStatus;
    if (!haToken) haNote.title = "Set a Long-Lived Token in Home Assistant settings if your instance requires authentication.";
    panel.appendChild(haNote);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Camera";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "camera";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
        helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-camera"></span></span>',
    };
  },
});
