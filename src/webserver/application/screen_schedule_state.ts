import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installScreenScheduleStateModule(): GlobalDescriptors {
    // ── Screen Schedule State ──────────────────────────────────────────────
    var _screenScheduleController: any = createScreenScheduleController({
        trigger: normalizeScheduleTrigger,
        sensorActivation: normalizeScheduleSensorActivation,
        hour: normalizeHour,
        mode: normalizeScheduleMode,
        wakeTimeout: normalizeScheduleWakeTimeout,
        wakeBrightness: normalizeScheduleWakeBrightness,
        dimmedBrightness: normalizeScheduleDimmedBrightness,
        clockBrightness: normalizeScheduleClockBrightness,
    });
    function screenScheduleControllerState(this: any) {
        return {
            trigger: state.scheduleTrigger,
            sensorActivation: state.scheduleSensorActivation,
            onHour: state.scheduleOnHour,
            offHour: state.scheduleOffHour,
            mode: state.scheduleMode,
            wakeTimeout: state.scheduleWakeTimeout,
            wakeBrightness: state.scheduleWakeBrightness,
            dimmedBrightness: state.scheduleDimmedBrightness,
            clockBrightness: state.scheduleClockBrightness,
        };
    }
    function applyScreenScheduleControllerState(this: any, next?: any) {
        state.scheduleTrigger = next.trigger;
        state.scheduleSensorActivation = next.sensorActivation;
        state.scheduleOnHour = next.onHour;
        state.scheduleOffHour = next.offHour;
        state.scheduleMode = next.mode;
        state.scheduleWakeTimeout = next.wakeTimeout;
        state.scheduleWakeBrightness = next.wakeBrightness;
        state.scheduleDimmedBrightness = next.dimmedBrightness;
        state.scheduleClockBrightness = next.clockBrightness;
        state.scheduleEnabled = next.trigger !== "disabled";
    }
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
        applyScreenScheduleControllerState(_screenScheduleController.normalize(screenScheduleControllerState()));
        var uiState: any = _screenScheduleController.uiState(screenScheduleControllerState());
        state.brightnessMode = normalizeBrightnessMode(state.brightnessMode);
        state.brightnessDawnTime = normalizeTimeOfDay(state.brightnessDawnTime, "06:00");
        state.brightnessDuskTime = normalizeTimeOfDay(state.brightnessDuskTime, "18:00");
        if (els.setBrightnessModeButtons) {
            for (var brightnessModeKey in els.setBrightnessModeButtons) {
                els.setBrightnessModeButtons[brightnessModeKey].classList.toggle(
                    "active", brightnessModeKey === state.brightnessMode);
            }
        }
        if (els.setManualBrightnessField) {
            els.setManualBrightnessField.className =
                "sp-cond-field" + (state.brightnessMode === "manual" ? " sp-visible" : "");
        }
        if (els.setBrightnessAutomaticFields) {
            els.setBrightnessAutomaticFields.className =
                "sp-cond-field" + (state.brightnessMode !== "manual" ? " sp-visible" : "");
        }
        if (els.setBrightnessDawnTime)
            els.setBrightnessDawnTime.value = state.brightnessDawnTime;
        if (els.setBrightnessDuskTime)
            els.setBrightnessDuskTime.value = state.brightnessDuskTime;
        if (els.setBrightnessManualTimes) {
            els.setBrightnessManualTimes.className =
                "sp-cond-field" + (state.brightnessMode === "fixed_times" ? " sp-visible" : "");
        }
        if (els.setDimBrightnessField || els.setSensorDimBrightnessField)
            syncClockScreensaverControls();
        updateSunInfo();
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
        if (els.setScheduleSensorActivation) {
            setSelectValue(els.setScheduleSensorActivation, state.scheduleSensorActivation, scheduleSensorActivationOption(state.scheduleSensorActivation));
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
                "sp-cond-field" + (uiState.screenOffOptionsVisible ? " sp-visible" : "");
        }
        if (els.setScheduleDimmedOptions) {
            els.setScheduleDimmedOptions.className =
                "sp-cond-field" + (uiState.dimmedOptionsVisible ? " sp-visible" : "");
        }
        if (els.setScheduleClockOptions) {
            els.setScheduleClockOptions.className =
                "sp-cond-field" + (uiState.clockOptionsVisible ? " sp-visible" : "");
        }
        if (els.setScheduleTimes) {
            els.setScheduleTimes.className = "sp-schedule-times" + (uiState.timeControlsVisible ? "" : " sp-hidden");
        }
        if (els.setScheduleSensor) {
            els.setScheduleSensor.className = "sp-schedule-times sp-schedule-sensor" + (uiState.sensorControlsVisible ? "" : " sp-hidden");
        }
        if (els.setScheduleActions) {
            els.setScheduleActions.className = "sp-schedule-times" + (uiState.actionsVisible ? "" : " sp-hidden");
        }
        if (els.setScheduleBadge) {
            els.setScheduleBadge.className = "sp-card-badge" + (uiState.enabled ? "" : " sp-hidden");
        }
    }
    return {
        "_screenScheduleController": liveGlobal(() => _screenScheduleController, (value?: any) => { _screenScheduleController = value; }),
        "screenScheduleControllerState": staticGlobal(screenScheduleControllerState),
        "applyScreenScheduleControllerState": staticGlobal(applyScreenScheduleControllerState),
        "formatDuration": staticGlobal(formatDuration),
        "formatHour": staticGlobal(formatHour),
        "syncScreenScheduleUi": staticGlobal(syncScreenScheduleUi),
    };
}
