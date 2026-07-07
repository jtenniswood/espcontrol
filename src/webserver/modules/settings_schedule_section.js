// ── Settings Schedule Section ──────────────────────────────────────
// @web-module-requires: state, screen_schedule_state, screen_schedule_post_api, config_codec, controls, controls_shell, settings_page_helpers

function buildScreenScheduleSettingsCard() {
  var scheduleBody = document.createElement("div");
  scheduleBody.appendChild(infoPanel(
    "sp-night-schedule-info",
    "Time-based Night Schedule overrides screensaver presence wake and Media Cover Art while it is active. Use Sensor mode when you want presence to control the night schedule."
  ));
  scheduleBody.appendChild(fieldLabel("Mode"));
  var scheduleModeSegment = segmentControl([
    ["disabled", "Disabled"],
    ["time", "Time"],
    ["sensor", "Sensor"],
  ], state.scheduleTrigger, function (mode) {
    setScheduleTrigger(mode);
  }, "sp-segment sp-screensaver-mode");
  scheduleBody.appendChild(scheduleModeSegment.segment);
  els.setScheduleModeButtons = {
    disabled: scheduleModeSegment.buttons.disabled,
    time: scheduleModeSegment.buttons.time,
    sensor: scheduleModeSegment.buttons.sensor,
  };

  var scheduleTimes = document.createElement("div");
  scheduleTimes.className = "sp-schedule-times";

  var onHour = createHourSelect("Daytime", "sp-set-schedule-on-hour", state.scheduleOnHour, function (hour) {
    state.scheduleOnHour = hour;
    postScreenScheduleOnHour(hour);
    syncScreenScheduleUi();
  });
  scheduleTimes.appendChild(onHour.wrap);
  els.setScheduleOnHour = onHour.select;

  var offHour = createHourSelect("Night Time", "sp-set-schedule-off-hour", state.scheduleOffHour, function (hour) {
    state.scheduleOffHour = hour;
    postScreenScheduleOffHour(hour);
    syncScreenScheduleUi();
  });
  scheduleTimes.appendChild(offHour.wrap);
  els.setScheduleOffHour = offHour.select;

  var scheduleModeControl = selectField("At Night Time", "sp-set-schedule-mode", [
    { value: "screen_off", label: "Screen Off" },
    { value: "screen_dimmed", label: "Screen Dimmed" },
    { value: "clock", label: "Clock" },
  ], state.scheduleMode, function () {
    state.scheduleMode = normalizeScheduleMode(this.value);
    postScreenScheduleMode(state.scheduleMode);
    syncScreenScheduleUi();
  });
  var scheduleModeSelect = scheduleModeControl.select;
  scheduleTimes.appendChild(scheduleModeControl.field);
  els.setScheduleMode = scheduleModeSelect;

  var offScreenOptions = condField();
  var wakeTimeoutOptions = [
    { label: "10 seconds", value: 10 },
    { label: "30 seconds", value: 30 },
    { label: "1 minute", value: 60 },
    { label: "2 minutes", value: 120 },
    { label: "5 minutes", value: 300 },
    { label: "10 minutes", value: 600 },
    { label: "30 minutes", value: 1800 },
    { label: "1 hour", value: 3600 },
  ];
  var wakeTimeoutControl = selectField(
    "When Woken, Idle Time to Screen Off",
    "sp-set-schedule-wake-timeout",
    wakeTimeoutOptions,
    state.scheduleWakeTimeout,
    function () {
    state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(this.value);
    postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
    syncScreenScheduleUi();
  });
  var wakeTimeoutSelect = wakeTimeoutControl.select;
  offScreenOptions.appendChild(wakeTimeoutControl.field);
  els.setScheduleWakeTimeout = wakeTimeoutSelect;

  var wakeBrightnessSlider = createRangeSlider(
    "When Woken, Screen Brightness",
    state.scheduleWakeBrightness,
    postScreenScheduleWakeBrightness
  );
  wakeBrightnessSlider.range.id = "sp-set-schedule-wake-brightness";
  wakeBrightnessSlider.range.addEventListener("change", function () {
    state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(this.value);
    syncScreenScheduleUi();
  });
  offScreenOptions.appendChild(wakeBrightnessSlider.wrap);
  els.setScheduleWakeBrightness = wakeBrightnessSlider.range;
  els.setScheduleWakeBrightnessVal = wakeBrightnessSlider.val;
  scheduleTimes.appendChild(offScreenOptions);
  els.setScheduleOffOptions = offScreenOptions;

  var dimmedOptions = condField();
  var dimmedBrightnessSlider = createRangeSlider(
    "Dimmed Screen Brightness",
    state.scheduleDimmedBrightness,
    postScreenScheduleDimmedBrightness
  );
  dimmedBrightnessSlider.range.id = "sp-set-schedule-dimmed-brightness";
  dimmedBrightnessSlider.range.min = "1";
  dimmedBrightnessSlider.range.step = "1";
  dimmedBrightnessSlider.range.addEventListener("input", function () {
    state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(this.value);
    syncScreenScheduleUi();
  });
  dimmedOptions.appendChild(dimmedBrightnessSlider.wrap);
  scheduleTimes.appendChild(dimmedOptions);
  els.setScheduleDimmedOptions = dimmedOptions;
  els.setScheduleDimmedBrightness = dimmedBrightnessSlider.range;
  els.setScheduleDimmedBrightnessVal = dimmedBrightnessSlider.val;

  var clockOptions = condField();
  var clockBrightnessSlider = createRangeSlider(
    "Clock Brightness",
    state.scheduleClockBrightness,
    postScreenScheduleClockBrightness
  );
  clockBrightnessSlider.range.id = "sp-set-schedule-clock-brightness";
  clockBrightnessSlider.range.min = "1";
  clockBrightnessSlider.range.step = "1";
  clockBrightnessSlider.range.addEventListener("input", function () {
    state.scheduleClockBrightness = normalizeScheduleClockBrightness(this.value);
    syncScreenScheduleUi();
  });
  clockOptions.appendChild(clockBrightnessSlider.wrap);
  clockOptions.appendChild(fieldLabel("Clock Text Colour"));
  var clockTextColor = colorField(
    "sp-set-schedule-clock-text-color",
    state.scheduleClockTextColor,
    function (hex) {
      state.scheduleClockTextColor = normalizeHexColor(hex, "FFFFFF");
      postText(entityName("screen_schedule_clock_text_color"), state.scheduleClockTextColor);
    }
  );
  clockOptions.appendChild(clockTextColor);
  scheduleTimes.appendChild(clockOptions);
  els.setScheduleClockOptions = clockOptions;
  els.setScheduleClockBrightness = clockBrightnessSlider.range;
  els.setScheduleClockBrightnessVal = clockBrightnessSlider.val;
  els.setScheduleClockTextColor = clockTextColor;

  scheduleBody.appendChild(scheduleTimes);
  els.setScheduleTimes = scheduleTimes;

  var scheduleSensor = document.createElement("div");
  scheduleSensor.className = "sp-schedule-times";
  var schedulePresenceField = document.createElement("div");
  schedulePresenceField.className = "sp-field";
  schedulePresenceField.appendChild(fieldLabel("Presence Entity", "sp-set-schedule-presence"));
  var schedulePresInp = entityInput("sp-set-schedule-presence", state.presenceEntity, "Presence sensor entity", ["binary_sensor", "sensor"]);
  schedulePresenceField.appendChild(schedulePresInp);
  scheduleSensor.appendChild(schedulePresenceField);
  bindTextPost(schedulePresInp, entityName("presence_sensor_entity"), {
    post: postPresenceSensorEntity,
  });
  scheduleBody.appendChild(scheduleSensor);
  els.setScheduleSensor = scheduleSensor;
  els.setSchedulePresence = schedulePresInp;

  function setScheduleTrigger(trigger) {
    state._scheduleTriggerReceived = true;
    state.scheduleTrigger = normalizeScheduleTrigger(trigger, state.scheduleEnabled);
    state.scheduleEnabled = state.scheduleTrigger !== "disabled";
    postScreenScheduleTrigger(state.scheduleTrigger);
    postScreenScheduleEnabled(state.scheduleEnabled);
    syncScreenScheduleUi();
  }

  var scheduleBadge = statusBadge("Schedule on");
  els.setScheduleBadge = scheduleBadge;
  syncScreenScheduleUi();
  var scheduleCard = makeCollapsibleCard("Night Schedule", scheduleBody, true, scheduleBadge);


  return scheduleCard;
}
