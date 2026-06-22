// Lock card: lock/unlock toggle with safe default-to-lock behavior and state display.
function lockCommandMode(mode) {
  return mode === "lock" || mode === "unlock";
}

function lockModeOptionValues() {
  var spec = cardContractOptionSpec("lock", "lock_mode");
  return spec && spec.values ? spec.values.slice() : ["", "lock", "unlock"];
}

function normalizeLockMode(mode) {
  mode = String(mode || "");
  return lockModeOptionValues().indexOf(mode) >= 0 ? mode : "";
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

var LOCK_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "lock-type",
    options: [
      ["", "Toggle"],
      ["lock", "Lock"],
      ["unlock", "Unlock"],
    ],
    value: function (b) {
      return normalizeLockMode(b.sensor);
    },
  },
  entity: {
    label: "Entity",
    idSuffix: "entity",
    placeholder: "e.g. lock.front_door",
    domains: function () { return cardContractDomains("lock"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add an entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "label",
    field: "label",
    rerender: true,
  },
  preview: {
    badge: "lock",
  },
};

registerButtonType("lock", {
  label: function () { return cardContractCardLabel("lock"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("lock"); },
  pickerKey: function () { return cardContractPickerKey("lock"); },
  hidden: function () { return cardContractHidden("lock"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("lock"); },
  cardMetadata: LOCK_CARD_METADATA,
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

    helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, LOCK_CARD_METADATA, {
      mode: Object.assign({}, LOCK_CARD_METADATA.mode, {
        value: function () { return mode; },
        onChange: function () {
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
        },
      }),
    }));

    helpers.renderCardTextField(panel, b, helpers, Object.assign({}, LOCK_CARD_METADATA.labelField, {
      placeholder: lockCommandMode(mode) ? "e.g. " + lockModeDefaultLabel(mode) + " Front Door" : "e.g. Front Door",
    }));

    helpers.renderCardEntityField(panel, b, helpers, LOCK_CARD_METADATA);

    var lockedIconVal = b.icon && b.icon !== "Auto" ? b.icon : "Lock";
    var unlockedIconVal = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : "Lock Open";
    if (lockCommandMode(mode)) {
      helpers.renderCardIconPicker(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        value: b.icon && b.icon !== "Auto" ? b.icon : lockModeDefaultIcon(mode),
        fallback: lockModeDefaultIcon(mode),
        label: "Icon",
      });
    } else {
      helpers.renderCardIconPair(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        value: lockedIconVal,
        fallback: "Lock",
        label: "Locked Icon",
      }, {
        pickerIdSuffix: "icon-on-picker",
        idSuffix: "icon-on",
        field: "icon_on",
        value: unlockedIconVal,
        fallback: "Lock Open",
        label: "Unlocked Icon",
      });
    }
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeLockMode(b.sensor);
    var label = b.label || (lockCommandMode(mode) ? lockModeDefaultLabel(mode) : b.entity || "Lock");
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: lockModeDefaultIcon(mode),
      badge: LOCK_CARD_METADATA.preview.badge,
    });
  },
});
