// ── Modal Tab Options ──────────────────────────────────────────────
// @web-module-requires: config_option_core

function lightControlTabDefinitions() {
  var labels = {
    power: "Power",
    brightness: "Brightness",
    temperature: "Colour Temperature",
    color: "Colour Presets",
  };
  var spec = cardContractOptionSpec("light_control", LIGHT_CONTROL_TABS_OPTION);
  var values = spec && spec.values ? spec.values : [];
  return values.map(function (value) {
    return { value: value, label: labels[value] || value };
  });
}

function lightControlDefaultTabs() {
  return cardContractOptionDefaultValue(
    "light_control",
    LIGHT_CONTROL_TABS_OPTION,
    "power|brightness|temperature|color"
  ).split("|");
}

function normalizeLightControlTabs(value) {
  var raw = String(value || "").trim();
  var parts = raw ? raw.split("|") : lightControlDefaultTabs();
  var definitions = lightControlTabDefinitions();
  var valid = {};
  definitions.forEach(function (tab) { valid[tab.value] = true; });
  var out = [];
  parts.forEach(function (part) {
    part = String(part || "").trim();
    if (valid[part] && out.indexOf(part) < 0) out.push(part);
  });
  return out.length ? out : ["power"];
}

function lightControlTabs(b) {
  return normalizeLightControlTabs(configOptionValue(b && b.options, LIGHT_CONTROL_TABS_OPTION));
}

function lightControlTabsAreDefault(tabs) {
  tabs = normalizeLightControlTabs((tabs || []).join("|"));
  var defaults = lightControlDefaultTabs();
  if (tabs.length !== defaults.length) return false;
  for (var i = 0; i < defaults.length; i++) {
    if (tabs[i] !== defaults[i]) return false;
  }
  return true;
}

function normalizeLightControlOptions(options) {
  var tabs = normalizeLightControlTabs(configOptionValue(options, LIGHT_CONTROL_TABS_OPTION));
  return lightControlTabsAreDefault(tabs)
    ? ""
    : setConfigOptionValue("", LIGHT_CONTROL_TABS_OPTION, tabs.join("|"));
}

function setLightControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeLightControlTabs((tabs || []).join("|"));
  b.options = lightControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, LIGHT_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, LIGHT_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeLightControlOptions(b.options);
  return b.options;
}

function coverControlTabDefinitions() {
  var labels = {
    position: "Position",
    controls: "Controls",
    tilt: "Tilt",
  };
  var spec = cardContractOptionSpec("cover", COVER_CONTROL_TABS_OPTION);
  var values = spec && spec.values ? spec.values : [];
  return values.map(function (value) {
    return { value: value, label: labels[value] || value };
  });
}

function coverControlDefaultTabs() {
  return cardContractOptionDefaultValue(
    "cover",
    COVER_CONTROL_TABS_OPTION,
    "position|controls|tilt"
  ).split("|");
}

function normalizeTabList(value, definitions, defaults, fallback) {
  var raw = String(value || "").trim();
  var parts = raw ? raw.split("|") : defaults;
  var valid = {};
  definitions.forEach(function (tab) { valid[tab.value] = true; });
  var out = [];
  parts.forEach(function (part) {
    part = String(part || "").trim();
    if (valid[part] && out.indexOf(part) < 0) out.push(part);
  });
  return out.length ? out : [fallback];
}

function tabListIsDefault(tabs, defaults) {
  tabs = tabs || [];
  if (tabs.length !== defaults.length) return false;
  for (var i = 0; i < defaults.length; i++) {
    if (tabs[i] !== defaults[i]) return false;
  }
  return true;
}

function normalizeCoverControlTabs(value) {
  return normalizeTabList(
    value,
    coverControlTabDefinitions(),
    coverControlDefaultTabs(),
    "position"
  );
}

function coverControlTabs(b) {
  return normalizeCoverControlTabs(configOptionValue(b && b.options, COVER_CONTROL_TABS_OPTION));
}

function coverControlTabsAreDefault(tabs) {
  return tabListIsDefault(
    normalizeCoverControlTabs((tabs || []).join("|")),
    coverControlDefaultTabs()
  );
}

function normalizeCoverOptions(options) {
  var tabs = normalizeCoverControlTabs(configOptionValue(options, COVER_CONTROL_TABS_OPTION));
  return coverControlTabsAreDefault(tabs)
    ? ""
    : setConfigOptionValue("", COVER_CONTROL_TABS_OPTION, tabs.join("|"));
}

function normalizeCoverOptionsForMode(options, mode) {
  return normalizeCoverMode(mode, true) === "modal" ? normalizeCoverOptions(options) : "";
}

function setCoverControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeCoverControlTabs((tabs || []).join("|"));
  b.options = coverControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, COVER_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, COVER_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeCoverOptions(b.options);
  return b.options;
}

function climateControlTabDefinitions() {
  return [
    { value: "temperature", label: "Temperature" },
    { value: "mode", label: "Mode" },
    { value: "preset", label: "Preset" },
    { value: "fan", label: "Fan" },
    { value: "swing", label: "Swing" },
  ];
}

function climateControlDefaultTabs() {
  return climateControlTabDefinitions().map(function (tab) { return tab.value; });
}

function normalizeClimateControlTabs(value) {
  return normalizeTabList(
    value,
    climateControlTabDefinitions(),
    climateControlDefaultTabs(),
    "temperature"
  );
}

function climateControlTabs(b) {
  return normalizeClimateControlTabs(configOptionValue(b && b.options, CLIMATE_CONTROL_TABS_OPTION));
}

function climateControlTabsAreDefault(tabs) {
  return tabListIsDefault(
    normalizeClimateControlTabs((tabs || []).join("|")),
    climateControlDefaultTabs()
  );
}

function setClimateControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeClimateControlTabs((tabs || []).join("|"));
  b.options = climateControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, CLIMATE_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, CLIMATE_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeClimateOptions(b.options, true);
  return b.options;
}

function fanControlTabDefinitions() {
  return [
    { value: "power", label: "Power" },
    { value: "speed", label: "Speed" },
    { value: "preset", label: "Preset" },
    { value: "oscillation", label: "Oscillation" },
    { value: "direction", label: "Direction" },
  ];
}

function fanControlDefaultTabs() {
  return fanControlTabDefinitions().map(function (tab) { return tab.value; });
}

function normalizeFanControlTabs(value) {
  return normalizeTabList(
    value,
    fanControlTabDefinitions(),
    fanControlDefaultTabs(),
    "power"
  );
}

function fanControlTabs(b) {
  return normalizeFanControlTabs(configOptionValue(b && b.options, FAN_CONTROL_TABS_OPTION));
}

function fanControlTabsAreDefault(tabs) {
  return tabListIsDefault(
    normalizeFanControlTabs((tabs || []).join("|")),
    fanControlDefaultTabs()
  );
}

function normalizeFanControlOptions(options) {
  var tabs = normalizeFanControlTabs(configOptionValue(options, FAN_CONTROL_TABS_OPTION));
  return fanControlTabsAreDefault(tabs)
    ? ""
    : setConfigOptionValue("", FAN_CONTROL_TABS_OPTION, tabs.join("|"));
}

function setFanControlTabs(b, tabs) {
  if (!b) return "";
  tabs = normalizeFanControlTabs((tabs || []).join("|"));
  b.options = fanControlTabsAreDefault(tabs)
    ? setConfigOptionValue(b.options, FAN_CONTROL_TABS_OPTION, "")
    : setConfigOptionValue(b.options, FAN_CONTROL_TABS_OPTION, tabs.join("|"));
  b.options = normalizeFanControlOptions(b.options);
  return b.options;
}

function renderModalTabSettings(panel, b, helpers, config) {
  var section = document.createElement("div");
  panel.appendChild(section);

  b.options = config.normalizeOptions(b.options);
  var tabs = config.tabs(b);
  var definitions = config.definitions();
  var definitionByValue = {};
  definitions.forEach(function (definition) {
    definitionByValue[definition.value] = definition;
  });
  var orderedDefinitions = [];
  tabs.forEach(function (tab) {
    if (definitionByValue[tab]) orderedDefinitions.push(definitionByValue[tab]);
  });
  definitions.forEach(function (definition) {
    if (tabs.indexOf(definition.value) < 0) orderedDefinitions.push(definition);
  });

  if (!config.hideHeading) {
    var heading = document.createElement("div");
    heading.className = "sp-field";
    heading.appendChild(helpers.fieldLabel("Modal Tabs"));
    section.appendChild(heading);
  }

  var list = document.createElement("div");
  list.className = "sp-light-tab-list";
  section.appendChild(list);

  function listRows() {
    return Array.prototype.slice.call(list.querySelectorAll(".sp-light-tab-row"));
  }

  function saveTabsFromRows() {
    var nextTabs = [];
    listRows().forEach(function (row) {
      var input = row.querySelector("input[type=checkbox]");
      if (input && input.checked) nextTabs.push(row.getAttribute("data-tab"));
    });
    if (!nextTabs.length) return false;
    saveTabs(nextTabs);
    return true;
  }

  function saveTabs(nextTabs) {
    config.setTabs(b, nextTabs);
    b._modalSettingsOpen = true;
    helpers.saveField("options", b.options);
    renderButtonSettings();
  }

  function moveRow(row, direction) {
    var sibling = direction < 0 ? row.previousElementSibling : row.nextElementSibling;
    if (!sibling) return;
    if (direction < 0) {
      list.insertBefore(row, sibling);
    } else {
      list.insertBefore(sibling, row);
    }
    saveTabsFromRows();
  }

  orderedDefinitions.forEach(function (definition) {
    var tabIndex = tabs.indexOf(definition.value);
    var visible = tabIndex >= 0;

    var row = document.createElement("div");
    row.className = "sp-light-tab-row";
    row.setAttribute("data-tab", definition.value);
    row.draggable = true;

    var controls = document.createElement("div");
    controls.className = "sp-light-tab-controls";

    var drag = document.createElement("button");
    drag.type = "button";
    drag.className = "sp-light-tab-drag mdi mdi-drag";
    drag.setAttribute("aria-label", "Drag " + definition.label);
    drag.tabIndex = -1;

    var moveUp = document.createElement("button");
    moveUp.type = "button";
    moveUp.className = "sp-light-tab-move mdi mdi-chevron-up";
    moveUp.setAttribute("aria-label", "Move " + definition.label + " up");

    var moveDown = document.createElement("button");
    moveDown.type = "button";
    moveDown.className = "sp-light-tab-move mdi mdi-chevron-down";
    moveDown.setAttribute("aria-label", "Move " + definition.label + " down");

    controls.appendChild(drag);
    controls.appendChild(moveUp);
    controls.appendChild(moveDown);
    row.appendChild(controls);

    var label = document.createElement("label");
    label.className = "sp-light-tab-label";
    label.htmlFor = helpers.idPrefix + config.idPrefix + definition.value;
    label.textContent = definition.label;
    row.appendChild(label);

    var toggle = document.createElement("label");
    toggle.className = "sp-toggle";
    var input = document.createElement("input");
    input.type = "checkbox";
    input.id = helpers.idPrefix + config.idPrefix + definition.value;
    input.checked = visible;
    var track = document.createElement("span");
    track.className = "sp-toggle-track";
    toggle.appendChild(input);
    toggle.appendChild(track);
    row.appendChild(toggle);

    input.addEventListener("change", function () {
      if (!this.checked) {
        var visibleCount = listRows().filter(function (item) {
          var itemInput = item.querySelector("input[type=checkbox]");
          return itemInput && itemInput.checked;
        }).length;
        if (visibleCount < 1) {
          this.checked = true;
          return;
        }
      }
      saveTabsFromRows();
    });

    moveUp.addEventListener("click", function () { moveRow(row, -1); });
    moveDown.addEventListener("click", function () { moveRow(row, 1); });

    row.addEventListener("dragstart", function (event) {
      row.classList.add("sp-dragging");
      event.dataTransfer.effectAllowed = "move";
      event.dataTransfer.setData("text/plain", definition.value);
    });
    row.addEventListener("dragend", function () {
      row.classList.remove("sp-dragging");
    });
    row.addEventListener("dragover", function (event) {
      var dragging = list.querySelector(".sp-dragging");
      if (!dragging || dragging === row) return;
      event.preventDefault();
      var rect = row.getBoundingClientRect();
      var after = event.clientY > rect.top + rect.height / 2;
      list.insertBefore(dragging, after ? row.nextSibling : row);
    });
    row.addEventListener("drop", function (event) {
      event.preventDefault();
      saveTabsFromRows();
    });

    list.appendChild(row);
  });

  return section;
}
