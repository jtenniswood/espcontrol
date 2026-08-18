import {
    configOptionEnabled,
    configOptionValue,
    setConfigOption,
    setConfigOptionValue,
} from "../model/config_primitives";
import {
    cardContractOptionName,
    cardContractOptions,
} from "../generated/card_contract";
    // ── Config Option Core ─────────────────────────────────────────────
    var SENSOR_STATE_LABELS_OPTION: any = cardContractOptionName("state_labels");
    var SENSOR_STATE_INPUT_OPTION: any = cardContractOptionName("state_input");
    var SENSOR_STATE_OUTPUT_OPTION: any = cardContractOptionName("state_output");
    var SENSOR_STATE_INPUT_2_OPTION: any = cardContractOptionName("state_input_2");
    var SENSOR_STATE_OUTPUT_2_OPTION: any = cardContractOptionName("state_output_2");
    var SENSOR_STATE_LOW_LABEL_OPTION: any = cardContractOptionName("state_low_label");
    var SENSOR_STATE_HIGH_LABEL_OPTION: any = cardContractOptionName("state_high_label");
    var CARD_ON_PATTERN_OPTION: any = cardContractOptionName("on_pattern");
    var SENSOR_LARGE_NUMBERS_OPTION: any = cardContractOptionName("large_numbers");
    var SENSOR_LARGE_NUMBERS_OFF_VALUE: any = "off";
    var SENSOR_TIME_UNIT_OPTION: any = cardContractOptionName("time_unit");
    var SENSOR_ACTIVE_COLOR_OPTION: any = cardContractOptionName("active_color");
    var SWITCH_CONFIRM_OFF_OPTION: any = cardContractOptionName("confirm_off");
    var SWITCH_CONFIRM_ON_OPTION: any = cardContractOptionName("confirm_on");
    var SWITCH_CONFIRM_MESSAGE_OPTION: any = cardContractOptionName("confirm_message");
    var SWITCH_CONFIRM_YES_OPTION: any = cardContractOptionName("confirm_yes");
    var SWITCH_CONFIRM_NO_OPTION: any = cardContractOptionName("confirm_no");
    var SWITCH_CONFIRM_DEFAULT_MESSAGE: any = "Turn off this device?";
    var SWITCH_CONFIRM_ON_DEFAULT_MESSAGE: any = "Turn on this device?";
    var SWITCH_CONFIRM_BOTH_DEFAULT_MESSAGE: any = "Toggle this device?";
    var SWITCH_CONFIRM_DEFAULT_YES: any = "Yes";
    var SWITCH_CONFIRM_DEFAULT_NO: any = "No";
    var ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE: any = "Run this script?";
    var ACTION_SCRIPT_FIELDS_OPTION: any = "script_fields";
    var ALARM_PIN_ARM_OPTION: any = cardContractOptionName("pin_arm");
    var ALARM_PIN_DISARM_OPTION: any = cardContractOptionName("pin_disarm");
    var ALARM_ACTIONS_OPTION: any = cardContractOptionName("actions");
    var ALARM_ICON_DISPLAY_OPTION: any = cardContractOptionName("icon_display");
    var ALARM_LABEL_DISPLAY_OPTION: any = cardContractOptionName("label_display");
    var GARAGE_LABEL_DISPLAY_OPTION: any = cardContractOptionName("label_display");
    var GATE_LABEL_DISPLAY_OPTION: any = cardContractOptionName("label_display");
    var CLIMATE_LABEL_DISPLAY_OPTION: any = cardContractOptionName("label_display");
    var CLIMATE_NUMBER_DISPLAY_OPTION: any = cardContractOptionName("number_display");
    var CLIMATE_TEMPERATURE_STEP_OPTION: any = cardContractOptionName("temperature_step");
    var MEDIA_VOLUME_MAX_OPTION: any = cardContractOptionName("volume_max");
    var MEDIA_SPEAKER_GROUP_ENTITY_OPTION: any = cardContractOptionName("speaker_group_entity");
    var MEDIA_LABEL_DISPLAY_OPTION: any = cardContractOptionName("label_display");
    var MEDIA_NUMBER_DISPLAY_OPTION: any = cardContractOptionName("number_display");
    var MEDIA_PLAYLIST_CONTENT_ID_OPTION: any = cardContractOptionName("playlist_content_id");
    var MEDIA_PLAYLIST_CONTENT_TYPE_OPTION: any = cardContractOptionName("playlist_content_type");
    var MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION: any = cardContractOptionName("playlist_player_source");
    var MEDIA_COVER_ART_OPTION: any = cardContractOptionName("media_cover_art");
    var MEDIA_COVER_ART_DETAILS_OPTION: any = cardContractOptionName("cover_art_details");
    var MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION: any = cardContractOptionName("cover_art_secondary_entity");
    var SUBPAGE_KIND_OPTION: any = cardContractOptionName("subpage_kind");
    var IMAGE_LABEL_OPTION: any = cardContractOptionName("image_label");
    var IMAGE_ICON_OPTION: any = cardContractOptionName("image_icon");
    var IMAGE_MODAL_MODE_OPTION: any = cardContractOptionName("image_modal_mode");
    var LIGHT_CONTROL_TABS_OPTION: any = cardContractOptionName("light_tabs");
    var COVER_CONTROL_TABS_OPTION: any = cardContractOptionName("cover_tabs");
    var CLIMATE_CONTROL_TABS_OPTION: any = cardContractOptionName("climate_tabs");
    var FAN_LIGHT_ENTITY_OPTION: any = cardContractOptionName("fan_light_entity");
    var FAN_CONTROL_TABS_OPTION: any = cardContractOptionName("fan_tabs");
    function largeNumbersExplicitlyDisabled(this: any, options?: any) {
        return configOptionValue(options, SENSOR_LARGE_NUMBERS_OPTION) === SENSOR_LARGE_NUMBERS_OFF_VALUE;
    }
    function copyLargeNumbersOption(this: any, out?: any, options?: any) {
        if (largeNumbersExplicitlyDisabled(options)) {
            return setConfigOptionValue(out, SENSOR_LARGE_NUMBERS_OPTION, SENSOR_LARGE_NUMBERS_OFF_VALUE);
        }
        if (configOptionEnabled(options, SENSOR_LARGE_NUMBERS_OPTION)) {
            return setConfigOption(out, SENSOR_LARGE_NUMBERS_OPTION, true);
        }
        return out;
    }
    function cardContractOptionSpec(this: any, type?: any, name?: any) {
        var options: any = cardContractOptions(type);
        for (var i: any = 0; i < options.length; i++) {
            if (options[i].name === name)
                return options[i];
        }
        return null;
    }
    function cardContractOptionSupportedFor(this: any, type?: any, name?: any, context?: any) {
        var spec: any = cardContractOptionSpec(type, name);
        if (!spec)
            return false;
        var rule: any = spec.supportedWhen || {};
        if (rule.never)
            return false;
        context = context || {};
        var precision: any = context.precision || "";
        if (rule.precision && rule.precision.indexOf(precision) < 0)
            return false;
        if (rule.precisionNot && rule.precisionNot.indexOf(precision) >= 0)
            return false;
        return true;
    }
    function cardContractOptionDefaultValue(this: any, type?: any, name?: any, fallback?: any) {
        var spec: any = cardContractOptionSpec(type, name);
        return spec && typeof spec.defaultValue === "string" ? spec.defaultValue : fallback;
    }
export {
    SENSOR_STATE_LABELS_OPTION,
    SENSOR_STATE_INPUT_OPTION,
    SENSOR_STATE_OUTPUT_OPTION,
    SENSOR_STATE_INPUT_2_OPTION,
    SENSOR_STATE_OUTPUT_2_OPTION,
    SENSOR_STATE_LOW_LABEL_OPTION,
    SENSOR_STATE_HIGH_LABEL_OPTION,
    CARD_ON_PATTERN_OPTION,
    SENSOR_LARGE_NUMBERS_OPTION,
    SENSOR_LARGE_NUMBERS_OFF_VALUE,
    SENSOR_TIME_UNIT_OPTION,
    SENSOR_ACTIVE_COLOR_OPTION,
    SWITCH_CONFIRM_OFF_OPTION,
    SWITCH_CONFIRM_ON_OPTION,
    SWITCH_CONFIRM_MESSAGE_OPTION,
    SWITCH_CONFIRM_YES_OPTION,
    SWITCH_CONFIRM_NO_OPTION,
    SWITCH_CONFIRM_DEFAULT_MESSAGE,
    SWITCH_CONFIRM_ON_DEFAULT_MESSAGE,
    SWITCH_CONFIRM_BOTH_DEFAULT_MESSAGE,
    SWITCH_CONFIRM_DEFAULT_YES,
    SWITCH_CONFIRM_DEFAULT_NO,
    ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE,
    ACTION_SCRIPT_FIELDS_OPTION,
    ALARM_PIN_ARM_OPTION,
    ALARM_PIN_DISARM_OPTION,
    ALARM_ACTIONS_OPTION,
    ALARM_ICON_DISPLAY_OPTION,
    ALARM_LABEL_DISPLAY_OPTION,
    GARAGE_LABEL_DISPLAY_OPTION,
    GATE_LABEL_DISPLAY_OPTION,
    CLIMATE_LABEL_DISPLAY_OPTION,
    CLIMATE_NUMBER_DISPLAY_OPTION,
    CLIMATE_TEMPERATURE_STEP_OPTION,
    MEDIA_VOLUME_MAX_OPTION,
    MEDIA_SPEAKER_GROUP_ENTITY_OPTION,
    MEDIA_LABEL_DISPLAY_OPTION,
    MEDIA_NUMBER_DISPLAY_OPTION,
    MEDIA_PLAYLIST_CONTENT_ID_OPTION,
    MEDIA_PLAYLIST_CONTENT_TYPE_OPTION,
    MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION,
    MEDIA_COVER_ART_OPTION,
    MEDIA_COVER_ART_DETAILS_OPTION,
    MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION,
    SUBPAGE_KIND_OPTION,
    IMAGE_LABEL_OPTION,
    IMAGE_ICON_OPTION,
    IMAGE_MODAL_MODE_OPTION,
    LIGHT_CONTROL_TABS_OPTION,
    COVER_CONTROL_TABS_OPTION,
    CLIMATE_CONTROL_TABS_OPTION,
    FAN_LIGHT_ENTITY_OPTION,
    FAN_CONTROL_TABS_OPTION,
    largeNumbersExplicitlyDisabled,
    copyLargeNumbersOption,
    cardContractOptionSpec,
    cardContractOptionSupportedFor,
    cardContractOptionDefaultValue,
};
