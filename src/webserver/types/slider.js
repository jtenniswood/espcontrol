// Slider and cover button types: draggable brightness/position control.
// Factory creates both "slider" (light.turn_on w/ brightness) and "cover"
// variants. Slider cards are always vertical. For covers, b.sensor stores
// "modal", "", "tilt", "toggle", or a one-tap cover command.
function coverCommandMode(mode) {
  return mode === "open" || mode === "close" || mode === "stop" || mode === "set_position";
}

function coverModeOptionValues(allowCommands) {
  var spec = cardContractOptionSpec("cover", "cover_mode");
  var values = spec && spec.values ? spec.values : ["modal", "", "tilt", "toggle", "open", "close", "stop", "set_position"];
  return values.filter(function (value) {
    return allowCommands || !coverCommandMode(value);
  });
}

function normalizeCoverMode(mode, allowCommands) {
  mode = String(mode || "");
  return coverModeOptionValues(allowCommands).indexOf(mode) >= 0 ? mode : "";
}

function coverModeOptionsForSettings(currentMode) {
  return [
    ["modal", "All Controls"],
    ["", "Slider: Position"],
    ["tilt", "Slider: Tilt"],
    ["toggle", "Toggle"],
    ["open", "Open"],
    ["close", "Close"],
    ["stop", "Stop"],
    ["set_position", "Set Position"],
  ];
}

function normalizeCoverPosition(value) {
  var n = parseInt(value, 10);
  var spec = cardContractOptionSpec("cover", "cover_position") || {};
  var fallback = parseInt(spec.defaultValue, 10);
  var min = typeof spec.min === "number" ? spec.min : 0;
  var max = typeof spec.max === "number" ? spec.max : 100;
  if (!isFinite(fallback)) fallback = 50;
  if (!isFinite(n)) n = fallback;
  if (n < min) n = min;
  if (n > max) n = max;
  return String(n);
}

function renderCoverControlTabSettings(panel, b, helpers) {
  renderModalTabSettings(panel, b, helpers, {
    definitions: coverControlTabDefinitions,
    tabs: coverControlTabs,
    normalizeOptions: normalizeCoverOptions,
    setTabs: setCoverControlTabs,
    idPrefix: "cover-tab-",
    hideHeading: true,
  });
}

function sliderCardMetadata(opts) {
  return {
    entity: {
      label: "Entity",
      idSuffix: "entity",
      placeholder: opts.entityPlaceholder,
      domains: function () { return cardContractDomains(opts.type); },
      bindName: "entity",
      rerender: true,
      requiredMessage: "Add an entity before saving.",
    },
    labelField: {
      label: "Label",
      idSuffix: "label",
      field: "label",
      placeholder: opts.placeholder,
      rerender: true,
    },
    coverInteraction: {
      mode: {
        label: "Type",
        idSuffix: "cover-interaction",
        options: function (b) { return coverModeOptionsForSettings(normalizeCoverMode(b && b.sensor, true)); },
        value: function (b) {
          return normalizeCoverMode(b.sensor, true);
        },
      },
    },
    coverPosition: {
      label: "Position",
      idSuffix: "cover-position",
      min: 0,
      max: 100,
      step: 1,
      placeholder: "e.g. 50",
      value: function (b) {
        return normalizeCoverPosition(b.unit);
      },
    },
    preview: {
      badge: opts.badgeIcon,
    },
  };
}

function sliderTypeFactory(opts) {
  var metadata = sliderCardMetadata(opts);
  return {
    label: function () { return cardContractCardLabel(opts.type); },
    allowInSubpage: function () { return cardContractAllowInSubpage(opts.type); },
    pickerKey: function () { return cardContractPickerKey(opts.type); },
    hidden: function () { return cardContractHidden(opts.type); },
    hideLabel: !!opts.hideLabel,
    labelPlaceholder: opts.placeholder,
    defaultConfig: function () { return cardContractDefaultConfig(opts.type); },
    cardMetadata: metadata,
    onSelect: function (b) {
      b.sensor = opts.type === "cover" ? "modal" : "";
      b.unit = "";
      b.icon = opts.defaultIcon;
      b.icon_on = opts.defaultIconOn;
    },
    renderSettings: function (panel, b, slot, helpers) {
      var cardSettingsPanel = null;
      var modalSettingsPanel = null;
      var modalSettingsDisclosure = null;

      function labelField() {
        helpers.renderCardTextField(panel, b, helpers, metadata.labelField);
      }

      if (opts.lightControlType) renderLightControlTypeField(panel, b, helpers);

      var coverMode = "";
      var coverPositionField = null;
      var coverPositionInput = null;
      var singleIconSection = null;
      var offIconSection = null;
      var coverTabsSection = null;
      var syncCoverIconUi = function () {};
      var syncCoverUi = function () {
        syncCoverControlTabs();
        syncCoverIconUi();
      };

      function syncCoverControlTabs() {
        if (!opts.coverControlTabs || !coverTabsSection) return;
        coverTabsSection.innerHTML = "";
        if (coverMode === "modal") {
          if (modalSettingsDisclosure) modalSettingsDisclosure.style.display = "";
          renderCoverControlTabSettings(coverTabsSection, b, helpers);
          return;
        }
        if (modalSettingsDisclosure) modalSettingsDisclosure.style.display = "none";
        var previousOptions = b.options || "";
        b.options = "";
        if (b.options !== previousOptions) helpers.saveField("options", b.options);
      }

      function syncIconSection(section, value) {
        if (!section) return;
        var picker = section.querySelector(".sp-icon-picker");
        if (picker && picker._setIcon) {
          picker._setIcon(value);
          return;
        }
        var preview = section.querySelector(".sp-icon-picker-preview");
        if (preview) preview.className = "sp-icon-picker-preview mdi mdi-" + iconSlug(value);
        var input = section.querySelector(".sp-icon-picker-input");
        if (input) input.value = value;
      }

      function coverModeDefaultIcon(mode) {
        if (mode === "open") return opts.defaultIconOn;
        if (mode === "stop") return "Stop";
        return opts.defaultIcon;
      }

      function useCoverModeDefaultIcon() {
        return opts.interactionMode && (
          !b.icon ||
          b.icon === "Auto" ||
          b.icon === opts.defaultIcon ||
          b.icon === opts.defaultIconOn ||
          b.icon === "Minus" ||
          b.icon === "Stop"
        );
      }

      function applyCoverModeDefaultIcon(mode) {
        if (!useCoverModeDefaultIcon()) return;
        var icon = coverModeDefaultIcon(mode);
        if (b.icon === icon) return;
        b.icon = icon;
        helpers.saveField("icon", b.icon);
        syncIconSection(singleIconSection, b.icon);
        syncIconSection(offIconSection, b.icon);
      }

      if (opts.interactionMode) {
        var storedCoverMode = normalizeCoverMode(b.sensor, true);
        coverMode = storedCoverMode;
        if (b.sensor !== storedCoverMode) {
          b.sensor = storedCoverMode;
          helpers.saveField("sensor", storedCoverMode);
        }
        if (storedCoverMode !== "set_position" && b.unit) {
          b.unit = "";
          helpers.saveField("unit", "");
        }
        if (coverCommandMode(storedCoverMode) && b.icon_on !== "Auto") {
          b.icon_on = "Auto";
          helpers.saveField("icon_on", "Auto");
        }
        if (coverCommandMode(storedCoverMode)) {
          applyCoverModeDefaultIcon(storedCoverMode);
        }

        var interactionField = helpers.renderCardModeSelector(panel, b, helpers, {
          mode: Object.assign({}, metadata.coverInteraction.mode, {
            value: function () { return coverMode; },
            onChange: function () { setCoverMode(this.value, true); },
          }),
        });
        var interactionSelect = interactionField.select;

        var positionControl = helpers.renderCardNumberField(panel, b, helpers, metadata.coverPosition);
        coverPositionField = positionControl.field;
        coverPositionInput = positionControl.input;
        if (coverMode === "set_position" && b.unit !== coverPositionInput.value) {
          b.unit = coverPositionInput.value;
          helpers.saveField("unit", b.unit);
        }

        function setCoverPosition(value) {
          if (!coverPositionInput) return;
          var position = normalizeCoverPosition(value);
          coverPositionInput.value = position;
          b.unit = position;
          helpers.saveField("unit", position);
        }

        function setCoverMode(mode, persist) {
          coverMode = normalizeCoverMode(mode, true);
          interactionSelect.value = coverMode;
          if (coverMode === "set_position") {
            setCoverPosition(b.unit);
          } else if (b.unit) {
            b.unit = "";
            helpers.saveField("unit", "");
            coverPositionInput.value = "50";
          }
          if (coverCommandMode(coverMode)) {
            b.icon_on = "Auto";
            helpers.saveField("icon_on", "Auto");
            applyCoverModeDefaultIcon(coverMode);
          }
          if (persist) {
            b.sensor = coverMode;
            helpers.saveField("sensor", coverMode);
          } else {
            b.sensor = coverMode;
          }
          syncCoverUi();
        }

        interactionSelect.addEventListener("change", function () { setCoverMode(this.value, true); });
        coverPositionInput.addEventListener("change", function () { setCoverPosition(this.value); });
        coverPositionInput.addEventListener("blur", function () { setCoverPosition(this.value); });
      }

      if (opts.renderLabelInSettings && !opts.labelAfterEntity && !opts.coverControlTabs) labelField();

      helpers.renderCardEntityField(panel, b, helpers, metadata);

      if (opts.coverControlTabs) {
        cardSettingsPanel = document.createElement("div");
        modalSettingsPanel = document.createElement("div");
        panel.appendChild(inlineDisclosure("Card Settings", cardSettingsPanel, false));
        modalSettingsDisclosure = inlineDisclosure("Modal Settings", modalSettingsPanel, b._modalSettingsOpen === true);
        panel.appendChild(modalSettingsDisclosure);
        panel = cardSettingsPanel;
      }

      if (opts.renderLabelInSettings && (opts.labelAfterEntity || opts.coverControlTabs)) labelField();

      if (opts.coverControlTabs) {
        coverTabsSection = document.createElement("div");
        modalSettingsPanel.appendChild(coverTabsSection);
      }

      function iconField(label, inputSuffix, field, currentVal, defaultVal) {
        var picker = helpers.renderCardIconPicker(panel, b, helpers, {
          pickerIdSuffix: inputSuffix + "-picker",
          idSuffix: inputSuffix,
          field: field,
          value: currentVal,
          fallback: defaultVal,
          label: label,
        });
        var iconPicker = picker.querySelector(".sp-icon-picker");
        if (iconPicker && iconPicker._setIcon) iconPicker._setIcon(currentVal);
        return picker;
      }

      if (opts.alwaysShowIconPair) {
        var offIconVal = b.icon && b.icon !== "Auto" ? b.icon : opts.defaultIcon;
        var onIconDefault = opts.onIconInheritsOff ? offIconVal : opts.defaultIconOn;
        var onIconVal = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : onIconDefault;
        singleIconSection = iconField("Icon", "cover-icon", "icon", offIconVal, opts.defaultIcon);
        offIconSection = iconField(
          opts.iconOffFieldLabel || "Closed Icon", "icon", "icon", offIconVal, opts.defaultIcon
        );
        var onIconSection = iconField(
          opts.iconOnFieldLabel || "Open Icon", "icon-on", "icon_on", onIconVal, opts.defaultIconOn
        );
        syncCoverIconUi = function () {
          var singleIcon = opts.interactionMode && coverCommandMode(coverMode);
          singleIconSection.style.display = singleIcon ? "" : "none";
          offIconSection.style.display = singleIcon ? "none" : "";
          onIconSection.style.display = singleIcon ? "none" : "";
          if (coverPositionField) {
            coverPositionField.style.display = coverMode === "set_position" ? "" : "none";
          }
        };
        syncCoverUi();
      } else {
        helpers.renderCardIconPicker(panel, b, helpers, {
          pickerIdSuffix: "icon-picker",
          idSuffix: "icon",
          field: "icon",
          fallback: "Auto",
          label: "Icon",
        });
      }

      if (!opts.interactionMode && b.sensor) {
        b.sensor = "";
        helpers.saveField("sensor", "");
      }

      if (!opts.alwaysShowIconPair) {
        var hasIconOn = b.icon_on && b.icon_on !== "Auto";
        var iconOnToggleSection = helpers.toggleSection(opts.iconOnLabel, helpers.idPrefix + "iconon-toggle", hasIconOn);
        var iconOnToggle = iconOnToggleSection.toggle;
        var iconOnCond = iconOnToggleSection.section;
        panel.appendChild(iconOnToggle.row);
        if (hasIconOn) iconOnCond.classList.add("sp-visible");

        var iconOnVal = hasIconOn ? b.icon_on : "Auto";
        var iconOnSection = helpers.renderCardIconPicker(iconOnCond, b, helpers, {
          pickerIdSuffix: "icon-on-picker",
          idSuffix: "icon-on",
          field: "icon_on",
          value: iconOnVal,
          fallback: "Auto",
          label: opts.iconOnFieldLabel,
        });
        var iconOnPicker = iconOnSection.querySelector(".sp-icon-picker");

        panel.appendChild(iconOnCond);

        iconOnToggle.input.addEventListener("change", function () {
          if (this.checked) {
            iconOnCond.classList.add("sp-visible");
          } else {
            b.icon_on = "Auto";
            helpers.saveField("icon_on", "Auto");
            iconOnCond.classList.remove("sp-visible");
            var ionPreview = iconOnPicker.querySelector(".sp-icon-picker-preview");
            if (ionPreview) ionPreview.className = "sp-icon-picker-preview mdi mdi-cog";
            var ionInput = iconOnPicker.querySelector(".sp-icon-picker-input");
            if (ionInput) ionInput.value = "Auto";
          }
        });
      }
    },
    renderPreview: function (b, helpers) {
      var label = b.label || b.entity || opts.fallbackLabel;
      var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : opts.fallbackIcon;
      if (opts.interactionMode && (b.sensor === "toggle" || coverCommandMode(b.sensor))) {
        return {
          iconHtml: '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
          labelHtml: cardBadgeLabelHtml(helpers, label, metadata.preview.badge),
        };
      }
      return {
        iconHtml:
          '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
          '<span class="sp-slider-preview"><span class="sp-slider-track">' +
            '<span class="sp-slider-fill"></span>' +
          '</span></span>',
        labelHtml: cardBadgeLabelHtml(helpers, label, metadata.preview.badge),
      };
    },
  };
}

registerButtonType("light_brightness", sliderTypeFactory({
  type: "light_brightness",
  placeholder: "e.g. Living Room",
  entityPlaceholder: "e.g. light.living_room",
  defaultIcon: "Lightbulb Outline",
  defaultIconOn: "Lightbulb",
  fallbackLabel: "Brightness",
  fallbackIcon: "lightbulb",
  badgeIcon: "tune-vertical-variant",
  alwaysShowIconPair: true,
  onIconInheritsOff: false,
  iconOffFieldLabel: "Off Icon",
  iconOnFieldLabel: "On Icon",
  hideLabel: true,
  renderLabelInSettings: true,
  labelAfterEntity: true,
  lightControlType: true,
}));

registerButtonType("slider", sliderTypeFactory({
  type: "slider",
  placeholder: "e.g. Living Room",
  entityPlaceholder: "e.g. light.living_room",
  defaultIcon: "Auto",
  defaultIconOn: "Auto",
  fallbackLabel: "Slider",
  fallbackIcon: "lightbulb",
  badgeIcon: "tune-vertical-variant",
  alwaysShowIconPair: true,
  onIconInheritsOff: true,
  iconOffFieldLabel: "Off Icon",
  iconOnFieldLabel: "On Icon",
}));

registerButtonType("cover", sliderTypeFactory({
  type: "cover",
  placeholder: "e.g. Office Blind",
  entityPlaceholder: "e.g. cover.office_blind",
  defaultIcon: "Blinds",
  defaultIconOn: "Blinds Open",
  fallbackLabel: "Cover",
  fallbackIcon: "blinds",
  badgeIcon: "blinds-horizontal",
  alwaysShowIconPair: true,
  hideLabel: true,
  renderLabelInSettings: true,
  interactionMode: true,
  coverControlTabs: true,
}));
