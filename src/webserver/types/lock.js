// Lock card: lock/unlock toggle with safe default-to-lock behavior and state display.
function lockCommandMode(mode) {
  return mode === "lock" || mode === "unlock";
}

function normalizeLockMode(mode) {
  return lockCommandMode(mode) ? mode : "";
}

function lockModeDefaultIcon(mode) {
  return mode === "unlock" ? "Lock Open" : "Lock";
}

function lockModeDefaultLabel(mode) {
  if (mode === "lock") return "Lock";
  if (mode === "unlock") return "Unlock";
  return "Lock";
}

function lockUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Lock" || icon === "Lock Open";
}

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
    var mode = normalizeLockMode(b.sensor);
    if (b.sensor !== mode) {
      b.sensor = mode;
      helpers.saveField("sensor", mode);
    }
    b.unit = "";
    b.precision = "";
    if (lockCommandMode(mode) && b.icon_on !== "Auto") {
      b.icon_on = "Auto";
      helpers.saveField("icon_on", "Auto");
    } else if (!lockCommandMode(mode) && (!b.icon_on || b.icon_on === "Auto")) {
      b.icon_on = "Lock Open";
      helpers.saveField("icon_on", "Lock Open");
    }

    var typeOptions = [
      ["", "Toggle"],
      ["lock", "Lock"],
      ["unlock", "Unlock"],
    ];
    var typeField = helpers.selectField("Type", helpers.idPrefix + "lock-type", typeOptions, mode);
    var typeSelect = typeField.select;
    panel.appendChild(typeField.field);

    typeSelect.addEventListener("change", function () {
      var oldMode = mode;
      var hadDefaultIcon = lockUsesDefaultIcon(b.icon);
      mode = normalizeLockMode(this.value);
      b.sensor = mode;
      helpers.saveField("sensor", mode);
      b.unit = "";
      b.precision = "";
      helpers.saveField("unit", "");
      helpers.saveField("precision", "");
      if (hadDefaultIcon || b.icon === lockModeDefaultIcon(oldMode)) {
        b.icon = lockModeDefaultIcon(mode);
        helpers.saveField("icon", b.icon);
      }
      if (lockCommandMode(mode)) {
        b.icon_on = "Auto";
      } else if (!b.icon_on || b.icon_on === "Auto") {
        b.icon_on = "Lock Open";
      }
      helpers.saveField("icon_on", b.icon_on);
      renderButtonSettings();
    });

    panel.appendChild(helpers.textField(
      "Label", helpers.idPrefix + "label", b.label,
      lockCommandMode(mode) ? "e.g. " + lockModeDefaultLabel(mode) + " Front Door" : "e.g. Front Door",
      "label", true).field);

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
    if (lockCommandMode(mode)) {
      panel.appendChild(iconField(
        "Icon", "icon", "icon", b.icon && b.icon !== "Auto" ? b.icon : lockModeDefaultIcon(mode),
        lockModeDefaultIcon(mode)));
    } else {
      panel.appendChild(iconField("Locked Icon", "icon", "icon", lockedIconVal, "Lock"));
      panel.appendChild(iconField("Unlocked Icon", "icon-on", "icon_on", unlockedIconVal, "Lock Open"));
    }
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeLockMode(b.sensor);
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(lockModeDefaultIcon(mode));
    var label = b.label || (lockCommandMode(mode) ? lockModeDefaultLabel(mode) : b.entity || "Lock");
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-lock"></span></span>',
    };
  },
});
