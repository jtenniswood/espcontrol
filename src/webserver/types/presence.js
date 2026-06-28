// Read-only presence card: shows a sensor where Detected is active and Clear is inactive.
var PRESENCE_CARD_METADATA = {
  entity: {
    label: "Sensor Entity",
    idSuffix: "sensor",
    placeholder: "e.g. binary_sensor.living_room_presence",
    domains: function () { return cardContractDomains("presence"); },
    bindName: "sensor",
    rerender: true,
    requiredMessage: "Add a presence sensor before saving.",
  },
  labelField: {
    label: "Label",
    idSuffix: "label",
    field: "label",
    placeholder: "e.g. Living Room",
    rerender: true,
  },
  iconOff: {
    pickerIdSuffix: "clear-icon-picker",
    idSuffix: "icon",
    field: "icon",
    label: "Clear Icon",
    fallback: "Motion Sensor Off",
  },
  iconOn: {
    pickerIdSuffix: "detected-icon-picker",
    idSuffix: "icon-on",
    field: "icon_on",
    label: "Detected Icon",
    fallback: "Motion Sensor",
  },
  activeColor: {
    label: "Lit When Detected",
    idSuffix: "presence-active-color",
    checked: presenceActiveColorEnabled,
  },
};

registerButtonType("presence", {
  label: function () { return cardContractCardLabel("presence"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("presence"); },
  pickerKey: function () { return cardContractPickerKey("presence"); },
  hidden: function () { return cardContractHidden("presence"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("presence"); },
  cardMetadata: PRESENCE_CARD_METADATA,
  onSelect: function (b) {
    var defaults = cardContractDefaultConfig("presence");
    Object.keys(defaults).forEach(function (key) { b[key] = defaults[key]; });
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.entity = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizePresenceOptions(b.options);
    if (!b.icon || b.icon === "Auto") b.icon = "Motion Sensor Off";
    if (!b.icon_on || b.icon_on === "Auto") b.icon_on = "Motion Sensor";

    helpers.renderBasicCardFields(panel, b, helpers, PRESENCE_CARD_METADATA);
    helpers.renderCardActiveColorToggle(panel, b, helpers,
      PRESENCE_CARD_METADATA.activeColor, setPresenceActiveColorEnabled);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || b.sensor || "Presence";
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: "Motion Sensor Off",
      badge: "motion-sensor",
    });
  },
});
