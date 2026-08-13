import { state } from "../state/app_instance";
import {
    normalizeBrightnessMode,
    normalizeHour,
    normalizeScheduleWakeTimeout,
    normalizeTimeOfDay,
    scheduleModeOption,
    scheduleSensorActivationOption,
} from "../model/settings";
import { setSelectValue } from "./ui_primitives";
import type { ScreenScheduleController } from "../features/screen_schedule_controller";
import type { UiRuntimeState } from "./state";

export interface ScreenScheduleStateFeature {
    readonly controller: ScreenScheduleController;
    controllerState(): any;
    applyControllerState(next: any): void;
    formatDuration(seconds?: any): string;
    formatHour(hour?: any): string;
    syncUi(): void;
}

export function createScreenScheduleStateFeature(
    screenScheduleController: ScreenScheduleController,
    runtime: UiRuntimeState,
    dependencies: {
        syncClockScreensaverControls(): void;
        updateSunInfo(): void;
    },
): ScreenScheduleStateFeature {
    const els = runtime.els;
    function controllerState() {
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
    function applyControllerState(next?: any) {
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
    function formatDuration(seconds?: any) {
        seconds = normalizeScheduleWakeTimeout(seconds);
        if (seconds < 60)
            return seconds + " second" + (seconds === 1 ? "" : "s");
        if (seconds % 60 === 0) {
            var minutes: any = seconds / 60;
            return minutes + " minute" + (minutes === 1 ? "" : "s");
        }
        return seconds + " seconds";
    }
    function formatHour(hour?: any) {
        hour = normalizeHour(hour, 0);
        var suffix: any = hour < 12 ? "AM" : "PM";
        var h: any = hour % 12;
        if (h === 0)
            h = 12;
        return h + ":00 " + suffix;
    }
    function syncUi() {
        applyControllerState(screenScheduleController.normalize(controllerState()));
        var uiState: any = screenScheduleController.uiState(controllerState());
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
            dependencies.syncClockScreensaverControls();
        dependencies.updateSunInfo();
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
        controller: screenScheduleController,
        controllerState,
        applyControllerState,
        formatDuration,
        formatHour,
        syncUi,
    };
}
