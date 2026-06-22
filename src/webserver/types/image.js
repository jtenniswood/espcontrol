// Read-only Home Assistant camera/image entity card.
var IMAGE_CARD_METADATA = {
  entity: {
    label: "Camera Entity",
    idSuffix: "entity",
    placeholder: "e.g. camera.front_door",
    domains: function () { return cardContractDomains("image"); },
    bindName: "entity",
    rerender: true,
    requiredMessage: "Add a camera entity before saving.",
  },
};

function imageModalModeOptions() {
  return [
    ["fill", "Crop to fit"],
    ["fit", "Show full image"],
  ];
}

function renderImageLabelSettings(panel, b, helpers) {
  var labelToggle = helpers.toggleRow(
    "Show Label",
    helpers.idPrefix + "image-label-toggle",
    imageLabelEnabled(b)
  );
  panel.appendChild(labelToggle.row);

  var labelField = helpers.renderCardTextField(panel, b, helpers, {
      text: {
        label: "Label",
        idSuffix: "image-label",
        placeholder: "Uses entity name when blank",
        bindName: "label",
        rerender: true,
      },
  });

  var iconToggle = helpers.toggleRow(
    "Show Icon",
    helpers.idPrefix + "image-icon-toggle",
    imageIconEnabled(b)
  );
  panel.appendChild(iconToggle.row);

  if (imageIconEnabled(b) && (!b.icon || b.icon === "Auto")) b.icon = "Camera";
  var iconField = helpers.renderCardIconPicker(panel, b, helpers, {
    label: "Icon",
    idSuffix: "image-icon",
    pickerIdSuffix: "image-icon-picker",
    fallback: "Camera",
    value: function () { return b.icon && b.icon !== "Auto" ? b.icon : "Camera"; },
    onChange: function () { renderPreview(); },
  });
  iconField.classList.add("sp-cond-field");

  function syncLabelField() {
    labelField.field.hidden = !imageLabelEnabled(b);
  }

  function syncIconField() {
    iconField.classList.toggle("sp-visible", imageIconEnabled(b));
  }

  labelToggle.input.addEventListener("change", function () {
    setImageLabelEnabled(b, this.checked);
    helpers.saveField("options", b.options);
    helpers.saveField("label", b.label);
    syncLabelField();
    renderPreview();
  });
  iconToggle.input.addEventListener("change", function () {
    setImageIconEnabled(b, this.checked);
    if (this.checked && (!b.icon || b.icon === "Auto")) {
      b.icon = "Camera";
      helpers.saveField("icon", b.icon);
    } else if (!this.checked) {
      b.icon = "Auto";
      helpers.saveField("icon", b.icon);
    }
    helpers.saveField("options", b.options);
    syncIconField();
    renderPreview();
  });
  syncLabelField();
  syncIconField();
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

registerButtonType("image", {
  label: function () { return cardContractCardLabel("image"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("image"); },
  pickerKey: function () { return cardContractPickerKey("image"); },
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
    if (imageIconEnabled(b)) {
      if (!b.icon || b.icon === "Auto") b.icon = "Camera";
    } else {
      b.icon = "Auto";
    }
    b.icon_on = "Auto";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.options = normalizeImageOptions(b.options);
    if (!imageLabelEnabled(b)) b.label = "";
    helpers.renderCardEntityField(panel, b, helpers, IMAGE_CARD_METADATA);
    renderImageLabelSettings(panel, b, helpers);
    renderImageModalSettings(panel, b, helpers);
  },
  renderPreview: function (b, helpers) {
    var tertiaryColor = (typeof state !== "undefined" && state.sensorColor) ? state.sensorColor : "212121";
    var label = imageLabelEnabled(b) ? String((b && b.label) || "Camera").trim() : "";
    var iconName = b && b.icon && b.icon !== "Auto" ? iconSlug(b.icon) : "camera";
    var icon = imageIconEnabled(b) ? '<span class="sp-image-preview-icon mdi mdi-' + iconName + '"></span>' : "";
    return {
      buttonClass: "sp-image-card",
      iconHtml:
        '<span class="sp-image-preview" style="background:#' + helpers.escHtml(tertiaryColor) + '">' +
        icon +
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
