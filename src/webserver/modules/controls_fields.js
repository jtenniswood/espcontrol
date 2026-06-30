// ── Settings helpers ───────────────────────────────────────────────────

function makeCollapsibleCard(title, bodyElement, defaultCollapsed, badgeElement, actionElement) {
  var card = document.createElement("div");
  card.className = "card";
  var header = document.createElement("div");
  header.className = "card-header";
  var h3 = document.createElement("h3");
  h3.textContent = title;
  var rightWrap = document.createElement("div");
  rightWrap.className = "card-header-right";
  var chevron = createDisclosureChevron("card-chevron");
  if (badgeElement) rightWrap.appendChild(badgeElement);
  if (actionElement) rightWrap.appendChild(actionElement);
  rightWrap.appendChild(chevron);
  header.appendChild(h3);
  header.appendChild(rightWrap);
  var body = document.createElement("div");
  body.className = "card-body";
  body.appendChild(bodyElement);
  card.appendChild(header);
  card.appendChild(body);
  if (defaultCollapsed) card.classList.add("collapsed");
  header.onclick = function () { card.classList.toggle("collapsed"); };
  return card;
}

function fieldLabel(text, forId) {
  var el = document.createElement("label");
  el.className = "sp-field-label";
  el.textContent = text;
  if (forId) el.htmlFor = forId;
  return el;
}

function textInput(id, value, placeholder) {
  var el = document.createElement("input");
  el.type = "text";
  el.className = "sp-input";
  if (id) el.id = id;
  el.value = value;
  el.placeholder = placeholder || "";
  return el;
}

function colorField(id, value, onChange) {
  var row = document.createElement("div");
  row.className = "sp-color-row";

  var swatch = document.createElement("div");
  swatch.className = "sp-color-swatch";
  swatch.style.backgroundColor = "#" + (value.length === 6 ? value : "000000");

  var picker = document.createElement("input");
  picker.type = "color";
  picker.value = "#" + (value.length === 6 ? value : "000000");
  swatch.appendChild(picker);
  row.appendChild(swatch);

  var inp = document.createElement("input");
  inp.type = "text";
  inp.className = "sp-input";
  inp.id = id;
  inp.value = value;
  inp.placeholder = "6-digit hex e.g. 0073FF";
  row.appendChild(inp);

  picker.addEventListener("input", function () {
    var hex = this.value.replace("#", "").toUpperCase();
    inp.value = hex;
    swatch.style.backgroundColor = "#" + hex;
    onChange(hex);
  });

  inp.addEventListener("blur", function () {
    var hex = this.value.replace(/^#/, "").toUpperCase();
    if (/^[0-9A-F]{6}$/i.test(hex)) {
      swatch.style.backgroundColor = "#" + hex;
      picker.value = "#" + hex;
    }
    onChange(hex);
  });
  inp.addEventListener("keydown", function (e) { if (e.key === "Enter") this.blur(); });

  row._syncColor = function (hex) {
    if (document.activeElement !== inp) inp.value = hex;
    swatch.style.backgroundColor = "#" + (hex.length === 6 ? hex : "000000");
    picker.value = "#" + (hex.length === 6 ? hex : "000000");
  };

  return row;
}

function toggleRow(label, id, checked) {
  var row = document.createElement("div");
  row.className = "sp-toggle-row";
  var lbl = document.createElement("label");
  lbl.className = "sp-toggle-label";
  lbl.htmlFor = id;
  lbl.textContent = label;
  row.appendChild(lbl);
  var toggle = document.createElement("label");
  toggle.className = "sp-toggle";
  var inp = document.createElement("input");
  inp.type = "checkbox";
  inp.id = id;
  inp.checked = !!checked;
  var track = document.createElement("span");
  track.className = "sp-toggle-track";
  toggle.appendChild(inp);
  toggle.appendChild(track);
  row.appendChild(toggle);
  return { row: row, input: inp };
}

function cardMetadataValue(value, b, helpers) {
  return typeof value === "function" ? value(b, helpers) : value;
}

function cardLargeNumbersSupportsCardSize(b, helpers, metadata) {
  helpers = helpers || {};
  metadata = metadata || {};
  var large = metadata.largeNumbers || {};
  var cardSize = helpers.cardSize || 1;
  if (large.supportedCardSizes) {
    return large.supportedCardSizes.indexOf(cardSize) !== -1;
  }
  if (large.supportedCardSize) {
    return !!cardMetadataValue(large.supportedCardSize, b, helpers);
  }
  return cardSize === CARD_SIZE_LARGE;
}

function cardLargeNumbersMetadata(b) {
  var typeDef = BUTTON_TYPES[(b && b.type) || ""] || null;
  return typeDef && typeDef.cardMetadata ? typeDef.cardMetadata : {};
}

function cardLargeNumbersActiveForCardSize(b, helpers, metadata) {
  helpers = helpers || {};
  if (!cardLargeNumbersSupported(b) ||
      !cardLargeNumbersSupportsCardSize(b, helpers, metadata || cardLargeNumbersMetadata(b))) {
    return false;
  }
  if (largeNumbersExplicitlyDisabled(b && b.options)) return false;
  return (helpers.cardSize || CARD_SIZE_SINGLE) === CARD_SIZE_LARGE || cardLargeNumbersEnabled(b);
}

function cardLargeNumbersHidePreviewLabel(b, helpers, metadata) {
  if (!cardLargeNumbersActiveForCardSize(b, helpers, metadata)) return false;
  metadata = metadata || cardLargeNumbersMetadata(b);
  var large = metadata.largeNumbers || {};
  var cardSize = (helpers && helpers.cardSize) || 1;
  if (large.hideLabelCardSizes) return large.hideLabelCardSizes.indexOf(cardSize) !== -1;
  if (large.hideLabel) return !!cardMetadataValue(large.hideLabel, b, helpers || {});
  return false;
}

function applyCardMetadataFields(b, helpers, fields) {
  fields = fields || {};
  for (var key in fields) {
    var value = cardMetadataValue(fields[key], b, helpers);
    b[key] = value;
    helpers.saveField(key, value);
  }
}

function renderCardModeSelector(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var mode = metadata.mode || {};
  var field = helpers.selectField(
    mode.label || "Type",
    helpers.idPrefix + (mode.idSuffix || "mode"),
    cardMetadataValue(mode.options, b, helpers) || [],
    cardMetadataValue(mode.value, b, helpers) || "",
    function () {
      if (mode.onChange) mode.onChange.call(this, b, helpers);
    }
  );
  panel.appendChild(field.field);
  return field;
}

function renderCardLargeNumbersToggle(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var large = metadata.largeNumbers || {};
  if (!large.showSettingForAnyCardSize && !cardLargeNumbersSupportsCardSize(b, helpers, metadata)) return null;
  if (large.isVisible && !large.isVisible(b, helpers)) return null;
  var toggle = helpers.toggleRow(
    cardMetadataValue(large.label, b, helpers) || "Large Numbers",
    helpers.idPrefix + (large.idSuffix || "large-numbers"),
    cardLargeNumbersActiveForCardSize(b, helpers, metadata)
  );
  panel.appendChild(toggle.row);
  toggle.input.addEventListener("change", function () {
    setSensorLargeNumbersEnabled(b, this.checked);
    helpers.saveField("options", b.options);
    if (large.onChange) large.onChange.call(this, b, helpers);
  });
  return toggle;
}

function syncCardLargeNumbersToggle(toggle, b, helpers, visible) {
  if (!toggle) return;
  toggle.row.style.display = visible ? "" : "none";
  if (!visible && (cardLargeNumbersEnabled(b) || largeNumbersExplicitlyDisabled(b && b.options))) {
    b.options = setConfigOption(b.options, SENSOR_LARGE_NUMBERS_OPTION, false);
    b.options = setConfigOptionValue(b.options, SENSOR_LARGE_NUMBERS_OPTION, "");
    toggle.input.checked = false;
    helpers.saveField("options", b.options);
  }
}

function renderCardEntityField(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var entity = metadata.entity || {};
  var bindName = Object.prototype.hasOwnProperty.call(entity, "bindName") ? entity.bindName : "entity";
  var value = entity.value != null ? cardMetadataValue(entity.value, b, helpers) : (bindName ? b[bindName] : "");
  var domains = cardMetadataValue(entity.domains, b, helpers) || [];
  var field = helpers.entityField(
    cardMetadataValue(entity.label, b, helpers) || "Entity",
    helpers.idPrefix + (entity.idSuffix || "entity"),
    value || "",
    cardMetadataValue(entity.placeholder, b, helpers) || "",
    domains,
    bindName,
    entity.rerender !== false,
    cardMetadataValue(entity.requiredMessage, b, helpers) || ""
  );
  panel.appendChild(field.field);
  return field;
}

function renderCardTextField(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var text = metadata.text || metadata;
  var hasBindName = Object.prototype.hasOwnProperty.call(text, "bindName");
  var bindName = hasBindName ? text.bindName : (text.field || "label");
  var value = text.value != null ? cardMetadataValue(text.value, b, helpers) : b[bindName];
  var control = helpers.textField(
    text.label || "Label",
    helpers.idPrefix + (text.idSuffix || bindName),
    value || "",
    cardMetadataValue(text.placeholder, b, helpers) || "",
    bindName,
    text.rerender !== false
  );
  panel.appendChild(control.field);
  return control;
}

function renderCardNumberField(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var number = metadata.number || metadata;
  var inputId = helpers.idPrefix + (number.idSuffix || "number");
  var field = document.createElement("div");
  field.className = "sp-field";
  field.appendChild(helpers.fieldLabel(number.label || "Number", inputId));
  var input = document.createElement("input");
  input.type = "number";
  input.className = "sp-input";
  input.id = inputId;
  if (number.min != null) input.min = String(number.min);
  if (number.max != null) input.max = String(number.max);
  if (number.step != null) input.step = String(number.step);
  input.placeholder = number.placeholder || "";
  input.value = cardMetadataValue(number.value, b, helpers) || "";
  field.appendChild(input);
  panel.appendChild(field);
  return { field: field, input: input };
}

function renderCardIconPicker(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var icon = metadata.icon || metadata;
  var fieldName = icon.field || "icon";
  var fallback = cardMetadataValue(icon.fallback, b, helpers) || "Auto";
  var currentValue = icon.value != null ? cardMetadataValue(icon.value, b, helpers) : (b[fieldName] || fallback);
  var picker = helpers.iconPickerField(
    helpers.idPrefix + (icon.pickerIdSuffix || fieldName + "-picker"),
    helpers.idPrefix + (icon.idSuffix || fieldName),
    currentValue,
    function (opt) {
      b[fieldName] = opt || fallback;
      helpers.saveField(fieldName, b[fieldName]);
      if (icon.onChange) icon.onChange(b, helpers, b[fieldName]);
    },
    icon.label || "Icon"
  );
  panel.appendChild(picker);
  return picker;
}

function renderCardOptionToggle(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var toggle = metadata.toggle || metadata;
  var row = helpers.toggleRow(
    toggle.label || "Enabled",
    helpers.idPrefix + (toggle.idSuffix || "toggle"),
    !!cardMetadataValue(toggle.checked, b, helpers)
  );
  panel.appendChild(row.row);
  row.input.addEventListener("change", function () {
    if (toggle.onChange) toggle.onChange.call(this, b, helpers, this.checked);
  });
  return row;
}

function renderCardIconPair(panel, b, helpers, offMetadata, onMetadata) {
  return {
    off: helpers.renderCardIconPicker(panel, b, helpers, offMetadata),
    on: helpers.renderCardIconPicker(panel, b, helpers, onMetadata),
  };
}

function renderCardActiveColorToggle(panel, b, helpers, metadata, setEnabled) {
  return helpers.renderCardOptionToggle(panel, b, helpers, Object.assign({}, metadata, {
    onChange: function (button, cardHelpers, checked) {
      setEnabled(button, checked);
      cardHelpers.saveField("options", button.options);
    },
  }));
}

function renderBasicCardFields(panel, b, helpers, metadata, options) {
  options = options || {};
  if (options.entity !== false && metadata.entity) {
    helpers.renderCardEntityField(panel, b, helpers, metadata);
  }
  if (options.label !== false && metadata.labelField) {
    helpers.renderCardTextField(panel, b, helpers, metadata.labelField);
  }
  if (options.icon !== false && metadata.icon) {
    helpers.renderCardIconPicker(panel, b, helpers, metadata.icon);
  }
  if (options.iconPair !== false && (metadata.iconOff || metadata.iconOn)) {
    renderCardIconPair(panel, b, helpers, metadata.iconOff, metadata.iconOn);
  }
}

function renderCardSegmentControl(panel, b, helpers, metadata) {
  metadata = metadata || {};
  var segment = metadata.segment || metadata;
  var control = helpers.segmentControl(
    segment.options || [],
    cardMetadataValue(segment.value, b, helpers) || "",
    function (value, button) {
      if (segment.onSelect) segment.onSelect(b, helpers, value, button, control);
    }
  );
  panel.appendChild(helpers.fieldWithControl(segment.label || "Type", segment.inputId || null, control.segment));
  return control;
}

function renderCardBackgroundControl(panel, b, helpers) {
  if (!cardBackgroundSupported(b)) return null;
  var field = document.createElement("div");
  field.className = "sp-field sp-card-bg-field";
  field.appendChild(fieldLabel("Background Image", helpers.idPrefix + "bg-image"));

  var preview = document.createElement("div");
  preview.className = "sp-card-bg-preview";
  field.appendChild(preview);

  var select = document.createElement("select");
  select.className = "sp-select";
  select.id = helpers.idPrefix + "bg-image";
  field.appendChild(select);

  var actions = document.createElement("div");
  actions.className = "sp-card-bg-actions";
  var upload = document.createElement("input");
  upload.type = "file";
  upload.accept = "image/*";
  upload.className = "sp-card-bg-file";
  var uploadBtn = document.createElement("button");
  uploadBtn.type = "button";
  uploadBtn.className = "sp-action-btn";
  uploadBtn.innerHTML = '<span class="mdi mdi-upload"></span>Upload';
  var clearBtn = document.createElement("button");
  clearBtn.type = "button";
  clearBtn.className = "sp-action-btn";
  clearBtn.innerHTML = '<span class="mdi mdi-close"></span>Clear';
  var deleteBtn = document.createElement("button");
  deleteBtn.type = "button";
  deleteBtn.className = "sp-action-btn sp-card-bg-delete-btn";
  deleteBtn.innerHTML = '<span class="mdi mdi-trash-can-outline"></span>';
  actions.appendChild(upload);
  actions.appendChild(uploadBtn);
  actions.appendChild(clearBtn);
  actions.appendChild(deleteBtn);
  field.appendChild(actions);

  var dim = document.createElement("input");
  dim.type = "range";
  dim.min = "0";
  dim.max = "90";
  dim.step = "5";
  dim.value = cardBackgroundDim(b.options);
  var dimField = document.createElement("div");
  dimField.className = "sp-card-bg-dim";
  var dimLabel = document.createElement("span");
  dimLabel.textContent = "Dim Overlay";
  dimField.appendChild(dimLabel);
  dimField.appendChild(dim);
  field.appendChild(dimField);

  function selectedImage() {
    return cardBackgroundImage(b.options);
  }

  function setBackground(id) {
    id = normalizeCardBackgroundImageId(id);
    b.options = setConfigOptionValue(b.options, CARD_BACKGROUND_IMAGE_OPTION, id);
    if (id) {
      b.options = setConfigOptionValue(b.options, CARD_BACKGROUND_DIM_OPTION, cardBackgroundDim(b.options));
    } else {
      b.options = setConfigOptionValue(b.options, CARD_BACKGROUND_DIM_OPTION, "");
    }
    helpers.saveField("options", b.options);
    refreshPreview();
    renderPreview();
  }

  function refreshPreview() {
    var id = selectedImage();
    preview.style.backgroundImage = id ? "url('" + cardImageUrl(id) + "')" : "";
    preview.classList.toggle("sp-card-bg-preview-empty", !id);
    clearBtn.disabled = !id;
    deleteBtn.disabled = !id;
  }

  function fillSelect(items) {
    var current = selectedImage();
    select.innerHTML = "";
    var none = document.createElement("option");
    none.value = "";
    none.textContent = "No image";
    select.appendChild(none);
    (items || []).forEach(function (item) {
      var option = document.createElement("option");
      option.value = item.id;
      option.textContent = item.name || item.id;
      select.appendChild(option);
    });
    if (current && !(items || []).some(function (item) { return item.id === current; })) {
      var missing = document.createElement("option");
      missing.value = current;
      missing.textContent = current + " (missing)";
      select.appendChild(missing);
    }
    select.value = current;
    refreshPreview();
  }

  select.addEventListener("change", function () { setBackground(this.value); });
  clearBtn.addEventListener("click", function () {
    select.value = "";
    setBackground("");
  });
  uploadBtn.addEventListener("click", function () { upload.click(); });
  upload.addEventListener("change", function () {
    var file = upload.files && upload.files[0];
    if (!file) return;
    uploadBtn.disabled = true;
    uploadCardImage(file).then(function (item) {
      setBackground(item.id);
      return listCardImages(true);
    }).then(fillSelect).catch(function (err) {
      showBanner(err && err.message || "Could not upload image.", "error");
    }).then(function () {
      upload.value = "";
      uploadBtn.disabled = false;
    });
  });
  deleteBtn.addEventListener("click", function () {
    var id = selectedImage();
    if (!id) return;
    deleteCardImage(id).then(function () {
      setBackground("");
      return listCardImages(true);
    }).then(fillSelect);
  });
  dim.addEventListener("input", function () {
    if (!selectedImage()) return;
    b.options = setConfigOptionValue(b.options, CARD_BACKGROUND_DIM_OPTION, normalizeCardBackgroundDim(this.value));
    helpers.saveField("options", b.options);
    renderPreview();
  });

  fillSelect(_cardImageLibrary || []);
  listCardImages(false).then(fillSelect);
  panel.appendChild(field);
  return field;
}

function cardSensorPreviewHtml(b, helpers, value, unit, extraClass, valueClass) {
  var className = "sp-sensor-preview" + (extraClass ? " " + extraClass : "") +
    (cardLargeNumbersActiveForCardSize(b, helpers) ? " sp-sensor-preview-large" : "");
  return '<span class="' + className + '">' +
    '<span class="sp-sensor-value' + (valueClass ? " " + valueClass : "") + '">' + helpers.escHtml(value) + '</span>' +
    (unit != null ? '<span class="sp-sensor-unit">' + helpers.escHtml(unit) + '</span>' : "") +
  '</span>';
}

function cardBadgeLabelHtml(helpers, label, badgeIcon) {
  return '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
    helpers.escHtml(label) +
  '</span><span class="sp-type-badge mdi mdi-' + badgeIcon + '"></span></span>';
}

function cardIconHtml(iconSlugName, extraHtml) {
  return '<span class="sp-btn-icon mdi mdi-' + iconSlugName + '"></span>' + (extraHtml || "");
}

function cardIconSlug(b, helpers, fallback, field) {
  field = field || "icon";
  var value = b && b[field];
  if (value && value !== "Auto") return iconSlug(value);
  return iconSlug(cardMetadataValue(fallback, b, helpers) || "Auto");
}

function cardBadgePreview(b, helpers, options) {
  options = options || {};
  return {
    iconHtml: cardIconHtml(
      cardIconSlug(b, helpers, options.iconFallback, options.iconField),
      options.iconExtraHtml || ""
    ),
    labelHtml: cardBadgeLabelHtml(helpers, options.label || "Configure", options.badge),
  };
}

function condField() {
  var el = document.createElement("div");
  el.className = "sp-cond-field";
  return el;
}

function createRangeSlider(label, initial, postName) {
  var wrap = document.createElement("div");
  wrap.className = "sp-field";
  wrap.appendChild(fieldLabel(label));
  var row = document.createElement("div");
  row.className = "sp-range-row";
  var range = document.createElement("input");
  range.type = "range";
  range.className = "sp-range";
  range.min = "10";
  range.max = "100";
  range.step = "5";
  range.value = String(initial);
  var val = document.createElement("span");
  val.className = "sp-range-val";
  val.textContent = initial + "%";
  range.addEventListener("input", function () { val.textContent = this.value + "%"; });
  range.addEventListener("change", function () {
    if (typeof postName === "function") postName(this.value);
    else if (postName) postNumber(postName, this.value);
  });
  row.appendChild(range);
  row.appendChild(val);
  wrap.appendChild(row);
  return { wrap: wrap, range: range, val: val };
}
