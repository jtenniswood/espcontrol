import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installScreenScheduleStateModule(): GlobalDescriptors {
    // ── Screen Schedule State ──────────────────────────────────────────────
    function formatDuration(this: any, seconds?: any) {
        seconds = normalizeScheduleWakeTimeout(seconds);
        if (seconds < 60)
            return seconds + " second" + (seconds === 1 ? "" : "s");
        if (seconds % 60 === 0) {
            var minutes: any = seconds / 60;
            return minutes + " minute" + (minutes === 1 ? "" : "s");
        }
        return seconds + " seconds";
    }
    function formatHour(this: any, hour?: any) {
        hour = normalizeHour(hour, 0);
        var suffix: any = hour < 12 ? "AM" : "PM";
        var h: any = hour % 12;
        if (h === 0)
            h = 12;
        return h + ":00 " + suffix;
    }
    function syncScreenScheduleUi(this: any) {
        state.scheduleTrigger = normalizeScheduleTrigger(state.scheduleTrigger, state.scheduleEnabled);
        state.scheduleEnabled = state.scheduleTrigger !== "disabled";
        state.scheduleOnHour = normalizeHour(state.scheduleOnHour, 6);
        state.scheduleOffHour = normalizeHour(state.scheduleOffHour, 23);
        state.scheduleMode = normalizeScheduleMode(state.scheduleMode);
        state.scheduleWakeTimeout = normalizeScheduleWakeTimeout(state.scheduleWakeTimeout);
        state.scheduleWakeBrightness = normalizeScheduleWakeBrightness(state.scheduleWakeBrightness);
        state.scheduleDimmedBrightness = normalizeScheduleDimmedBrightness(state.scheduleDimmedBrightness);
        state.scheduleClockBrightness = normalizeScheduleClockBrightness(state.scheduleClockBrightness);
        state.brightnessDawnTime = normalizeTimeOfDay(state.brightnessDawnTime, "06:00");
        state.brightnessDuskTime = normalizeTimeOfDay(state.brightnessDuskTime, "18:00");
        if (els.setAutomaticBrightnessToggle) {
            els.setAutomaticBrightnessToggle.checked = !!state.automaticBrightnessEnabled;
        }
        if (els.setBrightnessDawnTime)
            els.setBrightnessDawnTime.value = state.brightnessDawnTime;
        if (els.setBrightnessDuskTime)
            els.setBrightnessDuskTime.value = state.brightnessDuskTime;
        if (els.setBrightnessManualTimes) {
            els.setBrightnessManualTimes.className =
                "sp-cond-field" + (!state.automaticBrightnessEnabled ? " sp-visible" : "");
        }
        if (els.setScheduleToggle)
            els.setScheduleToggle.checked = !!state.scheduleEnabled;
        if (els.setScheduleModeButtons) {
            els.setScheduleModeButtons.disabled.className = state.scheduleTrigger === "disabled" ? "active" : "";
            els.setScheduleModeButtons.time.className = state.scheduleTrigger === "time" ? "active" : "";
            els.setScheduleModeButtons.sensor.className = state.scheduleTrigger === "sensor" ? "active" : "";
        }
        if (els.setScheduleOnHour)
            els.setScheduleOnHour.value = String(state.scheduleOnHour);
        if (els.setScheduleOffHour)
            els.setScheduleOffHour.value = String(state.scheduleOffHour);
        if (els.setScheduleMode) {
            setSelectValue(els.setScheduleMode, state.scheduleMode, scheduleModeOption(state.scheduleMode));
        }
        setSelectValue(els.setScheduleWakeTimeout, state.scheduleWakeTimeout, formatDuration(state.scheduleWakeTimeout));
        if (els.setScheduleWakeBrightness) {
            els.setScheduleWakeBrightness.value = state.scheduleWakeBrightness;
            els.setScheduleWakeBrightnessVal.textContent = Math.round(state.scheduleWakeBrightness) + "%";
        }
        if (els.setScheduleDimmedBrightness) {
            els.setScheduleDimmedBrightness.value = state.scheduleDimmedBrightness;
            els.setScheduleDimmedBrightnessVal.textContent = Math.round(state.scheduleDimmedBrightness) + "%";
        }
        if (els.setScheduleClockBrightness) {
            els.setScheduleClockBrightness.value = state.scheduleClockBrightness;
            els.setScheduleClockBrightnessVal.textContent = Math.round(state.scheduleClockBrightness) + "%";
        }
        if (els.setScheduleClockTextColor && els.setScheduleClockTextColor._syncColor) {
            els.setScheduleClockTextColor._syncColor(state.scheduleClockTextColor);
        }
        if (els.setScheduleOffOptions) {
            els.setScheduleOffOptions.className =
                "sp-cond-field" + (state.scheduleMode === "screen_off" ? " sp-visible" : "");
        }
        if (els.setScheduleDimmedOptions) {
            els.setScheduleDimmedOptions.className =
                "sp-cond-field" + (state.scheduleMode === "screen_dimmed" ? " sp-visible" : "");
        }
        if (els.setScheduleClockOptions) {
            els.setScheduleClockOptions.className =
                "sp-cond-field" + (state.scheduleMode === "clock" ? " sp-visible" : "");
        }
        if (els.setScheduleTimes) {
            els.setScheduleTimes.className = "sp-schedule-times" + (state.scheduleTrigger === "time" ? "" : " sp-hidden");
        }
        if (els.setScheduleSensor) {
            els.setScheduleSensor.className = "sp-schedule-times" + (state.scheduleTrigger === "sensor" ? "" : " sp-hidden");
        }
        if (els.setScheduleActions) {
            els.setScheduleActions.className = "sp-schedule-times" + (state.scheduleTrigger === "time" || state.scheduleTrigger === "sensor" ? "" : " sp-hidden");
        }
        if (els.setScheduleBadge) {
            els.setScheduleBadge.className = "sp-card-badge" + (state.scheduleEnabled ? "" : " sp-hidden");
        }
    }
    return {
        "formatDuration": staticGlobal(formatDuration),
        "formatHour": staticGlobal(formatHour),
        "syncScreenScheduleUi": staticGlobal(syncScreenScheduleUi),
    };
}
