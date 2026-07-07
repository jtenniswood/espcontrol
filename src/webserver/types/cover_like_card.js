function coverLikeModeValues(cardType, optionName, fallbackModes) {
  var spec = cardContractOptionSpec(cardType, optionName);
  return spec && spec.values ? spec.values.slice() : fallbackModes.map(function (entry) { return entry[0]; });
}

function normalizeCoverLikeMode(mode, values) {
  mode = String(mode || "");
  return values.indexOf(mode) >= 0 ? mode : "";
}

function registerCoverLikeCardType(config) {
  var metadata = config.metadata;

  function normalizeMode(mode) {
    return normalizeCoverLikeMode(
      mode,
      coverLikeModeValues(config.type, config.optionName, metadata.mode.options)
    );
  }

  function commandMode(mode) {
    return config.commandModes.indexOf(normalizeMode(mode)) >= 0;
  }

  function commandPlaceholder(mode) {
    return "e.g. " + config.defaultLabel(mode) + " " + config.shortLabel;
  }

  function syncModeFields(b, helpers, mode) {
    b.unit = "";
    b.precision = "";
    var normalizedOptions = config.normalizeOptions(b.options, mode);
    if (b.options !== normalizedOptions) {
      b.options = normalizedOptions;
      helpers.saveField("options", normalizedOptions);
    }
    if (commandMode(mode) && b.icon_on !== "Auto") {
      b.icon_on = "Auto";
      helpers.saveField("icon_on", "Auto");
    } else if (!commandMode(mode) && (!b.icon_on || b.icon_on === "Auto")) {
      b.icon_on = config.openIcon;
      helpers.saveField("icon_on", config.openIcon);
    }
  }

  registerButtonType(config.type, {
    label: function () { return cardContractCardLabel(config.type); },
    allowInSubpage: function () { return cardContractAllowInSubpage(config.type); },
    pickerKey: function () { return cardContractPickerKey(config.type); },
    hidden: function () { return cardContractHidden(config.type); },
    hideLabel: true,
    defaultConfig: function () { return cardContractDefaultConfig(config.type); },
    cardMetadata: metadata,
    onSelect: function (b) {
      b.label = "";
      b.sensor = "";
      b.unit = "";
      b.precision = "";
      b.icon = config.closedIcon;
      b.icon_on = config.openIcon;
      b.options = "";
    },
    renderSettings: function (panel, b, slot, helpers) {
      var mode = normalizeMode(b.sensor);
      if (b.sensor !== mode) {
        b.sensor = mode;
        helpers.saveField("sensor", mode);
      }
      syncModeFields(b, helpers, mode);

      helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, metadata, {
        mode: Object.assign({}, metadata.mode, {
          value: function () { return mode; },
          onChange: function () {
            var oldMode = mode;
            var hadDefaultIcon = config.usesDefaultIcon(b.icon);
            mode = normalizeMode(this.value);
            b.sensor = mode;
            helpers.saveField("sensor", mode);
            b.unit = "";
            b.precision = "";
            helpers.saveField("unit", "");
            helpers.saveField("precision", "");
            b.options = config.normalizeOptions(b.options, mode);
            helpers.saveField("options", b.options);
            if (hadDefaultIcon || b.icon === config.defaultIcon(oldMode)) {
              b.icon = config.defaultIcon(mode);
              helpers.saveField("icon", b.icon);
            }
            b.icon_on = commandMode(mode) ? "Auto" : (b.icon_on && b.icon_on !== "Auto" ? b.icon_on : config.openIcon);
            helpers.saveField("icon_on", b.icon_on);
            renderButtonSettings();
          },
        }),
      }));

      var labelHost = document.createElement("div");
      var labelControl = helpers.renderCardTextField(labelHost, b, helpers, Object.assign({}, metadata.labelField, {
        placeholder: commandMode(mode) ? commandPlaceholder(mode) : config.labelPlaceholder,
      }));

      function setLabelVisible(value) {
        labelControl.field.style.display = value === "label" ? "" : "none";
      }

      var labelMode = config.labelDisplayMode(b);
      helpers.renderCardSegmentControl(panel, b, helpers, {
        segment: Object.assign({}, metadata.display, {
          value: function () { return labelMode; },
          onSelect: function (button, cardHelpers, value) {
            labelMode = value;
            config.setLabelDisplayMode(button, value);
            cardHelpers.saveField("options", button.options);
            setLabelVisible(value);
            scheduleRender();
          },
        }),
      });
      setLabelVisible(labelMode);

      panel.appendChild(labelControl.field);
      helpers.renderCardEntityField(panel, b, helpers, metadata);

      var closedIconVal = b.icon && b.icon !== "Auto" ? b.icon : config.closedIcon;
      var iconOnVal = b.icon_on && b.icon_on !== "Auto" ? b.icon_on : config.openIcon;
      if (commandMode(mode)) {
        helpers.renderCardIconPicker(panel, b, helpers, {
          pickerIdSuffix: "icon-picker",
          idSuffix: "icon",
          field: "icon",
          value: b.icon && b.icon !== "Auto" ? b.icon : config.defaultIcon(mode),
          fallback: config.defaultIcon(mode),
          label: "Icon",
        });
      } else {
        helpers.renderCardIconPair(panel, b, helpers, {
          pickerIdSuffix: "icon-picker",
          idSuffix: "icon",
          field: "icon",
          value: closedIconVal,
          fallback: config.closedIcon,
          label: "Closed Icon",
        }, {
          pickerIdSuffix: "icon-on-picker",
          idSuffix: "icon-on",
          field: "icon_on",
          value: iconOnVal,
          fallback: config.openIcon,
          label: "Open Icon",
        });
      }
    },
    renderPreview: function (b, helpers) {
      var mode = normalizeMode(b.sensor);
      var label = b.label || (commandMode(mode) ? config.defaultLabel(mode) : b.entity || config.defaultCardLabel);
      if (config.labelDisplayMode(b) === "status") label = config.statusLabel || "Closed";
      return cardBadgePreview(b, helpers, {
        label: label,
        iconFallback: config.defaultIcon(mode),
        badge: metadata.preview.badge,
      });
    },
  });
}
