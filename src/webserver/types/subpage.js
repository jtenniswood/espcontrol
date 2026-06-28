// Navigation folder: tap opens a nested grid screen with its own button layout
var SUBPAGE_CARD_METADATA = {
  kind: {
    label: "Type",
    idSuffix: "subpage-kind",
    options: function () { return subpageKindOptions(); },
  },
  labelField: {
    label: "Label",
    placeholder: "e.g. Lighting",
  },
  icon: {
    field: "icon",
    fallback: "Auto",
    label: "Icon",
  },
  showState: {
    label: "Show State",
    idSuffix: "state-toggle",
    checked: function (b) { return subpageStateDisplayMode(b) !== "off"; },
  },
  stateMode: {
    label: "Type",
    options: [
      ["icon", "Icon"],
      ["numeric", "Numeric"],
      ["text", "Text"],
    ],
  },
  iconStateEntity: {
    label: "State Entity",
    idSuffix: "icon-state-entity",
    placeholder: "e.g. cover.office_blind",
    domains: ["light", "switch", "input_boolean", "binary_sensor", "cover", "lock", "media_player", "fan", "person", "device_tracker"],
    bindName: null,
    value: function (b) {
      return subpageStateDisplayMode(b) === "icon" ? (b.entity || "") : "";
    },
  },
  presetEntity: {
    label: function (b) {
      var defaults = subpagePresetDefaults(subpageKind(b));
      return defaults ? defaults.label + " Entity" : "Entity";
    },
    idSuffix: "preset-state-entity",
    placeholder: function (b) {
      var defaults = subpagePresetDefaults(subpageKind(b));
      return defaults ? defaults.placeholder : "e.g. light.living_room";
    },
    domains: function (b) {
      var defaults = subpagePresetDefaults(subpageKind(b));
      return defaults && defaults.entityDomains ? defaults.entityDomains : [];
    },
    bindName: "entity",
    rerender: true,
    requiredMessage: function (b) {
      var defaults = subpagePresetDefaults(subpageKind(b));
      return "Add a " + (defaults ? defaults.label.toLowerCase() : "status") + " entity before saving.";
    },
  },
  sensorEntity: {
    label: "Sensor Entity",
    idSuffix: "sensor",
    placeholder: "e.g. sensor.open_windows",
    domains: ["sensor", "binary_sensor", "text_sensor"],
    bindName: null,
    value: function (b) {
      return b.sensor && b.sensor !== "indicator" ? b.sensor : "";
    },
  },
  iconOn: {
    field: "icon_on",
    fallback: "Auto",
    label: "On Icon",
  },
  unitField: {
    label: "Unit",
    idSuffix: "unit",
    placeholder: "e.g. %",
    bindName: "unit",
    rerender: false,
  },
  largeNumbers: {
    label: "Large State Numbers",
    idSuffix: "large-state-numbers",
    supported: function (b) {
      return subpageStateDisplayMode(b) === "numeric";
    },
  },
  preview: {
    badge: "chevron-right",
  },
};

registerButtonType("subpage", {
  label: "Subpage",
  allowInSubpage: false,
  hideLabel: true,
  labelPlaceholder: "e.g. Lighting",
  cardMetadata: SUBPAGE_CARD_METADATA,
  onSelect: function (b) {
    b.entity = ""; b.sensor = ""; b.unit = ""; b.icon = "Auto"; b.icon_on = "Auto"; b.options = "";
  },
  renderSettings: function (panel, b, slot, helpers) {
    var kind = subpageKind(b);
    helpers.renderCardModeSelector(panel, b, helpers, {
      mode: Object.assign({}, SUBPAGE_CARD_METADATA.kind, {
        value: function () { return kind; },
        onChange: function (button, cardHelpers) {
          var nextKind = normalizeSubpageKind(this.value);
          button.options = setConfigOptionValue(button.options, SUBPAGE_KIND_OPTION, nextKind);
          applySubpagePresetConfig(button, true);
          button.options = normalizeSubpageOptions(button.options, button.sensor, button.precision);
          cardHelpers.saveField("options", button.options);
          cardHelpers.saveField("label", button.label || "");
          cardHelpers.saveField("icon", button.icon || "Auto");
          cardHelpers.saveField("icon_on", button.icon_on || "Auto");
          cardHelpers.saveField("sensor", button.sensor || "");
          cardHelpers.saveField("unit", button.unit || "");
          cardHelpers.saveField("precision", button.precision || "");
          renderButtonSettings();
        },
      }),
    });

    if (subpagePresetDefaults(kind)) {
      helpers.renderCardTextField(panel, b, helpers, SUBPAGE_CARD_METADATA.labelField);
      helpers.renderCardIconPicker(panel, b, helpers, SUBPAGE_CARD_METADATA.icon);
      helpers.renderCardEntityField(panel, b, helpers, {
        entity: SUBPAGE_CARD_METADATA.presetEntity,
      });
      appendEditSubpageButton(panel, slot);
      return;
    }

    var mode = subpageStateDisplayMode(b);
    var showState = mode !== "off";
    var sensorEntity = b.sensor && b.sensor !== "indicator" ? b.sensor : "";
    var iconStateEntity = mode === "icon" ? (b.entity || "") : "";

    helpers.renderCardTextField(panel, b, helpers, SUBPAGE_CARD_METADATA.labelField);

    var iconSectionMain = helpers.renderCardIconPicker(panel, b, helpers, SUBPAGE_CARD_METADATA.icon);

    var showStateToggle = helpers.renderCardOptionToggle(panel, b, helpers, SUBPAGE_CARD_METADATA.showState);
    var stateCond = condField();
    if (showState) stateCond.classList.add("sp-visible");

    var modeControl = helpers.renderCardSegmentControl(stateCond, b, helpers, Object.assign({}, SUBPAGE_CARD_METADATA.stateMode, {
      value: function () { return mode; },
      onSelect: function (b, helpers, value) { setMode(value, true); },
    }));
    var iconBtn = modeControl.buttons.icon;
    var numericBtn = modeControl.buttons.numeric;
    var textBtn = modeControl.buttons.text;

    var stateIconSection = condField();
    var iconEntityField = helpers.renderCardEntityField(stateIconSection, b, helpers, {
      entity: SUBPAGE_CARD_METADATA.iconStateEntity,
    });
    var iconEntityInp = iconEntityField.input;
    helpers.renderCardIconPicker(stateIconSection, b, helpers, SUBPAGE_CARD_METADATA.iconOn);
    stateCond.appendChild(stateIconSection);

    var sensorField = condField();
    var sensorEntityField = helpers.renderCardEntityField(sensorField, b, helpers, {
      entity: SUBPAGE_CARD_METADATA.sensorEntity,
    });
    var sensorInp = sensorEntityField.input;
    helpers.requireField(sensorInp, "Add a sensor entity before saving.", function () {
      return showState && (mode === "numeric" || mode === "text");
    });

    function saveSensorEntity() {
      sensorEntity = sensorInp.value;
      if (showState && mode !== "icon") {
        b.sensor = sensorEntity;
        helpers.saveField("sensor", b.sensor);
      }
    }
    sensorInp.addEventListener("input", saveSensorEntity);
    sensorInp.addEventListener("change", saveSensorEntity);
    sensorInp.addEventListener("blur", saveSensorEntity);
    sensorInp.addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        saveSensorEntity();
        this.blur();
      }
    });

    function saveIconStateEntity() {
      iconStateEntity = iconEntityInp.value;
      if (showState && mode === "icon") {
        b.entity = iconStateEntity;
        helpers.saveField("entity", b.entity);
      }
    }
    iconEntityInp.addEventListener("input", saveIconStateEntity);
    iconEntityInp.addEventListener("change", saveIconStateEntity);
    iconEntityInp.addEventListener("blur", saveIconStateEntity);
    iconEntityInp.addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        saveIconStateEntity();
        this.blur();
      }
    });

    var numericSection = condField();

    var unitField = helpers.renderCardTextField(numericSection, b, helpers, SUBPAGE_CARD_METADATA.unitField);
    var unitInp = unitField.input;

    var precisionField = helpers.precisionField(helpers.idPrefix + "precision",
      mode === "numeric" ? (b.precision || "0") : "0", function () {
      if (mode !== "numeric") return;
      b.precision = this.value === "0" ? "" : this.value;
      helpers.saveField("precision", b.precision);
    });
    var precisionSelect = precisionField.select;
    numericSection.appendChild(precisionField.field);
    helpers.renderCardLargeNumbersToggle(numericSection, b, helpers, SUBPAGE_CARD_METADATA);

    sensorField.appendChild(numericSection);
    stateCond.appendChild(sensorField);

    panel.appendChild(stateCond);

    function setMode(nextMode, persist) {
      mode = nextMode;
      showState = mode !== "off";
      showStateToggle.input.checked = showState;
      var iconLabel = iconSectionMain.querySelector(".sp-field-label");
      if (iconLabel) iconLabel.textContent = mode === "icon" ? "Off Icon" : "Icon";
      stateCond.classList.toggle("sp-visible", showState);
      iconBtn.classList.toggle("active", mode === "icon");
      numericBtn.classList.toggle("active", mode === "numeric");
      textBtn.classList.toggle("active", mode === "text");
      stateIconSection.classList.toggle("sp-visible", mode === "icon");
      sensorField.classList.toggle("sp-visible", mode === "numeric" || mode === "text");
      numericSection.classList.toggle("sp-visible", mode === "numeric");
      if (mode !== "numeric" && mode !== "text") helpers.clearFieldError(sensorInp);
      if (!persist) return;

      if (mode === "off") {
        b.sensor = "";
        b.entity = "";
        b.unit = "";
        b.precision = "";
        b.icon_on = "Auto";
        iconStateEntity = "";
        iconEntityInp.value = "";
        helpers.saveField("sensor", "");
        helpers.saveField("entity", "");
        helpers.saveField("unit", "");
        helpers.saveField("precision", "");
        helpers.saveField("icon_on", "Auto");
      } else if (mode === "icon") {
        b.sensor = "indicator";
        b.entity = iconStateEntity;
        b.unit = "";
        b.precision = "";
        helpers.saveField("sensor", "indicator");
        helpers.saveField("entity", b.entity);
        helpers.saveField("unit", "");
        helpers.saveField("precision", "");
      } else if (mode === "numeric") {
        b.sensor = sensorEntity;
        b.entity = "";
        b.unit = unitInp.value;
        b.precision = precisionSelect.value === "0" ? "" : precisionSelect.value;
        b.icon_on = "Auto";
        iconStateEntity = "";
        iconEntityInp.value = "";
        helpers.saveField("sensor", b.sensor);
        helpers.saveField("entity", "");
        helpers.saveField("unit", b.unit);
        helpers.saveField("precision", b.precision);
        helpers.saveField("icon_on", "Auto");
      } else if (mode === "text") {
        b.sensor = sensorEntity;
        b.entity = "";
        b.unit = "";
        b.precision = "text";
        b.icon_on = "Auto";
        iconStateEntity = "";
        iconEntityInp.value = "";
        helpers.saveField("sensor", b.sensor);
        helpers.saveField("entity", "");
        helpers.saveField("unit", "");
        helpers.saveField("precision", "text");
        helpers.saveField("icon_on", "Auto");
      }
    }

    showStateToggle.input.addEventListener("change", function () {
      setMode(this.checked ? (mode === "off" ? "icon" : mode) : "off", true);
    });
    setMode(mode, false);

    appendEditSubpageButton(panel, slot);
  },
  renderPreview: function (b, helpers) {
    var defaults = subpagePresetDefaults(subpageKind(b));
    var label = b.label || (defaults && defaults.label) || b.entity || "Configure";
    var mode = subpageStateDisplayMode(b);

    if (mode === "icon") {
      var stateIconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) :
        (defaults ? iconSlug(defaults.icon) : "cog");
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + stateIconName + '"></span>',
        labelHtml: subpageBadgeLabelHtml(helpers, label),
      };
    }

    if (mode === "numeric") {
      var unit = b.unit || "";
      var prec = parseInt(b.precision || "0", 10) || 0;
      var sampleVal = (0).toFixed(prec);
      return {
        iconHtml: cardSensorPreviewHtml(b, helpers, sampleVal, unit),
        labelHtml: subpageBadgeLabelHtml(helpers, b.label || b.sensor || "Subpage"),
      };
    }

    if (mode === "text") {
      var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "cog";
      return {
        iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
        labelHtml: subpageBadgeLabelHtml(helpers, "State"),
      };
    }

    return {
      labelHtml: subpageBadgeLabelHtml(helpers, label),
    };
  },
  contextMenuItems: function (slot, b, helpers) {
    helpers.addCtxItem("cog", "Edit Subpage", function () { enterSubpage(slot); });
  },
});

function subpageBadgeLabelHtml(helpers, label) {
  return '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
    helpers.escHtml(label) +
  '</span><span class="sp-subpage-badge mdi mdi-' +
    SUBPAGE_CARD_METADATA.preview.badge +
  '"></span></span>';
}

function appendEditSubpageButton(panel, slot) {
  var configBtn = document.createElement("button");
  configBtn.className = "sp-action-btn sp-edit-subpage-btn";
  configBtn.textContent = "Edit Subpage";
  configBtn.addEventListener("click", function () { closeSettings(); enterSubpage(slot); });
  panel.appendChild(configBtn);
}
