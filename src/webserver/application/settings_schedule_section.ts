import { state } from "../state/app_instance";
import { normalizeHexColor } from "../model/settings";
import type { ConfigCodecFeature } from "./config_codec";
import type { UiRuntimeState } from "./state";
import type { ScreenScheduleStateFeature } from "./screen_schedule_state";
import type { EntityStateFeature } from "./entity_state";
import type { ApplicationApiFeature } from "./api";
import type { ScreenSchedulePostApiFeature } from "./screen_schedule_post_api";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { SettingsPageHelpersFeature } from "./settings_page_helpers";

export interface SettingsScheduleSectionFeature {
    buildScreenScheduleSettingsCard(...args: any[]): any;
}

export function createSettingsScheduleSectionFeature(codec: Pick<ConfigCodecFeature, "bindTextPost">, runtime: UiRuntimeState, schedule: ScreenScheduleStateFeature, entityState: Pick<EntityStateFeature, "entityName" | "entityInput">, requestApi: Pick<ApplicationApiFeature, "postText">, schedulePostApi: ScreenSchedulePostApiFeature, fields: Pick<ControlsFieldsFeature, "colorField" | "condField" | "createRangeSlider" | "fieldLabel" | "makeCollapsibleCard" | "segmentControl" | "selectField">, helpers: Pick<SettingsPageHelpersFeature, "createHourSelect" | "infoPanel" | "statusBadge">): SettingsScheduleSectionFeature {
    const { createHourSelect, infoPanel, statusBadge } = helpers;
    const { colorField, condField, createRangeSlider, fieldLabel, makeCollapsibleCard, segmentControl, selectField } = fields;
    const { entityName, entityInput } = entityState;
    const { bindTextPost } = codec;
    const els = runtime.els;
    const {
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
    } = schedulePostApi;
    const {
        controller: _screenScheduleController,
        controllerState: screenScheduleControllerState,
        applyControllerState: applyScreenScheduleControllerState,
        syncUi: syncScreenScheduleUi,
    } = schedule;
    // ── Settings Schedule Section ──────────────────────────────────────
    function buildScreenScheduleSettingsCard(this: any) {
        var scheduleBody: any = document.createElement("div");
        scheduleBody.appendChild(infoPanel("sp-night-schedule-info", "Time-based Night Schedule overrides screensaver presence wake and Media Cover Art while it is active."));
        scheduleBody.appendChild(fieldLabel("Mode"));
        var scheduleModeSegment: any = segmentControl([
            ["disabled", "Disabled"],
            ["time", "Time"],
            ["sensor", "Sensor"],
        ], state.scheduleTrigger, function (this: any, mode?: any) {
            setScheduleTrigger(mode);
        }, "sp-segment sp-screensaver-mode");
        scheduleBody.appendChild(scheduleModeSegment.segment);
        els.setScheduleModeButtons = {
            disabled: scheduleModeSegment.buttons.disabled,
            time: scheduleModeSegment.buttons.time,
            sensor: scheduleModeSegment.buttons.sensor,
        };
        var scheduleTimes: any = document.createElement("div");
        scheduleTimes.className = "sp-schedule-times";
        var onHour: any = createHourSelect("Daytime", "sp-set-schedule-on-hour", state.scheduleOnHour, function (this: any, hour?: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setOnHour(screenScheduleControllerState(), hour));
            postScreenScheduleOnHour(state.scheduleOnHour);
            syncScreenScheduleUi();
        });
        scheduleTimes.appendChild(onHour.wrap);
        els.setScheduleOnHour = onHour.select;
        var offHour: any = createHourSelect("Night Time", "sp-set-schedule-off-hour", state.scheduleOffHour, function (this: any, hour?: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setOffHour(screenScheduleControllerState(), hour));
            postScreenScheduleOffHour(state.scheduleOffHour);
            syncScreenScheduleUi();
        });
        scheduleTimes.appendChild(offHour.wrap);
        els.setScheduleOffHour = offHour.select;
        scheduleBody.appendChild(scheduleTimes);
        els.setScheduleTimes = scheduleTimes;
        var scheduleSensor: any = document.createElement("div");
        scheduleSensor.className = "sp-schedule-times sp-schedule-sensor";
        var schedulePresenceField: any = document.createElement("div");
        schedulePresenceField.className = "sp-field";
        schedulePresenceField.appendChild(fieldLabel("Sensor Entity", "sp-set-schedule-presence"));
        var schedulePresInp: any = entityInput("sp-set-schedule-presence", state.scheduleSensorEntity, "Sensor Entity", ["binary_sensor", "sensor"]);
        schedulePresenceField.appendChild(schedulePresInp);
        scheduleSensor.appendChild(schedulePresenceField);
        bindTextPost(schedulePresInp, entityName("screen_schedule_sensor_entity"), {
            post: postScreenScheduleSensorEntity,
        });
        var sensorActivationControl: any = selectField("Activate Night Schedule When", "sp-set-schedule-sensor-activation", [
            { value: "off", label: "Sensor Is Off" },
            { value: "on", label: "Sensor Is On" },
        ], state.scheduleSensorActivation, function (this: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setSensorActivation(screenScheduleControllerState(), this.value));
            postScreenScheduleSensorActivation(state.scheduleSensorActivation);
            syncScreenScheduleUi();
        });
        scheduleSensor.appendChild(sensorActivationControl.field);
        els.setScheduleSensorActivation = sensorActivationControl.select;
        scheduleBody.appendChild(scheduleSensor);
        els.setScheduleSensor = scheduleSensor;
        els.setSchedulePresence = schedulePresInp;
        var scheduleActions: any = document.createElement("div");
        scheduleActions.className = "sp-schedule-times";
        scheduleActions.id = "sp-set-schedule-actions";
        var scheduleModeControl: any = selectField("At Night Time", "sp-set-schedule-mode", [
            { value: "screen_off", label: "Screen Off" },
            { value: "screen_dimmed", label: "Screen Dimmed" },
            { value: "clock", label: "Clock" },
        ], state.scheduleMode, function (this: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setMode(screenScheduleControllerState(), this.value));
            postScreenScheduleMode(state.scheduleMode);
            syncScreenScheduleUi();
        });
        var scheduleModeSelect: any = scheduleModeControl.select;
        scheduleActions.appendChild(scheduleModeControl.field);
        els.setScheduleMode = scheduleModeSelect;
        var offScreenOptions: any = condField();
        var wakeTimeoutOptions: any = [
            { label: "10 seconds", value: 10 },
            { label: "30 seconds", value: 30 },
            { label: "1 minute", value: 60 },
            { label: "2 minutes", value: 120 },
            { label: "5 minutes", value: 300 },
            { label: "10 minutes", value: 600 },
            { label: "30 minutes", value: 1800 },
            { label: "1 hour", value: 3600 },
        ];
        var wakeTimeoutControl: any = selectField("When Woken, Idle Time to Screen Off", "sp-set-schedule-wake-timeout", wakeTimeoutOptions, state.scheduleWakeTimeout, function (this: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setWakeTimeout(screenScheduleControllerState(), this.value));
            postScreenScheduleWakeTimeout(state.scheduleWakeTimeout);
            syncScreenScheduleUi();
        });
        var wakeTimeoutSelect: any = wakeTimeoutControl.select;
        offScreenOptions.appendChild(wakeTimeoutControl.field);
        els.setScheduleWakeTimeout = wakeTimeoutSelect;
        var wakeBrightnessSlider: any = createRangeSlider("When Woken, Screen Brightness", state.scheduleWakeBrightness, postScreenScheduleWakeBrightness);
        wakeBrightnessSlider.range.id = "sp-set-schedule-wake-brightness";
        wakeBrightnessSlider.range.addEventListener("change", function (this: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setWakeBrightness(screenScheduleControllerState(), this.value));
            syncScreenScheduleUi();
        });
        offScreenOptions.appendChild(wakeBrightnessSlider.wrap);
        els.setScheduleWakeBrightness = wakeBrightnessSlider.range;
        els.setScheduleWakeBrightnessVal = wakeBrightnessSlider.val;
        scheduleActions.appendChild(offScreenOptions);
        els.setScheduleOffOptions = offScreenOptions;
        var dimmedOptions: any = condField();
        var dimmedBrightnessSlider: any = createRangeSlider("Dimmed Screen Brightness", state.scheduleDimmedBrightness, postScreenScheduleDimmedBrightness);
        dimmedBrightnessSlider.range.id = "sp-set-schedule-dimmed-brightness";
        dimmedBrightnessSlider.range.min = "1";
        dimmedBrightnessSlider.range.step = "1";
        dimmedBrightnessSlider.range.addEventListener("input", function (this: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setDimmedBrightness(screenScheduleControllerState(), this.value));
            syncScreenScheduleUi();
        });
        dimmedOptions.appendChild(dimmedBrightnessSlider.wrap);
        scheduleActions.appendChild(dimmedOptions);
        els.setScheduleDimmedOptions = dimmedOptions;
        els.setScheduleDimmedBrightness = dimmedBrightnessSlider.range;
        els.setScheduleDimmedBrightnessVal = dimmedBrightnessSlider.val;
        var clockOptions: any = condField();
        var clockBrightnessSlider: any = createRangeSlider("Clock Brightness", state.scheduleClockBrightness, postScreenScheduleClockBrightness);
        clockBrightnessSlider.range.id = "sp-set-schedule-clock-brightness";
        clockBrightnessSlider.range.min = "1";
        clockBrightnessSlider.range.step = "1";
        clockBrightnessSlider.range.addEventListener("input", function (this: any) {
            applyScreenScheduleControllerState(_screenScheduleController.setClockBrightness(screenScheduleControllerState(), this.value));
            syncScreenScheduleUi();
        });
        clockOptions.appendChild(clockBrightnessSlider.wrap);
        clockOptions.appendChild(fieldLabel("Clock Text Colour"));
        var clockTextColor: any = colorField("sp-set-schedule-clock-text-color", state.scheduleClockTextColor, function (this: any, hex?: any) {
            state.scheduleClockTextColor = normalizeHexColor(hex, "FFFFFF");
            requestApi.postText(entityName("screen_schedule_clock_text_color"), state.scheduleClockTextColor);
        });
        clockOptions.appendChild(clockTextColor);
        scheduleActions.appendChild(clockOptions);
        els.setScheduleClockOptions = clockOptions;
        els.setScheduleClockBrightness = clockBrightnessSlider.range;
        els.setScheduleClockBrightnessVal = clockBrightnessSlider.val;
        els.setScheduleClockTextColor = clockTextColor;
        scheduleBody.appendChild(scheduleActions);
        els.setScheduleActions = scheduleActions;
        function setScheduleTrigger(this: any, trigger?: any) {
            state._scheduleTriggerReceived = true;
            applyScreenScheduleControllerState(_screenScheduleController.setTrigger(screenScheduleControllerState(), trigger));
            postScreenScheduleTrigger(state.scheduleTrigger);
            postScreenScheduleEnabled(state.scheduleEnabled);
            syncScreenScheduleUi();
        }
        var scheduleBadge: any = statusBadge("Schedule on");
        els.setScheduleBadge = scheduleBadge;
        syncScreenScheduleUi();
        var scheduleCard: any = makeCollapsibleCard("Night Schedule", scheduleBody, true, scheduleBadge);
        return scheduleCard;
    }
    return {
        buildScreenScheduleSettingsCard,
    };
}
