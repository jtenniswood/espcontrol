import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import {
    migrateSavedConfigVacuumLegacy,
    normalizeSavedConfigVacuumIconOn,
    normalizeSavedConfigVacuumOptions,
    normalizeSavedConfigVacuumPrecision,
    normalizeSavedConfigVacuumSensor,
} from "../generated/saved_config_vacuum";
import { migrateSavedConfigSensorLegacy, normalizeSavedConfigSensor } from "../generated/saved_config_sensor";
import { migrateSavedConfigActionLegacy, normalizeSavedConfigAction } from "../generated/saved_config_action";
import { normalizeSavedConfigMedia } from "../generated/saved_config_media";
import { normalizeSavedConfigStatic } from "../generated/saved_config_static";
import { normalizeSavedConfigFan } from "../generated/saved_config_fan";
import { normalizeSavedConfigDateTime } from "../generated/saved_config_date_time";
import { normalizeSavedConfigMower } from "../generated/saved_config_mower";
import { normalizeSavedConfigOccupancy } from "../generated/saved_config_occupancy";
import { normalizeSavedConfigAccess } from "../generated/saved_config_access";
import { normalizeSavedConfigSecurity } from "../generated/saved_config_security";
import { migrateSavedConfigWeatherLegacy, normalizeSavedConfigWeather } from "../generated/saved_config_weather";
import { normalizeSavedConfigImage } from "../generated/saved_config_image";
import { normalizeSavedConfigClimate } from "../generated/saved_config_climate";
import { normalizeSavedConfigLightControl } from "../generated/saved_config_light_control";
import { normalizeSavedConfigWebhook } from "../generated/saved_config_webhook";
import { normalizeSavedConfigSubpage } from "../generated/saved_config_subpage";
import { normalizeSavedConfigSwitch } from "../generated/saved_config_switch";
export function installConfigCodecModule(): GlobalDescriptors {
    // ── Subpage helpers ────────────────────────────────────────────────────
    function normalizeWithRegisteredCardType(this: any, b?: any) {
        if (!b || typeof BUTTON_TYPES === "undefined")
            return false;
        if (b.type === "action" || b.type === "lawn_mower")
            return false;
        var typeDef: any = BUTTON_TYPES[b.type || ""];
        if (!typeDef || typeof typeDef.normalizeConfig !== "function")
            return false;
        typeDef.normalizeConfig(b);
        return true;
    }
    function cardRequiresSquareSize(this: any, b?: any) {
        return !!(b && b.type === "media" && mediaEditorMode(b.sensor) === "cover_art");
    }
    function normalizeCardSizeForConfig(this: any, b?: any, size?: any) {
        size = size || CARD_SIZE_SINGLE;
        if (!cardRequiresSquareSize(b))
            return size;
        return size === CARD_SIZE_LARGE || size === CARD_SIZE_EXTRA_LARGE
            ? size
            : CARD_SIZE_SINGLE;
    }
    function normalizeSavedConfigSensorFields(this: any, b?: any, wasLegacyTextSensor?: any) {
        if (!b)
            return;
        if (wasLegacyTextSensor && !b.icon)
            b.icon = "Auto";
        if (sensorCardIsLocal(b)) {
            b.type = "sensor";
            b.sensor = SENSOR_CARD_LOCAL_SENSOR;
            b.icon_on = "Auto";
            b.options = "";
            if (b.precision !== "text" && b.precision !== "1" && b.precision !== "2")
                b.precision = "";
            if (b.precision !== "text" && (!b.icon || b.icon === "Auto"))
                b.icon = "Auto";
        }
    }
    function normalizeSavedConfigMediaFields(this: any, b?: any) {
        if (!b)
            return;
        var rawMediaMode: any = b.sensor;
        if (rawMediaMode === "controls" && (!b.icon || b.icon === "Speaker"))
            b.icon = "Auto";
        var mediaConfig: any = EspControlModel.decodeMediaCardConfigV1(b);
        b.sensor = mediaConfig ? mediaConfig.mode : mediaEditorMode(b.sensor);
        if (b.sensor === "previous" && b.label === "Skip Previous")
            b.label = "Previous";
        if (b.sensor === "next" && b.label === "Skip Next")
            b.label = "Next";
        if (b.sensor === "volume") {
            if (!b.label || b.label === "Media")
                b.label = "Volume";
            b.icon = "Auto";
        }
        if (b.sensor === "playlist") {
            if (!b.label || b.label === "Media")
                b.label = "Playlist";
            if (!b.icon || b.icon === "Auto")
                b.icon = "Music";
        }
        if (b.sensor === "position" && (!b.label || b.label === "Track"))
            b.label = "Position";
        if (b.sensor === "now_playing")
            b.precision = mediaConfig && mediaConfig.nowPlayingControl !== "none"
                ? mediaConfig.nowPlayingControl
                : "";
        else if (b.sensor === "cover_art")
            b.precision = "";
        else if (mediaStateDisplayModeSupported(b.sensor) && mediaConfig && mediaConfig.stateDisplay === "state")
            b.precision = "state";
        else
            b.precision = "";
    }
    function normalizeSavedConfigFanFields(this: any, b?: any) {
        if (!b)
            return;
        if (!b.icon || b.icon === "Auto")
            b.icon = fanCardDefaultIcon(b.type);
        if (b.type === "fan_switch") {
            if (!b.icon_on || b.icon_on === "Auto")
                b.icon_on = "Fan";
        }
        else {
            b.icon_on = "Auto";
        }
    }
    function normalizeSavedConfigDateTimeFields(this: any, b?: any) {
        if (!b || b.entity)
            return;
        if (b.type === "calendar")
            b.entity = cardContractDefaultConfig("calendar").entity;
        else if (b.type === "timezone")
            b.entity = cardContractDefaultConfig("timezone").entity;
    }
    function normalizeSavedConfigDateTimeOptions(this: any, options?: any, b?: any) {
        return normalizeDateTimeOptions(b && b.type || "", options || "", b && b.precision || "");
    }
    function normalizeSavedConfigMowerFields(this: any, b?: any) {
        if (!b)
            return;
        b.sensor = normalizeLawnMowerMode(b.sensor);
        if (!b.icon || b.icon === "Auto")
            b.icon = lawnMowerModeDefaultIcon(b.sensor);
    }
    function normalizeSavedConfigOccupancyFields(this: any, b?: any) {
        if (!b)
            return;
        if (b.type === "door_window") {
            b.precision = normalizeDoorWindowSubtype(b.precision);
            if (!b.icon || b.icon === "Auto")
                b.icon = doorWindowClosedIcon(b.precision);
            if (!b.icon_on || b.icon_on === "Auto")
                b.icon_on = doorWindowOpenIcon(b.precision);
        }
        else if (b.type === "presence") {
            if (!b.icon || b.icon === "Auto")
                b.icon = "Motion Sensor Off";
            if (!b.icon_on || b.icon_on === "Auto")
                b.icon_on = "Motion Sensor";
        }
    }
    function normalizeSavedConfigOccupancyOptions(this: any, options?: any, b?: any) {
        return b && b.type === "door_window"
            ? normalizeDoorWindowOptions(options || "")
            : normalizePresenceOptions(options || "");
    }
    function normalizeSavedConfigAccessFields(this: any, b?: any) {
        if (!b)
            return;
        if (b.type === "garage") {
            b.sensor = normalizeGarageMode(b.sensor);
            if (b.sensor)
                b.icon_on = "Auto";
        }
        else if (b.type === "gate") {
            b.sensor = normalizeGateMode(b.sensor);
            if (b.sensor)
                b.icon_on = "Auto";
        }
        else if (b.type === "cover") {
            b.sensor = normalizeCoverMode(b.sensor, true);
            if (b.sensor !== "set_position")
                b.unit = "";
        }
        else if (b.type === "lock") {
            b.sensor = normalizeLockMode(b.sensor);
            b.icon_on = b.sensor ? "Auto" : ((!b.icon_on || b.icon_on === "Auto") ? "Lock Open" : b.icon_on);
        }
    }
    function normalizeSavedConfigAccessOptions(this: any, options?: any, b?: any) {
        if (!b)
            return "";
        if (b.type === "garage")
            return normalizeGarageOptions(options || "", b.sensor);
        if (b.type === "gate")
            return normalizeGateOptions(options || "", b.sensor);
        return normalizeCoverOptionsForMode(options || "", b.sensor);
    }
    function normalizeSavedConfigSecurityFields(this: any, b?: any) {
        if (!b)
            return;
        if (b.type === "alarm") {
            if (!b.icon || b.icon === "Auto")
                b.icon = "Security";
            return;
        }
        b.sensor = alarmActionInfo(b.sensor) ? b.sensor : "away";
        if (!b.label)
            b.label = alarmActionInfo(b.sensor).label;
        if (!b.icon || b.icon === "Auto" || b.icon === alarmActionLegacyIcon(b.sensor))
            b.icon = alarmActionInfo(b.sensor).icon;
    }
    function normalizeSavedConfigSecurityOptions(this: any, options?: any, _b?: any) {
        return normalizeAlarmOptions(options || "");
    }
    function normalizeSavedConfigWeatherFields(this: any, b?: any, wasLegacyForecast?: any) {
        if (!b)
            return;
        if (wasLegacyForecast && b.label === "Weather")
            b.label = "";
        b.precision = normalizeWeatherCardMode(b.precision);
    }
    function normalizeSavedConfigWeatherOptions(this: any, options?: any, b?: any) {
        return b && cardLargeNumbersSupported(b) ? copyLargeNumbersOption("", options || "") : "";
    }
    function normalizeSavedConfigImageFields(this: any, b?: any) {
        if (!b)
            return;
        b.icon = imageIconEnabled(b) ? (b.icon && b.icon !== "Auto" ? b.icon : "Camera") : "Auto";
        if (!imageLabelEnabled(b))
            b.label = "";
    }
    function normalizeSavedConfigImageOptions(this: any, options?: any, _b?: any) {
        return normalizeImageOptions(options || "");
    }
    function normalizeSavedConfigClimateFields(this: any, b?: any) {
        if (!b)
            return;
        if (!b.icon)
            b.icon = "Thermostat";
        if (!b.icon_on)
            b.icon_on = "Auto";
        b.precision = normalizeClimatePrecisionConfig(b.precision);
    }
    function normalizeSavedConfigClimateOptions(this: any, options?: any, _b?: any) {
        return normalizeClimateOptions(options || "", true);
    }
    function normalizeSavedConfigLightControlOptions(this: any, options?: any, _b?: any) {
        return normalizeLightControlOptions(options || "");
    }
    function normalizeSavedConfigWebhookFields(this: any, b?: any) {
        if (!b)
            return;
        b.sensor = webhookMethod(b.sensor);
        if (b.sensor === "GET" || b.sensor === "DELETE")
            b.unit = "";
        if (!b.icon)
            b.icon = "Auto";
    }
    function normalizeSavedConfigWebhookOptions(this: any, options?: any, _b?: any) {
        var headers: any = configOptionValue(options || "", "webhook_headers");
        return headers ? setConfigOptionValue("", "webhook_headers", headers) : "";
    }
    function normalizeSavedConfigSubpageFields(this: any, b?: any) {
        applySubpagePresetConfig(b);
    }
    function normalizeSavedConfigSubpageOptions(this: any, options?: any, b?: any) {
        return normalizeSubpageOptions(options || "", b && b.sensor, b && b.precision);
    }
    function normalizeButtonConfig(this: any, b?: any) {
        if (b)
            b.options = b.options || "";
        if (b)
            migrateSavedConfigActionLegacy(b);
        var wasLegacyTextSensor: any = !!(b && b.type === "text_sensor");
        if (b)
            migrateSavedConfigSensorLegacy(b);
        if (b && migrateSavedConfigVacuumLegacy(b)) {
            if (!b.icon || b.icon === "Auto")
                b.icon = vacuumModeDefaultIcon(b.sensor);
        }
        var normalizedSavedFan: any = !!(b && normalizeSavedConfigFan(b, normalizeSavedConfigFanFields, normalizeFanControlOptions));
        var normalizedSavedMower: any = !!(b && normalizeSavedConfigMower(b, normalizeSavedConfigMowerFields));
        var wasLegacyWeatherForecast: any = !!(b && migrateSavedConfigWeatherLegacy(b));
        if (b)
            normalizeSavedConfigWeather(b, wasLegacyWeatherForecast, normalizeSavedConfigWeatherFields, normalizeSavedConfigWeatherOptions);
        if (b)
            normalizeSavedConfigMedia(b, normalizeSavedConfigMediaFields, normalizeMediaOptions);
        if (b)
            normalizeSavedConfigClimate(b, normalizeSavedConfigClimateFields, normalizeSavedConfigClimateOptions);
        var normalizedSavedAccess: any = !!(b && normalizeSavedConfigAccess(b, normalizeSavedConfigAccessFields, normalizeSavedConfigAccessOptions));
        if (b)
            normalizeSavedConfigSecurity(b, normalizeSavedConfigSecurityFields, normalizeSavedConfigSecurityOptions);
        if (b)
            normalizeSavedConfigWebhook(b, normalizeSavedConfigWebhookFields, normalizeSavedConfigWebhookOptions);
        normalizeWithRegisteredCardType(b);
        var normalizedSavedStatic: any = !!(b && normalizeSavedConfigStatic(b));
        if (b)
            normalizeSavedConfigDateTime(b, normalizeSavedConfigDateTimeFields, normalizeSavedConfigDateTimeOptions);
        if (b && b.type === "todo") {
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.icon_on = "Auto";
            if (!b.icon || b.icon === "Auto")
                b.icon = "Check";
            b.options = normalizeTodoOptions(b.options);
        }
        if (b)
            normalizeSavedConfigImage(b, normalizeSavedConfigImageFields, normalizeSavedConfigImageOptions);
        if (b)
            normalizeSavedConfigLightControl(b, normalizeSavedConfigLightControlOptions);
        if (b)
            normalizeSavedConfigSubpage(b, normalizeSavedConfigSubpageFields, normalizeSavedConfigSubpageOptions);
        if (b)
            normalizeSavedConfigAction(b, normalizeSavedConfigActionFields, normalizeActionOptions);
        var normalizedSavedSensor: any = !!(b && normalizeSavedConfigSensor(b, wasLegacyTextSensor, normalizeSavedConfigSensorFields, normalizeSensorOptions));
        var normalizedSavedOccupancy: any = !!(b && normalizeSavedConfigOccupancy(b, normalizeSavedConfigOccupancyFields, normalizeSavedConfigOccupancyOptions));
        var normalizedSavedSwitch: any = !!(b && !normalizedSavedSensor && normalizeSavedConfigSwitch(b, normalizeSwitchConfirmationOptions));
        if (b && !normalizedSavedSensor && !normalizedSavedSwitch && !normalizedSavedAccess && !normalizedSavedOccupancy && !normalizedSavedStatic && !normalizedSavedFan && !normalizedSavedMower && b.type !== "action" && b.type !== "alarm" && b.type !== "alarm_action" && !isClimateCardType(b.type) && b.type !== "webhook" && b.type !== "todo" && b.type !== "media" && b.type !== "subpage" && b.type !== "image" && b.type !== "light_control" && b.type !== "vacuum" && !cardLargeNumbersSupported(b)) {
            b.options = "";
        }
        return b;
    }
    function isBrightnessSliderType(this: any, type?: any) {
        return cardContractIsBrightnessSliderType(type);
    }
    function isFanCardType(this: any, type?: any) {
        return cardContractIsFanCardType(type);
    }
    function isClimateCardType(this: any, type?: any) {
        return type === "climate" || type === "climate_control";
    }
    function isOptionSelectType(this: any, type?: any) {
        return cardContractIsOptionSelectType(type);
    }
    function fanCardDefaultIcon(this: any, type?: any) {
        return cardContractFanDefaultIcon(type);
    }
    function buttonConfigChangedByNormalize(this: any, raw?: any) {
        var before: any = EspControlModel.cloneCardConfig(raw || {});
        var after: any = normalizeButtonConfig(EspControlModel.cloneCardConfig(before));
        return EspControlModel.cardConfigChanged(before, after);
    }
    function trimConfigFields(this: any, fields?: any) {
        return EspControlModel.trimConfigFields(fields);
    }
    function buttonConfigFields(this: any, b?: any) {
        var type: any = b && b.type || "";
        if (b && type === "subpage" && subpageKind(b)) {
            b = EspControlModel.cloneCardConfig(b);
            applySubpagePresetConfig(b);
        }
        var isActionOptionSelect: any = !!(b && (actionCardIsOptionSelect(b) || isOptionSelectType(type)));
        if (isActionOptionSelect)
            type = "action";
        if (type === "local")
            type = "action";
        if (type === "local_sensor")
            type = "sensor";
        var label: any = b && b.label || "";
        if (type === "calendar" || type === "clock" || type === "timezone")
            label = "";
        if (type === "screen_lock")
            label = "";
        var sensor: any = isActionOptionSelect ? ACTION_CARD_OPTION_SELECT_ACTION :
            (isBrightnessSliderType(type) || type === "calendar" || type === "clock" || isClimateCardType(type) || type === "light_switch" || type === "light_control" || type === "alarm" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.sensor || "");
        if (type === "lock" && sensor !== "lock" && sensor !== "unlock")
            sensor = "";
        if (b && b.type === "local")
            sensor = ACTION_CARD_LOCAL_ACTION;
        if (b && (b.type === "local_sensor" || sensorCardIsLocal(b)))
            sensor = SENSOR_CARD_LOCAL_SENSOR;
        var isLocalAction: any = type === "action" && sensor === ACTION_CARD_LOCAL_ACTION;
        var unit: any = (isActionOptionSelect || type === "calendar" || type === "clock" || isClimateCardType(type) || type === "light_switch" || type === "light_control" || type === "alarm" || type === "alarm_action" || type === "lock" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.unit || "");
        if (isLocalAction)
            unit = "";
        var icon: any = b && b.icon || "Auto";
        if (isActionOptionSelect && (!icon || icon === "Auto" || icon === "Chevron Down"))
            icon = "Flash";
        if (isLocalAction && (!icon || icon === "Auto" || icon === "Flash"))
            icon = "Gesture Tap";
        if (type === "alarm" && (!icon || icon === "Auto"))
            icon = "Security";
        if (type === "calendar" || type === "clock" || type === "timezone")
            icon = "Auto";
        if (type === "screen_lock")
            icon = "Lock";
        if (type === "alarm_action" && (!icon || icon === "Auto"))
            icon = (alarmActionInfo(sensor) || alarmActionSpecs()[0]).icon;
        if (isFanCardType(type) && (!icon || icon === "Auto"))
            icon = fanCardDefaultIcon(type);
        var iconOn: any = (isActionOptionSelect || type === "alarm" || type === "alarm_action" || (isFanCardType(type) && type !== "fan_switch")) ? "Auto" : (b && b.icon_on || "Auto");
        if (type === "calendar" || type === "clock" || type === "timezone")
            iconOn = "Auto";
        if (isLocalAction)
            iconOn = "Auto";
        if (type === "fan_switch" && (!iconOn || iconOn === "Auto"))
            iconOn = "Fan";
        if (type === "lock")
            iconOn = sensor ? "Auto" : ((!iconOn || iconOn === "Auto") ? "Lock Open" : iconOn);
        if (type === "screen_lock")
            iconOn = "Lock Open";
        var precision: any = (isActionOptionSelect || type === "clock" || type === "light_switch" || type === "light_control" || type === "alarm" || type === "alarm_action" || type === "lock" || type === "screen_lock" || type === "timezone" || isFanCardType(type)) ? "" : (b && b.precision || "");
        if (isLocalAction)
            precision = "";
        if (sensor === SENSOR_CARD_LOCAL_SENSOR && precision !== "text" && precision !== "1" && precision !== "2")
            precision = "";
        if (type === "media") {
            sensor = mediaEditorMode(sensor);
            if (sensor === "now_playing" && configOptionEnabled(b && b.options, MEDIA_COVER_ART_OPTION))
                sensor = "cover_art";
            precision = sensor === "now_playing"
                ? mediaNowPlayingControls({ sensor: sensor, precision: precision })
                : (mediaStateDisplayModeSupported(sensor) && precision === "state" ? "state" : "");
        }
        if (type === "vacuum") {
            sensor = normalizeSavedConfigVacuumSensor(sensor);
            unit = vacuumModeNeedsArea(sensor) ? unit : "";
            precision = normalizeSavedConfigVacuumPrecision(precision);
            iconOn = normalizeSavedConfigVacuumIconOn(iconOn);
            if (!icon || icon === "Auto")
                icon = vacuumModeDefaultIcon(sensor);
        }
        if (type === "lawn_mower") {
            sensor = normalizeLawnMowerMode(sensor);
            unit = "";
            precision = "";
            iconOn = "Auto";
            if (!icon || icon === "Auto")
                icon = lawnMowerModeDefaultIcon(sensor);
        }
        if (isClimateCardType(type))
            precision = normalizeClimatePrecisionConfig(precision);
        if (type === "calendar" && precision !== "datetime")
            precision = "";
        if (type === "weather") {
            sensor = "";
            precision = normalizeWeatherCardMode(precision);
        }
        if (type === "todo") {
            sensor = "";
            unit = "";
            precision = "";
            iconOn = "Auto";
            if (!icon || icon === "Auto")
                icon = "Check";
        }
        if (type === "image") {
            iconOn = "Auto";
            sensor = "";
            unit = "";
            precision = "";
            if (!imageLabelEnabled(b))
                label = "";
        }
        if (type === "door_window")
            precision = normalizeDoorWindowSubtype(precision);
        var options: any = b && b.options || "";
        if (type === "") {
            options = normalizeSwitchConfirmationOptions(options);
        }
        else if (type === "alarm" || type === "alarm_action") {
            options = normalizeAlarmOptions(options);
        }
        else if (type === "garage") {
            options = normalizeGarageOptions(options, sensor);
        }
        else if (type === "gate") {
            options = normalizeGateOptions(options, sensor);
        }
        else if (type === "cover") {
            sensor = normalizeCoverMode(sensor, true);
            options = normalizeCoverOptionsForMode(options, sensor);
        }
        else if (isClimateCardType(type)) {
            type = "climate_control";
            options = normalizeClimateOptions(options, true);
        }
        else if (type === "media") {
            options = normalizeMediaOptions(options, sensor);
        }
        else if (type === "weather") {
            options = cardLargeNumbersSupported({ type: type, precision: precision }) ? copyLargeNumbersOption("", options) : "";
        }
        else if (type === "subpage") {
            options = normalizeSubpageOptions(options, sensor, precision);
        }
        else if (type === "webhook" && typeof normalizeWebhookConfig === "function") {
            var webhookButton: any = EspControlModel.cloneCardConfig(b || {});
            normalizeWebhookConfig(webhookButton);
            sensor = webhookButton.sensor;
            unit = webhookButton.unit;
            iconOn = webhookButton.icon_on || "Auto";
            precision = webhookButton.precision || "";
            options = webhookButton.options || "";
        }
        else if (type === "lock" || type === "screen_lock") {
            options = "";
        }
        else if (type === "calendar" || type === "clock" || type === "timezone") {
            options = normalizeDateTimeOptions(type, options, precision);
        }
        else if (type === "vacuum") {
            options = normalizeSavedConfigVacuumOptions(options);
        }
        else if (type === "lawn_mower") {
            options = "";
        }
        else if (type === "todo") {
            options = normalizeTodoOptions(options);
        }
        else if (type === "sensor") {
            options = sensor === SENSOR_CARD_LOCAL_SENSOR ? "" : normalizeSensorOptions(options, precision);
        }
        else if (type === "door_window") {
            options = normalizeDoorWindowOptions(options);
        }
        else if (type === "presence") {
            options = normalizePresenceOptions(options);
        }
        else if (type === "image") {
            options = normalizeImageOptions(options);
        }
        else if (type === "light_control") {
            options = normalizeLightControlOptions(options);
        }
        else if (type === "fan_control") {
            options = normalizeFanControlOptions(options);
        }
        else if (type === "action") {
            options = sensor === ACTION_CARD_LOCAL_ACTION ? "" : normalizeActionOptions(options, sensor);
        }
        else if (isActionOptionSelect || isFanCardType(type)) {
            options = "";
        }
        else if (type !== "action" && type !== "alarm_action" && !isClimateCardType(type) && type !== "cover" && type !== "garage" && type !== "gate" && type !== "webhook" && type !== "screen_lock" && type !== "media" && type !== "presence" && type !== "light_control" && type !== "fan_control" && !cardLargeNumbersSupported({ type: type, precision: precision })) {
            options = "";
        }
        if (type === "image") {
            icon = configOptionEnabled(options, IMAGE_ICON_OPTION)
                ? (icon && icon !== "Auto" ? icon : "Camera")
                : "Auto";
        }
        if (type === "door_window") {
            b = b || {};
            b.entity = "";
            unit = "";
            if (!icon || icon === "Auto")
                icon = doorWindowClosedIcon(precision);
            if (!iconOn || iconOn === "Auto")
                iconOn = doorWindowOpenIcon(precision);
        }
        if (type === "presence") {
            b = b || {};
            b.entity = "";
            unit = "";
            precision = "";
            if (!icon || icon === "Auto")
                icon = "Motion Sensor Off";
            if (!iconOn || iconOn === "Auto")
                iconOn = "Motion Sensor";
        }
        if (type === "calendar") {
            b = b || {};
            if (!b.entity)
                b.entity = cardContractDefaultConfig("calendar").entity;
        }
        if (type === "clock") {
            b = b || {};
            b.entity = "";
        }
        if (type === "timezone") {
            b = b || {};
            if (!b.entity)
                b.entity = cardContractDefaultConfig("timezone").entity;
        }
        if (!type && !sensor) {
            unit = "";
            precision = "";
        }
        return trimConfigFields([
            (type === "door_window" || type === "presence" || type === "screen_lock") ? "" : (b && b.entity || ""),
            label,
            icon,
            iconOn,
            sensor,
            unit,
            type,
            precision,
            options,
        ]);
    }
    function encodeConfigField(this: any, value?: any) {
        return EspControlModel.encodeConfigField(value);
    }
    function decodeConfigField(this: any, value?: any) {
        return EspControlModel.decodeConfigField(value);
    }
    function legacyButtonConfigSafe(this: any, fields?: any) {
        return EspControlModel.legacyButtonConfigSafe(fields);
    }
    function serializeButtonConfig(this: any, b?: any) {
        var fields: any = buttonConfigFields(b || {});
        if (legacyButtonConfigSafe(fields))
            return fields.join(";");
        return "~" + fields.map(encodeConfigField).join(",");
    }
    function parseRawButtonConfig(this: any, str?: any) {
        return EspControlModel.parseRawButtonConfig(str);
    }
    function parseButtonConfig(this: any, str?: any) {
        return normalizeButtonConfig(parseRawButtonConfig(str));
    }
    function hasLegacySliderDirection(this: any, b?: any) {
        return !!(b && isBrightnessSliderType(b.type) && b.sensor);
    }
    function buttonConfigHasLegacySliderDirection(this: any, str?: any) {
        return hasLegacySliderDirection(parseRawButtonConfig(str || ""));
    }
    function buttonConfigNeedsMigration(this: any, str?: any) {
        return buttonConfigChangedByNormalize(parseRawButtonConfig(str || ""));
    }
    function parseBackOrderToken(this: any, value?: any) {
        return EspControlModel.parseBackOrderToken(value);
    }
    function backOrderToken(this: any, baseToken?: any, label?: any) {
        return EspControlModel.backOrderToken(baseToken, label);
    }
    function backLabelFromOrder(this: any, order?: any) {
        return EspControlModel.backLabelFromOrder(order);
    }
    function parseSubpageOrder(this: any, orderStr?: any) {
        return EspControlModel.parseSubpageOrder(orderStr);
    }
    function subpageOrderForSerialize(this: any, sp?: any) {
        return EspControlModel.subpageOrderForSerialize((sp && sp.order) || [], sp && sp.backLabel);
    }
    function subpageSerializedOrder(this: any, sp?: any) {
        if (!sp)
            return [];
        if (sp.order && sp.order.length)
            return subpageOrderForSerialize(sp);
        if (sp.grid && sp.grid.length)
            return serializeSubpageGrid(sp);
        return [];
    }
    function parseSubpageConfig(this: any, str?: any, raw?: any) {
        var parsed: any = EspControlModel.parseRawSubpageConfig(str, subpageTypeFromCode);
        if (raw)
            return parsed;
        var compactButtonTokens: any = String(str || "").charAt(0) === "~"
            ? String(str || "").split("|").slice(1)
            : [];
        parsed.buttons = parsed.buttons.map(function (this: any, button?: any, index?: any) {
            var normalized: any = normalizeButtonConfig(button);
            if (button && button.type === "calendar" && (!button.entity || compactButtonTokens[index] === "D"))
                normalized.entity = "";
            return normalized;
        });
        return parsed;
    }
    function subpageTypeCode(this: any, type?: any) {
        return cardContractSubpageTypeCode(type);
    }
    function subpageTypeFromCode(this: any, code?: any) {
        return cardContractSubpageTypeFromCode(code);
    }
    function encodeSubpageField(this: any, value?: any) {
        return encodeConfigField(value);
    }
    function decodeSubpageField(this: any, value?: any) {
        return decodeConfigField(value);
    }
    function parseCompactSubpageConfig(this: any, str?: any, raw?: any) {
        var parsed: any = EspControlModel.parseCompactSubpageConfig(str, subpageTypeFromCode);
        if (raw)
            return parsed;
        var compactButtonTokens: any = String(str || "").split("|").slice(1);
        parsed.buttons = parsed.buttons.map(function (this: any, button?: any, index?: any) {
            var normalized: any = normalizeButtonConfig(button);
            if (button && button.type === "calendar" && compactButtonTokens[index] === "D")
                normalized.entity = "";
            return normalized;
        });
        return parsed;
    }
    function subpageConfigHasLegacySliderDirection(this: any, str?: any) {
        var sp: any = parseSubpageConfig(str, true);
        for (var i: any = 0; i < sp.buttons.length; i++) {
            if (hasLegacySliderDirection(sp.buttons[i]))
                return true;
        }
        return false;
    }
    function subpageConfigNeedsMigration(this: any, str?: any) {
        var sp: any = parseSubpageConfig(str, true);
        for (var i: any = 0; i < sp.buttons.length; i++) {
            if (buttonConfigChangedByNormalize(sp.buttons[i]))
                return true;
        }
        return false;
    }
    function serializeSubpageConfig(this: any, sp?: any) {
        var order: any = subpageSerializedOrder(sp);
        var legacy: any = legacySubpageConfigSafe(sp) ? serializeLegacySubpageConfig(sp) : "";
        var compact: any = serializeCompactSubpageConfig(sp);
        return EspControlModel.chooseSerializedSubpageConfig(order, sp && sp.buttons ? sp.buttons.length : 0, legacy, compact);
    }
    function subpageLegacyButtonFields(this: any, b?: any) {
        var fields: any = buttonConfigFields(b || {});
        if (fields.length > 1 && fields[fields.length - 1] === "Auto") {
            while (fields.length > 1 && (fields[fields.length - 1] === "Auto" || !fields[fields.length - 1]))
                fields.pop();
        }
        return fields;
    }
    function subpageCompactButtonFields(this: any, b?: any) {
        var fields: any = buttonConfigFields(b || {});
        var compact: any = [
            subpageTypeCode(fields[6] || ""),
            encodeSubpageField(fields[0]),
            encodeSubpageField(fields[1]),
            fields[2] && fields[2] !== "Auto" ? encodeSubpageField(fields[2]) : "",
            fields[3] && fields[3] !== "Auto" ? encodeSubpageField(fields[3]) : "",
            encodeSubpageField(fields[4]),
            encodeSubpageField(fields[5]),
            encodeSubpageField(fields[7]),
            encodeSubpageField(fields[8]),
        ];
        while (compact.length > 1 && !compact[compact.length - 1])
            compact.pop();
        return compact;
    }
    function legacySubpageConfigSafe(this: any, sp?: any) {
        var fields: any = ((sp && sp.buttons) || []).map(subpageLegacyButtonFields);
        return EspControlModel.legacySubpageFieldsSafe(fields);
    }
    function serializeLegacySubpageConfig(this: any, sp?: any) {
        if (!sp)
            return "";
        return EspControlModel.serializeLegacySubpageConfig(subpageSerializedOrder(sp), ((sp && sp.buttons) || []).map(subpageLegacyButtonFields));
    }
    function serializeCompactSubpageConfig(this: any, sp?: any) {
        if (!sp || !sp.buttons || sp.buttons.length === 0)
            return "";
        return EspControlModel.serializeCompactSubpageConfig(subpageSerializedOrder(sp), sp.buttons.map(subpageCompactButtonFields));
    }
    function applySubpageRaw(this: any, slot?: any) {
        var raw: any = state.subpageRaw[slot];
        var combined: any = (raw && raw.main || "") + (raw && raw.ext || "") +
            (raw && raw.ext2 || "") + (raw && raw.ext3 || "") +
            (raw && raw.ext4 || "") + (raw && raw.ext5 || "") +
            (raw && raw.ext6 || "") + (raw && raw.ext7 || "");
        var pending: any = state.subpageSavePending[slot];
        if (pending) {
            if (combined !== pending) {
                if (state.editingSubpage === slot)
                    scheduleRender();
                return;
            }
            delete state.subpageSavePending[slot];
        }
        var local: any = state.subpages[slot];
        var localHasData: any = local && ((local.buttons && local.buttons.length > 0) ||
            (local.order && local.order.length > 0));
        if (state.editingSubpage === slot && localHasData) {
            var localSerialized: any = serializeSubpageConfig(local);
            if (combined !== localSerialized) {
                scheduleRender();
                return;
            }
        }
        if (combined) {
            var migrateConfig: any = subpageConfigNeedsMigration(combined);
            var sp: any = parseSubpageConfig(combined);
            sp.sizes = sp.sizes || {};
            buildSubpageGrid(sp);
            state.subpages[slot] = sp;
            if (migrateConfig)
                scheduleSliderSubpageMigration(slot);
        }
        else {
            delete state.subpages[slot];
        }
        if (state.editingSubpage === slot) {
            scheduleRender();
        }
    }
    function getSubpage(this: any, homeSlot?: any) {
        if (!state.subpages[homeSlot]) {
            state.subpages[homeSlot] = { order: [], buttons: [], grid: [], sizes: {}, backLabel: "Back" };
        }
        else if (!state.subpages[homeSlot].backLabel) {
            state.subpages[homeSlot].backLabel = backLabelFromOrder(state.subpages[homeSlot].order);
        }
        return state.subpages[homeSlot];
    }
    function buildSubpageGrid(this: any, sp?: any) {
        var result: any = EspControlModel.buildSubpageGrid(sp, NUM_SLOTS, GRID_COLS);
        sp.grid = result.grid;
        sp.sizes = result.sizes;
        return sp.grid;
    }
    function serializeSubpageGrid(this: any, sp?: any) {
        return EspControlModel.serializeSubpageGrid(sp.grid, sp.sizes || {}, sp.backLabel || "Back");
    }
    function enterSubpage(this: any, homeSlot?: any) {
        state.editingSubpage = homeSlot;
        state.subpageSelectedSlots = [];
        state.subpageLastClicked = -1;
        var sp: any = getSubpage(homeSlot);
        buildSubpageGrid(sp);
        renderPreview();
        renderButtonSettings();
    }
    function exitSubpage(this: any) {
        state.editingSubpage = null;
        state.subpageSelectedSlots = [];
        state.subpageLastClicked = -1;
        renderPreview();
        renderButtonSettings();
    }
    function saveSubpageConfig(this: any, homeSlot?: any) {
        var sp: any = getSubpage(homeSlot);
        sp.order = serializeSubpageGrid(sp);
        saveSubpageEntity(homeSlot);
    }
    function subpageFirstFreeSlot(this: any, sp?: any) {
        var used: any = {};
        sp.grid.forEach(function (this: any, s?: any) {
            if (s > 0)
                used[s] = true;
        });
        for (var i: any = 1; i <= sp.buttons.length + 1; i++) {
            if (!used[i])
                return i;
        }
        return sp.buttons.length + 1;
    }
    function bindTextPost(this: any, input?: any, postName?: any, opts?: any) {
        input.addEventListener("blur", function (this: any) {
            if (opts && opts.onBlur)
                opts.onBlur(this.value);
            if (opts && opts.post)
                opts.post(this.value);
            else
                postText(postName, this.value);
            if (opts && opts.rerender)
                renderPreview();
        });
        input.addEventListener("keydown", function (this: any, e?: any) {
            if (e.key === "Enter")
                this.blur();
        });
    }
    return {
        "normalizeWithRegisteredCardType": staticGlobal(normalizeWithRegisteredCardType),
        "normalizeButtonConfig": staticGlobal(normalizeButtonConfig),
        "cardRequiresSquareSize": staticGlobal(cardRequiresSquareSize),
        "normalizeCardSizeForConfig": staticGlobal(normalizeCardSizeForConfig),
        "isBrightnessSliderType": staticGlobal(isBrightnessSliderType),
        "isFanCardType": staticGlobal(isFanCardType),
        "isClimateCardType": staticGlobal(isClimateCardType),
        "isOptionSelectType": staticGlobal(isOptionSelectType),
        "fanCardDefaultIcon": staticGlobal(fanCardDefaultIcon),
        "buttonConfigChangedByNormalize": staticGlobal(buttonConfigChangedByNormalize),
        "trimConfigFields": staticGlobal(trimConfigFields),
        "buttonConfigFields": staticGlobal(buttonConfigFields),
        "encodeConfigField": staticGlobal(encodeConfigField),
        "decodeConfigField": staticGlobal(decodeConfigField),
        "legacyButtonConfigSafe": staticGlobal(legacyButtonConfigSafe),
        "serializeButtonConfig": staticGlobal(serializeButtonConfig),
        "parseRawButtonConfig": staticGlobal(parseRawButtonConfig),
        "parseButtonConfig": staticGlobal(parseButtonConfig),
        "hasLegacySliderDirection": staticGlobal(hasLegacySliderDirection),
        "buttonConfigHasLegacySliderDirection": staticGlobal(buttonConfigHasLegacySliderDirection),
        "buttonConfigNeedsMigration": staticGlobal(buttonConfigNeedsMigration),
        "parseBackOrderToken": staticGlobal(parseBackOrderToken),
        "backOrderToken": staticGlobal(backOrderToken),
        "backLabelFromOrder": staticGlobal(backLabelFromOrder),
        "parseSubpageOrder": staticGlobal(parseSubpageOrder),
        "subpageOrderForSerialize": staticGlobal(subpageOrderForSerialize),
        "subpageSerializedOrder": staticGlobal(subpageSerializedOrder),
        "parseSubpageConfig": staticGlobal(parseSubpageConfig),
        "subpageTypeCode": staticGlobal(subpageTypeCode),
        "subpageTypeFromCode": staticGlobal(subpageTypeFromCode),
        "encodeSubpageField": staticGlobal(encodeSubpageField),
        "decodeSubpageField": staticGlobal(decodeSubpageField),
        "parseCompactSubpageConfig": staticGlobal(parseCompactSubpageConfig),
        "subpageConfigHasLegacySliderDirection": staticGlobal(subpageConfigHasLegacySliderDirection),
        "subpageConfigNeedsMigration": staticGlobal(subpageConfigNeedsMigration),
        "serializeSubpageConfig": staticGlobal(serializeSubpageConfig),
        "subpageLegacyButtonFields": staticGlobal(subpageLegacyButtonFields),
        "subpageCompactButtonFields": staticGlobal(subpageCompactButtonFields),
        "legacySubpageConfigSafe": staticGlobal(legacySubpageConfigSafe),
        "serializeLegacySubpageConfig": staticGlobal(serializeLegacySubpageConfig),
        "serializeCompactSubpageConfig": staticGlobal(serializeCompactSubpageConfig),
        "applySubpageRaw": staticGlobal(applySubpageRaw),
        "getSubpage": staticGlobal(getSubpage),
        "buildSubpageGrid": staticGlobal(buildSubpageGrid),
        "serializeSubpageGrid": staticGlobal(serializeSubpageGrid),
        "enterSubpage": staticGlobal(enterSubpage),
        "exitSubpage": staticGlobal(exitSubpage),
        "saveSubpageConfig": staticGlobal(saveSubpageConfig),
        "subpageFirstFreeSlot": staticGlobal(subpageFirstFreeSlot),
        "bindTextPost": staticGlobal(bindTextPost),
    };
}
