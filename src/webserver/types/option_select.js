// Option Select card: displays and changes a select/input_select entity option.
registerButtonType("option_select", {
  label: "Option Select",
  allowInSubpage: true,
  labelPlaceholder: "e.g. WLED Preset",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "";
    b.unit = "";
    b.icon = "Chevron Down";
    b.icon_on = "Auto";
    b.precision = "";
    b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    b.icon_on = "Auto";
    b.precision = "";
    b.options = "";

    panel.appendChild(helpers.entityField(
      "Select Entity",
      helpers.idPrefix + "entity",
      b.entity,
      "e.g. select.wled_preset",
      ["select", "input_select"],
      "entity",
      true,
      "Add a select or input_select entity before saving."
    ).field);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Option Select";
    return {
      iconHtml:
        '<span class="sp-sensor-preview">' +
          '<span class="sp-sensor-value">Option</span>' +
        '</span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-chevron-down"></span></span>',
    };
  },
});
