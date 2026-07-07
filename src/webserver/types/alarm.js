// Alarm cards: one-tap alarm_control_panel actions.
var ALARM_CONTROL_PANEL_VALUE = "control_panel";

function alarmControlPanelValue() {
  return alarmBehaviorSpec().controlPanelValue || ALARM_CONTROL_PANEL_VALUE;
}

function alarmUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Security" || icon === "Shield Home" || icon === "Alarm";
}

function alarmCardTypeOptions() {
  var options = [
    { value: alarmControlPanelValue(), label: "Combined Control" },
  ];
  var actions = alarmActionSpecs();
  for (var i = 0; i < actions.length; i++) options.push(actions[i]);
  return options;
}

function alarmCardTypeOptionsForSettings() {
  return alarmCardTypeOptions();
}

function alarmLabelIsGenerated(label) {
  if (!label) return true;
  var actions = alarmActionSpecs();
  for (var i = 0; i < actions.length; i++) {
    if (label === actions[i].label) return true;
  }
  return false;
}

function alarmIconIsGenerated(icon) {
  if (!icon || icon === "Auto" || alarmUsesDefaultIcon(icon)) return true;
  var actions = alarmActionSpecs();
  for (var i = 0; i < actions.length; i++) {
    if (alarmActionIconIsGenerated(actions[i].value, icon)) return true;
  }
  return false;
}

function setAlarmCardType(b, value, helpers) {
  var info = alarmActionInfo(value);
  var wasAlarmAction = b.type === "alarm_action";

  if (value === alarmControlPanelValue() || !info) {
    var shouldUseControlLabel = wasAlarmAction && alarmLabelIsGenerated(b.label);
    var shouldUseControlIcon = alarmIconIsGenerated(b.icon);
    b.type = "alarm";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (shouldUseControlLabel) b.label = "";
    if (shouldUseControlIcon) b.icon = "Security";
    b.options = normalizeAlarmOptions(b.options);

    helpers.saveField("type", b.type);
    helpers.saveField("sensor", "");
    helpers.saveField("unit", "");
    helpers.saveField("precision", "");
    helpers.saveField("icon_on", "Auto");
    helpers.saveField("label", b.label || "");
    helpers.saveField("icon", b.icon || "Security");
    helpers.saveField("options", b.options || "");
    renderButtonSettings();
    return;
  }

  info = info || alarmActionSpecs()[0];
  var oldInfo = alarmActionInfo(b.sensor);
  var shouldUseGeneratedLabel = !wasAlarmAction || alarmLabelIsGenerated(b.label);
  var shouldUseGeneratedIcon = !wasAlarmAction || alarmIconIsGenerated(b.icon) ||
    (oldInfo && alarmActionIconIsGenerated(oldInfo.value, b.icon));

  b.type = "alarm_action";
  b.sensor = info.value;
  b.unit = "";
  b.precision = "";
  b.icon_on = "Auto";
  if (shouldUseGeneratedLabel) b.label = info.label;
  if (shouldUseGeneratedIcon) b.icon = info.icon;
  b.options = normalizeAlarmOptions(b.options);

  helpers.saveField("type", b.type);
  helpers.saveField("sensor", b.sensor || "");
  helpers.saveField("unit", "");
  helpers.saveField("precision", "");
  helpers.saveField("icon_on", "Auto");
  helpers.saveField("label", b.label || "");
  helpers.saveField("icon", b.icon || "Auto");
  helpers.saveField("options", b.options || "");
  renderButtonSettings();
}

var ALARM_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "alarm-card-type",
    options: alarmCardTypeOptionsForSettings,
    value: function (b) {
      return b.type === "alarm"
        ? alarmControlPanelValue()
        : (alarmActionInfo(b.sensor) || alarmActionSpecs()[0]).value;
    },
  },
  entity: {
    label: "Alarm Entity",
    placeholder: "e.g. alarm_control_panel.house",
    domains: function (b) { return cardContractDomains(b && b.type === "alarm_action" ? "alarm_action" : "alarm"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add an alarm_control_panel entity before saving.",
  },
  labelDisplay: {
    label: "Label Display",
    options: [
      ["name", "Name"],
      ["status", "Status"],
    ],
  },
  iconDisplay: {
    label: "Icon Display",
    options: [
      ["static", "Static"],
      ["status", "Status"],
    ],
  },
};

function renderAlarmCardTypeField(panel, b, helpers) {
  helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, ALARM_CARD_METADATA, {
    mode: Object.assign({}, ALARM_CARD_METADATA.mode, {
      options: alarmCardTypeOptionsForSettings(helpers.isSub),
      onChange: function () {
        setAlarmCardType(b, this.value, helpers);
      },
    }),
  }));
}

function renderAlarmVisibleActionsField(panel, b, helpers) {
  var actions = alarmActionSpecs();
  if (!actions.length) return null;
  var field = document.createElement("div");
  field.className = "sp-field";
  field.appendChild(helpers.fieldLabel("Visible Actions", helpers.idPrefix + "alarm-visible-actions"));
  var inputs = [];

  function selectedActions() {
    var selected = [];
    for (var i = 0; i < inputs.length; i++) {
      if (inputs[i].input.checked) selected.push(inputs[i].value);
    }
    return selected;
  }

  function syncInputs(values) {
    values = values || alarmVisibleActions(b);
    var selectedCount = values.length;
    for (var i = 0; i < inputs.length; i++) {
      inputs[i].input.checked = values.indexOf(inputs[i].value) >= 0;
      inputs[i].input.disabled = !inputs[i].input.checked && selectedCount >= alarmMaxVisibleActions();
    }
  }

  var visible = alarmVisibleActions(b);
  for (var i = 0; i < actions.length; i++) {
    var action = actions[i];
    var row = helpers.toggleRow(
      action.label,
      helpers.idPrefix + "alarm-visible-action-" + action.value,
      visible.indexOf(action.value) >= 0
    );
    field.appendChild(row.row);
    inputs.push({ value: action.value, input: row.input });
    row.input.addEventListener("change", function () {
      var selected = selectedActions();
      setAlarmVisibleActions(b, selected);
      helpers.saveField("options", b.options);
      syncInputs(alarmVisibleActions(b));
      scheduleRender();
    });
  }
  syncInputs(visible);
  panel.appendChild(field);
  return field;
}

registerButtonType("alarm", {
  label: function () { return cardContractCardLabel("alarm"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("alarm"); },
  pickerKey: function () { return cardContractPickerKey("alarm"); },
  hidden: function () { return cardContractHidden("alarm"); },
  hideLabel: true,
  labelPlaceholder: "e.g. House Alarm",
  defaultConfig: function () { return cardContractDefaultConfig("alarm"); },
  cardMetadata: ALARM_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Security";
    b.icon_on = "Auto";
    b.options = "";
  },
  renderSettingsBeforeLabel: function (panel, b, slot, helpers) {
    renderAlarmCardTypeField(panel, b, helpers);
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Security";
    var normalizedOptions = normalizeAlarmOptions(b.options);
    if (b.options !== normalizedOptions) {
      b.options = normalizedOptions;
      helpers.saveField("options", normalizedOptions);
    }

    helpers.renderCardEntityField(panel, b, helpers, {
      entity: Object.assign({}, ALARM_CARD_METADATA.entity, {
        idSuffix: "alarm-entity",
      }),
    });

    var cardSettingsDisclosure = helpers.disclosureSection(
      "Card Settings",
      helpers.idPrefix + "alarm-card-settings",
      false
    );
    var cardSettings = cardSettingsDisclosure.section;
    var modalSettingsDisclosure = helpers.disclosureSection(
      "Modal Settings",
      helpers.idPrefix + "alarm-modal-settings",
      false
    );
    var modalSettings = modalSettingsDisclosure.section;

    var labelHost = condField();
    helpers.renderCardTextField(labelHost, b, helpers, {
      label: "Label",
      idSuffix: "alarm-label",
      field: "label",
      placeholder: "e.g. House Alarm",
      rerender: true,
    });

    function setLabelVisible(value) {
      labelHost.classList.toggle("sp-visible", value === "name");
    }

    helpers.renderCardSegmentControl(cardSettings, b, helpers, {
      segment: Object.assign({}, ALARM_CARD_METADATA.labelDisplay, {
        value: function () { return alarmLabelDisplayMode(b); },
        onSelect: function (button, cardHelpers, value) {
          setAlarmLabelDisplayMode(button, value);
          cardHelpers.saveField("options", button.options);
          setLabelVisible(value);
          scheduleRender();
        },
      }),
    });
    setLabelVisible(alarmLabelDisplayMode(b));
    cardSettings.appendChild(labelHost);

    var iconHost = condField();
    helpers.renderCardIconPicker(iconHost, b, helpers, {
      pickerIdSuffix: "alarm-icon-picker",
      idSuffix: "alarm-icon",
      field: "icon",
      fallback: "Security",
      label: "Icon",
    });

    function setIconVisible(value) {
      iconHost.classList.toggle("sp-visible", value === "static");
    }

    helpers.renderCardSegmentControl(cardSettings, b, helpers, {
      segment: Object.assign({}, ALARM_CARD_METADATA.iconDisplay, {
        value: function () { return alarmIconDisplayMode(b); },
        onSelect: function (button, cardHelpers, value) {
          setAlarmIconDisplayMode(button, value);
          cardHelpers.saveField("options", button.options);
          setIconVisible(value);
          scheduleRender();
        },
      }),
    });
    setIconVisible(alarmIconDisplayMode(b));
    cardSettings.appendChild(iconHost);
    panel.appendChild(cardSettingsDisclosure.panel);

    renderAlarmVisibleActionsField(modalSettings, b, helpers);

    function savePinOptions() {
      setAlarmPinRequired(b, "arm", armPinToggle.input.checked);
      setAlarmPinRequired(b, "disarm", disarmPinToggle.input.checked);
      helpers.saveField("options", b.options);
    }

    var pinSettingsDisclosure = helpers.disclosureSection(
      "PIN Settings",
      helpers.idPrefix + "alarm-pin-settings",
      false
    );
    var pinSettings = pinSettingsDisclosure.section;

    var armPinToggle = helpers.renderCardOptionToggle(pinSettings, b, helpers, {
      label: "PIN required for arming",
      idSuffix: "alarm-pin-arm",
      checked: function () { return alarmPinRequired(b, "arm"); },
      onChange: savePinOptions,
    });
    var disarmPinToggle = helpers.renderCardOptionToggle(pinSettings, b, helpers, {
      label: "PIN required for disarming",
      idSuffix: "alarm-pin-disarm",
      checked: function () { return alarmPinRequired(b, "disarm"); },
      onChange: savePinOptions,
    });
    modalSettings.appendChild(pinSettingsDisclosure.panel);
    panel.appendChild(modalSettingsDisclosure.panel);
  },
  renderPreview: function (b, helpers) {
    var label = (b.label && b.label.trim()) || (b.entity && b.entity.trim()) || "Alarm";
    if (alarmLabelDisplayMode(b) === "status") label = "Disarmed";
    var iconName = iconSlug(b.icon && b.icon !== "Auto" ? b.icon : "Security");
    if (alarmIconDisplayMode(b) === "status") iconName = iconSlug("Shield Off");
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml: '<span class="sp-btn-label">' + helpers.escHtml(label) + '</span>',
    };
  },
});

registerButtonType("alarm_action", {
  label: function () { return cardContractCardLabel("alarm_action"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("alarm_action"); },
  labelPlaceholder: "e.g. Arm Away",
  pickerKey: function () { return cardContractPickerKey("alarm_action"); },
  hidden: function () { return cardContractHidden("alarm_action"); },
  defaultConfig: function () { return cardContractDefaultConfig("alarm_action"); },
  cardMetadata: ALARM_CARD_METADATA,
  isAvailable: function () { return false; },
  onSelect: function (b) {
    var info = alarmActionSpecs()[0];
    b.entity = "";
    b.label = info.label;
    b.sensor = info.value;
    b.unit = "";
    b.icon = info.icon;
    b.icon_on = "Auto";
    b.precision = "";
    b.options = "";
  },
  renderSettingsBeforeLabel: function (panel, b, slot, helpers) {
    b.sensor = alarmActionInfo(b.sensor) ? b.sensor : "away";
    renderAlarmCardTypeField(panel, b, helpers);
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.sensor = alarmActionInfo(b.sensor) ? b.sensor : "away";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = normalizeAlarmOptions(b.options);

    helpers.renderCardEntityField(panel, b, helpers, {
      entity: Object.assign({}, ALARM_CARD_METADATA.entity, {
        idSuffix: "alarm-action-entity",
      }),
    });

    helpers.renderCardIconPicker(panel, b, helpers, {
      pickerIdSuffix: "alarm-action-icon-picker",
      idSuffix: "alarm-action-icon",
      field: "icon",
      fallback: function () { return alarmActionInfo(b.sensor).icon; },
      label: "Icon",
    });

    var pinMode = b.sensor === "disarm" ? "disarm" : "arm";
    helpers.renderCardOptionToggle(panel, b, helpers, {
      label: "PIN required",
      idSuffix: "alarm-action-pin",
      checked: function () { return alarmPinRequired(b, pinMode); },
      onChange: function (button, cardHelpers, checked) {
        setAlarmPinRequired(button, pinMode, checked);
        cardHelpers.saveField("options", button.options);
      },
    });
  },
  renderPreview: function (b, helpers) {
    var info = alarmActionInfo(b.sensor) || alarmActionSpecs()[0];
    var label = b.label || info.label;
    var iconName = iconSlug(b.icon || info.icon);
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml: '<span class="sp-btn-label">' + helpers.escHtml(label) + '</span>',
    };
  },
});
