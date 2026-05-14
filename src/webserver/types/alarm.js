// Alarm card: state-aware alarm_control_panel card with generated action page.
function alarmUsesDefaultIcon(icon) {
  return !icon || icon === "Auto" || icon === "Security" || icon === "Alarm";
}

registerButtonType("alarm", {
  label: "Alarm",
  allowInSubpage: false,
  hideLabel: true,
  labelPlaceholder: "e.g. House Alarm",
  experimental: "developer",
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

    var entityField = helpers.entityField(
      "Alarm Entity",
      helpers.idPrefix + "alarm-entity",
      b.entity,
      "e.g. alarm_control_panel.house",
      ["alarm_control_panel"],
      "entity",
      true,
      "Add an alarm_control_panel entity before saving."
    );
    panel.appendChild(entityField.field);

    panel.appendChild(helpers.textField(
      "Label", helpers.idPrefix + "alarm-label", b.label, "e.g. House Alarm", "label", true).field);

    panel.appendChild(helpers.iconPickerField(
      helpers.idPrefix + "alarm-icon-picker", helpers.idPrefix + "alarm-icon",
      b.icon || "Security", function (opt) {
        b.icon = opt || "Security";
        helpers.saveField("icon", b.icon);
      }, "Icon"
    ));

    function savePinOptions() {
      setAlarmPinRequired(b, "arm", armPinToggle.input.checked);
      setAlarmPinRequired(b, "disarm", disarmPinToggle.input.checked);
      helpers.saveField("options", b.options);
    }

    var armPinToggle = helpers.toggleRow(
      "PIN required for arming",
      helpers.idPrefix + "alarm-pin-arm",
      alarmPinRequired(b, "arm")
    );
    var disarmPinToggle = helpers.toggleRow(
      "PIN required for disarming",
      helpers.idPrefix + "alarm-pin-disarm",
      alarmPinRequired(b, "disarm")
    );
    panel.appendChild(armPinToggle.row);
    panel.appendChild(disarmPinToggle.row);
    armPinToggle.input.addEventListener("change", savePinOptions);
    disarmPinToggle.input.addEventListener("change", savePinOptions);

    var visible = alarmVisibleActions(b);
    var actionStack = document.createElement("div");
    actionStack.className = "sp-field-stack";
    var actionInputs = {};
    ALARM_ACTIONS.forEach(function (action) {
      var row = helpers.toggleRow(
        action.label,
        helpers.idPrefix + "alarm-action-" + action.value,
        visible.indexOf(action.value) >= 0
      );
      actionInputs[action.value] = row.input;
      actionStack.appendChild(row.row);
      row.input.addEventListener("change", saveActionOptions);
    });
    panel.appendChild(helpers.fieldWithControl("Visible Actions", null, actionStack));

    function saveActionOptions() {
      var selected = [];
      ALARM_ACTIONS.forEach(function (action) {
        if (actionInputs[action.value] && actionInputs[action.value].checked) {
          selected.push(action.value);
        }
      });
      if (!selected.length) {
        selected = alarmActionValues();
        ALARM_ACTIONS.forEach(function (action) {
          if (actionInputs[action.value]) actionInputs[action.value].checked = true;
        });
      }
      setAlarmVisibleActions(b, selected);
      helpers.saveField("options", b.options);
      scheduleRender();
    }
  },
  renderPreview: function (b, helpers) {
    var label = (b.label && b.label.trim()) || (b.entity && b.entity.trim()) || "Alarm";
    var iconName = alarmUsesDefaultIcon(b.icon) ? "security" : iconSlug(b.icon);
    return {
      iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml:
        '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
        helpers.escHtml(label) + '</span><span class="sp-alarm-badge mdi mdi-security"></span></span>',
    };
  },
});
