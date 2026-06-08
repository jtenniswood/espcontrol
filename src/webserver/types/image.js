// Read-only Home Assistant camera/image entity card.
var IMAGE_CARD_METADATA = {
  entity: {
    label: "Image Entity",
    idSuffix: "entity",
    placeholder: "e.g. camera.front_door or image.front_door",
    domains: function () { return cardContractDomains("image"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a camera or image entity before saving.",
  },
};

function imageRefreshIntervalOptions() {
  return [
    ["off", "Off"],
    ["10", "10 seconds"],
    ["30", "30 seconds"],
    ["60", "1 minute"],
    ["300", "5 minutes"],
  ];
}

function imageRefreshModeOptions() {
  return [
    ["changes_timer", "Changes + interval"],
    ["timer", "Interval only"],
  ];
}

function imageModalModeOptions() {
  return [
    ["fill", "Crop to fit"],
    ["fit", "Show full image"],
  ];
}

function renderImageLabelSettings(panel, b, helpers) {
  var toggle = helpers.toggleRow(
    "Show Label",
    helpers.idPrefix + "image-label-toggle",
    imageLabelEnabled(b)
  );
  panel.appendChild(toggle.row);

  var labelField = helpers.renderCardTextField(panel, b, helpers, {
      text: {
        label: "Label",
        idSuffix: "image-label",
        placeholder: "Uses entity name when blank",
        bindName: "label",
        rerender: true,
      },
  });

  function syncLabelField() {
    labelField.field.hidden = !imageLabelEnabled(b);
  }

  toggle.input.addEventListener("change", function () {
    setImageLabelEnabled(b, this.checked);
    helpers.saveField("options", b.options);
    helpers.saveField("label", b.label);
    syncLabelField();
    renderPreview();
  });
  syncLabelField();
}

function renderImageModalSettings(panel, b, helpers) {
  var modeField = helpers.selectField(
    "Expanded Image",
    helpers.idPrefix + "image-modal-mode",
    imageModalModeOptions(),
    imageModalMode(b)
  );
  panel.appendChild(modeField.field);
  modeField.select.addEventListener("change", function () {
    setImageModalMode(b, this.value);
    helpers.saveField("options", b.options);
  });
}

function renderImageRefreshSettings(panel, b, helpers) {
  var intervalField = helpers.selectField(
    "Refresh Interval",
    helpers.idPrefix + "image-refresh",
    imageRefreshIntervalOptions(),
    imageRefreshInterval(b)
  );
  panel.appendChild(intervalField.field);

  var modeField = helpers.selectField(
    "Refresh Mode",
    helpers.idPrefix + "image-refresh-mode",
    imageRefreshModeOptions(),
    imageRefreshMode(b)
  );
  panel.appendChild(modeField.field);

  function syncModeVisibility() {
    modeField.field.hidden = imageRefreshInterval(b) === "off";
    modeField.select.value = imageRefreshMode(b);
  }

  intervalField.select.addEventListener("change", function () {
    setImageRefreshInterval(b, this.value);
    helpers.saveField("options", b.options);
    syncModeVisibility();
  });
  modeField.select.addEventListener("change", function () {
    setImageRefreshMode(b, this.value);
    helpers.saveField("options", b.options);
  });
  syncModeVisibility();
}

registerButtonType("image", {
  label: function () { return cardContractCardLabel("image"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("image"); },
  pickerKey: function () { return cardContractPickerKey("image"); },
  experimental: function () { return cardContractExperimental("image"); },
  hidden: function () { return cardContractHidden("image"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("image"); },
  cardMetadata: IMAGE_CARD_METADATA,
  onSelect: function (b) {
    b.label = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeImageOptions(b.options);
  },
  renderSettings: function (panel, b, slot, helpers) {
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeImageOptions(b.options);
    if (!imageLabelEnabled(b)) b.label = "";
    helpers.renderCardEntityField(panel, b, helpers, IMAGE_CARD_METADATA);
    renderImageLabelSettings(panel, b, helpers);
    renderImageModalSettings(panel, b, helpers);
    renderImageRefreshSettings(panel, b, helpers);
  },
  renderPreview: function (b, helpers) {
    var tertiaryColor = (typeof state !== "undefined" && state.sensorColor) ? state.sensorColor : "212121";
    var label = imageLabelEnabled(b) ? String((b && b.label) || "Camera").trim() : "";
    return {
      buttonClass: "sp-image-card",
      iconHtml:
        '<span class="sp-image-preview" style="background:#' + helpers.escHtml(tertiaryColor) + '">' +
        '<span class="sp-image-preview-icon mdi mdi-image"></span>' +
        '</span>',
      labelHtml: label
        ? '<span class="sp-image-label"><span class="sp-image-label-stack">' +
          '<span class="sp-image-label-text sp-image-label-shadow" aria-hidden="true">' +
          helpers.escHtml(label) +
          '</span><span class="sp-image-label-text sp-image-label-main">' +
          helpers.escHtml(label) +
          '</span></span></span>'
        : "",
    };
  },
});
