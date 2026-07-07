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

  var timeScheduleControls = createScheduleNightActionControls(
    "sp-set-schedule",
    "At Night Time",
    function () { return state.scheduleMode; },
    function (mode) {
      state.scheduleMode = mode;
      postScreenScheduleMode(mode);
    }
  );
  timeScheduleControls.fields.forEach(function (field) {
    scheduleTimes.appendChild(field);
  });
  els.setScheduleMode = timeScheduleControls.modeSelect;
  els.setScheduleWakeTimeout = timeScheduleControls.wakeTimeoutSelect;
  els.setScheduleWakeBrightness = timeScheduleControls.wakeBrightness;
  els.setScheduleWakeBrightnessVal = timeScheduleControls.wakeBrightnessVal;
  els.setScheduleOffOptions = timeScheduleControls.offOptions;
  els.setScheduleDimmedOptions = timeScheduleControls.dimmedOptions;
  els.setScheduleDimmedBrightness = timeScheduleControls.dimmedBrightness;
  els.setScheduleDimmedBrightnessVal = timeScheduleControls.dimmedBrightnessVal;
  els.setScheduleClockOptions = timeScheduleControls.clockOptions;
  els.setScheduleClockBrightness = timeScheduleControls.clockBrightness;
  els.setScheduleClockBrightnessVal = timeScheduleControls.clockBrightnessVal;
  els.setScheduleClockTextColor = timeScheduleControls.clockTextColor;

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
  var sensorScheduleControls = createScheduleNightActionControls(
    "sp-set-schedule-sensor",
    "When presence is detected",
    function () { return state.schedulePresenceDetectedMode; },
    function (mode) {
      state.schedulePresenceDetectedMode = mode;
      postScreenSchedulePresenceDetectedMode(mode);
    }
  );
  sensorScheduleControls.fields.forEach(function (field) {
    scheduleSensor.appendChild(field);
  });
  var sensorNotDetectedControls = createScheduleNightActionControls(
    "sp-set-schedule-sensor-not-detected",
    "When presence is not detected",
    function () { return state.schedulePresenceNotDetectedMode; },
    function (mode) {
      state.schedulePresenceNotDetectedMode = mode;
      postScreenSchedulePresenceNotDetectedMode(mode);
    }
  );
  sensorNotDetectedControls.fields.forEach(function (field) {
    scheduleSensor.appendChild(field);
  });
  scheduleBody.appendChild(scheduleSensor);
  els.setScheduleSensor = scheduleSensor;
  els.setSchedulePresence = schedulePresInp;
  els.setScheduleSensorMode = sensorScheduleControls.modeSelect;
  els.setScheduleSensorNotDetectedMode = sensorNotDetectedControls.modeSelect;
  els.setScheduleSensorWakeTimeout = sensorScheduleControls.wakeTimeoutSelect;
  els.setScheduleSensorWakeBrightness = sensorScheduleControls.wakeBrightness;
  els.setScheduleSensorWakeBrightnessVal = sensorScheduleControls.wakeBrightnessVal;
  els.setScheduleSensorOffOptions = sensorScheduleControls.offOptions;
  els.setScheduleSensorDimmedOptions = sensorScheduleControls.dimmedOptions;
  els.setScheduleSensorDimmedBrightness = sensorScheduleControls.dimmedBrightness;
  els.setScheduleSensorDimmedBrightnessVal = sensorScheduleControls.dimmedBrightnessVal;
  els.setScheduleSensorClockOptions = sensorScheduleControls.clockOptions;
  els.setScheduleSensorClockBrightness = sensorScheduleControls.clockBrightness;
  els.setScheduleSensorClockBrightnessVal = sensorScheduleControls.clockBrightnessVal;
  els.setScheduleSensorClockTextColor = sensorScheduleControls.clockTextColor;
  els.setScheduleSensorNotDetectedOffOptions = sensorNotDetectedControls.offOptions;
  els.setScheduleSensorNotDetectedWakeTimeout = sensorNotDetectedControls.wakeTimeoutSelect;
  els.setScheduleSensorNotDetectedWakeBrightness = sensorNotDetectedControls.wakeBrightness;
  els.setScheduleSensorNotDetectedWakeBrightnessVal = sensorNotDetectedControls.wakeBrightnessVal;
  els.setScheduleSensorNotDetectedDimmedOptions = sensorNotDetectedControls.dimmedOptions;
  els.setScheduleSensorNotDetectedDimmedBrightness = sensorNotDetectedControls.dimmedBrightness;
  els.setScheduleSensorNotDetectedDimmedBrightnessVal = sensorNotDetectedControls.dimmedBrightnessVal;
  els.setScheduleSensorNotDetectedClockOptions = sensorNotDetectedControls.clockOptions;
  els.setScheduleSensorNotDetectedClockBrightness = sensorNotDetectedControls.clockBrightness;
  els.setScheduleSensorNotDetectedClockBrightnessVal = sensorNotDetectedControls.clockBrightnessVal;
  els.setScheduleSensorNotDetectedClockTextColor = sensorNotDetectedControls.clockTextColor;

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

function createScheduleModeField(label, id, getMode, postMode) {
  var scheduleModeControl = selectField(label, id, [
    { value: "screen_off", label: "Screen Off" },
    { value: "screen_dimmed", label: "Screen Dimmed" },
    { value: "always_on", label: "Always On" },
    { value: "clock", label: "Clock" },
  ], getMode(), function () {
    var mode = normalizeScheduleMode(this.value);
    postMode(mode);
    syncScreenScheduleUi();
  });
  return {
    field: scheduleModeControl.field,
    select: scheduleModeControl.select,
  };
}

function createScheduleNightActionControls(idPrefix, label, getMode, postMode) {
  var modeControl = createScheduleModeField(label, idPrefix + "-mode", getMode, postMode);
  var fields = [modeControl.field];

  var offScreenOptions = condField();
  var wakeTimeoutControl = selectField(
    "When Woken, Idle Time to Screen Off",
    idPrefix + "-wake-timeout",
    [
      { label: "10 seconds", value: 10 },
      { label: "30 seconds", value: 30 },
      { label: "1 minute", value: 60 },
      { label: "2 minutes", value: 120 },
      { label: "5 minutes", value: 300 },
      { label: "10 minutes", value: 600 },
      { label: "30 minutes", value: 1800 },
      { label: "1 hour", value: 3600 },
    ],
    state.scheduleWakeTimeout,
    function () {
      state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(this.value);
      postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
      syncScreenScheduleUi();
    }
  );
  offScreenOptions.appendChild(wakeTimeoutControl.field);

  var wakeBrightnessSlider = createRangeSlider(
    "When Woken, Screen Brightness",
    state.scheduleWakeBrightness,
    postScreenScheduleWakeBrightness
  );
  wakeBrightnessSlider.range.id = idPrefix + "-wake-brightness";
  wakeBrightnessSlider.range.addEventListener("change", function () {
    state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(this.value);
    syncScreenScheduleUi();
  });
  offScreenOptions.appendChild(wakeBrightnessSlider.wrap);
  fields.push(offScreenOptions);

  var dimmedOptions = condField();
  var dimmedBrightnessSlider = createRangeSlider(
    "Dimmed Screen Brightness",
    state.scheduleDimmedBrightness,
    postScreenScheduleDimmedBrightness
  );
  dimmedBrightnessSlider.range.id = idPrefix + "-dimmed-brightness";
  dimmedBrightnessSlider.range.min = "1";
  dimmedBrightnessSlider.range.step = "1";
  dimmedBrightnessSlider.range.addEventListener("input", function () {
    state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(this.value);
    syncScreenScheduleUi();
  });
  dimmedOptions.appendChild(dimmedBrightnessSlider.wrap);
  fields.push(dimmedOptions);

  var clockOptions = condField();
  var clockBrightnessSlider = createRangeSlider(
    "Clock Brightness",
    state.scheduleClockBrightness,
    postScreenScheduleClockBrightness
  );
  clockBrightnessSlider.range.id = idPrefix + "-clock-brightness";
  clockBrightnessSlider.range.min = "1";
  clockBrightnessSlider.range.step = "1";
  clockBrightnessSlider.range.addEventListener("input", function () {
    state.scheduleClockBrightness = normalizeScheduleClockBrightness(this.value);
    syncScreenScheduleUi();
  });
  clockOptions.appendChild(clockBrightnessSlider.wrap);
  clockOptions.appendChild(fieldLabel("Clock Text Colour"));
  var clockTextColor = colorField(
    idPrefix + "-clock-text-color",
    state.scheduleClockTextColor,
    function (hex) {
      state.scheduleClockTextColor = normalizeHexColor(hex, "FFFFFF");
      postText(entityName("screen_schedule_clock_text_color"), state.scheduleClockTextColor);
    }
  );
  clockOptions.appendChild(clockTextColor);
  fields.push(clockOptions);

  return {
    fields: fields,
    modeSelect: modeControl.select,
    wakeTimeoutSelect: wakeTimeoutControl.select,
    wakeBrightness: wakeBrightnessSlider.range,
    wakeBrightnessVal: wakeBrightnessSlider.val,
    offOptions: offScreenOptions,
    dimmedOptions: dimmedOptions,
    dimmedBrightness: dimmedBrightnessSlider.range,
    dimmedBrightnessVal: dimmedBrightnessSlider.val,
    clockOptions: clockOptions,
    clockBrightness: clockBrightnessSlider.range,
    clockBrightnessVal: clockBrightnessSlider.val,
    clockTextColor: clockTextColor,
  };
}
