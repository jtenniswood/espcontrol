import { state } from "../state/app_instance";
import {
    brightnessModeOption,
    normalizeScheduleTrigger,
    normalizeTimeOfDay,
    scheduleModeOption,
    scheduleSensorActivationOption,
} from "../model/settings";
import type { EntityStateFeature } from "./entity_state";
import type { ApplicationApiFeature } from "./api";
export interface ScreenSchedulePostApiFeature {
    postBrightnessMode(value?: any): void;
    postDisplayBacklightBrightness(value?: any): void;
    postBrightnessDawnTime(value?: any): void;
    postBrightnessDuskTime(value?: any): void;
    postScreenScheduleEnabled(on?: any): void;
    postScreenScheduleTrigger(value?: any): void;
    postScreenScheduleSensorActivation(value?: any): void;
    postScreenScheduleSensorEntity(value?: any): any;
    postScreenScheduleOnHour(value?: any): void;
    postScreenScheduleOffHour(value?: any): void;
    postScreenScheduleMode(value?: any): void;
    postScreenScheduleWakeTimeout(value?: any): void;
    postScreenScheduleWakeBrightness(value?: any): void;
    postScreenScheduleDimmedBrightness(value?: any): void;
    postScreenScheduleClockBrightness(value?: any): void;
}

export function createScreenSchedulePostApiFeature(
    entityState: Pick<EntityStateFeature, "entityName" | "entityObjectIds">,
    requestApi: Pick<ApplicationApiFeature, "postWithObjectIds" | "postTextWithObjectIds" | "postNumberWithObjectIds" | "postSelectWithObjectIds" | "postSwitchWithObjectIds">,
): ScreenSchedulePostApiFeature {
    const { entityName, entityObjectIds } = entityState;
    const {
        postWithObjectIds,
        postTextWithObjectIds,
        postNumberWithObjectIds,
        postSelectWithObjectIds,
        postSwitchWithObjectIds,
    } = requestApi;
    // ── Screen Schedule Post API ──────────────────────────────────────────
    var SCREEN_SCHEDULE_UNAVAILABLE: any = "Screen schedule is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE: any = "The schedule trigger setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_SENSOR_ACTIVATION_UNAVAILABLE: any = "The schedule sensor activation setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_SENSOR_ENTITY_UNAVAILABLE: any = "The schedule sensor setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE: any = "The schedule wake timeout setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE: any = "The schedule wake brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_MODE_UNAVAILABLE: any = "The schedule mode setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE: any = "The schedule dimmed brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE: any = "The schedule clock brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
    var BRIGHTNESS_MODE_UNAVAILABLE: any = "Brightness modes are not available on this firmware. Update the device firmware, then reload this page.";
    var DISPLAY_BACKLIGHT_UNAVAILABLE: any = "Manual backlight control is not available on this firmware. Update the device firmware, then reload this page.";
    var BRIGHTNESS_TIME_UNAVAILABLE: any = "Manual brightness times are not available on this firmware. Update the device firmware, then reload this page.";
    function postBrightnessMode(this: any, value?: any) {
        postSelectWithObjectIds(entityName("screen_brightness_mode"), entityObjectIds("screen_brightness_mode"), brightnessModeOption(value), BRIGHTNESS_MODE_UNAVAILABLE);
    }
    function postDisplayBacklightBrightness(this: any, value?: any) {
        var percent: any = parseFloat(value);
        if (!isFinite(percent))
            percent = 100;
        percent = Math.max(1, Math.min(100, percent));
        var brightness255: any = Math.round(percent * 2.55);
        postWithObjectIds("light", entityName("display_backlight"), entityObjectIds("display_backlight"), "turn_on?brightness=" + brightness255, DISPLAY_BACKLIGHT_UNAVAILABLE);
    }
    function postBrightnessDawnTime(this: any, value?: any) {
        postTextWithObjectIds(entityName("screen_brightness_dawn_time"), entityObjectIds("screen_brightness_dawn_time"), normalizeTimeOfDay(value, state.brightnessDawnTime || "06:00"), BRIGHTNESS_TIME_UNAVAILABLE);
    }
    function postBrightnessDuskTime(this: any, value?: any) {
        postTextWithObjectIds(entityName("screen_brightness_dusk_time"), entityObjectIds("screen_brightness_dusk_time"), normalizeTimeOfDay(value, state.brightnessDuskTime || "18:00"), BRIGHTNESS_TIME_UNAVAILABLE);
    }
    function postScreenScheduleEnabled(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_schedule_enabled"), entityObjectIds("screen_schedule_enabled"), on, SCREEN_SCHEDULE_UNAVAILABLE);
    }
    function postScreenScheduleTrigger(this: any, value?: any) {
        postTextWithObjectIds(entityName("screen_schedule_trigger"), entityObjectIds("screen_schedule_trigger"), normalizeScheduleTrigger(value, state.scheduleEnabled), SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE);
    }
    function postScreenScheduleSensorActivation(this: any, value?: any) {
        postSelectWithObjectIds(entityName("screen_schedule_sensor_activation"), entityObjectIds("screen_schedule_sensor_activation"), scheduleSensorActivationOption(value), SCREEN_SCHEDULE_SENSOR_ACTIVATION_UNAVAILABLE);
    }
    function postScreenScheduleSensorEntity(this: any, value?: any) {
        return postTextWithObjectIds(entityName("screen_schedule_sensor_entity"), entityObjectIds("screen_schedule_sensor_entity"), value, SCREEN_SCHEDULE_SENSOR_ENTITY_UNAVAILABLE);
    }
    function postScreenScheduleOnHour(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_schedule_on_hour"), entityObjectIds("screen_schedule_on_hour"), value, SCREEN_SCHEDULE_UNAVAILABLE);
    }
    function postScreenScheduleOffHour(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_schedule_off_hour"), entityObjectIds("screen_schedule_off_hour"), value, SCREEN_SCHEDULE_UNAVAILABLE);
    }
    function postScreenScheduleMode(this: any, value?: any) {
        postSelectWithObjectIds(entityName("screen_schedule_mode"), entityObjectIds("screen_schedule_mode"), scheduleModeOption(value), SCREEN_SCHEDULE_MODE_UNAVAILABLE);
    }
    function postScreenScheduleWakeTimeout(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_schedule_wake_timeout"), entityObjectIds("screen_schedule_wake_timeout"), value, SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE);
    }
    function postScreenScheduleWakeBrightness(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_schedule_wake_brightness"), entityObjectIds("screen_schedule_wake_brightness"), value, SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE);
    }
    function postScreenScheduleDimmedBrightness(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_schedule_dimmed_brightness"), entityObjectIds("screen_schedule_dimmed_brightness"), value, SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE);
    }
    function postScreenScheduleClockBrightness(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_schedule_clock_brightness"), entityObjectIds("screen_schedule_clock_brightness"), value, SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE);
    }
    return {
        postBrightnessMode,
        postDisplayBacklightBrightness,
        postBrightnessDawnTime,
        postBrightnessDuskTime,
        postScreenScheduleEnabled,
        postScreenScheduleTrigger,
        postScreenScheduleSensorActivation,
        postScreenScheduleSensorEntity,
        postScreenScheduleOnHour,
        postScreenScheduleOffHour,
        postScreenScheduleMode,
        postScreenScheduleWakeTimeout,
        postScreenScheduleWakeBrightness,
        postScreenScheduleDimmedBrightness,
        postScreenScheduleClockBrightness,
    };
}
