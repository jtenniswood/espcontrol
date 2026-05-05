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
        var lf = document.createElement("div");
        lf.className = "sp-field";
        lf.appendChild(helpers.fieldLabel("Label", helpers.idPrefix + "label"));
        var labelInp = helpers.textInput(helpers.idPrefix + "label", b.label, opts.placeholder);
        lf.appendChild(labelInp);
        panel.appendChild(lf);
        helpers.bindField(labelInp, "label", true);
      }

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

        var imf = document.createElement("div");
        imf.className = "sp-field";
        imf.appendChild(helpers.fieldLabel("Interaction", helpers.idPrefix + "cover-interaction"));
        var interactionSelect = document.createElement("select");
        interactionSelect.className = "sp-select";
        interactionSelect.id = helpers.idPrefix + "cover-interaction";
        var interactionOptions = [
          ["", "Slider: Position"],
          ["tilt", "Slider: Tilt"],
          ["toggle", "Toggle"],
          ["open", "Open"],
          ["close", "Close"],
          ["stop", "Stop"],
          ["set_position", "Set Position"],
        ];
        interactionOptions.forEach(function (entry) {
          var option = document.createElement("option");
          option.value = entry[0];
          option.textContent = entry[1];
          interactionSelect.appendChild(option);
        });
        interactionSelect.value = coverMode;
        imf.appendChild(interactionSelect);
        panel.appendChild(imf);

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

      if (opts.renderLabelInSettings) labelField();

      var ef = document.createElement("div");
      ef.className = "sp-field";
      ef.appendChild(helpers.fieldLabel("Entity ID", helpers.idPrefix + "entity"));
      var entityInp = helpers.textInput(helpers.idPrefix + "entity", b.entity, opts.entityPlaceholder);
      ef.appendChild(entityInp);
      panel.appendChild(ef);
      helpers.bindField(entityInp, "entity", true);
      helpers.requireField(entityInp, "Add an entity before saving.");

      function iconField(label, inputSuffix, field, currentVal, defaultVal) {
        var section = document.createElement("div");
        section.className = "sp-field";
        section.appendChild(helpers.fieldLabel(label, helpers.idPrefix + inputSuffix));
        var picker = document.createElement("div");
        picker.className = "sp-icon-picker";
        picker.id = helpers.idPrefix + inputSuffix + "-picker";
        picker.innerHTML =
          '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(currentVal) + '"></span>' +
          '<input class="sp-icon-picker-input" id="' + helpers.idPrefix + inputSuffix + '" type="text" ' +
          'placeholder="Search icons\u2026" value="' + escAttr(currentVal) + '" autocomplete="off">' +
          '<div class="sp-icon-dropdown"></div>';
        section.appendChild(picker);
        initIconPicker(picker, currentVal, function (opt) {
          b[field] = opt || defaultVal;
          helpers.saveField(field, b[field]);
        });
        return section;
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
        panel.appendChild(helpers.makeIconPicker(
          helpers.idPrefix + "icon-picker", helpers.idPrefix + "icon",
          b.icon || "Auto", function (opt) {
            b.icon = opt;
            helpers.saveField("icon", opt);
          }
        ));
      }

      if (!opts.interactionMode && b.sensor) {
        b.sensor = "";
        helpers.saveField("sensor", "");
      }

      if (!opts.alwaysShowIconPair) {
        var hasIconOn = b.icon_on && b.icon_on !== "Auto";
        var iconOnToggle = helpers.toggleRow(opts.iconOnLabel, helpers.idPrefix + "iconon-toggle", hasIconOn);
        panel.appendChild(iconOnToggle.row);

        var iconOnCond = condField();
        if (hasIconOn) iconOnCond.classList.add("sp-visible");

        var iconOnSection = document.createElement("div");
        iconOnSection.className = "sp-field";
        iconOnSection.appendChild(helpers.fieldLabel(opts.iconOnFieldLabel, helpers.idPrefix + "icon-on"));
        var iconOnVal = hasIconOn ? b.icon_on : "Auto";
        var iconOnPicker = document.createElement("div");
        iconOnPicker.className = "sp-icon-picker";
        iconOnPicker.id = helpers.idPrefix + "icon-on-picker";
        iconOnPicker.innerHTML =
          '<span class="sp-icon-picker-preview mdi mdi-' + iconSlug(iconOnVal) + '"></span>' +
          '<input class="sp-icon-picker-input" id="' + helpers.idPrefix + 'icon-on" type="text" ' +
          'placeholder="Search icons\u2026" value="' + escAttr(iconOnVal) + '" autocomplete="off">' +
          '<div class="sp-icon-dropdown"></div>';
        iconOnSection.appendChild(iconOnPicker);
        iconOnCond.appendChild(iconOnSection);

        initIconPicker(iconOnPicker, iconOnVal, function (opt) {
          b.icon_on = opt;
          helpers.saveField("icon_on", opt);
        });

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

registerButtonType("slider", sliderTypeFactory({
  label: "Slider",
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
  label: "Cover",
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
}));
