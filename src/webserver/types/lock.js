// Lock card: lock/unlock toggle with safe default-to-lock behavior and state display.
registerButtonType("lock", {
  label: "Lock",
  allowInSubpage: true,
  hideLabel: true,
  onSelect: function (b) {
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Lock";
    b.icon_on = "Lock Open";
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";

    panel.appendChild(helpers.textField(
      "Label", helpers.idPrefix + "label", b.label, "e.g. Front Door", "label", true).field);

    var entityField = helpers.entityField(
      "Entity", helpers.idPrefix + "entity", b.entity, "e.g. lock.front_door",
      ["lock"], "entity", true, "Add an entity before saving.");
    panel.appendChild(entityField.field);

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

    var lockedIconVal = b.icon && b.icon !== "Auto" ? b.icon : "Lock";
    var unlockedIconVal = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : "Lock Open";
    panel.appendChild(iconField("Locked Icon", "icon", "icon", lockedIconVal, "Lock"));
    panel.appendChild(iconField("Unlocked Icon", "icon-on", "icon_on", unlockedIconVal, "Lock Open"));
  },
  renderPreview: function (b, helpers) {
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "lock";
    var label = b.label || b.entity || "Lock";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-lock"></span></span>',
    };
  },
});
