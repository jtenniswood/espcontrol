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
      "Entity",
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
