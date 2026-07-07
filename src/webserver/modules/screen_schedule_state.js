// ── Screen Schedule State ──────────────────────────────────────────────
// @web-module-requires: state

function normalizeHour(value, fallback) {
  return EspControlModel.normalizeHour(value, fallback);
}

function normalizeTimeOfDay(value, fallback) {
  return EspControlModel.normalizeTimeOfDay(value, fallback);
}

function normalizeScheduleWakeTimeout(value) {
  return EspControlModel.normalizeScheduleWakeTimeout(value);
}

function normalizeScheduleWakeBrightness(value) {
  return EspControlModel.normalizeScheduleWakeBrightness(value);
}

function normalizeScheduleClockBrightness(value) {
  return EspControlModel.normalizeScheduleClockBrightness(value);
}

function normalizeHexColor(value, fallback) {
  return EspControlModel.normalizeHexColor(value, fallback);
}

function normalizeScheduleDimmedBrightness(value) {
  return EspControlModel.normalizeScheduleDimmedBrightness(value);
}

function normalizeScheduleMode(value) {
  return EspControlModel.normalizeScheduleMode(value);
}

function normalizeScheduleTrigger(value, scheduleEnabled) {
  return EspControlModel.normalizeScheduleTrigger(value, scheduleEnabled);
}

function scheduleModeOption(value) {
  return EspControlModel.scheduleModeOption(value);
}

function formatDuration(seconds) {
  seconds = normalizeScheduleWakeTimeout(seconds);
  if (seconds < 60) return seconds + " second" + (seconds === 1 ? "" : "s");
  if (seconds % 60 === 0) {
    var minutes = seconds / 60;
    return minutes + " minute" + (minutes === 1 ? "" : "s");
  }
  return seconds + " seconds";
}

function formatHour(hour) {
  hour = normalizeHour(hour, 0);
  var suffix = hour < 12 ? "AM" : "PM";
  var h = hour % 12;
  if (h === 0) h = 12;
  return h + ":00 " + suffix;
}

function syncScreenScheduleUi() {
  state.scheduleTrigger = normalizeScheduleTrigger(state.scheduleTrigger, state.scheduleEnabled);
  state.scheduleEnabled = state.scheduleTrigger !== "disabled";
  state.scheduleOnHour = normalizeHour(state.scheduleOnHour, 6);
  state.scheduleOffHour = normalizeHour(state.scheduleOffHour, 23);
  state.scheduleMode = normalizeScheduleMode(state.scheduleMode);
  state.schedulePresenceDetectedMode = normalizeScheduleMode(state.schedulePresenceDetectedMode);
  state.schedulePresenceNotDetectedMode = normalizeScheduleMode(state.schedulePresenceNotDetectedMode);
  state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(state.scheduleWakeTimeout);
  state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(state.scheduleWakeBrightness);
  state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(state.scheduleDimmedBrightness);
  state.scheduleClockBrightness = normalizeScheduleClockBrightness(state.scheduleClockBrightness);
  state.brightnessDawnTime = normalizeTimeOfDay(state.brightnessDawnTime, "06:00");
  state.brightnessDuskTime = normalizeTimeOfDay(state.brightnessDuskTime, "18:00");
  if (els.setAutomaticBrightnessToggle) {
    els.setAutomaticBrightnessToggle.checked = !!state.automaticBrightnessEnabled;
  }
  if (els.setBrightnessDawnTime) els.setBrightnessDawnTime.value = state.brightnessDawnTime;
  if (els.setBrightnessDuskTime) els.setBrightnessDuskTime.value = state.brightnessDuskTime;
  if (els.setBrightnessManualTimes) {
    els.setBrightnessManualTimes.className =
      "sp-cond-field" + (!state.automaticBrightnessEnabled ? " sp-visible" : "");
  }
  if (els.setScheduleToggle) els.setScheduleToggle.checked = !!state.scheduleEnabled;
  if (els.setScheduleModeButtons) {
    els.setScheduleModeButtons.disabled.className = state.scheduleTrigger === "disabled" ? "active" : "";
    els.setScheduleModeButtons.time.className = state.scheduleTrigger === "time" ? "active" : "";
    els.setScheduleModeButtons.sensor.className = state.scheduleTrigger === "sensor" ? "active" : "";
  }
  if (els.setScheduleOnHour) els.setScheduleOnHour.value = String(state.scheduleOnHour);
  if (els.setScheduleOffHour) els.setScheduleOffHour.value = String(state.scheduleOffHour);
  if (els.setScheduleMode) {
    setSelectValue(els.setScheduleMode, state.scheduleMode, scheduleModeOption(state.scheduleMode));
  }
  if (els.setScheduleSensorMode) {
    setSelectValue(
      els.setScheduleSensorMode,
      state.schedulePresenceDetectedMode,
      scheduleModeOption(state.schedulePresenceDetectedMode)
    );
  }
  if (els.setScheduleSensorNotDetectedMode) {
    setSelectValue(
      els.setScheduleSensorNotDetectedMode,
      state.schedulePresenceNotDetectedMode,
      scheduleModeOption(state.schedulePresenceNotDetectedMode)
    );
  }
  setSelectValue(els.setScheduleWakeTimeout, state.scheduleWakeTimeout, formatDuration(state.scheduleWakeTimeout));
  setSelectValue(els.setScheduleSensorWakeTimeout, state.scheduleWakeTimeout, formatDuration(state.scheduleWakeTimeout));
  if (els.setScheduleWakeBrightness) {
    els.setScheduleWakeBrightness.value = state.scheduleWakeBrightness;
    els.setScheduleWakeBrightnessVal.textContent = Math.round(state.scheduleWakeBrightness) + "%";
  }
  if (els.setScheduleSensorWakeBrightness) {
    els.setScheduleSensorWakeBrightness.value = state.scheduleWakeBrightness;
    els.setScheduleSensorWakeBrightnessVal.textContent = Math.round(state.scheduleWakeBrightness) + "%";
  }
  if (els.setScheduleDimmedBrightness) {
    els.setScheduleDimmedBrightness.value = state.scheduleDimmedBrightness;
    els.setScheduleDimmedBrightnessVal.textContent = Math.round(state.scheduleDimmedBrightness) + "%";
  }
  if (els.setScheduleSensorDimmedBrightness) {
    els.setScheduleSensorDimmedBrightness.value = state.scheduleDimmedBrightness;
    els.setScheduleSensorDimmedBrightnessVal.textContent = Math.round(state.scheduleDimmedBrightness) + "%";
  }
  if (els.setScheduleClockBrightness) {
    els.setScheduleClockBrightness.value = state.scheduleClockBrightness;
    els.setScheduleClockBrightnessVal.textContent = Math.round(state.scheduleClockBrightness) + "%";
  }
  if (els.setScheduleSensorClockBrightness) {
    els.setScheduleSensorClockBrightness.value = state.scheduleClockBrightness;
    els.setScheduleSensorClockBrightnessVal.textContent = Math.round(state.scheduleClockBrightness) + "%";
  }
  if (els.setScheduleSensorNotDetectedWakeBrightness) {
    els.setScheduleSensorNotDetectedWakeBrightness.value = state.scheduleWakeBrightness;
    els.setScheduleSensorNotDetectedWakeBrightnessVal.textContent = Math.round(state.scheduleWakeBrightness) + "%";
  }
  if (els.setScheduleSensorNotDetectedDimmedBrightness) {
    els.setScheduleSensorNotDetectedDimmedBrightness.value = state.scheduleDimmedBrightness;
    els.setScheduleSensorNotDetectedDimmedBrightnessVal.textContent = Math.round(state.scheduleDimmedBrightness) + "%";
  }
  if (els.setScheduleSensorNotDetectedClockBrightness) {
    els.setScheduleSensorNotDetectedClockBrightness.value = state.scheduleClockBrightness;
    els.setScheduleSensorNotDetectedClockBrightnessVal.textContent = Math.round(state.scheduleClockBrightness) + "%";
  }
  if (els.setScheduleClockTextColor && els.setScheduleClockTextColor._syncColor) {
    els.setScheduleClockTextColor._syncColor(state.scheduleClockTextColor);
  }
  if (els.setScheduleSensorClockTextColor && els.setScheduleSensorClockTextColor._syncColor) {
    els.setScheduleSensorClockTextColor._syncColor(state.scheduleClockTextColor);
  }
  if (els.setScheduleSensorNotDetectedClockTextColor && els.setScheduleSensorNotDetectedClockTextColor._syncColor) {
    els.setScheduleSensorNotDetectedClockTextColor._syncColor(state.scheduleClockTextColor);
  }
  if (els.setScheduleOffOptions) {
    els.setScheduleOffOptions.className =
      "sp-cond-field" + (state.scheduleMode === "screen_off" ? " sp-visible" : "");
  }
  if (els.setScheduleSensorOffOptions) {
    els.setScheduleSensorOffOptions.className =
      "sp-cond-field" + (state.schedulePresenceDetectedMode === "screen_off" ? " sp-visible" : "");
  }
  if (els.setScheduleSensorNotDetectedOffOptions) {
    els.setScheduleSensorNotDetectedOffOptions.className =
      "sp-cond-field" + (state.schedulePresenceNotDetectedMode === "screen_off" ? " sp-visible" : "");
  }
  if (els.setScheduleDimmedOptions) {
    els.setScheduleDimmedOptions.className =
      "sp-cond-field" + (state.scheduleMode === "screen_dimmed" ? " sp-visible" : "");
  }
  if (els.setScheduleSensorDimmedOptions) {
    els.setScheduleSensorDimmedOptions.className =
      "sp-cond-field" + (state.schedulePresenceDetectedMode === "screen_dimmed" ? " sp-visible" : "");
  }
  if (els.setScheduleSensorNotDetectedDimmedOptions) {
    els.setScheduleSensorNotDetectedDimmedOptions.className =
      "sp-cond-field" + (state.schedulePresenceNotDetectedMode === "screen_dimmed" ? " sp-visible" : "");
  }
  if (els.setScheduleClockOptions) {
    els.setScheduleClockOptions.className =
      "sp-cond-field" + (state.scheduleMode === "clock" ? " sp-visible" : "");
  }
  if (els.setScheduleSensorClockOptions) {
    els.setScheduleSensorClockOptions.className =
      "sp-cond-field" + (state.schedulePresenceDetectedMode === "clock" ? " sp-visible" : "");
  }
  if (els.setScheduleSensorNotDetectedClockOptions) {
    els.setScheduleSensorNotDetectedClockOptions.className =
      "sp-cond-field" + (state.schedulePresenceNotDetectedMode === "clock" ? " sp-visible" : "");
  }
  if (els.setScheduleTimes) {
    els.setScheduleTimes.className = "sp-schedule-times" + (state.scheduleTrigger === "time" ? "" : " sp-hidden");
  }
  if (els.setScheduleSensor) {
    els.setScheduleSensor.className = "sp-schedule-times" + (state.scheduleTrigger === "sensor" ? "" : " sp-hidden");
  }
  if (els.setScheduleBadge) {
    els.setScheduleBadge.className = "sp-card-badge" + (state.scheduleEnabled ? "" : " sp-hidden");
  }
}
