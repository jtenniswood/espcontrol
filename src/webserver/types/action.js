// Action card: one-tap Home Assistant shortcuts for scenes, scripts, buttons, and helpers.
var ACTION_CARD_ACTIONS = [
  { value: "scene.turn_on", label: "Run Scene", placeholder: "e.g. scene.movie_mode", icon: "movie-open", domains: ["scene"] },
  { value: "script.turn_on", label: "Run Script", placeholder: "e.g. script.goodnight", icon: "script-text-play", domains: ["script"] },
  { value: "automation.trigger", label: "Trigger Automation", placeholder: "e.g. automation.goodnight", icon: "home-automation", domains: ["automation"] },
  { value: "button.press", label: "Press Button", placeholder: "e.g. button.restart_router", icon: "gesture-tap-button", domains: ["button"] },
  { value: "input_button.press", label: "Press Input Button", placeholder: "e.g. input_button.doorbell", icon: "gesture-tap-button", domains: ["input_button"] },
  { value: "input_boolean.toggle", label: "Toggle Helper", placeholder: "e.g. input_boolean.guest_mode", icon: "toggle-switch-variant", domains: ["input_boolean"] },
  { value: "input_number.set_value", label: "Set Number Helper", placeholder: "e.g. input_number.target_level", icon: "counter", domains: ["input_number"] },
  { value: "input_select.select_option", label: "Option Select", placeholder: "e.g. select.wled_preset", icon: "form-dropdown", domains: ["select", "input_select"] },
];
var ACTION_CARD_OPTION_SELECT_ACTION = "input_select.select_option";

function actionCardInfo(value) {
  for (var i = 0; i < ACTION_CARD_ACTIONS.length; i++) {
    if (ACTION_CARD_ACTIONS[i].value === value) return ACTION_CARD_ACTIONS[i];
  }
  return null;
}

function actionCardIsOptionSelect(b) {
  var value = typeof b === "string" ? b : b && b.sensor;
  return value === ACTION_CARD_OPTION_SELECT_ACTION || value === "select.select_option";
}

function normalizeActionCardConfig(b) {
  if (b && b.sensor === "select.select_option") b.sensor = ACTION_CARD_OPTION_SELECT_ACTION;
  if (!b.sensor) b.sensor = "scene.turn_on";
  if (!actionCardInfo(b.sensor)) b.sensor = "scene.turn_on";
  b.precision = "";
  if (actionCardStateDisplayMode(b) !== "icon") b.icon_on = "Auto";
  if (actionCardIsOptionSelect(b)) {
    b.unit = "";
    b.options = "";
    if (!b.icon || b.icon === "Auto" || b.icon === "Chevron Down") b.icon = "Flash";
  } else {
    b.options = normalizeActionOptions(b.options, b.sensor);
  }
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
  if (value === "icon") return "icon";
  if (value === "text") return "text";
  return value === "1" || value === "2" ? value : "0";
}

function actionCardStateDisplayMode(b) {
  var rawPrecision = configOptionValue(b && b.options, ACTION_CARD_STATE_PRECISION_OPTION);
  if (rawPrecision === "icon") return "icon";
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
  if (mode === "icon") {
    options = setConfigOptionValue(options, ACTION_CARD_STATE_UNIT_OPTION, "");
    options = setConfigOptionValue(options, ACTION_CARD_STATE_PRECISION_OPTION, "icon");
  } else if (mode === "text") {
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
  return value === "input_number.set_value";
}

var ACTION_CARD_METADATA = {
  mode: {
    label: "Action",
    idSuffix: "action",
    options: ACTION_CARD_ACTIONS,
    value: function (b) {
      return b.sensor || "scene.turn_on";
    },
  },
  entity: {
    idSuffix: "entity",
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add an entity before saving.",
  },
  stateMode: {
    label: "Type",
    options: [
      ["icon", "Icon"],
      ["numeric", "Numeric"],
      ["text", "Text"],
    ],
  },
  largeNumbers: {
    label: "Large State Numbers",
    idSuffix: "large-state-numbers",
    supported: function (b) {
      return !actionCardIsOptionSelect(b) && actionCardStateDisplayMode(b) === "numeric";
    },
  },
  stateUnitField: {
    label: "Unit",
    idSuffix: "action-state-unit",
    placeholder: "e.g. %",
    bindName: null,
  },
  confirmationToggle: {
    label: "Confirmation Required",
    idSuffix: "script-confirm-toggle",
    checked: function (b) { return actionScriptConfirmationEnabled(b); },
  },
  confirmationMessage: {
    label: "Message",
    idSuffix: "script-confirm-message",
    placeholder: "Run this script?",
    bindName: null,
    value: function (b) { return actionScriptConfirmationMessage(b); },
  },
  confirmationYes: {
    label: "Confirm Button",
    idSuffix: "script-confirm-yes",
    placeholder: "Yes",
    bindName: null,
    value: function (b) { return actionScriptConfirmationYesText(b); },
  },
  confirmationNo: {
    label: "Cancel Button",
    idSuffix: "script-confirm-no",
    placeholder: "No",
    bindName: null,
    value: function (b) { return actionScriptConfirmationNoText(b); },
  },
  preview: {
    optionBadge: "chevron-down",
    actionBadge: "flash",
  },
};

registerButtonType("action", {
  label: "Action",
  allowInSubpage: true,
  labelPlaceholder: "e.g. Movie Mode",
  cardMetadata: ACTION_CARD_METADATA,
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

    var actionField = helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, ACTION_CARD_METADATA, {
      mode: Object.assign({}, ACTION_CARD_METADATA.mode, {
        onChange: function () {
          b.sensor = this.value;
          helpers.saveField("sensor", b.sensor);
          if (!actionCardNeedsExtraValue(b.sensor)) {
            b.unit = "";
            helpers.saveField("unit", "");
          }
          if (actionCardIsOptionSelect(b)) {
            b.options = "";
            helpers.saveField("options", "");
          } else {
            b.options = normalizeActionOptions(b.options, b.sensor);
            helpers.saveField("options", b.options);
          }
          b.icon_on = "Auto";
          b.precision = "";
          helpers.saveField("icon_on", "Auto");
          helpers.saveField("precision", "");
          renderButtonSettings();
        },
      }),
    }));
    var actionSelect = actionField.select;
    actionSelect.value = b.sensor;
  },
  renderSettings: function (panel, b, slot, helpers) {
    normalizeActionCardConfig(b);

    var info = actionCardInfo(b.sensor) || ACTION_CARD_ACTIONS[0];
    var isOptionSelect = actionCardIsOptionSelect(b);
    var entityField = helpers.renderCardEntityField(panel, b, helpers, {
      entity: Object.assign({}, ACTION_CARD_METADATA.entity, {
        label: isOptionSelect ? "Select Entity" : "Action Entity",
        placeholder: info.placeholder,
        domains: info.domains,
      }),
    });
    var entityInp = entityField.input;

    if (actionCardNeedsExtraValue(b.sensor)) {
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
    }

    if (!isOptionSelect) {
      helpers.renderCardIconPicker(panel, b, helpers, {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        fallback: "Flash",
      });
    }

    entityInp._entityDomains = info.domains || [];
    refreshEntityDatalist(entityInp);
    if (isOptionSelect) return;

    if (actionCardIsScript(b)) {
      var confirmOn = actionScriptConfirmationEnabled(b);
      var confirmToggle = helpers.renderCardOptionToggle(panel, b, helpers, ACTION_CARD_METADATA.confirmationToggle);
      var confirmSection = condField();
      confirmSection.classList.add("sp-action-confirm-section");
      if (confirmOn) confirmSection.classList.add("sp-visible");

      var messageField = helpers.renderCardTextField(confirmSection, b, helpers, ACTION_CARD_METADATA.confirmationMessage);
      var messageInput = messageField.input;
      messageInput.maxLength = 72;

      var yesField = helpers.renderCardTextField(confirmSection, b, helpers, ACTION_CARD_METADATA.confirmationYes);
      var yesInput = yesField.input;
      yesInput.maxLength = 20;

      var noField = helpers.renderCardTextField(confirmSection, b, helpers, ACTION_CARD_METADATA.confirmationNo);
      var noInput = noField.input;
      noInput.maxLength = 20;

      panel.appendChild(confirmSection);

      function saveScriptConfirmationOptions() {
        setActionScriptConfirmationOptions(
          b,
          confirmToggle.input.checked,
          messageInput.value || actionScriptConfirmationDefaultMessage(),
          yesInput.value || SWITCH_CONFIRM_DEFAULT_YES,
          noInput.value || SWITCH_CONFIRM_DEFAULT_NO
        );
        helpers.saveField("options", b.options);
      }

      confirmToggle.input.addEventListener("change", function () {
        confirmSection.classList.toggle("sp-visible", this.checked);
        if (this.checked) {
          if (!messageInput.value) messageInput.value = actionScriptConfirmationDefaultMessage();
          if (!yesInput.value) yesInput.value = SWITCH_CONFIRM_DEFAULT_YES;
          if (!noInput.value) noInput.value = SWITCH_CONFIRM_DEFAULT_NO;
        }
        saveScriptConfirmationOptions();
      });

      [messageInput, yesInput, noInput].forEach(function (input) {
        input.addEventListener("input", saveScriptConfirmationOptions);
        input.addEventListener("change", saveScriptConfirmationOptions);
        input.addEventListener("blur", saveScriptConfirmationOptions);
        input.addEventListener("keydown", function (e) {
          if (e.key === "Enter") {
            saveScriptConfirmationOptions();
            this.blur();
          }
        });
      });
    }

    var stateEntity = actionCardStateEntity(b);
    var stateMode = actionCardStateDisplayMode(b);
    var stateUnit = actionCardStateUnit(b);
    var statePrecision = actionCardStatePrecision(b);

    var mode = helpers.renderCardSegmentControl(panel, b, helpers, {
      segment: Object.assign({}, ACTION_CARD_METADATA.stateMode, {
        value: function () { return stateMode; },
        onSelect: function (button, cardHelpers, value) {
          setStateMode(value, true);
        },
      }),
    });
    var iconBtn = mode.buttons.icon;
    var numericBtn = mode.buttons.numeric;
    var textBtn = mode.buttons.text;

    var stateEntityField = helpers.renderCardEntityField(panel, b, helpers, {
      entity: {
        label: "Sensor Entity",
        idSuffix: "action-state-entity",
        value: function () { return stateEntity; },
        placeholder: "e.g. sensor.printer_percent_complete",
        domains: ["sensor", "binary_sensor", "text_sensor"],
        bindName: null,
        rerender: false,
      },
    });
    var stateEntityInp = stateEntityField.input;

    var iconOnSection = helpers.renderCardIconPicker(panel, b, helpers, {
      pickerIdSuffix: "icon-on-picker",
      idSuffix: "icon-on",
      field: "icon_on",
      fallback: "Auto",
      label: "On Icon",
    });

    var numericSection = condField();
    var stateUnitField = helpers.renderCardTextField(numericSection, b, helpers, Object.assign({}, ACTION_CARD_METADATA.stateUnitField, {
      value: function () { return stateUnit; },
    }));
    var stateUnitInp = stateUnitField.input;

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
    helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, ACTION_CARD_METADATA);
    panel.appendChild(numericSection);

    function saveStateOptions() {
      stateEntity = stateEntityInp.value;
      stateUnit = stateUnitInp.value;
      helpers.saveField("options", setActionCardStateOptions(
        b, stateEntity, stateMode, stateUnit, statePrecision));
    }

    function setStateMode(modeValue, persist) {
      stateMode = modeValue === "icon" || modeValue === "text" ? modeValue : "numeric";
      iconBtn.classList.toggle("active", stateMode === "icon");
      numericBtn.classList.toggle("active", stateMode === "numeric");
      textBtn.classList.toggle("active", stateMode === "text");
      iconOnSection.style.display = stateMode === "icon" ? "" : "none";
      numericSection.classList.toggle("sp-visible", stateMode === "numeric");
      if (!persist) return;
      if (stateMode === "icon" || stateMode === "text") {
        stateUnit = "";
        stateUnitInp.value = "";
        statePrecision = "0";
        statePrecisionSelect.value = "0";
      }
      if (stateMode !== "icon") {
        b.icon_on = "Auto";
        helpers.saveField("icon_on", "Auto");
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
    if (actionCardIsOptionSelect(b)) {
      return {
        iconHtml: cardSensorPreviewHtml(b, helpers, "Option", null),
        labelHtml: cardBadgeLabelHtml(helpers, label, ACTION_CARD_METADATA.preview.optionBadge),
      };
    }
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "flash";
    if (actionCardStateEntity(b) && actionCardStateDisplayMode(b) === "numeric" &&
        cardLargeNumbersActiveForCardSize(b, helpers, ACTION_CARD_METADATA)) {
      return {
        iconHtml: cardSensorPreviewHtml(b, helpers, "42", actionCardStateUnit(b) || ""),
        labelHtml: cardBadgeLabelHtml(helpers, label, ACTION_CARD_METADATA.preview.actionBadge),
      };
    }
    var stateBadge = actionCardStateEntity(b)
      ? '<span class="sp-sensor-badge mdi mdi-' +
        (actionCardStateDisplayMode(b) === "icon" ? "toggle-switch" :
          (actionCardStateDisplayMode(b) === "text" ? "format-text" : "gauge")) +
        '"></span>'
      : "";
    return {
      iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, label, ACTION_CARD_METADATA.preview.actionBadge),
    };
  },
});
