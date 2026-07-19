import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installScreenSchedulePostApiModule(): GlobalDescriptors {
    // ── Screen Schedule Post API ──────────────────────────────────────────
    var SCREEN_SCHEDULE_UNAVAILABLE: any = "Screen schedule is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE: any = "The schedule trigger setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_SENSOR_ACTIVATION_UNAVAILABLE: any = "The schedule sensor activation setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE: any = "The schedule wake timeout setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE: any = "The schedule wake brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_MODE_UNAVAILABLE: any = "The schedule mode setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE: any = "The schedule dimmed brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
    var SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE: any = "The schedule clock brightness setting is not available on this firmware. Update the device firmware, then reload this page.";
    var AUTOMATIC_BRIGHTNESS_UNAVAILABLE: any = "Automatic brightness control is not available on this firmware. Update the device firmware, then reload this page.";
    var BRIGHTNESS_TIME_UNAVAILABLE: any = "Manual brightness times are not available on this firmware. Update the device firmware, then reload this page.";
    function postAutomaticBrightnessEnabled(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_automatic_brightness"), entityObjectIds("screen_automatic_brightness"), on, AUTOMATIC_BRIGHTNESS_UNAVAILABLE);
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
        "SCREEN_SCHEDULE_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_UNAVAILABLE = value; }),
        "SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_TRIGGER_UNAVAILABLE = value; }),
        "SCREEN_SCHEDULE_SENSOR_ACTIVATION_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_SENSOR_ACTIVATION_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_SENSOR_ACTIVATION_UNAVAILABLE = value; }),
        "SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_WAKE_TIMEOUT_UNAVAILABLE = value; }),
        "SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_WAKE_BRIGHTNESS_UNAVAILABLE = value; }),
        "SCREEN_SCHEDULE_MODE_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_MODE_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_MODE_UNAVAILABLE = value; }),
        "SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_DIMMED_BRIGHTNESS_UNAVAILABLE = value; }),
        "SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE": liveGlobal(() => SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE, (value?: any) => { SCREEN_SCHEDULE_CLOCK_BRIGHTNESS_UNAVAILABLE = value; }),
        "AUTOMATIC_BRIGHTNESS_UNAVAILABLE": liveGlobal(() => AUTOMATIC_BRIGHTNESS_UNAVAILABLE, (value?: any) => { AUTOMATIC_BRIGHTNESS_UNAVAILABLE = value; }),
        "BRIGHTNESS_TIME_UNAVAILABLE": liveGlobal(() => BRIGHTNESS_TIME_UNAVAILABLE, (value?: any) => { BRIGHTNESS_TIME_UNAVAILABLE = value; }),
        "postAutomaticBrightnessEnabled": staticGlobal(postAutomaticBrightnessEnabled),
        "postBrightnessDawnTime": staticGlobal(postBrightnessDawnTime),
        "postBrightnessDuskTime": staticGlobal(postBrightnessDuskTime),
        "postScreenScheduleEnabled": staticGlobal(postScreenScheduleEnabled),
        "postScreenScheduleTrigger": staticGlobal(postScreenScheduleTrigger),
        "postScreenScheduleSensorActivation": staticGlobal(postScreenScheduleSensorActivation),
        "postScreenScheduleOnHour": staticGlobal(postScreenScheduleOnHour),
        "postScreenScheduleOffHour": staticGlobal(postScreenScheduleOffHour),
        "postScreenScheduleMode": staticGlobal(postScreenScheduleMode),
        "postScreenScheduleWakeTimeout": staticGlobal(postScreenScheduleWakeTimeout),
        "postScreenScheduleWakeBrightness": staticGlobal(postScreenScheduleWakeBrightness),
        "postScreenScheduleDimmedBrightness": staticGlobal(postScreenScheduleDimmedBrightness),
        "postScreenScheduleClockBrightness": staticGlobal(postScreenScheduleClockBrightness),
    };
}
