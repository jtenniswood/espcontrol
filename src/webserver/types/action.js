// Action card: one-tap Home Assistant shortcuts for scenes, scripts, buttons, and helpers.
var ACTION_CARD_ACTIONS = [
  { value: "scene.turn_on", label: "Run Scene", placeholder: "e.g. scene.movie_mode", icon: "movie-open", domains: ["scene"] },
  { value: "script.turn_on", label: "Run Script", placeholder: "e.g. script.goodnight", icon: "script-text-play", domains: ["script"] },
  { value: "automation.trigger", label: "Trigger Automation", placeholder: "e.g. automation.goodnight", icon: "home-automation", domains: ["automation"] },
  { value: "button.press", label: "Press Button", placeholder: "e.g. button.restart_router", icon: "gesture-tap-button", domains: ["button"] },
  { value: "input_button.press", label: "Press Input Button", placeholder: "e.g. input_button.doorbell", icon: "gesture-tap-button", domains: ["input_button"] },
  { value: "input_boolean.toggle", label: "Toggle Helper", placeholder: "e.g. input_boolean.guest_mode", icon: "toggle-switch-variant", domains: ["input_boolean"] },
  { value: "input_number.set_value", label: "Set Number Helper", placeholder: "e.g. input_number.target_level", icon: "counter", domains: ["input_number"] },
  { value: "input_select.select_option", label: "Select Option Helper", placeholder: "e.g. input_select.house_mode", icon: "form-dropdown", domains: ["input_select"] },
];

function actionCardInfo(value) {
  for (var i = 0; i < ACTION_CARD_ACTIONS.length; i++) {
    if (ACTION_CARD_ACTIONS[i].value === value) return ACTION_CARD_ACTIONS[i];
  }
  return null;
}

function normalizeActionCardConfig(b) {
  if (!b.sensor) b.sensor = "scene.turn_on";
  if (!actionCardInfo(b.sensor)) b.sensor = "scene.turn_on";
  b.icon_on = "Auto";
  b.precision = "";
}

var ACTION_CARD_STATE_ENTITY_OPTION = "state_entity";
var ACTION_CARD_STATE_UNIT_OPTION = "state_unit";
var ACTION_CARD_STATE_PRECISION_OPTION = "state_precision";

function actionCardStateEntity(b) {
  return configOptionValue(b && b.options, ACTION_CARD_STATE_ENTITY_OPTION);
}

function actionCardStateUnit(b) {
  return configOptionValue(b && b.options, ACTION_CARD_STATE_UNIT_OPTION);
}

function actionCardStatePrecision(b) {
  var value = configOptionValue(b && b.options, ACTION_CARD_STATE_PRECISION_OPTION);
  if (value === "text") return "text";
  return value === "1" || value === "2" ? value : "0";
}

function actionCardStateDisplayMode(b) {
  var rawPrecision = configOptionValue(b && b.options, ACTION_CARD_STATE_PRECISION_OPTION);
  if (rawPrecision === "text") return "text";
  if (rawPrecision === "0" || rawPrecision === "1" || rawPrecision === "2" || actionCardStateUnit(b)) {
    return "numeric";
  }
  return actionCardStateEntity(b) ? "text" : "numeric";
}

function setActionCardStateOptions(b, entity, mode, unit, precision) {
  if (!b) return "";
  var options = b.options;
  entity = String(entity || "").trim();
  if (!entity) {
    options = setConfigOptionValue(options, ACTION_CARD_STATE_ENTITY_OPTION, "");
    options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, "");
    options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, "");
    b.options = options;
    return b.options;
  }
  options = setConfigOptionValue(options, ACTION_CARD_STATE_ENTITY_OPTION, entity);
  if (mode === "text") {
    options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, "");
    options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, "text");
  } else {
    options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, unit || "");
    options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, precision || "0");
  }
  b.options = options;
  return b.options;
}

function actionCardNeedsExtraValue(value) {
  return value === "input_number.set_value" || value === "input_select.select_option";
}

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
    b.options = "";
  },
  renderSettingsBeforeLabel: function (panel, b, slot, helpers) {
    normalizeActionCardConfig(b);

    var actionField = helpers.selectField("Action", helpers.idPrefix + "action", ACTION_CARD_ACTIONS, b.sensor);
    var actionSelect = actionField.select;
    panel.appendChild(actionField.field);

    actionSelect.addEventListener("change", function () {
      b.sensor = this.value;
      helpers.saveField("sensor", b.sensor);
      if (!actionCardNeedsExtraValue(b.sensor)) {
        b.unit = "";
        helpers.saveField("unit", "");
      }
      b.icon_on = "Auto";
      b.precision = "";
      helpers.saveField("icon_on", "Auto");
      helpers.saveField("precision", "");
      renderButtonSettings();
    });
  },
  renderSettings: function (panel, b, slot, helpers) {
    normalizeActionCardConfig(b);

    var info = actionCardInfo(b.sensor) || ACTION_CARD_ACTIONS[0];
    var entityField = helpers.entityField(
      "Action Entity",
      helpers.idPrefix + "entity",
      b.entity,
      info.placeholder,
      info.domains,
      "entity",
      true,
      "Add an entity before saving."
    );
    var entityInp = entityField.input;
    panel.appendChild(entityField.field);

    var valueInput = helpers.textInput(
      helpers.idPrefix + "action-value",
      b.unit,
      "e.g. 50"
    );
    var valueLabel = helpers.fieldLabel("Value", helpers.idPrefix + "action-value");
    var valueField = document.createElement("div");
    valueField.className = "sp-field";
    valueField.appendChild(valueLabel);
    valueField.appendChild(valueInput);
    panel.appendChild(valueField);
    helpers.bindField(valueInput, "unit", true);

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
      b.icon || "Flash", function (opt) {
        b.icon = opt;
        helpers.saveField("icon", opt);
      }
    ));

    valueField.style.display = actionCardNeedsExtraValue(b.sensor) ? "" : "none";
    valueLabel.textContent = b.sensor === "input_select.select_option" ? "Option" : "Value";
    valueInput.placeholder = b.sensor === "input_select.select_option" ? "e.g. Away" : "e.g. 50";
    entityInp._entityDomains = info.domains || [];
    refreshEntityDatalist(entityInp);

    var stateEntity = actionCardStateEntity(b);
    var stateMode = actionCardStateDisplayMode(b);
    var stateUnit = actionCardStateUnit(b);
    var statePrecision = actionCardStatePrecision(b);

    var mode = helpers.segmentControl([
      ["numeric", "Numeric"],
      ["text", "Text"],
    ], stateMode, function (value) { setStateMode(value, true); });
    var numericBtn = mode.buttons.numeric;
    var textBtn = mode.buttons.text;
    panel.appendChild(helpers.fieldWithControl("Type", null, mode.segment));

    var stateEntityField = helpers.entityField(
      "Sensor Entity",
      helpers.idPrefix + "action-state-entity",
      stateEntity,
      "e.g. sensor.printer_percent_complete",
      ["sensor", "binary_sensor", "text_sensor"]
    );
    var stateEntityInp = stateEntityField.input;
    panel.appendChild(stateEntityField.field);

    var numericSection = condField();
    var stateUnitField = helpers.textField(
      "Unit", helpers.idPrefix + "action-state-unit", stateUnit, "e.g. %");
    var stateUnitInp = stateUnitField.input;
    stateUnitInp.className = "sp-input";
    numericSection.appendChild(stateUnitField.field);

    var statePrecisionField = helpers.precisionField(
      helpers.idPrefix + "action-state-precision",
      stateMode === "numeric" ? statePrecision : "0",
      function () {
        statePrecision = this.value || "0";
        saveStateOptions();
      }
    );
    var statePrecisionSelect = statePrecisionField.select;
    numericSection.appendChild(statePrecisionField.field);
    panel.appendChild(numericSection);

    function saveStateOptions() {
      stateEntity = stateEntityInp.value;
      stateUnit = stateUnitInp.value;
      helpers.saveField("options", setActionCardStateOptions(
        b, stateEntity, stateMode, stateUnit, statePrecision));
    }

    function setStateMode(modeValue, persist) {
      stateMode = modeValue === "text" ? "text" : "numeric";
      numericBtn.classList.toggle("active", stateMode === "numeric");
      textBtn.classList.toggle("active", stateMode === "text");
      numericSection.classList.toggle("sp-visible", stateMode === "numeric");
      if (!persist) return;
      if (stateMode === "text") {
        stateUnit = "";
        stateUnitInp.value = "";
        statePrecision = "0";
        statePrecisionSelect.value = "0";
      }
      saveStateOptions();
    }

    setStateMode(stateMode, false);

    stateEntityInp.addEventListener("input", saveStateOptions);
    stateEntityInp.addEventListener("change", saveStateOptions);
    stateEntityInp.addEventListener("blur", saveStateOptions);
    stateEntityInp.addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        saveStateOptions();
        this.blur();
      }
    });
    stateUnitInp.addEventListener("input", saveStateOptions);
    stateUnitInp.addEventListener("change", saveStateOptions);
    stateUnitInp.addEventListener("blur", saveStateOptions);
    stateUnitInp.addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        saveStateOptions();
        this.blur();
      }
    });
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.entity || "Action";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "flash";
    var stateBadge = actionCardStateEntity(b)
      ? '<span class="sp-sensor-badge mdi mdi-' +
        (actionCardStateDisplayMode(b) === "text" ? "format-text" : "gauge") +
        '"></span>'
      : "";
    return {
      iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
        '<span class="sp-type-badge mdi mdi-flash"></span></span>',
    };
  },
});
