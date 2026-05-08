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

    var rf = document.createElement("div");
    rf.className = "sp-field";
    rf.appendChild(helpers.fieldLabel("Internal Relay", helpers.idPrefix + "internal-relay"));
    var relaySelect = document.createElement("select");
    relaySelect.className = "sp-select";
    relaySelect.id = helpers.idPrefix + "internal-relay";
    if (!relays.length) {
      var emptyOpt = document.createElement("option");
      emptyOpt.value = "";
      emptyOpt.textContent = "No relays";
      relaySelect.appendChild(emptyOpt);
      relaySelect.disabled = true;
    } else {
      relays.forEach(function (relay) {
        var opt = document.createElement("option");
        opt.value = relay.key;
        opt.textContent = relay.label;
        if (b.entity === relay.key) opt.selected = true;
        relaySelect.appendChild(opt);
      });
    }
    relaySelect.addEventListener("change", function () {
      b.entity = this.value;
      helpers.saveField("entity", b.entity);
    });
    rf.appendChild(relaySelect);
    panel.appendChild(rf);

    var mf = document.createElement("div");
    mf.className = "sp-field";
    mf.appendChild(helpers.fieldLabel("Mode", helpers.idPrefix + "internal-mode"));
    var modeSeg = document.createElement("div");
    modeSeg.className = "sp-segment";
    var switchBtn = document.createElement("button");
    switchBtn.type = "button";
    switchBtn.textContent = "Switch";
    var pushBtn = document.createElement("button");
    pushBtn.type = "button";
    pushBtn.textContent = "Push Button";
    modeSeg.appendChild(switchBtn);
    modeSeg.appendChild(pushBtn);
    mf.appendChild(modeSeg);
    panel.appendChild(mf);

    function makeLabeledIconPicker(label, inputSuffix, pickerSuffix, value, onSelect) {
      var section = document.createElement("div");
      section.className = "sp-field";
      section.appendChild(helpers.fieldLabel(label, helpers.idPrefix + inputSuffix));
      var picker = document.createElement("div");
      picker.className = "sp-icon-picker";
      picker.id = helpers.idPrefix + pickerSuffix;
      picker.innerHTML =
        '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(value) + '"></span>' +
        '<input class="sp-icon-picker-input" id="' + helpers.idPrefix + inputSuffix + '" type="text" ' +
        'placeholder="Search icons\u2026" value="' + escAttr(value) + '" autocomplete="off">' +
        '<div class="sp-icon-dropdown"></div>';
      section.appendChild(picker);
      initIconPicker(picker, value, onSelect);
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

    switchBtn.addEventListener("click", function () { setMode("switch"); });
    pushBtn.addEventListener("click", function () { setMode("push"); });
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
