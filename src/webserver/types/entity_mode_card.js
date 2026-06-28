function entityModeValues(cardType, optionName, fallbackModes) {
  var spec = cardContractOptionSpec(cardType, optionName);
  return spec && spec.values ? spec.values.slice() : fallbackModes.map(function (entry) { return entry[0]; });
}

function normalizeEntityMode(mode, values, fallback) {
  mode = String(mode || "");
  return values.indexOf(mode) >= 0 ? mode : fallback;
}

function entityModeCardUsesDefaultIcon(icon, icons) {
  if (!icon || icon === "Auto") return true;
  return icons.indexOf(icon) >= 0;
}

function normalizeEntityModeCardConfig(b, options) {
  if (!b) return;
  var mode = options.normalizeMode(b.sensor);
  b.sensor = mode;
  if (options.keepUnit && options.keepUnit(mode)) {
    b.unit = b.unit || "";
  } else {
    b.unit = "";
  }
  b.precision = "";
  b.options = "";
  b.icon_on = "Auto";
  if (!b.icon || b.icon === "Auto") b.icon = options.defaultIcon(mode);
}

function applyEntityModeCardModeChange(b, helpers, previousMode, nextMode, options) {
  var hadDefaultIcon = options.usesDefaultIcon(b.icon);
  b.sensor = nextMode;
  if (options.keepUnit && options.keepUnit(nextMode)) {
    b.unit = b.unit || "";
  } else {
    b.unit = "";
    helpers.saveField("unit", "");
  }
  b.precision = "";
  b.options = "";
  b.icon_on = "Auto";
  helpers.saveField("sensor", nextMode);
  helpers.saveField("precision", "");
  helpers.saveField("options", "");
  helpers.saveField("icon_on", "Auto");
  if (hadDefaultIcon || b.icon === options.defaultIcon(previousMode)) {
    b.icon = options.defaultIcon(nextMode);
    helpers.saveField("icon", b.icon);
  }
}
