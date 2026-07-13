// ── Settings Page Helpers ──────────────────────────────────────────
// @web-module-requires: state, screen_schedule_state, screensaver_state, screensaver_timeout, clock_bar_state, clock_bar_post_api, artwork_state, artwork_post_api, controls, controls_shell

// ── Settings UI helpers ─────────────────────────────────────────────

var _settingsUiFeature = createSettingsUiFeature({
  document: document,
  textSpan: textSpan,
  createDisclosureChevron: createDisclosureChevron,
});

function settingsStatusHeader(title) {
  return _settingsUiFeature.settingsStatusHeader(title);
}

function appendSettingsSection(parent, title, cards) {
  _settingsUiFeature.appendSettingsSection(parent, title, cards);
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
  return _settingsUiFeature.infoPanel(id, text);
}

function statusBadge(label) {
  return _settingsUiFeature.statusBadge(label);
}

function inlineDisclosure(title, bodyElement, defaultOpen) {
  return _settingsUiFeature.inlineDisclosure(title, bodyElement, defaultOpen);
}

// ── Settings sync helpers ───────────────────────────────────────────

function syncClockScreensaverControls() {
  var controlState = screensaverControlState(
    state.screensaverAction,
    state.clockBrightnessDay,
    state.clockBrightnessNight,
    state.screensaverDimmedBrightness
  );
  var mode = controlState.mode;
  var clockDisplay = controlState.clockVisible ? "" : "none";
  var dimDisplay = controlState.dimVisible ? "" : "none";

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
    els.setDimBrightnessVal.textContent = controlState.dimBrightnessLabel;
  }
  if (els.setSensorDimBrightness) {
    els.setSensorDimBrightness.value = state.screensaverDimmedBrightness;
    els.setSensorDimBrightnessVal.textContent = controlState.dimBrightnessLabel;
  }
  if (els.setClockBrightnessDay) {
    els.setClockBrightnessDay.value = state.clockBrightnessDay;
    els.setClockBrightnessDayVal.textContent = controlState.dayBrightnessLabel;
  }
  if (els.setClockBrightnessNight) {
    els.setClockBrightnessNight.value = state.clockBrightnessNight;
    els.setClockBrightnessNightVal.textContent = controlState.nightBrightnessLabel;
  }
  if (els.setSensorClockBrightnessDay) {
    els.setSensorClockBrightnessDay.value = state.clockBrightnessDay;
    els.setSensorClockBrightnessDayVal.textContent = controlState.dayBrightnessLabel;
  }
  if (els.setSensorClockBrightnessNight) {
    els.setSensorClockBrightnessNight.value = state.clockBrightnessNight;
    els.setSensorClockBrightnessNightVal.textContent = controlState.nightBrightnessLabel;
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
  if (els.setCoverArtTrackOverlayDuration) {
    var value = state.coverArtTrackOverlayDuration;
    setSelectValue(
      els.setCoverArtTrackOverlayDuration,
      value,
      timedSettingLabel(value, formatDuration));
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
