// Local display card: toggles screen lock on the device without Home Assistant.
var SCREEN_LOCK_CARD_METADATA = {
  preview: {
    badge: "lock",
  },
};

registerButtonType("screen_lock", {
  label: function () { return cardContractCardLabel("screen_lock"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("screen_lock"); },
  pickerKey: function () { return cardContractPickerKey("screen_lock"); },
  hidden: function () { return cardContractHidden("screen_lock"); },
  hideLabel: true,
  labelPlaceholder: "e.g. Screen Lock",
  defaultConfig: function () { return cardContractDefaultConfig("screen_lock"); },
  cardMetadata: SCREEN_LOCK_CARD_METADATA,
  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = "";
    b.icon = "Lock";
    b.icon_on = "Lock Open";
  },
  renderPreview: function (b, helpers) {
    return cardBadgePreview(b, helpers, {
      label: "Screen Unlocked",
      iconFallback: "Lock Open",
      badge: SCREEN_LOCK_CARD_METADATA.preview.badge,
    });
  },
});
