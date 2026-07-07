// ── Settings Page Helpers ──────────────────────────────────────────
// @web-module-requires: state, screen_schedule_state, screensaver_state, screensaver_timeout, clock_bar_state, clock_bar_post_api, artwork_state, artwork_post_api, controls, controls_shell

// ── Settings UI helpers ─────────────────────────────────────────────

function settingsStatusHeader(title) {
  var header = document.createElement("div");
  header.className = "sp-settings-status-header";

  var label = document.createElement("div");
  label.className = "sp-settings-status-title";
  label.textContent = title;
  header.appendChild(label);

  return header;
}

function appendSettingsSection(parent, title, cards) {
  var visibleCards = cards.filter(Boolean);
  if (!visibleCards.length) return;

  parent.appendChild(settingsStatusHeader(title));
  visibleCards.forEach(function (card) {
    parent.appendChild(card);
  });
}

function openVoiceServicesSettings() {
  if (isConfigLocked() || !els.voiceServicesCard) return;
  switchTab("settings");
  els.voiceServicesCard.classList.remove("collapsed");
  els.voiceServicesCard.scrollIntoView({ block: "center", behavior: "smooth" });
  if (els.setVoiceServicesToggle) {
    window.setTimeout(function () { els.setVoiceServicesToggle.focus(); }, 150);
  }
}

function coverArtTrackOverlayDurationSupported() {
  return !!(CFG && CFG.coverArtSquareOverlay);
}

function infoPanel(id, text) {
  var panel = document.createElement("div");
  panel.className = "sp-info-panel";
  panel.id = id;
  panel.setAttribute("role", "note");
  var icon = document.createElement("span");
  icon.className = "mdi mdi-information-outline";
  icon.setAttribute("aria-hidden", "true");
  var message = document.createElement("span");
  message.textContent = text;
  panel.appendChild(icon);
  panel.appendChild(message);
  return panel;
}

function statusBadge(label) {
  var badge = document.createElement("span");
  badge.setAttribute("aria-label", label);
  badge.appendChild(textSpan("", "sp-card-badge-dot"));
  badge.appendChild(textSpan("ON"));
  return badge;
}

function inlineDisclosure(title, bodyElement, defaultOpen) {
  var panel = document.createElement("div");
  panel.className = "sp-disclosure" + (defaultOpen ? " sp-open" : "");
  var button = document.createElement("button");
  button.type = "button";
  button.className = "sp-disclosure-button";
  button.setAttribute("aria-expanded", defaultOpen ? "true" : "false");
  var label = document.createElement("span");
  label.textContent = title;
  var chevron = createDisclosureChevron("sp-disclosure-chevron");
  button.appendChild(label);
  button.appendChild(chevron);
  var body = document.createElement("div");
  body.className = "sp-disclosure-body";
  body.appendChild(bodyElement);
  button.addEventListener("click", function () {
    var open = !panel.classList.contains("sp-open");
    panel.classList.toggle("sp-open", open);
    button.setAttribute("aria-expanded", open ? "true" : "false");
  });
  panel.appendChild(button);
  panel.appendChild(body);
  return panel;
}

// ── Settings sync helpers ───────────────────────────────────────────

function syncClockScreensaverControls() {
  var mode = normalizeScreensaverAction(state.screensaverAction);
  var dayBrightness = Math.round(state.clockBrightnessDay) + "%";
  var nightBrightness = Math.round(state.clockBrightnessNight) + "%";
  var dimBrightness = Math.round(state.screensaverDimmedBrightness) + "%";
  var clockDisplay = mode === "clock" ? "" : "none";
  var dimDisplay = mode === "dim" ? "" : "none";

  state.clockScreensaverOn = mode === "clock";
  syncClockBarUi();

  if (els.setClockSelect) els.setClockSelect.value = mode;
  if (els.setSensorClockSelect) els.setSensorClockSelect.value = mode;
  syncOptionalClockBrightness(els.setClockBrightnessField, els.setDimBrightnessField || els.setClockField, clockDisplay);
  syncOptionalClockBrightness(els.setSensorClockBrightnessField, els.setSensorDimBrightnessField || els.setSensorClockField, clockDisplay);
  syncOptionalClockBrightness(els.setDimBrightnessField, els.setClockField, dimDisplay);
  syncOptionalClockBrightness(els.setSensorDimBrightnessField, els.setSensorClockField, dimDisplay);
  if (els.setDimBrightness) {
    els.setDimBrightness.value = state.screensaverDimmedBrightness;
    els.setDimBrightnessVal.textContent = dimBrightness;
  }
  if (els.setSensorDimBrightness) {
    els.setSensorDimBrightness.value = state.screensaverDimmedBrightness;
    els.setSensorDimBrightnessVal.textContent = dimBrightness;
  }
  if (els.setClockBrightnessDay) {
    els.setClockBrightnessDay.value = state.clockBrightnessDay;
    els.setClockBrightnessDayVal.textContent = dayBrightness;
  }
  if (els.setClockBrightnessNight) {
    els.setClockBrightnessNight.value = state.clockBrightnessNight;
    els.setClockBrightnessNightVal.textContent = nightBrightness;
  }
  if (els.setSensorClockBrightnessDay) {
    els.setSensorClockBrightnessDay.value = state.clockBrightnessDay;
    els.setSensorClockBrightnessDayVal.textContent = dayBrightness;
  }
  if (els.setSensorClockBrightnessNight) {
    els.setSensorClockBrightnessNight.value = state.clockBrightnessNight;
    els.setSensorClockBrightnessNightVal.textContent = nightBrightness;
  }
}

function syncMediaPlayerSleepPreventionUi() {
  if (els.setMediaPlayerSleepPreventionToggle) {
    els.setMediaPlayerSleepPreventionToggle.checked = !!state.mediaPlayerSleepPreventionOn;
  }
  if (els.setSensorMediaPlayerSleepPreventionToggle) {
    els.setSensorMediaPlayerSleepPreventionToggle.checked = !!state.mediaPlayerSleepPreventionOn;
  }
}

function syncCoverArtScreensaverUi() {
  if (els.setCoverArtToggle) {
    els.setCoverArtToggle.checked = !!state.coverArtScreensaverOn;
  }
  if (els.setCoverArtOptions) {
    els.setCoverArtOptions.classList.toggle(
      "sp-visible",
      !!state.coverArtScreensaverOn);
  }
  if (els.setCoverArtOnlyOptions) {
    els.setCoverArtOnlyOptions.classList.toggle(
      "sp-visible",
      !!state.coverArtScreensaverOn);
  }
  if (els.setCoverArtBadge) {
    els.setCoverArtBadge.className = "sp-card-badge" + (state.coverArtScreensaverOn ? "" : " sp-hidden");
  }
  if (els.setCoverArtDelay) {
    var coverArtDelay = Math.max(0, parseFloat(state.coverArtDelay) || 0);
    state.coverArtDelay = coverArtDelay;
    setSelectValue(
      els.setCoverArtDelay,
      coverArtDelay,
      coverArtDelay > 0 ? formatDuration(coverArtDelay) : "Immediately");
  }
  if (els.setCoverArtTouchPause) {
    var coverArtTouchPause = Math.max(0, parseFloat(state.coverArtTouchPause) || 0);
    state.coverArtTouchPause = coverArtTouchPause;
    setSelectValue(
      els.setCoverArtTouchPause,
      coverArtTouchPause,
      coverArtTouchPause > 0 ? formatDuration(coverArtTouchPause) : "Immediately");
  }
  if (els.setCoverArtTrackOverlayDuration) {
    var value = state.coverArtTrackOverlayDuration;
    setSelectValue(
      els.setCoverArtTrackOverlayDuration,
      value,
      value < 0 ? "Always" : value > 0 ? formatDuration(value) : "Never");
  }
  if (els.setCoverArtHideExternalInputToggle) {
    els.setCoverArtHideExternalInputToggle.checked = !!state.coverArtHideExternalInputOn;
  }
  if (els.setHomeAssistantArtworkProtocol) {
    els.setHomeAssistantArtworkProtocol.value =
      normalizeHomeAssistantArtworkProtocol(state.homeAssistantArtworkProtocol);
  }
  if (els.setCoverArtHomeAssistantPort) {
    els.setCoverArtHomeAssistantPort.value = String(
      normalizeHomeAssistantArtworkPort(state.coverArtHomeAssistantPort));
  }
  if (els.setCoverArtFilterToggle) {
    state.coverArtFilteringEnabled = !!state.coverArtFilteringEnabled || !!state.coverArtAttributeConditions;
    els.setCoverArtFilterToggle.checked = !!state.coverArtFilteringEnabled;
  }
  if (els.setCoverArtFilterOptions) {
    els.setCoverArtFilterOptions.classList.toggle("sp-visible", !!state.coverArtFilteringEnabled);
  }
  syncInput(els.setCoverArtConditions, state.coverArtAttributeConditions || "");
}

function syncOptionalClockBrightness(field, previousField, display) {
  if (field) field.style.display = display;
  if (previousField) previousField.style.marginBottom = display === "none" ? "20px" : "";
}

function createScreensaverThenControls(selectId) {
  var clockField = document.createElement("div");
  clockField.className = "sp-field";
  clockField.appendChild(fieldLabel("Then", selectId));
  var clockSelect = document.createElement("select");
  clockSelect.className = "sp-select";
  clockSelect.id = selectId;
  [
    { value: "off", label: "Display Off" },
    { value: "dim", label: "Screen Dimmed" },
    { value: "clock", label: "Clock" },
  ].forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt.value;
    o.textContent = opt.label;
    clockSelect.appendChild(o);
  });
  clockSelect.value = normalizeScreensaverAction(state.screensaverAction);
  clockSelect.addEventListener("change", function () {
    state.screensaverAction = normalizeScreensaverAction(this.value);
    state.clockScreensaverOn = state.screensaverAction === "clock";
    syncClockScreensaverControls();
    postScreensaverAction(state.screensaverAction);
    postClockScreensaver(state.clockScreensaverOn);
  });
  clockField.appendChild(clockSelect);

  var dimBrightnessField = document.createElement("div");
  dimBrightnessField.style.display = normalizeScreensaverAction(state.screensaverAction) === "dim" ? "" : "none";
  var dimSlider = createRangeSlider("Dimmed Screen Brightness", state.screensaverDimmedBrightness, postScreensaverDimmedBrightness);
  dimSlider.range.min = "1";
  dimSlider.range.step = "1";
  dimSlider.range.addEventListener("input", function () {
    state.screensaverDimmedBrightness = normalizeScreensaverDimmedBrightness(this.value);
    syncClockScreensaverControls();
  });
  dimBrightnessField.appendChild(dimSlider.wrap);

  var clockBrightnessField = document.createElement("div");
  clockBrightnessField.className = "sp-clock-brightness-field";
  clockBrightnessField.style.display = normalizeScreensaverAction(state.screensaverAction) === "clock" ? "" : "none";
  var daySlider = createRangeSlider("Daytime Clock Brightness", state.clockBrightnessDay, postClockBrightnessDay);
  daySlider.range.min = "1";
  daySlider.range.step = "1";
  daySlider.range.addEventListener("input", function () {
    state.clockBrightnessDay = normalizeClockBrightness(this.value, 35);
    syncClockScreensaverControls();
  });
  clockBrightnessField.appendChild(daySlider.wrap);
  var nightSlider = createRangeSlider("Nighttime Clock Brightness", state.clockBrightnessNight, postClockBrightnessNight);
  nightSlider.range.min = "1";
  nightSlider.range.step = "1";
  nightSlider.range.addEventListener("input", function () {
    state.clockBrightnessNight = normalizeClockBrightness(this.value, state.clockBrightnessDay);
    syncClockScreensaverControls();
  });
  clockBrightnessField.appendChild(nightSlider.wrap);

  return {
    clockField: clockField,
    clockSelect: clockSelect,
    dimBrightnessField: dimBrightnessField,
    dimBrightness: dimSlider.range,
    dimBrightnessVal: dimSlider.val,
    brightnessField: clockBrightnessField,
    clockBrightnessDay: daySlider.range,
    clockBrightnessDayVal: daySlider.val,
    clockBrightnessNight: nightSlider.range,
    clockBrightnessNightVal: nightSlider.val,
  };
}

function createHourSelect(label, id, initial, onChange) {
  var wrap = document.createElement("div");
  wrap.className = "sp-field";
  wrap.appendChild(fieldLabel(label, id));
  var select = document.createElement("select");
  select.className = "sp-select";
  select.id = id;
  for (var h = 0; h < 24; h++) {
    var o = document.createElement("option");
    o.value = String(h);
    o.textContent = formatHour(h);
    select.appendChild(o);
  }
  select.value = String(normalizeHour(initial, 0));
  select.addEventListener("change", function () {
    onChange(normalizeHour(this.value, 0));
  });
  wrap.appendChild(select);
  return { wrap: wrap, select: select };
}

function createTimeInput(label, id, initial, fallback, onChange) {
  var wrap = document.createElement("div");
  wrap.className = "sp-field";
  wrap.appendChild(fieldLabel(label, id));
  var input = document.createElement("input");
  input.type = "time";
  input.className = "sp-input";
  input.id = id;
  input.step = "60";
  input.value = normalizeTimeOfDay(initial, fallback);
  input.addEventListener("change", function () {
    var value = normalizeTimeOfDay(this.value, fallback);
    this.value = value;
    onChange(value);
  });
  wrap.appendChild(input);
  return { wrap: wrap, input: input };
}

function createEntityToggleSection(label, id, checked, switchName, entityLabel, entityPostName, placeholder) {
  var toggle = toggleRow(label, id, checked);
  var field = condField();
  var inp = entityInput("", "", placeholder, ["sensor"]);
  field.appendChild(inp);
  toggle.input.addEventListener("change", function () { postSwitch(switchName, this.checked); });
  bindTextPost(inp, entityPostName, {});
  return { toggle: toggle, field: field, input: inp };
}
