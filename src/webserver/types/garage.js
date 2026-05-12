// Garage door card: cover toggle or one-tap open/close commands.
function garageCommandMode(mode) {
  return mode === "open" || mode === "close";
}

function normalizeGarageMode(mode) {
  return garageCommandMode(mode) ? mode : "";
}

function garageModeDefaultIcon(mode) {
  return mode === "open" ? "Garage Open" : "Garage";
}

function garageModeDefaultLabel(mode) {
  if (mode === "open") return "Open";
  if (mode === "close") return "Close";
  return "Garage Door";
}

function garageUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Garage" || icon === "Garage Open";
}

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
    var mode = normalizeGarageMode(b.sensor);
    if (b.sensor !== mode) {
      b.sensor = mode;
      helpers.saveField("sensor", mode);
    }
    b.unit = "";
    b.precision = "";
    if (garageCommandMode(mode) && b.icon_on !== "Auto") {
      b.icon_on = "Auto";
      helpers.saveField("icon_on", "Auto");
    } else if (!garageCommandMode(mode) && (!b.icon_on || b.icon_on === "Auto")) {
      b.icon_on = "Garage Open";
      helpers.saveField("icon_on", "Garage Open");
    }

    var interactionOptions = [
      ["", "Toggle"],
      ["open", "Open"],
      ["close", "Close"],
    ];
    var interactionField = helpers.selectField(
      "Interaction", helpers.idPrefix + "garage-interaction", interactionOptions, mode);
    var interactionSelect = interactionField.select;
    panel.appendChild(interactionField.field);

    interactionSelect.addEventListener("change", function () {
      var oldMode = mode;
      var hadDefaultIcon = garageUsesDefaultIcon(b.icon);
      mode = normalizeGarageMode(this.value);
      b.sensor = mode;
      helpers.saveField("sensor", mode);
      b.unit = "";
      b.precision = "";
      helpers.saveField("unit", "");
      helpers.saveField("precision", "");
      if (hadDefaultIcon || b.icon === garageModeDefaultIcon(oldMode)) {
        b.icon = garageModeDefaultIcon(mode);
        helpers.saveField("icon", b.icon);
      }
      if (garageCommandMode(mode)) {
        b.icon_on = "Auto";
      } else if (!b.icon_on || b.icon_on === "Auto") {
        b.icon_on = "Garage Open";
      }
      helpers.saveField("icon_on", b.icon_on);
      renderButtonSettings();
    });

    panel.appendChild(helpers.textField(
      "Label", helpers.idPrefix + "label", b.label,
      garageCommandMode(mode) ? "e.g. " + garageModeDefaultLabel(mode) + " Garage" : "e.g. Garage Door",
      "label", true).field);

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
    if (garageCommandMode(mode)) {
      panel.appendChild(iconField(
        "Icon", "icon", "icon", b.icon && b.icon !== "Auto" ? b.icon : garageModeDefaultIcon(mode),
        garageModeDefaultIcon(mode)));
    } else {
      panel.appendChild(iconField("Closed Icon", "icon", "icon", closedIconVal, "Garage"));
      panel.appendChild(iconField("Open Icon", "icon-on", "icon_on", iconOnVal, "Garage Open"));
    }
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeGarageMode(b.sensor);
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(garageModeDefaultIcon(mode));
    var label = b.label || (garageCommandMode(mode) ? garageModeDefaultLabel(mode) : b.entity || "Garage Door");
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-garage"></span></span>',
    };
  },
});
