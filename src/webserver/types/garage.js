// Garage door card: cover toggle with a label, transient state text, and open/closed icons.
registerButtonType("garage", {
  label: "Garage Door",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Garage";
    b.icon_on = "Garage Open";
  },
  renderSettings: function (panel, b, slot, helpers) {
    if (b.sensor) {
      b.sensor = "";
      helpers.saveField("sensor", "");
    }

    panel.appendChild(helpers.textField(
      "Label", helpers.idPrefix + "label", b.label, "e.g. Garage Door", "label", true).field);

    function iconField(label, inputSuffix, field, currentVal, defaultVal) {
      return helpers.iconPickerField(
        helpers.idPrefix + inputSuffix + "-picker",
        helpers.idPrefix + inputSuffix,
        currentVal,
        function (opt) {
        b[field] = opt || defaultVal;
        helpers.saveField(field, b[field]);
      }, label);
    }

    var entityField = helpers.entityField(
      "Entity", helpers.idPrefix + "entity", b.entity, "e.g. cover.garage_door",
      ["cover"], "entity", true, "Add an entity before saving.");
    panel.appendChild(entityField.field);

    var closedIconVal = b.icon && b.icon !== "Auto" ? b.icon : "Garage";
    var iconOnVal = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : "Garage Open";
    panel.appendChild(iconField("Closed Icon", "icon", "icon", closedIconVal, "Garage"));
    panel.appendChild(iconField("Open Icon", "icon-on", "icon_on", iconOnVal, "Garage Open"));
  },
  renderPreview: function (b, helpers) {
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "garage";
    var label = b.label || b.entity || "Garage Door";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-garage"></span></span>',
    };
  },
});
