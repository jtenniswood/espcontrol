// ── Subpage helpers ────────────────────────────────────────────────────
// @web-module-requires: model_generated, card_contract_generated, state, grid


function normalizeWithRegisteredCardType(b) {
  if (!b || typeof BUTTON_TYPES === "undefined") return false;
  var typeDef = BUTTON_TYPES[b.type || ""];
  if (!typeDef || typeof typeDef.normalizeConfig !== "function") return false;
  typeDef.normalizeConfig(b);
  return true;
}

function normalizeButtonConfig(b) {
  if (b) b.options = b.options || "";
  if (b && b.type === "local") {
    b.type = "action";
    b.sensor = ACTION_CARD_LOCAL_ACTION;
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto" || b.icon === "Flash") b.icon = "Gesture Tap";
  }
  if (b && b.type === "local_sensor") {
    b.type = "sensor";
    b.sensor = SENSOR_CARD_LOCAL_SENSOR;
    b.icon_on = "Auto";
    b.options = "";
    if (b.precision !== "text" && b.precision !== "1" && b.precision !== "2") b.precision = "";
    if (b.precision !== "text" && (!b.icon || b.icon === "Auto")) b.icon = "Auto";
  }
  if (b && b.type === "action" && b.sensor === "vacuum.start") {
    b.type = "vacuum";
    b.sensor = "start_stop";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Robot Vacuum";
  }
  if (b && b.type === "action" && b.sensor === "vacuum.return_to_base") {
    b.type = "vacuum";
    b.sensor = "dock";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Robot Vacuum Variant";
  }
  if (b && isBrightnessSliderType(b.type) && b.sensor) {
    b.sensor = "";
  }
  if (b && isFanCardType(b.type)) {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = b.type === "fan_control" ? normalizeFanControlOptions(b.options) : "";
    if (!b.icon || b.icon === "Auto") b.icon = fanCardDefaultIcon(b.type);
    if (b.type === "fan_switch") {
      if (!b.icon_on || b.icon_on === "Auto") b.icon_on = "Fan";
    } else {
      b.icon_on = "Auto";
    }
  }
  if (b && b.type === "weather_forecast") {
    var weatherAlias = cardContractMigrationAlias(b.type);
    b.type = weatherAlias && weatherAlias.type || "weather";
    b.precision = weatherAlias && weatherAlias.precision || "tomorrow";
    if (b.label === "Weather") b.label = "";
  }
  if (b && b.type === "weather") {
    b.sensor = "";
    b.precision = normalizeWeatherCardMode(b.precision);
    b.options = cardLargeNumbersSupported(b) ? copyLargeNumbersOption("", b.options) : "";
  }
  if (b && b.type === "text_sensor") {
    var textSensorAlias = cardContractMigrationAlias(b.type);
    b.type = textSensorAlias && textSensorAlias.type || "sensor";
    b.precision = textSensorAlias && textSensorAlias.precision || "text";
    b.entity = "";
    b.label = "";
    b.unit = "";
    b.icon_on = "Auto";
    if (!b.icon) b.icon = "Auto";
  }
  if (b && b.type === "media") {
    var rawMediaMode = b.sensor;
    if (rawMediaMode === "controls") {
      if (!b.icon || b.icon === "Speaker") b.icon = "Auto";
    }
    b.sensor = mediaEditorMode(b.sensor);
    if (b.sensor === "previous" && b.label === "Skip Previous") b.label = "Previous";
    if (b.sensor === "next" && b.label === "Skip Next") b.label = "Next";
    if (b.sensor === "volume") {
      if (!b.label || b.label === "Media") b.label = "Volume";
      b.icon = "Auto";
    }
    if (b.sensor === "playlist") {
      if (!b.label || b.label === "Media") b.label = "Playlist";
      if (!b.icon || b.icon === "Auto") b.icon = "Music";
    }
    if (b.sensor === "position" && (!b.label || b.label === "Track")) b.label = "Position";
    if (b.sensor === "now_playing") {
      b.precision = mediaNowPlayingControls(b);
    } else if (mediaStateDisplayModeSupported(b.sensor) && b.precision === "state") {
      b.precision = "state";
    } else {
      b.precision = "";
    }
    b.options = normalizeMediaOptions(b.options, b.sensor);
  }
  if (b && isClimateCardType(b.type)) {
    b.type = "climate_control";
    b.sensor = "";
    b.unit = "";
    if (!b.icon) b.icon = "Thermostat";
    if (!b.icon_on) b.icon_on = "Auto";
    b.precision = normalizeClimatePrecisionConfig(b.precision);
    b.options = normalizeClimateOptions(b.options, true);
  }
  if (b && b.type === "garage") {
    if (b.sensor !== "open" && b.sensor !== "close") b.sensor = "";
    b.unit = "";
    b.precision = "";
    if (b.sensor === "open" || b.sensor === "close") b.icon_on = "Auto";
    b.options = normalizeGarageOptions(b.options, b.sensor);
  }
  if (b && b.type === "gate") {
    if (b.sensor !== "open" && b.sensor !== "close" && b.sensor !== "stop") b.sensor = "";
    b.unit = "";
    b.precision = "";
    if (b.sensor === "open" || b.sensor === "close" || b.sensor === "stop") b.icon_on = "Auto";
    b.options = normalizeGateOptions(b.options, b.sensor);
  }
  if (b && b.type === "cover") {
    b.sensor = normalizeCoverMode(b.sensor, true);
    b.options = normalizeCoverOptionsForMode(b.options, b.sensor);
  }
  if (b && b.type === "lock") {
    b.sensor = (b.sensor === "lock" || b.sensor === "unlock") ? b.sensor : "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon_on = b.sensor ? "Auto" : ((!b.icon_on || b.icon_on === "Auto") ? "Lock Open" : b.icon_on);
  }
  if (b && b.type === "alarm") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Security";
    b.options = normalizeAlarmOptions(b.options);
  }
  if (b && b.type === "alarm_action") {
    b.sensor = alarmActionInfo(b.sensor) ? b.sensor : "away";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.label) b.label = alarmActionInfo(b.sensor).label;
    if (!b.icon || b.icon === "Auto" || b.icon === alarmActionLegacyIcon(b.sensor)) {
      b.icon = alarmActionInfo(b.sensor).icon;
    }
    b.options = normalizeAlarmOptions(b.options);
  }
  if (b && b.type === "webhook") {
    if (typeof normalizeWebhookConfig === "function") normalizeWebhookConfig(b);
  }
  normalizeWithRegisteredCardType(b);
  if (b && b.type === "screen_lock") {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon = "Lock";
    b.icon_on = "Lock Open";
  }
  if (b && b.type === "calendar") {
    if (!b.entity) b.entity = cardContractDefaultConfig("calendar").entity;
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = b.precision === "datetime" ? "datetime" : "";
    b.options = normalizeDateTimeOptions("calendar", b.options, b.precision);
  }
  if (b && b.type === "clock") {
    b.entity = "";
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeDateTimeOptions("clock", b.options, b.precision);
  }
  if (b && b.type === "timezone") {
    if (!b.entity) b.entity = cardContractDefaultConfig("timezone").entity;
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeDateTimeOptions("timezone", b.options, b.precision);
  }
  if (b && b.type === "todo") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    if (!b.icon || b.icon === "Auto") b.icon = "Check";
    b.options = normalizeTodoOptions(b.options);
  }
  if (b && b.type === "image") {
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeImageOptions(b.options);
    b.icon = imageIconEnabled(b) ? (b.icon && b.icon !== "Auto" ? b.icon : "Camera") : "Auto";
    if (!imageLabelEnabled(b)) b.label = "";
  }
  if (b && b.type === "light_switch") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
  }
  if (b && b.type === "light_control") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeLightControlOptions(b.options);
  }
  if (b && b.type === "fan_control") {
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeFanControlOptions(b.options);
  }
  if (b && b.type === "subpage") {
    applySubpagePresetConfig(b);
    b.options = normalizeSubpageOptions(b.options, b.sensor, b.precision);
  }
  if (b && b.type === "option_select") {
    b.type = "action";
    b.sensor = ACTION_CARD_OPTION_SELECT_ACTION;
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = "";
    if (!b.icon || b.icon === "Auto" || b.icon === "Chevron Down") b.icon = "Flash";
  }
  if (b && actionCardIsOptionSelect(b)) {
    b.sensor = ACTION_CARD_OPTION_SELECT_ACTION;
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = "";
    if (!b.icon || b.icon === "Auto" || b.icon === "Chevron Down") b.icon = "Flash";
  } else if (b && actionCardIsLocal(b)) {
    b.unit = "";
    b.precision = "";
    b.icon_on = "Auto";
    b.options = "";
    if (!b.icon || b.icon === "Auto" || b.icon === "Flash") b.icon = "Gesture Tap";
  } else if (b && b.type === "action") {
    b.options = normalizeActionOptions(b.options, b.sensor);
  }
  if (b && sensorCardIsLocal(b)) {
    b.type = "sensor";
    b.sensor = SENSOR_CARD_LOCAL_SENSOR;
    b.icon_on = "Auto";
    b.options = "";
    if (b.precision !== "text" && b.precision !== "1" && b.precision !== "2") b.precision = "";
    if (b.precision !== "text" && (!b.icon || b.icon === "Auto")) b.icon = "Auto";
  } else if (b && !b.type) {
    b.options = normalizeSwitchConfirmationOptions(b.options);
  } else if (b && b.type === "sensor") {
    b.options = normalizeSensorOptions(b.options, b.precision);
  } else if (b && b.type === "door_window") {
    b.entity = "";
    b.unit = "";
    b.precision = normalizeDoorWindowSubtype(b.precision);
    if (!b.icon || b.icon === "Auto") b.icon = doorWindowClosedIcon(b.precision);
    if (!b.icon_on || b.icon_on === "Auto") b.icon_on = doorWindowOpenIcon(b.precision);
    b.options = normalizeDoorWindowOptions(b.options);
  } else if (b && b.type === "presence") {
    b.entity = "";
    b.unit = "";
    b.precision = "";
    if (!b.icon || b.icon === "Auto") b.icon = "Motion Sensor Off";
    if (!b.icon_on || b.icon_on === "Auto") b.icon_on = "Motion Sensor";
    b.options = normalizePresenceOptions(b.options);
  } else if (b && b.type !== "action" && b.type !== "alarm" && b.type !== "alarm_action" && !isClimateCardType(b.type) && b.type !== "cover" && b.type !== "garage" && b.type !== "gate" && b.type !== "webhook" && b.type !== "screen_lock" && b.type !== "todo" && b.type !== "media" && b.type !== "presence" && b.type !== "subpage" && b.type !== "image" && b.type !== "light_control" && b.type !== "vacuum" && b.type !== "lawn_mower" && !isFanCardType(b.type) && !cardLargeNumbersSupported(b)) {
    b.options = "";
  }
  return b;
}

function isBrightnessSliderType(type) {
  return cardContractIsBrightnessSliderType(type);
}

function isFanCardType(type) {
  return cardContractIsFanCardType(type);
}

function isClimateCardType(type) {
  return type === "climate" || type === "climate_control";
}

function isOptionSelectType(type) {
  return cardContractIsOptionSelectType(type);
}

function fanCardDefaultIcon(type) {
  return cardContractFanDefaultIcon(type);
}











function buttonConfigChangedByNormalize(raw) {
  var before = EspControlModel.cloneCardConfig(raw || {});
  var after = normalizeButtonConfig(EspControlModel.cloneCardConfig(before));
  return EspControlModel.cardConfigChanged(before, after);
}

function trimConfigFields(fields) {
  return EspControlModel.trimConfigFields(fields);
}

function buttonConfigFields(b) {
  var type = b && b.type || "";
  if (b && type === "subpage" && subpageKind(b)) {
    b = EspControlModel.cloneCardConfig(b);
    applySubpagePresetConfig(b);
  }
  var isActionOptionSelect = !!(b && (actionCardIsOptionSelect(b) || isOptionSelectType(type)));
  if (isActionOptionSelect) type = "action";
  if (type === "local") type = "action";
  if (type === "local_sensor") type = "sensor";
  var label = b && b.label || "";
  if (type === "calendar" || type === "clock" || type === "timezone") label = "";
  if (type === "screen_lock") label = "";
  var sensor = isActionOptionSelect ? ACTION_CARD_OPTION_SELECT_ACTION :
    (isBrightnessSliderType(type) || type === "calendar" || type === "clock" || isClimateCardType(type) || type === "light_switch" || type === "light_control" || type === "alarm" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.sensor || "");
  if (type === "lock" && sensor !== "lock" && sensor !== "unlock") sensor = "";
  if (b && b.type === "local") sensor = ACTION_CARD_LOCAL_ACTION;
  if (b && (b.type === "local_sensor" || sensorCardIsLocal(b))) sensor = SENSOR_CARD_LOCAL_SENSOR;
  var isLocalAction = type === "action" && sensor === ACTION_CARD_LOCAL_ACTION;
  var unit = (isActionOptionSelect || type === "calendar" || type === "clock" || isClimateCardType(type) || type === "light_switch" || type === "light_control" || type === "alarm" || type === "alarm_action" || type === "lock" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.unit || "");
  if (isLocalAction) unit = "";
  var icon = b && b.icon || "Auto";
  if (isActionOptionSelect && (!icon || icon === "Auto" || icon === "Chevron Down")) icon = "Flash";
  if (isLocalAction && (!icon || icon === "Auto" || icon === "Flash")) icon = "Gesture Tap";
  if (type === "alarm" && (!icon || icon === "Auto")) icon = "Security";
  if (type === "calendar" || type === "clock" || type === "timezone") icon = "Auto";
  if (type === "screen_lock") icon = "Lock";
  if (type === "alarm_action" && (!icon || icon === "Auto")) icon = (alarmActionInfo(sensor) || alarmActionSpecs()[0]).icon;
  if (isFanCardType(type) && (!icon || icon === "Auto")) icon = fanCardDefaultIcon(type);
  var iconOn = (isActionOptionSelect || type === "alarm" || type === "alarm_action" || (isFanCardType(type) && type !== "fan_switch")) ? "Auto" : (b && b.icon_on || "Auto");
  if (type === "calendar" || type === "clock" || type === "timezone") iconOn = "Auto";
  if (isLocalAction) iconOn = "Auto";
  if (type === "fan_switch" && (!iconOn || iconOn === "Auto")) iconOn = "Fan";
  if (type === "lock") iconOn = sensor ? "Auto" : ((!iconOn || iconOn === "Auto") ? "Lock Open" : iconOn);
  if (type === "screen_lock") iconOn = "Lock Open";
  var precision = (isActionOptionSelect || type === "clock" || type === "light_switch" || type === "light_control" || type === "alarm" || type === "alarm_action" || type === "lock" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.precision || "");
  if (isLocalAction) precision = "";
  if (sensor === SENSOR_CARD_LOCAL_SENSOR && precision !== "text" && precision !== "1" && precision !== "2") precision = "";
  if (type === "media") {
    sensor = mediaEditorMode(sensor);
    precision = sensor === "now_playing"
      ? mediaNowPlayingControls({ sensor: sensor, precision: precision })
      : (mediaStateDisplayModeSupported(sensor) && precision === "state" ? "state" : "");
  }
  if (type === "vacuum") {
    sensor = normalizeVacuumMode(sensor);
    unit = (vacuumModeNeedsArea(sensor) || vacuumModalMode(sensor)) ? unit : "";
    precision = "";
    iconOn = "Auto";
    if (!icon || icon === "Auto") icon = vacuumModeDefaultIcon(sensor);
  }
  if (type === "lawn_mower") {
    sensor = normalizeLawnMowerMode(sensor);
    unit = "";
    precision = "";
    iconOn = "Auto";
    if (!icon || icon === "Auto") icon = lawnMowerModeDefaultIcon(sensor);
  }
  if (isClimateCardType(type)) precision = normalizeClimatePrecisionConfig(precision);
  if (type === "calendar" && precision !== "datetime") precision = "";
  if (type === "weather") {
    sensor = "";
    precision = normalizeWeatherCardMode(precision);
  }
  if (type === "todo") {
    sensor = "";
    unit = "";
    precision = "";
    iconOn = "Auto";
    if (!icon || icon === "Auto") icon = "Check";
  }
  if (type === "image") {
    iconOn = "Auto";
    sensor = "";
    unit = "";
    precision = "";
    if (!imageLabelEnabled(b)) label = "";
  }
  if (type === "door_window") precision = normalizeDoorWindowSubtype(precision);
  var options = b && b.options || "";
  if (type === "") {
    options = normalizeSwitchConfirmationOptions(options);
  } else if (type === "alarm" || type === "alarm_action") {
    options = normalizeAlarmOptions(options);
  } else if (type === "garage") {
    options = normalizeGarageOptions(options, sensor);
  } else if (type === "gate") {
    options = normalizeGateOptions(options, sensor);
  } else if (type === "cover") {
    sensor = normalizeCoverMode(sensor, true);
    options = normalizeCoverOptionsForMode(options, sensor);
  } else if (isClimateCardType(type)) {
    type = "climate_control";
    options = normalizeClimateOptions(options, true);
  } else if (type === "media") {
    options = normalizeMediaOptions(options, sensor);
  } else if (type === "weather") {
    options = cardLargeNumbersSupported({ type: type, precision: precision }) ? copyLargeNumbersOption("", options) : "";
  } else if (type === "subpage") {
    options = normalizeSubpageOptions(options, sensor, precision);
  } else if (type === "webhook" && typeof normalizeWebhookConfig === "function") {
    var webhookButton = EspControlModel.cloneCardConfig(b || {});
    normalizeWebhookConfig(webhookButton);
    sensor = webhookButton.sensor;
    unit = webhookButton.unit;
    iconOn = webhookButton.icon_on || "Auto";
    precision = webhookButton.precision || "";
    options = webhookButton.options || "";
  } else if (type === "lock" || type === "screen_lock") {
    options = "";
  } else if (type === "calendar" || type === "clock" || type === "timezone") {
    options = normalizeDateTimeOptions(type, options, precision);
  } else if (type === "vacuum") {
    options = normalizeVacuumOptions(options, sensor);
  } else if (type === "lawn_mower") {
    options = "";
  } else if (type === "todo") {
    options = normalizeTodoOptions(options);
  } else if (type === "sensor") {
    options = sensor === SENSOR_CARD_LOCAL_SENSOR ? "" : normalizeSensorOptions(options, precision);
  } else if (type === "door_window") {
    options = normalizeDoorWindowOptions(options);
  } else if (type === "presence") {
    options = normalizePresenceOptions(options);
  } else if (type === "image") {
    options = normalizeImageOptions(options);
  } else if (type === "light_control") {
    options = normalizeLightControlOptions(options);
  } else if (type === "fan_control") {
    options = normalizeFanControlOptions(options);
  } else if (type === "action") {
    options = sensor === ACTION_CARD_LOCAL_ACTION ? "" : normalizeActionOptions(options, sensor);
  } else if (isActionOptionSelect || isFanCardType(type)) {
    options = "";
  } else if (type !== "action" && type !== "alarm_action" && !isClimateCardType(type) && type !== "cover" && type !== "garage" && type !== "gate" && type !== "webhook" && type !== "screen_lock" && type !== "media" && type !== "presence" && type !== "light_control" && type !== "fan_control" && !cardLargeNumbersSupported({ type: type, precision: precision })) {
    options = "";
  }
  if (type === "image") {
    icon = configOptionEnabled(options, IMAGE_ICON_OPTION)
      ? (icon && icon !== "Auto" ? icon : "Camera")
      : "Auto";
  }
  if (type === "door_window") {
    b = b || {};
    b.entity = "";
    unit = "";
    if (!icon || icon === "Auto") icon = doorWindowClosedIcon(precision);
    if (!iconOn || iconOn === "Auto") iconOn = doorWindowOpenIcon(precision);
  }
  if (type === "presence") {
    b = b || {};
    b.entity = "";
    unit = "";
    precision = "";
    if (!icon || icon === "Auto") icon = "Motion Sensor Off";
    if (!iconOn || iconOn === "Auto") iconOn = "Motion Sensor";
  }
  if (type === "calendar") {
    b = b || {};
    if (!b.entity) b.entity = cardContractDefaultConfig("calendar").entity;
  }
  if (type === "clock") {
    b = b || {};
    b.entity = "";
  }
  if (type === "timezone") {
    b = b || {};
    if (!b.entity) b.entity = cardContractDefaultConfig("timezone").entity;
  }
  if (!type && !sensor) {
    unit = "";
    precision = "";
  }
  return trimConfigFields([
    (type === "door_window" || type === "presence" || type === "screen_lock") ? "" : (b && b.entity || ""),
    label,
    icon,
    iconOn,
    sensor,
    unit,
    type,
    precision,
    options,
  ]);
}

function encodeConfigField(value) {
  return EspControlModel.encodeConfigField(value);
}

function decodeConfigField(value) {
  return EspControlModel.decodeConfigField(value);
}

function legacyButtonConfigSafe(fields) {
  return EspControlModel.legacyButtonConfigSafe(fields);
}

function serializeButtonConfig(b) {
  var fields = buttonConfigFields(b || {});
  if (legacyButtonConfigSafe(fields)) return fields.join(";");
  return "~" + fields.map(encodeConfigField).join(",");
}

function parseRawButtonConfig(str) {
  return EspControlModel.parseRawButtonConfig(str);
}

function parseButtonConfig(str) {
  return normalizeButtonConfig(parseRawButtonConfig(str));
}

function hasLegacySliderDirection(b) {
  return !!(b && isBrightnessSliderType(b.type) && b.sensor);
}

function buttonConfigHasLegacySliderDirection(str) {
  return hasLegacySliderDirection(parseRawButtonConfig(str || ""));
}

function buttonConfigNeedsMigration(str) {
  return buttonConfigChangedByNormalize(parseRawButtonConfig(str || ""));
}

function parseBackOrderToken(value) {
  return EspControlModel.parseBackOrderToken(value);
}

function backOrderToken(baseToken, label) {
  return EspControlModel.backOrderToken(baseToken, label);
}

function backLabelFromOrder(order) {
  return EspControlModel.backLabelFromOrder(order);
}

function parseSubpageOrder(orderStr) {
  return EspControlModel.parseSubpageOrder(orderStr);
}

function subpageOrderForSerialize(sp) {
  return EspControlModel.subpageOrderForSerialize((sp && sp.order) || [], sp && sp.backLabel);
}

function subpageSerializedOrder(sp) {
  if (!sp) return [];
  if (sp.order && sp.order.length) return subpageOrderForSerialize(sp);
  if (sp.grid && sp.grid.length) return serializeSubpageGrid(sp);
  return [];
}

function parseSubpageConfig(str, raw) {
  var parsed = EspControlModel.parseRawSubpageConfig(str, subpageTypeFromCode);
  if (raw) return parsed;
  var compactButtonTokens = String(str || "").charAt(0) === "~"
    ? String(str || "").split("|").slice(1)
    : [];
  parsed.buttons = parsed.buttons.map(function (button, index) {
    var normalized = normalizeButtonConfig(button);
    if (button && button.type === "calendar" && (!button.entity || compactButtonTokens[index] === "D")) normalized.entity = "";
    return normalized;
  });
  return parsed;
}

function subpageTypeCode(type) {
  return cardContractSubpageTypeCode(type);
}

function subpageTypeFromCode(code) {
  return cardContractSubpageTypeFromCode(code);
}

function encodeSubpageField(value) {
  return encodeConfigField(value);
}

function decodeSubpageField(value) {
  return decodeConfigField(value);
}

function parseCompactSubpageConfig(str, raw) {
  var parsed = EspControlModel.parseCompactSubpageConfig(str, subpageTypeFromCode);
  if (raw) return parsed;
  var compactButtonTokens = String(str || "").split("|").slice(1);
  parsed.buttons = parsed.buttons.map(function (button, index) {
    var normalized = normalizeButtonConfig(button);
    if (button && button.type === "calendar" && compactButtonTokens[index] === "D") normalized.entity = "";
    return normalized;
  });
  return parsed;
}

function subpageConfigHasLegacySliderDirection(str) {
  var sp = parseSubpageConfig(str, true);
  for (var i = 0; i < sp.buttons.length; i++) {
    if (hasLegacySliderDirection(sp.buttons[i])) return true;
  }
  return false;
}

function subpageConfigNeedsMigration(str) {
  var sp = parseSubpageConfig(str, true);
  for (var i = 0; i < sp.buttons.length; i++) {
    if (buttonConfigChangedByNormalize(sp.buttons[i])) return true;
  }
  return false;
}

function serializeSubpageConfig(sp) {
  var order = subpageSerializedOrder(sp);
  var legacy = legacySubpageConfigSafe(sp) ? serializeLegacySubpageConfig(sp) : "";
  var compact = serializeCompactSubpageConfig(sp);
  return EspControlModel.chooseSerializedSubpageConfig(
    order,
    sp && sp.buttons ? sp.buttons.length : 0,
    legacy,
    compact
  );
}

function subpageLegacyButtonFields(b) {
  var fields = buttonConfigFields(b || {});
  if (fields.length > 1 && fields[fields.length - 1] === "Auto") {
    while (fields.length > 1 && (fields[fields.length - 1] === "Auto" || !fields[fields.length - 1])) fields.pop();
  }
  return fields;
}

function subpageCompactButtonFields(b) {
  var fields = buttonConfigFields(b || {});
  var compact = [
    subpageTypeCode(fields[6] || ""),
    encodeSubpageField(fields[0]),
    encodeSubpageField(fields[1]),
    fields[2] && fields[2] !== "Auto" ? encodeSubpageField(fields[2]) : "",
    fields[3] && fields[3] !== "Auto" ? encodeSubpageField(fields[3]) : "",
    encodeSubpageField(fields[4]),
    encodeSubpageField(fields[5]),
    encodeSubpageField(fields[7]),
    encodeSubpageField(fields[8]),
  ];
  while (compact.length > 1 && !compact[compact.length - 1]) compact.pop();
  return compact;
}

function legacySubpageConfigSafe(sp) {
  var fields = ((sp && sp.buttons) || []).map(subpageLegacyButtonFields);
  return EspControlModel.legacySubpageFieldsSafe(fields);
}

function serializeLegacySubpageConfig(sp) {
  if (!sp) return "";
  return EspControlModel.serializeLegacySubpageConfig(
    subpageSerializedOrder(sp),
    ((sp && sp.buttons) || []).map(subpageLegacyButtonFields)
  );
}

function serializeCompactSubpageConfig(sp) {
  if (!sp || !sp.buttons || sp.buttons.length === 0) return "";
  return EspControlModel.serializeCompactSubpageConfig(
    subpageSerializedOrder(sp),
    sp.buttons.map(subpageCompactButtonFields)
  );
}

function applySubpageRaw(slot) {
  var raw = state.subpageRaw[slot];
  var combined = (raw && raw.main || "") + (raw && raw.ext || "") +
    (raw && raw.ext2 || "") + (raw && raw.ext3 || "") +
    (raw && raw.ext4 || "") + (raw && raw.ext5 || "") +
    (raw && raw.ext6 || "") + (raw && raw.ext7 || "");
  var pending = state.subpageSavePending[slot];
  if (pending) {
    if (combined !== pending) {
      if (state.editingSubpage === slot) scheduleRender();
      return;
    }
    delete state.subpageSavePending[slot];
  }
  var local = state.subpages[slot];
  var localHasData = local && (
    (local.buttons && local.buttons.length > 0) ||
    (local.order && local.order.length > 0)
  );
  if (state.editingSubpage === slot && localHasData) {
    var localSerialized = serializeSubpageConfig(local);
    if (combined !== localSerialized) {
      scheduleRender();
      return;
    }
  }
  if (combined) {
    var migrateConfig = subpageConfigNeedsMigration(combined);
    var sp = parseSubpageConfig(combined);
    sp.sizes = sp.sizes || {};
    buildSubpageGrid(sp);
    state.subpages[slot] = sp;
    if (migrateConfig) scheduleSliderSubpageMigration(slot);
  } else {
    delete state.subpages[slot];
  }
  if (state.editingSubpage === slot) {
    scheduleRender();
  }
}

function getSubpage(homeSlot) {
  if (!state.subpages[homeSlot]) {
    state.subpages[homeSlot] = { order: [], buttons: [], grid: [], sizes: {}, backLabel: "Back" };
  } else if (!state.subpages[homeSlot].backLabel) {
    state.subpages[homeSlot].backLabel = backLabelFromOrder(state.subpages[homeSlot].order);
  }
  return state.subpages[homeSlot];
}

function buildSubpageGrid(sp) {
  var result = EspControlModel.buildSubpageGrid(sp, NUM_SLOTS, GRID_COLS);
  sp.grid = result.grid;
  sp.sizes = result.sizes;
  return sp.grid;
}

function serializeSubpageGrid(sp) {
  return EspControlModel.serializeSubpageGrid(sp.grid, sp.sizes || {}, sp.backLabel || "Back");
}

function enterSubpage(homeSlot) {
  state.editingSubpage = homeSlot;
  state.subpageSelectedSlots = [];
  state.subpageLastClicked = -1;
  var sp = getSubpage(homeSlot);
  buildSubpageGrid(sp);
  renderPreview();
  renderButtonSettings();
}

function exitSubpage() {
  state.editingSubpage = null;
  state.subpageSelectedSlots = [];
  state.subpageLastClicked = -1;
  renderPreview();
  renderButtonSettings();
}

function saveSubpageConfig(homeSlot) {
  var sp = getSubpage(homeSlot);
  sp.order = serializeSubpageGrid(sp);
  saveSubpageEntity(homeSlot);
}

function subpageFirstFreeSlot(sp) {
  var used = {};
  sp.grid.forEach(function (s) { if (s > 0) used[s] = true; });
  for (var i = 1; i <= sp.buttons.length + 1; i++) {
    if (!used[i]) return i;
  }
  return sp.buttons.length + 1;
}

function bindTextPost(input, postName, opts) {
  input.addEventListener("blur", function () {
    if (opts && opts.onBlur) opts.onBlur(this.value);
    if (opts && opts.post) opts.post(this.value);
    else postText(postName, this.value);
    if (opts && opts.rerender) renderPreview();
  });
  input.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });
}
