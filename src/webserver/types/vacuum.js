// Vacuum card: touchscreen-friendly controls for Home Assistant vacuum entities.
var VACUUM_CARD_MODES = [
  ["modal", "All Controls"],
  ["status", "Status"],
  ["start_stop", "Start / Stop"],
  ["dock", "Dock"],
  ["pause_resume", "Pause / Resume"],
  ["clean_spot", "Spot Clean"],
  ["locate", "Locate"],
  ["clean_area", "Clean Area"],
];

function vacuumModeValues() {
  return entityModeValues("vacuum", "vacuum_mode", VACUUM_CARD_MODES);
}

function normalizeVacuumMode(mode) {
  return normalizeEntityMode(mode, vacuumModeValues(), "modal");
}

function vacuumModeNeedsArea(mode) {
  return normalizeVacuumMode(mode) === "clean_area";
}

function vacuumModalMode(mode) {
  return normalizeVacuumMode(mode) === "modal";
}

function vacuumModeDefaultIcon(mode) {
  mode = normalizeVacuumMode(mode);
  if (mode === "dock") return "Robot Vacuum Variant";
  if (mode === "clean_spot") return "Vacuum";
  if (mode === "locate") return "Robot Vacuum Alert";
  if (mode === "clean_area") return "Vacuum Outline";
  return "Robot Vacuum";
}

function vacuumModeBadgeIcon(mode) {
  mode = normalizeVacuumMode(mode);
  if (mode === "modal") return "dots-grid";
  if (mode === "dock") return "home-import-outline";
  if (mode === "pause_resume") return "play-pause";
  if (mode === "clean_spot") return "vacuum";
  if (mode === "locate") return "map-marker-question";
  if (mode === "clean_area") return "map-marker-path";
  return "robot-vacuum";
}

function vacuumUsesDefaultIcon(icon) {
  return entityModeCardUsesDefaultIcon(icon, [
    "Robot Vacuum",
    "Robot Vacuum Alert",
    "Robot Vacuum Off",
    "Robot Vacuum Variant",
    "Robot Vacuum Variant Alert",
    "Robot Vacuum Variant Off",
    "Vacuum",
    "Vacuum Outline",
  ]);
}

function normalizeVacuumConfig(b) {
  normalizeEntityModeCardConfig(b, {
    normalizeMode: normalizeVacuumMode,
    defaultIcon: vacuumModeDefaultIcon,
    keepUnit: function (mode) {
      mode = normalizeVacuumMode(mode);
      return mode === "clean_area" || mode === "modal";
    },
  });
  b.options = normalizeVacuumOptions(b.options, b.sensor);
}

function vacuumControlTabDefinitions() {
  return [
    { value: "controls", label: "Controls" },
    { value: "rooms", label: "Rooms" },
    { value: "fan", label: "Fan Speed" },
  ];
}

function vacuumControlDefaultTabs() {
  return vacuumControlTabDefinitions().map(function (tab) { return tab.value; });
}

function normalizeVacuumControlTabs(value) {
  return normalizeTabList(
    value,
    vacuumControlTabDefinitions(),
    vacuumControlDefaultTabs(),
    "controls"
  );
}

function vacuumControlTabs(b) {
  return normalizeVacuumControlTabs(configOptionValue(b && b.options, "vacuum_tabs"));
}

function vacuumControlTabsAreDefault(tabs) {
  return tabListIsDefault(
    normalizeVacuumControlTabs((tabs || []).join("|")),
    vacuumControlDefaultTabs()
  );
}

function normalizeVacuumRooms(value) {
  var rows = String(value || "").split(/\r?\n/);
  var out = [];
  rows.forEach(function (row) {
    row = String(row || "").trim();
    if (!row) return;
    var sep = row.indexOf("=");
    if (sep <= 0 || sep >= row.length - 1) return;
    var label = row.slice(0, sep).trim();
    var area = row.slice(sep + 1).trim();
    if (!label || !area) return;
    out.push(label + "=" + area);
  });
  return out.join("\n");
}

function vacuumRoomsValue(b) {
  return normalizeVacuumRooms(configOptionValue(b && b.options, "vacuum_rooms"));
}

function normalizeVacuumOptions(options, mode) {
  if (!vacuumModalMode(mode)) return "";
  var out = "";
  var tabs = normalizeVacuumControlTabs(configOptionValue(options, "vacuum_tabs"));
  if (!vacuumControlTabsAreDefault(tabs)) {
    out = setConfigOptionValue(out, "vacuum_tabs", tabs.join("|"));
  }
  var rooms = normalizeVacuumRooms(configOptionValue(options, "vacuum_rooms"));
  if (rooms) out = setConfigOptionValue(out, "vacuum_rooms", rooms);
  return out;
}

function setVacuumControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeVacuumControlTabs((tabs || []).join("|"));
  b.options = vacuumControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, "vacuum_tabs", "")
    : setConfigOptionValue(b.options, "vacuum_tabs", tabs.join("|"));
  b.options = normalizeVacuumOptions(b.options, b.sensor);
  return b.options;
}

function setVacuumRooms(b, rooms) {
  if (!b) return "";
  rooms = normalizeVacuumRooms(rooms);
  b.options = setConfigOptionValue(b.options, "vacuum_rooms", rooms);
  b.options = normalizeVacuumOptions(b.options, b.sensor);
  return b.options;
}

function renderVacuumRoomsSettings(panel, b, helpers) {
  var field = document.createElement("div");
  field.className = "sp-field";
  var inputId = helpers.idPrefix + "vacuum-rooms";
  field.appendChild(helpers.fieldLabel("Rooms", inputId));
  var area = document.createElement("textarea");
  area.className = "sp-input";
  area.id = inputId;
  area.rows = 4;
  area.placeholder = "Kitchen=kitchen\nLiving Room=living_room";
  area.value = vacuumRoomsValue(b);
  area.addEventListener("change", function () {
    helpers.saveField("options", setVacuumRooms(b, area.value));
    renderButtonSettings();
  });
  field.appendChild(area);
  panel.appendChild(field);
}

var VACUUM_CARD_METADATA = {
  mode: {
    label: "Type",
    idSuffix: "vacuum-type",
    options: VACUUM_CARD_MODES,
    value: function (b) {
      return normalizeVacuumMode(b.sensor);
    },
  },
  entity: {
    label: "Vacuum Entity",
    idSuffix: "vacuum-entity",
    placeholder: "e.g. vacuum.kitchen",
    domains: function () { return cardContractDomains("vacuum"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a vacuum entity before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "vacuum-label",
    field: "label",
    rerender: true,
  },
};

registerButtonType("vacuum", {
  label: function () { return cardContractCardLabel("vacuum"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("vacuum"); },
  pickerKey: function () { return cardContractPickerKey("vacuum"); },
  hidden: function () { return cardContractHidden("vacuum"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("vacuum"); },
  cardMetadata: VACUUM_CARD_METADATA,
  normalizeConfig: normalizeVacuumConfig,
  onSelect: function (b) {
    var defaults = cardContractDefaultConfig("vacuum");
    Object.keys(defaults).forEach(function (key) {
      if (key !== "entity") b[key] = defaults[key];
    });
  },
  renderSettings: function (panel, b, slot, helpers) {
    var mode = normalizeVacuumMode(b.sensor);
    if (b.sensor !== mode) {
      b.sensor = mode;
      helpers.saveField("sensor", mode);
    }
    b.precision = "";
    b.icon_on = "Auto";
    b.options = normalizeVacuumOptions(b.options, mode);
    if (!(vacuumModeNeedsArea(mode) || vacuumModalMode(mode)) && b.unit) {
      b.unit = "";
      helpers.saveField("unit", "");
    }
    if (!b.icon || b.icon === "Auto") {
      b.icon = vacuumModeDefaultIcon(mode);
      helpers.saveField("icon", b.icon);
    }

    helpers.renderCardModeSelector(panel, b, helpers, Object.assign({}, VACUUM_CARD_METADATA, {
      mode: Object.assign({}, VACUUM_CARD_METADATA.mode, {
        value: function () { return mode; },
        onChange: function () {
          var oldMode = mode;
          mode = normalizeVacuumMode(this.value);
          applyEntityModeCardModeChange(b, helpers, oldMode, mode, {
            defaultIcon: vacuumModeDefaultIcon,
            keepUnit: function (nextMode) {
              nextMode = normalizeVacuumMode(nextMode);
              return nextMode === "clean_area" || nextMode === "modal";
            },
            usesDefaultIcon: vacuumUsesDefaultIcon,
          });
          b.options = normalizeVacuumOptions(b.options, mode);
          helpers.saveField("options", b.options);
          renderButtonSettings();
        },
      }),
    }));

    helpers.renderCardEntityField(panel, b, helpers, VACUUM_CARD_METADATA);

    helpers.renderCardTextField(panel, b, helpers, Object.assign({}, VACUUM_CARD_METADATA.labelField, {
      placeholder: mode === "clean_area" ? "e.g. Clean Kitchen" : "e.g. Kitchen Vacuum",
    }));

    if (vacuumModeNeedsArea(mode)) {
      helpers.renderCardTextField(panel, b, helpers, {
        label: "Area ID",
        idSuffix: "vacuum-area-id",
        field: "unit",
        placeholder: "e.g. kitchen",
        rerender: false,
      });
    }

    if (vacuumModalMode(mode)) {
      renderModalTabSettings(panel, b, helpers, {
        definitions: vacuumControlTabDefinitions,
        tabs: vacuumControlTabs,
        setTabs: setVacuumControlTabs,
        normalizeOptions: function (options) {
          return normalizeVacuumOptions(options, mode);
        },
      });

      helpers.renderCardTextField(panel, b, helpers, {
        label: "Fallback Area ID",
        idSuffix: "vacuum-fallback-area-id",
        field: "unit",
        placeholder: "e.g. kitchen",
        rerender: false,
      });

      renderVacuumRoomsSettings(panel, b, helpers);
    }

    helpers.renderCardIconPicker(panel, b, helpers, {
      pickerIdSuffix: "vacuum-icon-picker",
      idSuffix: "vacuum-icon",
      field: "icon",
      fallback: function () { return vacuumModeDefaultIcon(mode); },
      label: "Icon",
    });
  },
  renderPreview: function (b, helpers) {
    var mode = normalizeVacuumMode(b.sensor);
    var label = b.label || b.entity || "Vacuum";
    var iconName = b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : iconSlug(vacuumModeDefaultIcon(mode));
    var stateBadge = mode === "status" ? '<span class="sp-sensor-badge mdi mdi-format-text"></span>' : "";
    return {
      iconHtml: stateBadge + '<span class="sp-btn-icon mdi mdi-' + iconName + '"></span>',
      labelHtml: cardBadgeLabelHtml(helpers, label, vacuumModeBadgeIcon(mode)),
    };
  },
});
