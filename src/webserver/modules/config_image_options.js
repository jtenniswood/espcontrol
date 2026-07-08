// ── Image Card Options ─────────────────────────────────────────────
// @web-module-requires: state, config_option_core

function imageRefreshIntervalValues() {
  var spec = cardContractOptionSpec("image", IMAGE_REFRESH_OPTION);
  return spec && spec.values ? spec.values.slice() : ["off", "10", "30", "60", "300"];
}

function imageRefreshModeValues() {
  var spec = cardContractOptionSpec("image", IMAGE_REFRESH_MODE_OPTION);
  return spec && spec.values ? spec.values.slice() : ["changes_timer", "timer"];
}

function imageModalModeValues() {
  var spec = cardContractOptionSpec("image", IMAGE_MODAL_MODE_OPTION);
  return spec && spec.values ? spec.values.slice() : [];
}

function normalizeImageRefreshInterval(value) {
  value = String(value || "").trim();
  return imageRefreshIntervalValues().indexOf(value) >= 0 ? value : "off";
}

function normalizeImageRefreshMode(value) {
  value = String(value || "").trim();
  return imageRefreshModeValues().indexOf(value) >= 0 ? value : "changes_timer";
}

function normalizeImageModalMode(value) {
  value = String(value || "").trim();
  var fallback = cardContractOptionDefaultValue("image", IMAGE_MODAL_MODE_OPTION, "fill");
  return imageModalModeValues().indexOf(value) >= 0 ? value : fallback;
}

function imageRefreshInterval(b) {
  return normalizeImageRefreshInterval(configOptionValue(b && b.options, IMAGE_REFRESH_OPTION));
}

function imageRefreshMode(b) {
  return normalizeImageRefreshMode(configOptionValue(b && b.options, IMAGE_REFRESH_MODE_OPTION));
}

function imageCardLimit() {
  return IMAGE_CARD_LIMIT;
}

function imageCardLimitMessage() {
  if (IMAGE_CARD_LIMIT <= 0) return "Image cards are not available on this display.";
  return "Image cards use shared firmware download slots. You can save up to " +
    IMAGE_CARD_LIMIT + " image cards total across the main page and subpages.";
}

function isImageCard(button) {
  return !!button && (
    button.type === "image" ||
    (button.type === "media" && (
      button.sensor === "cover_art" ||
      (button.sensor === "now_playing" && configOptionEnabled(button.options, MEDIA_COVER_ART_OPTION))
    ))
  );
}

function activeGridSlots(grid) {
  var slots = [];
  var seen = {};
  (grid || []).forEach(function (slot) {
    if (slot <= 0 || seen[slot]) return;
    seen[slot] = true;
    slots.push(slot);
  });
  return slots;
}

function imageCardCountInButtons(buttons, grid) {
  var count = 0;
  var slots = activeGridSlots(grid);
  if (!slots.length && buttons && buttons.length) {
    for (var fallbackSlot = 1; fallbackSlot <= buttons.length; fallbackSlot++) {
      slots.push(fallbackSlot);
    }
  }
  slots.forEach(function (slot) {
    if (isImageCard(buttons && buttons[slot - 1])) count++;
  });
  return count;
}

function imageCardCountInSubpage(sp) {
  return imageCardCountInButtons(sp && sp.buttons, sp && sp.grid);
}

function imageCardCountInClipboardEntry(entry) {
  var count = isImageCard(entry) ? 1 : 0;
  if (entry && entry.subpageConfig) {
    count += imageCardCountInSubpage(parseSubpageConfig(entry.subpageConfig));
  }
  return count;
}

function imageCardCountInClipboardEntries(entries) {
  var count = 0;
  (entries || []).forEach(function (entry) {
    count += imageCardCountInClipboardEntry(entry);
  });
  return count;
}

function imageCardCountWithCandidate(candidate) {
  var count = 0;
  var matchedCandidate = false;

  activeGridSlots(state.grid).forEach(function (slot) {
    var button = state.buttons[slot - 1];
    if (candidate && !candidate.isSub && candidate.slot === slot) {
      button = candidate.button;
      matchedCandidate = true;
    }
    if (isImageCard(button)) count++;
  });

  for (var homeSlot in state.subpages) {
    var sp = state.subpages[homeSlot];
    activeGridSlots(sp && sp.grid).forEach(function (slot) {
      var button = sp && sp.buttons && sp.buttons[slot - 1];
      if (candidate && candidate.isSub &&
          String(candidate.homeSlot) === String(homeSlot) &&
          candidate.slot === slot) {
        button = candidate.button;
        matchedCandidate = true;
      }
      if (isImageCard(button)) count++;
    });
  }

  if (candidate && !matchedCandidate && isImageCard(candidate.button)) count++;
  return count;
}

function canAddImageCards(extraCount) {
  extraCount = parseInt(extraCount || 0, 10);
  if (!isFinite(extraCount) || extraCount <= 0) return true;
  return imageCardCountWithCandidate() + extraCount <= IMAGE_CARD_LIMIT;
}

function showImageCardLimitBanner() {
  showBanner(imageCardLimitMessage(), "error");
}

function imageModalMode(b) {
  return normalizeImageModalMode(configOptionValue(b && b.options, IMAGE_MODAL_MODE_OPTION));
}

function imageLabelEnabled(b) {
  return !!(b && configOptionEnabled(b.options, IMAGE_LABEL_OPTION));
}

function imageIconEnabled(b) {
  return !!(b && configOptionEnabled(b.options, IMAGE_ICON_OPTION));
}

function normalizeImageOptions(options) {
  var out = "";
  if (configOptionEnabled(options, IMAGE_LABEL_OPTION)) {
    out = setConfigOption(out, IMAGE_LABEL_OPTION, true);
  }
  if (configOptionEnabled(options, IMAGE_ICON_OPTION)) {
    out = setConfigOption(out, IMAGE_ICON_OPTION, true);
  }
  var modalMode = normalizeImageModalMode(configOptionValue(options, IMAGE_MODAL_MODE_OPTION));
  if (modalMode !== cardContractOptionDefaultValue("image", IMAGE_MODAL_MODE_OPTION, "fill")) {
    out = setConfigOptionValue(out, IMAGE_MODAL_MODE_OPTION, modalMode);
  }
  return out;
}

function setImageLabelEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, IMAGE_LABEL_OPTION, !!enabled);
  if (!enabled) b.label = "";
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageIconEnabled(b, enabled) {
  if (!b) return "";
  b.options = setConfigOption(b.options, IMAGE_ICON_OPTION, !!enabled);
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageModalMode(b, value) {
  if (!b) return "";
  var mode = normalizeImageModalMode(value);
  b.options = setConfigOptionValue(b.options, IMAGE_MODAL_MODE_OPTION, mode === "fill" ? "" : mode);
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageRefreshInterval(b, value) {
  if (!b) return "";
  var interval = normalizeImageRefreshInterval(value);
  b.options = setConfigOptionValue(b.options, IMAGE_REFRESH_OPTION, interval === "off" ? "" : interval);
  b.options = normalizeImageOptions(b.options);
  return b.options;
}

function setImageRefreshMode(b, value) {
  if (!b) return "";
  var mode = normalizeImageRefreshMode(value);
  b.options = setConfigOptionValue(
    b.options,
    IMAGE_REFRESH_MODE_OPTION,
    mode === "changes_timer" ? "" : mode
  );
  b.options = normalizeImageOptions(b.options);
  return b.options;
}
