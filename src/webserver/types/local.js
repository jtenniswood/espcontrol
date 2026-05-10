// Local action card: calls a register_local_action() callback on the device directly,
// without going through Home Assistant. Register actions in your device on_boot lambda:
//   register_local_action("my_key", "My Label", [=]() { id(my_component).do_thing(); });
registerButtonType("local", {
  label: "Local Action",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Zoom Mute",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Gesture Tap";
    b.icon_on = "Auto";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var kf = document.createElement("div");
    kf.className = "sp-field";
    kf.appendChild(helpers.fieldLabel("Action Key", helpers.idPrefix + "local-key"));
    var keyInp = helpers.textInput(helpers.idPrefix + "local-key", b.entity, "e.g. zoom_mute");
    kf.appendChild(keyInp);
    panel.appendChild(kf);
    helpers.bindField(keyInp, "entity", true);
    helpers.requireField(keyInp, "Add an action key before saving.");

    panel.appendChild(helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Gesture Tap", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }
    ));
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Local Action";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "gesture-tap";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
        helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-chip"></span></span>',
    };
  },
});
