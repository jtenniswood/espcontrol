// Momentary trigger card: stored as "push" for config compatibility.
// Fires an esphome.push_button_pressed event with no toggle state.
var PUSH_CARD_METADATA = {
  icon: {
    pickerIdSuffix: "icon-picker",
    idSuffix: "icon",
    field: "icon",
    fallback: "Auto",
  },
  preview: {
    badge: "gesture-tap",
  },
};

function pushActionSpec() {
  var card = cardContractCard("push");
  return card && card.behavior && card.behavior.pushAction || {};
}

function pushDefaultIcon() {
  return pushActionSpec().defaultIcon || "Gesture Tap";
}

function pushDefaultIconOn() {
  return pushActionSpec().defaultIconOn || "Auto";
}

registerButtonType("push", {
  label: function () { return cardContractCardLabel("push"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("push"); },
  pickerKey: function () { return cardContractPickerKey("push"); },
  hidden: function () { return cardContractHidden("push"); },
  labelPlaceholder: "e.g. Doorbell",
  defaultConfig: function () { return cardContractDefaultConfig("push"); },
  cardMetadata: PUSH_CARD_METADATA,
  onSelect: function (b) {
    b.entity = ""; b.sensor = ""; b.unit = ""; b.icon_on = pushDefaultIconOn();
    b.icon = pushDefaultIcon();
  },
  renderSettings: function (panel, b, slot, helpers) {
    helpers.renderBasicCardFields(panel, b, helpers, PUSH_CARD_METADATA);
  },
  renderPreview: function (b, helpers) {
    var label = b.label || "Trigger";
    return cardBadgePreview(b, helpers, {
      label: label,
      iconFallback: pushDefaultIcon(),
      badge: PUSH_CARD_METADATA.preview.badge,
    });
  },
});
