// Momentary trigger card: stored as "push" for config compatibility.
// Fires an esphome.push_button_pressed event with no toggle state.
registerButtonType("push", {
  label: "Trigger",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Doorbell",
  onSelect: function (b) {
    b.entity = ""; b.sensor = ""; b.unit = ""; b.icon_on = "Auto";
    b.icon = "Gesture Tap";
  },
  renderSettings: function (panel, b, slot, helpers) {
    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Auto", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }
    ));
  },
  renderPreview: function (b, helpers) {
    var label = b.label || "Trigger";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "gesture-tap";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-gesture-tap"></span></span>',
    };
  },
});
