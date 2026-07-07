// ── Settings Cover Art Section ─────────────────────────────────────
// @web-module-requires: state, artwork_state, artwork_post_api, config_codec, controls, controls_shell, settings_page_helpers

function buildCoverArtSettingsCard() {
  var coverArtBody = document.createElement("div");
  var coverArtToggle = toggleRow(
      "Show Cover Art",
      "sp-set-ss-cover-art-enable",
      state.coverArtScreensaverOn);
    coverArtBody.appendChild(coverArtToggle.row);
    coverArtToggle.input.addEventListener("change", function () {
      state.coverArtScreensaverOn = this.checked;
      syncCoverArtScreensaverUi();
      postCoverArtScreensaver(state.coverArtScreensaverOn);
    });
    els.setCoverArtToggle = coverArtToggle.input;

    var coverArtOptions = condField();
    var coverArtOnlyOptions = condField();
    var coverArtAdvancedBody = document.createElement("div");

    var sleepPreventionToggle = toggleRow(
      "Keep Screen Awake During Playback",
      "sp-set-ss-media-sleep-prevention",
      state.mediaPlayerSleepPreventionOn);
    coverArtOptions.appendChild(sleepPreventionToggle.row);
    sleepPreventionToggle.input.addEventListener("change", function () {
      state.mediaPlayerSleepPreventionOn = this.checked;
      syncMediaPlayerSleepPreventionUi();
      syncCoverArtScreensaverUi();
      postMediaPlayerSleepPrevention(state.mediaPlayerSleepPreventionOn);
    });
    els.setMediaPlayerSleepPreventionToggle = sleepPreventionToggle.input;

    var coverArtEntityField = document.createElement("div");
    coverArtEntityField.className = "sp-field";
    coverArtEntityField.appendChild(fieldLabel("Media Player Entity", "sp-set-ss-cover-art-player"));
    var coverArtEntityInp = entityInput(
      "sp-set-ss-cover-art-player",
      state.coverArtMediaPlayerEntity,
      "e.g. media_player.living_room",
      ["media_player"]);
    coverArtEntityField.appendChild(coverArtEntityInp);
    coverArtOnlyOptions.appendChild(coverArtEntityField);
    bindTextPost(coverArtEntityInp, entityName("screen_saver_cover_art_entity"), {
      onBlur: function (value) {
        state.coverArtMediaPlayerEntity = value;
        state.mediaPlayerSleepPreventionEntity = value;
      },
      post: function (value) {
        postCoverArtMediaPlayerEntity(value);
        postMediaPlayerSleepPreventionEntity(value);
      },
    });
    els.setCoverArtMediaPlayer = coverArtEntityInp;

    var coverArtDelayField = document.createElement("div");
    coverArtDelayField.className = "sp-field";
    coverArtDelayField.appendChild(fieldLabel("Show After", "sp-set-ss-cover-art-delay"));
    var coverArtDelaySelect = document.createElement("select");
    coverArtDelaySelect.className = "sp-select";
    coverArtDelaySelect.id = "sp-set-ss-cover-art-delay";
    [
      { label: "Immediately", value: 0 },
      { label: "5 seconds", value: 5 },
      { label: "10 seconds", value: 10 },
      { label: "30 seconds", value: 30 },
      { label: "1 minute", value: 60 },
      { label: "5 minutes", value: 300 },
    ].forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      coverArtDelaySelect.appendChild(o);
    });
    coverArtDelaySelect.addEventListener("change", function () {
      state.coverArtDelay = parseFloat(this.value) || 0;
      postCoverArtDelay(state.coverArtDelay);
    });
    coverArtDelayField.appendChild(coverArtDelaySelect);
    coverArtOnlyOptions.appendChild(coverArtDelayField);
    els.setCoverArtDelay = coverArtDelaySelect;

    var coverArtTouchPauseField = document.createElement("div");
    coverArtTouchPauseField.className = "sp-field";
    coverArtTouchPauseField.appendChild(fieldLabel("After Touch, Show Again", "sp-set-ss-cover-art-touch-pause"));
    var coverArtTouchPauseSelect = document.createElement("select");
    coverArtTouchPauseSelect.className = "sp-select";
    coverArtTouchPauseSelect.id = "sp-set-ss-cover-art-touch-pause";
    [
      { label: "Immediately", value: 0 },
      { label: "1 minute", value: 60 },
      { label: "2 minutes", value: 120 },
      { label: "3 minutes", value: 180 },
      { label: "5 minutes", value: 300 },
    ].forEach(function (opt) {
      var o = document.createElement("option");
      o.value = opt.value;
      o.textContent = opt.label;
      coverArtTouchPauseSelect.appendChild(o);
    });
    coverArtTouchPauseSelect.addEventListener("change", function () {
      state.coverArtTouchPause = parseFloat(this.value) || 0;
      postCoverArtTouchPause(state.coverArtTouchPause);
    });
    coverArtTouchPauseField.appendChild(coverArtTouchPauseSelect);
    coverArtOnlyOptions.appendChild(coverArtTouchPauseField);
    els.setCoverArtTouchPause = coverArtTouchPauseSelect;

    if (coverArtTrackOverlayDurationSupported()) {
      var trackOverlayField = document.createElement("div");
      trackOverlayField.className = "sp-field";
      trackOverlayField.appendChild(fieldLabel("Show Track Details For", "sp-set-ss-track-overlay"));
      var trackOverlaySelect = document.createElement("select");
      trackOverlaySelect.className = "sp-select";
      trackOverlaySelect.id = "sp-set-ss-track-overlay";
      [
        { label: "Never", value: 0 },
        { label: "3 seconds", value: 3 },
        { label: "5 seconds", value: 5 },
        { label: "10 seconds", value: 10 },
        { label: "15 seconds", value: 15 },
        { label: "20 seconds", value: 20 },
        { label: "30 seconds", value: 30 },
        { label: "60 seconds", value: 60 },
        { label: "Always", value: -1 },
      ].forEach(function (opt) {
        var o = document.createElement("option");
        o.value = opt.value;
        o.textContent = opt.label;
        trackOverlaySelect.appendChild(o);
      });
      trackOverlaySelect.addEventListener("change", function () {
        state.coverArtTrackOverlayDuration = parseFloat(this.value) || 0;
        postCoverArtTrackOverlayDuration(state.coverArtTrackOverlayDuration);
      });
      trackOverlayField.appendChild(trackOverlaySelect);
      coverArtOnlyOptions.appendChild(trackOverlayField);
      els.setCoverArtTrackOverlayDuration = trackOverlaySelect;
    }

    var coverArtHideExternalInputToggle = toggleRow(
      "Hide for external source inputs",
      "sp-set-ss-cover-art-hide-external-input",
      state.coverArtHideExternalInputOn);
    coverArtAdvancedBody.appendChild(coverArtHideExternalInputToggle.row);
    coverArtHideExternalInputToggle.input.addEventListener("change", function () {
      state.coverArtHideExternalInputOn = this.checked;
      postCoverArtHideExternalInput(state.coverArtHideExternalInputOn);
    });
    els.setCoverArtHideExternalInputToggle = coverArtHideExternalInputToggle.input;

    state.coverArtFilteringEnabled = !!state.coverArtAttributeConditions;
    var coverArtFilterToggle = toggleRow(
      "Advanced Filtering",
      "sp-set-ss-cover-art-filtering",
      state.coverArtFilteringEnabled);
    coverArtAdvancedBody.appendChild(coverArtFilterToggle.row);
    coverArtFilterToggle.input.addEventListener("change", function () {
      state.coverArtFilteringEnabled = this.checked;
      if (!state.coverArtFilteringEnabled) {
        state.coverArtAttributeConditions = "";
        syncInput(els.setCoverArtConditions, "");
        postCoverArtConditions("");
      }
      syncCoverArtScreensaverUi();
    });
    els.setCoverArtFilterToggle = coverArtFilterToggle.input;

    var coverArtFilterOptions = condField();
    var coverArtConditionsField = document.createElement("div");
    coverArtConditionsField.className = "sp-field";
    coverArtConditionsField.appendChild(fieldLabel("Only Show When", "sp-set-ss-cover-art-conditions"));
    var coverArtConditionsInp = document.createElement("input");
    coverArtConditionsInp.className = "sp-input";
    coverArtConditionsInp.id = "sp-set-ss-cover-art-conditions";
    coverArtConditionsInp.type = "text";
    coverArtConditionsInp.maxLength = 240;
    coverArtConditionsInp.placeholder = "app_id=com.apple.TVMusic; media_content_type=music";
    coverArtConditionsInp.value = state.coverArtAttributeConditions || "";
    coverArtConditionsField.appendChild(coverArtConditionsInp);
    coverArtFilterOptions.appendChild(coverArtConditionsField);
    coverArtAdvancedBody.appendChild(coverArtFilterOptions);
    bindTextPost(coverArtConditionsInp, entityName("screen_saver_cover_art_conditions"), {
      onBlur: function (value) {
        state.coverArtAttributeConditions = value;
        state.coverArtFilteringEnabled = !!value || state.coverArtFilteringEnabled;
        syncCoverArtScreensaverUi();
      },
      post: postCoverArtConditions,
    });
    els.setCoverArtConditions = coverArtConditionsInp;
    els.setCoverArtFilterOptions = coverArtFilterOptions;

    coverArtOnlyOptions.appendChild(inlineDisclosure(
      "Advanced Options",
      coverArtAdvancedBody,
      !!state.coverArtAttributeConditions || !state.coverArtHideExternalInputOn));

    els.setCoverArtOnlyOptions = coverArtOnlyOptions;
    coverArtOptions.appendChild(coverArtOnlyOptions);
    els.setCoverArtOptions = coverArtOptions;
    coverArtBody.appendChild(coverArtOptions);

  ssBody.appendChild(timerPanel);
  els.setSSTimeout = timeoutSelect;
  syncScreensaverTimeoutUi();

  var sensorPanel = document.createElement("div");
  var presenceField = document.createElement("div");
  presenceField.className = "sp-field";
  presenceField.appendChild(fieldLabel("Presence Entity", "sp-set-presence"));
  var presInp = entityInput("sp-set-presence", state.presenceEntity, "Presence sensor entity", ["binary_sensor", "sensor"]);
  presenceField.appendChild(presInp);
  sensorPanel.appendChild(presenceField);
  bindTextPost(presInp, entityName("presence_sensor_entity"), {
    post: postPresenceSensorEntity,
  });
  var sensorClockControls = createScreensaverThenControls("sp-set-sensor-clock-mode");
  sensorPanel.appendChild(sensorClockControls.clockField);
  sensorPanel.appendChild(sensorClockControls.dimBrightnessField);
  sensorPanel.appendChild(sensorClockControls.brightnessField);
  ssBody.appendChild(sensorPanel);
  els.setPresence = presInp;
  els.setSensorClockSelect = sensorClockControls.clockSelect;
  els.setSensorClockField = sensorClockControls.clockField;
  els.setSensorDimBrightnessField = sensorClockControls.dimBrightnessField;
  els.setSensorDimBrightness = sensorClockControls.dimBrightness;
  els.setSensorDimBrightnessVal = sensorClockControls.dimBrightnessVal;
  els.setSensorClockBrightnessDay = sensorClockControls.clockBrightnessDay;
  els.setSensorClockBrightnessDayVal = sensorClockControls.clockBrightnessDayVal;
  els.setSensorClockBrightnessNight = sensorClockControls.clockBrightnessNight;
  els.setSensorClockBrightnessNightVal = sensorClockControls.clockBrightnessNightVal;
  els.setSensorClockBrightnessField = sensorClockControls.brightnessField;
  syncClockScreensaverControls();
  syncMediaPlayerSleepPreventionUi();
  syncCoverArtScreensaverUi();

  var ssBadge = statusBadge("Screensaver on");
  els.setScreensaverBadge = ssBadge;

  function setSsMode(mode) {
    ssMode = mode;
    disabledBtn.className = mode === "disabled" ? "active" : "";
    timerBtn.className = mode === "timer" ? "active" : "";
    sensorBtn.className = mode === "sensor" ? "active" : "";
    timerPanel.style.display = mode === "timer" ? "" : "none";
    sensorPanel.style.display = mode === "sensor" ? "" : "none";
    if (els.setScreensaverBadge) {
      els.setScreensaverBadge.className = "sp-card-badge" + (mode === "disabled" ? " sp-hidden" : "");
    }
  }
  els.setSsMode = setSsMode;
  setSsMode(ssMode);

  var screensaverCard = makeCollapsibleCard("Screensaver", ssBody, true, ssBadge);

  var idleBody = document.createElement("div");
  idleBody.appendChild(fieldLabel("Return Home After"));
  var hsSelect = document.createElement("select");
  hsSelect.className = "sp-select";
  hsSelect.id = "sp-set-hs-timeout";
  var hsOptions = [
    { label: "Disabled", value: 0 },
    { label: "10 seconds", value: 10 },
    { label: "20 seconds", value: 20 },
    { label: "30 seconds", value: 30 },
    { label: "1 minute", value: 60 },
    { label: "2 minutes", value: 120 },
    { label: "5 minutes", value: 300 },
  ];
  hsOptions.forEach(function (opt) {
    var o = document.createElement("option");
    o.value = opt.value;
    o.textContent = opt.label;
    if (opt.value === state.homeScreenTimeout) o.selected = true;
    hsSelect.appendChild(o);
  });
  hsSelect.addEventListener("change", function () {
    state.homeScreenTimeout = parseFloat(this.value) || 0;
    syncIdleUi();
    postHomeScreenTimeout(this.value);
  });
  idleBody.appendChild(hsSelect);
  els.setHSTimeout = hsSelect;
  var idleBadge = statusBadge("Idle on");
  els.setIdleBadge = idleBadge;
  syncIdleUi();
  var idleCard = makeCollapsibleCard("Idle", idleBody, true, idleBadge);
  var coverArtBadge = statusBadge("Media cover art on");
  els.setCoverArtBadge = coverArtBadge;
  syncCoverArtScreensaverUi();
  var coverArtCard = makeCollapsibleCard("Cover Art", coverArtBody, true, coverArtBadge);


  return coverArtCard;
}
