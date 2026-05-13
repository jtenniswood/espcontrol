// Slider and cover button types: draggable brightness/position control.
// Factory creates both "slider" (light.turn_on w/ brightness) and "cover"
// variants. Slider cards are always vertical. For covers, b.sensor stores
// "", "tilt", "toggle", or a one-tap cover command.
function coverCommandMode(mode) {
  return mode === "open" || mode === "close" || mode === "stop" || mode === "set_position";
}

function normalizeCoverMode(mode, allowCommands) {
  if (mode === "tilt" || mode === "toggle") return mode;
  if (allowCommands && coverCommandMode(mode)) return mode;
  return "";
}

function normalizeCoverPosition(value) {
  var n = parseInt(value, 10);
  if (!isFinite(n)) n = 50;
  if (n < 0) n = 0;
  if (n > 100) n = 100;
  return String(n);
}

function sliderTypeFactory(opts) {
  return {
    label: opts.label,
    allowInSubpage: true,
    hideLabel: !!opts.hideLabel,
    labelPlaceholder: opts.placeholder,
    onSelect: function (b) {
      b.sensor = ""; b.unit = "";
      b.icon = opts.defaultIcon;
      b.icon_on = opts.defaultIconOn;
    },
    renderSettings: function (panel, b, slot, helpers) {
      function labelField() {
        panel.appendChild(helpers.textField(
          "Label",
          helpers.idPrefix + "label",
          b.label,
          opts.placeholder,
          "label",
          true
        ).field);
      }

      if (opts.lightControlType) renderLightControlTypeField(panel, b, helpers);

      var coverMode = "";
      var coverPositionField = null;
      var coverPositionInput = null;
      var singleIconSection = null;
      var offIconSection = null;
      var syncCoverUi = function () {};

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

        var interactionOptions = [
          ["", "Slider: Position"],
          ["tilt", "Slider: Tilt"],
          ["toggle", "Toggle"],
          ["open", "Open"],
          ["close", "Close"],
          ["stop", "Stop"],
          ["set_position", "Set Position"],
        ];
        var interactionField = helpers.selectField("Type", helpers.idPrefix + "cover-interaction", interactionOptions, coverMode);
        var interactionSelect = interactionField.select;
        panel.appendChild(interactionField.field);

        coverPositionField = document.createElement("div");
        coverPositionField.className = "sp-field";
        coverPositionField.appendChild(helpers.fieldLabel("Position", helpers.idPrefix + "cover-position"));
        coverPositionInput = document.createElement("input");
        coverPositionInput.type = "number";
        coverPositionInput.className = "sp-input";
        coverPositionInput.id = helpers.idPrefix + "cover-position";
        coverPositionInput.min = "0";
        coverPositionInput.max = "100";
        coverPositionInput.step = "1";
        coverPositionInput.placeholder = "e.g. 50";
        coverPositionInput.value = normalizeCoverPosition(b.unit);
        coverPositionField.appendChild(coverPositionInput);
        panel.appendChild(coverPositionField);
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

      if (opts.renderLabelInSettings && !opts.labelAfterEntity) labelField();

      panel.appendChild(helpers.entityField(
        "Entity",
        helpers.idPrefix + "entity",
        b.entity,
        opts.entityPlaceholder,
        opts.entityDomains,
        "entity",
        true,
        "Add an entity before saving."
      ).field);

      if (opts.renderLabelInSettings && opts.labelAfterEntity) labelField();

      function iconField(label, inputSuffix, field, currentVal, defaultVal) {
        return helpers.iconPickerField(helpers.idPrefix + inputSuffix + "-picker", helpers.idPrefix + inputSuffix, currentVal, function (opt) {
          b[field] = opt || defaultVal;
          helpers.saveField(field, b[field]);
        }, label);
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
        panel.appendChild(singleIconSection);
        panel.appendChild(offIconSection);
        panel.appendChild(onIconSection);
        syncCoverUi = function () {
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
        panel.appendChild(helpers.iconPickerField(
          helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
          b.icon || "Auto", function (opt) {
            b.icon = opt;
            helpers.saveField("icon", opt);
          },
          "Icon"
        ));
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
        var iconOnSection = helpers.iconPickerField(
          helpers.idPrefix + "icon-on-picker",
          helpers.idPrefix + "icon-on",
          iconOnVal,
          function (opt) {
            b.icon_on = opt;
            helpers.saveField("icon_on", opt);
          },
          opts.iconOnFieldLabel
        );
        var iconOnPicker = iconOnSection.querySelector(".sp-icon-picker");
        iconOnCond.appendChild(iconOnSection);

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
          labelHtml:
            '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
            '<span class="sp-type-badge mdi mdi-' + opts.badgeIcon + '"></span></span>',
        };
      }
      return {
        iconHtml:
          '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>' +
          '<span class="sp-slider-preview"><span class="sp-slider-track">' +
            '<span class="sp-slider-fill"></span>' +
          '</span></span>',
        labelHtml:
          '<span class="sp-btn-label-row"><span class="sp-btn-label">' + helpers.escHtml(label) + '</span>' +
          '<span class="sp-type-badge mdi mdi-' + opts.badgeIcon + '"></span></span>',
      };
    },
  };
}

registerButtonType("light_brightness", sliderTypeFactory({
  label: "Lights",
  placeholder: "e.g. Living Room",
  entityPlaceholder: "e.g. light.living_room",
  entityDomains: ["light"],
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
  label: "Slider",
  placeholder: "e.g. Living Room",
  entityPlaceholder: "e.g. light.living_room",
  entityDomains: ["light", "fan"],
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
  label: "Cover",
  placeholder: "e.g. Office Blind",
  entityPlaceholder: "e.g. cover.office_blind",
  entityDomains: ["cover"],
  defaultIcon: "Blinds",
  defaultIconOn: "Blinds Open",
  fallbackLabel: "Cover",
  fallbackIcon: "blinds",
  badgeIcon: "blinds-horizontal",
  alwaysShowIconPair: true,
  hideLabel: true,
  renderLabelInSettings: true,
  interactionMode: true,
}));
