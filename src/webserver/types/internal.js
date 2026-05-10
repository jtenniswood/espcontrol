// Internal relay card: controls built-in relay hardware locally on the device.
function internalRelayOptions() {
  return (CFG.features && CFG.features.internalRelays) || [];
}

function internalRelayDefaultIcon(mode) {
  return mode === "push" ? "Gesture Tap" : "Lightbulb Outline";
}

function internalRelayDefaultOnIcon() {
  return "Lightbulb";
}

function internalRelayUsesDefaultIcon(mode, icon) {
  if (!icon || icon === "Auto" || icon === internalRelayDefaultIcon(mode)) return true;
  return mode === "switch" && icon === "Power Plug";
}

function internalRelayUsesDefaultOnIcon(icon) {
  return !icon || icon === "Auto" || icon === internalRelayDefaultOnIcon() || icon === "Power";
}

function internalRelayMode(b) {
  return b.sensor === "push" ? "push" : "switch";
}

function internalRelayLabelFor(key) {
  var relays = internalRelayOptions();
  for (var i = 0; i < relays.length; i++) {
    if (relays[i].key === key) return relays[i].label;
  }
  return key ? key.replace(/_/g, " ").replace(/\b\w/g, function (ch) { return ch.toUpperCase(); }) : "Relay";
}

function ensureInternalRelaySelection(b) {
  var relays = internalRelayOptions();
  if (!relays.length) return;
  for (var i = 0; i < relays.length; i++) {
    if (relays[i].key === b.entity) return;
  }
  b.entity = relays[0].key;
}

registerButtonType("internal", {
  label: "Internal Switches",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Porch Light",
  isAvailable: function () {
    return internalRelayOptions().length > 0;
  },
  onSelect: function (b) {
    ensureInternalRelaySelection(b);
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = internalRelayDefaultIcon("switch");
    b.icon_on = internalRelayDefaultOnIcon();
  },
  renderSettings: function (panel, b, slot, helpers) {
    ensureInternalRelaySelection(b);
    var relays = internalRelayOptions();
    var mode = internalRelayMode(b);
    if (internalRelayUsesDefaultIcon(mode, b.icon)) b.icon = internalRelayDefaultIcon(mode);
    if (mode === "switch" && internalRelayUsesDefaultOnIcon(b.icon_on)) {
      b.icon_on = internalRelayDefaultOnIcon();
    }

    var relayField = helpers.selectField(
      "Internal Relay",
      helpers.idPrefix + "internal-relay",
      relays.length ? relays.map(function (relay) {
        return { value: relay.key, label: relay.label };
      }) : [["", "No relays"]],
      relays.length ? b.entity : ""
    );
    var relaySelect = relayField.select;
    relaySelect.disabled = !relays.length;
    relaySelect.addEventListener("change", function () {
      b.entity = this.value;
      helpers.saveField("entity", b.entity);
    });
    panel.appendChild(relayField.field);

    var modeControl = helpers.segmentControl([
      ["switch", "Switch"],
      ["push", "Push Button"],
    ], mode, function (value) { setMode(value); });
    var switchBtn = modeControl.buttons.switch;
    var pushBtn = modeControl.buttons.push;
    panel.appendChild(helpers.fieldWithControl("Mode", helpers.idPrefix + "internal-mode", modeControl.segment));

    function makeLabeledIconPicker(label, inputSuffix, pickerSuffix, value, onSelect) {
      var section = helpers.iconPickerField(
        helpers.idPrefix + pickerSuffix,
        helpers.idPrefix + inputSuffix,
        value,
        onSelect,
        label
      );
      var picker = section.querySelector(".sp-icon-picker");
      return { section: section, picker: picker };
    }

    function syncPicker(picker, value) {
      var preview = picker.querySelector(".sp-icon-picker-preview");
      if (preview) preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(value);
      var input = picker.querySelector(".sp-icon-picker-input");
      if (input) input.value = value;
    }

    var switchIconCond = condField();
    var pushIconCond = condField();
    panel.appendChild(switchIconCond);
    panel.appendChild(pushIconCond);

    var onIcon = makeLabeledIconPicker(
      "On Icon", "icon-on", "icon-on-picker",
      b.icon_on || internalRelayDefaultOnIcon(), function (opt) {
        b.icon_on = opt;
        helpers.saveField("icon_on", opt);
      }
    );
    var offIcon = makeLabeledIconPicker(
      "Off Icon", "icon-off", "icon-off-picker",
      b.icon || internalRelayDefaultIcon("switch"), function (opt) {
        syncIcon(opt);
      }
    );
    var pushIcon = makeLabeledIconPicker(
      "Icon", "icon", "icon-picker",
      b.icon || internalRelayDefaultIcon("push"), function (opt) {
        syncIcon(opt);
      }
    );
    switchIconCond.appendChild(onIcon.section);
    switchIconCond.appendChild(offIcon.section);
    pushIconCond.appendChild(pushIcon.section);

    function syncIcon(value) {
      b.icon = value;
      helpers.saveField("icon", value);
      syncPicker(offIcon.picker, value);
      syncPicker(pushIcon.picker, value);
    }

    function syncOnIcon(value) {
      b.icon_on = value;
      helpers.saveField("icon_on", value);
      syncPicker(onIcon.picker, value);
    }

    function syncModeUi() {
      switchBtn.classList.toggle("active", mode === "switch");
      pushBtn.classList.toggle("active", mode === "push");
      switchIconCond.classList.toggle("sp-visible", mode === "switch");
      pushIconCond.classList.toggle("sp-visible", mode === "push");
    }

    function setMode(nextMode) {
      if (mode === nextMode) return;
      var wasDefaultIcon = internalRelayUsesDefaultIcon(mode, b.icon);
      mode = nextMode;
      b.sensor = mode === "push" ? "push" : "";
      helpers.saveField("sensor", b.sensor);
      if (wasDefaultIcon) {
        syncIcon(internalRelayDefaultIcon(mode));
      }
      if (mode === "push") {
        syncOnIcon("Auto");
      } else if (!b.icon_on || b.icon_on === "Auto") {
        syncOnIcon(internalRelayDefaultOnIcon());
      }
      syncModeUi();
    }
    syncModeUi();
  },
  renderPreview: function (b, helpers) {
    var mode = internalRelayMode(b);
    var label = b.label || internalRelayLabelFor(b.entity);
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(internalRelayDefaultIcon(mode));
    var badge = mode === "push" ? "gesture-tap" : "power-plug";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-' + badge + '"></span></span>',
    };
  },
});
