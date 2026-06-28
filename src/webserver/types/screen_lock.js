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
    var defaults = cardContractDefaultConfig("screen_lock");
    Object.keys(defaults).forEach(function (key) { b[key] = defaults[key]; });
  },
  renderPreview: function (b, helpers) {
    return cardBadgePreview(b, helpers, {
      label: "Screen Unlocked",
      iconFallback: "Lock Open",
      badge: SCREEN_LOCK_CARD_METADATA.preview.badge,
    });
  },
});
