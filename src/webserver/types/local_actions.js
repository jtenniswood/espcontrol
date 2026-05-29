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
    // ── Action picker section (async) ───────────────────────────────
    // pickerSection IS the sp-field container — class toggled per state so
    // margin-bottom is always present and the icon picker below it spaces correctly.
    var pickerSection = document.createElement("div");
    pickerSection.className = "sp-field";
    panel.appendChild(pickerSection);

    // ── Icon picker ──────────────────────────────────────────────────
    panel.appendChild(helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Gesture Tap", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }
    ));

    // ── Picker builder (dropdown on success) ─────────────────────────
    function buildDropdown(actions) {
      pickerSection.innerHTML = "";
      pickerSection.className = "sp-field";
      pickerSection.appendChild(helpers.fieldLabel("Action", helpers.idPrefix + "action-sel"));

      var sel = document.createElement("select");
      sel.className = "sp-select";
      sel.id = helpers.idPrefix + "action-sel";

      var placeholder = document.createElement("option");
      placeholder.value = "";
      placeholder.textContent = "Choose an action…";
      sel.appendChild(placeholder);

      actions.forEach(function (a) {
        var opt = document.createElement("option");
        opt.value = a.key;
        opt.textContent = a.label ? a.label + " (" + a.key + ")" : a.key;
        if (a.key === b.entity) opt.selected = true;
        sel.appendChild(opt);
      });

      // Preserve current key even if it isn’t in the list
      if (b.entity && !actions.some(function (a) { return a.key === b.entity; })) {
        var curOpt = document.createElement("option");
        curOpt.value = b.entity;
        curOpt.textContent = b.entity + " (current)";
        curOpt.selected = true;
        sel.appendChild(curOpt);
      }

      sel.addEventListener("change", function () {
        var key = this.value;
        if (!key) return;
        b.entity = key;
        helpers.saveField("entity", key);
        var action = actions.find(function (a) { return a.key === key; });
        if (action && action.label && !b.label) {
          b.label = action.label;
          helpers.saveField("label", action.label);
          var labelInp = document.getElementById(helpers.idPrefix + "label");
          if (labelInp) labelInp.value = action.label;
        }
      });

      pickerSection.appendChild(sel);
    }

    // ── No-actions state ─────────────────────────────────────────────
    function buildEmpty() {
      pickerSection.innerHTML = "";
      pickerSection.className = "";
      var banner = document.createElement("div");
      banner.className = "sp-banner sp-error";
      banner.textContent =
        "No local actions are registered on this device. " +
        "Add register_local_action() calls to your device’s on_boot lambda.";
      pickerSection.appendChild(banner);
    }

    // ── Unreachable fallback ─────────────────────────────────────────
    function buildFallback() {
      pickerSection.innerHTML = "";
      pickerSection.className = "";
      var banner = document.createElement("div");
      banner.className = "sp-banner sp-error";
      banner.textContent = "Could not reach device. Enter the action key manually.";
      pickerSection.appendChild(banner);

      var kf = document.createElement("div");
      kf.className = "sp-field";
      kf.appendChild(helpers.fieldLabel("Action Key", helpers.idPrefix + "local-key"));
      var keyInp = helpers.textInput(helpers.idPrefix + "local-key", b.entity, "e.g. zoom_mute");
      kf.appendChild(keyInp);
      pickerSection.appendChild(kf);
      helpers.bindField(keyInp, "entity", true);
      helpers.requireField(keyInp, "Add an action key before saving.");
    }

    // ── Loading state then fetch ─────────────────────────────────────
    pickerSection.textContent = "Loading actions…";

    fetch("/local_actions")
      .then(function (resp) {
        if (!resp.ok) throw new Error("HTTP " + resp.status);
        return resp.json();
      })
      .then(function (data) {
        if (!data.length) {
          buildEmpty();
        } else {
          buildDropdown(data);
        }
      })
      .catch(function () {
        buildFallback();
      });
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
