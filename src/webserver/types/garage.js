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

    var lf = document.createElement("div");
    lf.className = "sp-field";
    lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
    var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, "e.g. Garage Door");
    lf.appendChild(labelInp);
    panel.appendChild(lf);
    helpers.bindField(labelInp, "label", true);

    function iconField(label, inputSuffix, field, currentVal, defaultVal) {
      var section = document.createElement("div");
      section.className = "sp-field";
      section.appendChild(helpers.fieldLabel(label, helpers.idPrefix + inputSuffix));
      var picker = document.createElement("div");
      picker.className = "sp-icon-picker";
      picker.id = helpers.idPrefix + inputSuffix + "-picker";
      picker.innerHTML =
        '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(currentVal) + '"></span>' +
        '<input class="sp-icon-picker-input" id="' + helpers.idPrefix + inputSuffix + '" type="text" ' +
        'placeholder="Search icons\u2026" value="' + escAttr(currentVal) + '" autocomplete="off">' +
        '<div class="sp-icon-dropdown"></div>';
      section.appendChild(picker);
      initIconPicker(picker, currentVal, function (opt) {
        b[field] = opt || defaultVal;
        helpers.saveField(field, b[field]);
      });
      return section;
    }

    var ef = document.createElement("div");
    ef.className = "sp-field";
    ef.appendChild(helpers.fieldLabel("Entity ID", helpers.idPrefix + "entity"));
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, "e.g. cover.garage_door");
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);
    helpers.requireField(entityInp, "Add an entity before saving.");

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
