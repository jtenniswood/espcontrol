// Action card: one-tap Home Assistant shortcuts for scenes, scripts, buttons, and helpers.
registerButtonType("action", {
  label: "Action",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Movie Mode",
  onSelect: function (b) {
    b.entity = "";
    b.sensor = "scene.turn_on";
    b.unit = "";
    b.icon = "Flash";
    b.icon_on = "Auto";
    b.precision = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var actions = [
      { value: "scene.turn_on", label: "Run Scene", placeholder: "e.g. scene.movie_mode", icon: "movie-open" },
      { value: "script.turn_on", label: "Run Script", placeholder: "e.g. script.goodnight", icon: "script-text-play" },
      { value: "automation.trigger", label: "Trigger Automation", placeholder: "e.g. automation.goodnight", icon: "home-automation" },
      { value: "button.press", label: "Press Button", placeholder: "e.g. button.restart_router", icon: "gesture-tap-button" },
      { value: "input_button.press", label: "Press Input Button", placeholder: "e.g. input_button.doorbell", icon: "gesture-tap-button" },
      { value: "lock.open", label: "Open Lock", placeholder: "e.g. lock.front_door", icon: "lock-open" },
      { value: "input_boolean.toggle", label: "Toggle Helper", placeholder: "e.g. input_boolean.guest_mode", icon: "toggle-switch-variant" },
      { value: "input_number.set_value", label: "Set Number Helper", placeholder: "e.g. input_number.target_level", icon: "counter" },
      { value: "input_select.select_option", label: "Select Option Helper", placeholder: "e.g. input_select.house_mode", icon: "form-dropdown" },
    ];

    function actionInfo(value) {
      for (var i = 0; i < actions.length; i++) {
        if (actions[i].value === value) return actions[i];
      }
      return null;
    }

    if (!b.sensor) b.sensor = "scene.turn_on";
    if (!actionInfo(b.sensor)) b.sensor = "scene.turn_on";
    b.icon_on = "Auto";
    b.precision = "";

    var af = document.createElement("div");
    af.className = "sp-field";
    af.appendChild(helpers.fieldLabel("Action", helpers.idPrefix + "action"));
    var actionSelect = document.createElement("select");
    actionSelect.className = "sp-select";
    actionSelect.id = helpers.idPrefix + "action";
    for (var ai = 0; ai < actions.length; ai++) {
      var opt = document.createElement("option");
      opt.value = actions[ai].value;
      opt.textContent = actions[ai].label;
      actionSelect.appendChild(opt);
    }
    actionSelect.value = b.sensor;
    af.appendChild(actionSelect);
    panel.appendChild(af);

    var ef = document.createElement("div");
    ef.className = "sp-field";
    var entityLabel = helpers.fieldLabel("Entity ID", helpers.idPrefix + "entity");
    ef.appendChild(entityLabel);
    var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, actionInfo(b.sensor).placeholder);
    ef.appendChild(entityInp);
    panel.appendChild(ef);
    helpers.bindField(entityInp, "entity", true);
    helpers.requireField(entityInp, "Add an entity before saving.");

    var valueField = document.createElement("div");
    valueField.className = "sp-field";
    var valueLabel = helpers.fieldLabel("Value", helpers.idPrefix + "action-value");
    valueField.appendChild(valueLabel);
    var valueInp = helpers.textInput(helpers.idPrefix + "action-value", b.unit, "e.g. 50");
    valueField.appendChild(valueInp);
    panel.appendChild(valueField);
    helpers.bindField(valueInp, "unit", true);

    panel.appendChild(helpers.makeIconPicker(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Flash", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }
    ));

    function updateForAction(persist) {
      var info = actionInfo(actionSelect.value) || actions[0];
      entityInp.placeholder = info.placeholder;
      var needsValue = actionSelect.value === "input_number.set_value";
      var needsOption = actionSelect.value === "input_select.select_option";
      valueField.style.display = needsValue || needsOption ? "" : "none";
      valueLabel.textContent = needsOption ? "Option" : "Value";
      valueInp.placeholder = needsOption ? "e.g. Away" : "e.g. 50";
      if (!persist) return;
      b.sensor = actionSelect.value;
      helpers.saveField("sensor", b.sensor);
      if (!needsValue && !needsOption) {
        b.unit = "";
        valueInp.value = "";
        helpers.saveField("unit", "");
      }
      b.icon_on = "Auto";
      b.precision = "";
      helpers.saveField("icon_on", "Auto");
      helpers.saveField("precision", "");
    }

    actionSelect.addEventListener("change", function () { updateForAction(true); });
    updateForAction(false);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Action";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "flash";
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-flash"></span></span>',
    };
  },
});
