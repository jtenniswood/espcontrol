import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
import { cardContractSupportsBackground } from "../generated/card_contract";
export function installConfigOptionCoreModule(): GlobalDescriptors {
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
    var MEDIA_LABEL_DISPLAY_OPTION: any = cardContractOptionName("label_display");
    var MEDIA_NUMBER_DISPLAY_OPTION: any = cardContractOptionName("number_display");
    var MEDIA_PLAYLIST_CONTENT_ID_OPTION: any = cardContractOptionName("playlist_content_id");
    var MEDIA_PLAYLIST_CONTENT_TYPE_OPTION: any = cardContractOptionName("playlist_content_type");
    var MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION: any = cardContractOptionName("playlist_player_source");
    var MEDIA_COVER_ART_OPTION: any = cardContractOptionName("media_cover_art");
    var MEDIA_COVER_ART_ACTION_OPTION: any = cardContractOptionName("cover_art_action");
    var MEDIA_COVER_ART_DETAILS_OPTION: any = cardContractOptionName("cover_art_details");
    var MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION: any = cardContractOptionName("cover_art_secondary_entity");
    var SUBPAGE_KIND_OPTION: any = cardContractOptionName("subpage_kind");
    var IMAGE_LABEL_OPTION: any = cardContractOptionName("image_label");
    var IMAGE_ICON_OPTION: any = cardContractOptionName("image_icon");
    var IMAGE_MODAL_MODE_OPTION: any = cardContractOptionName("image_modal_mode");
    var CARD_BACKGROUND_IMAGE_OPTION: any = cardContractOptionName("bg_image");
    var LIGHT_CONTROL_TABS_OPTION: any = cardContractOptionName("light_tabs");
    var COVER_CONTROL_TABS_OPTION: any = cardContractOptionName("cover_tabs");
    var CLIMATE_CONTROL_TABS_OPTION: any = cardContractOptionName("climate_tabs");
    var FAN_CONTROL_TABS_OPTION: any = cardContractOptionName("fan_tabs");
    var IMAGE_SLOT_CAPACITY: any = Math.max(0, parseInt(CFG.imageSlotCapacity, 10) || 0);
    var CARD_BACKGROUND_IMAGE_LIMIT: any = Math.max(0, parseInt(CFG.cardBackgroundImageLimit, 10) || parseInt(CFG.slots, 10) || 0);
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
    function cardBackgroundSupported(this: any, b?: any) {
        if (!b)
            return false;
        return cardContractSupportsBackground(b.type || "");
    }
    function normalizeCardBackgroundImageId(this: any, value?: any) {
        return EspControlModel.normalizeCardBackgroundAssetId(value);
    }
    function cardBackgroundImage(this: any, cardOrOptions?: any) {
        if (cardOrOptions && typeof cardOrOptions === "object")
            return EspControlModel.cardBackgroundAssetId(cardOrOptions);
        return normalizeCardBackgroundImageId(configOptionValue(cardOrOptions, CARD_BACKGROUND_IMAGE_OPTION));
    }
    function setCardBackgroundImage(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        return EspControlModel.setCardBackgroundAssetId(b, value);
    }
    function copyCardBackgroundOptions(this: any, out?: any, sourceOptions?: any, b?: any) {
        if (!cardBackgroundSupported(b))
            return out || "";
        var id: any = cardBackgroundImage(b) || cardBackgroundImage(sourceOptions);
        if (!id)
            return out || "";
        return setConfigOptionValue(out || "", CARD_BACKGROUND_IMAGE_OPTION, id);
    }
    function cardImageUrl(this: any, id?: any) {
        id = normalizeCardBackgroundImageId(id);
        return id ? "/card-images/" + id + ".jpg" : "";
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
    return {
        "SENSOR_STATE_LABELS_OPTION": liveGlobal(() => SENSOR_STATE_LABELS_OPTION, (value?: any) => { SENSOR_STATE_LABELS_OPTION = value; }),
        "SENSOR_STATE_INPUT_OPTION": liveGlobal(() => SENSOR_STATE_INPUT_OPTION, (value?: any) => { SENSOR_STATE_INPUT_OPTION = value; }),
        "SENSOR_STATE_OUTPUT_OPTION": liveGlobal(() => SENSOR_STATE_OUTPUT_OPTION, (value?: any) => { SENSOR_STATE_OUTPUT_OPTION = value; }),
        "SENSOR_STATE_INPUT_2_OPTION": liveGlobal(() => SENSOR_STATE_INPUT_2_OPTION, (value?: any) => { SENSOR_STATE_INPUT_2_OPTION = value; }),
        "SENSOR_STATE_OUTPUT_2_OPTION": liveGlobal(() => SENSOR_STATE_OUTPUT_2_OPTION, (value?: any) => { SENSOR_STATE_OUTPUT_2_OPTION = value; }),
        "SENSOR_STATE_LOW_LABEL_OPTION": liveGlobal(() => SENSOR_STATE_LOW_LABEL_OPTION, (value?: any) => { SENSOR_STATE_LOW_LABEL_OPTION = value; }),
        "SENSOR_STATE_HIGH_LABEL_OPTION": liveGlobal(() => SENSOR_STATE_HIGH_LABEL_OPTION, (value?: any) => { SENSOR_STATE_HIGH_LABEL_OPTION = value; }),
        "CARD_ON_PATTERN_OPTION": liveGlobal(() => CARD_ON_PATTERN_OPTION, (value?: any) => { CARD_ON_PATTERN_OPTION = value; }),
        "SENSOR_LARGE_NUMBERS_OPTION": liveGlobal(() => SENSOR_LARGE_NUMBERS_OPTION, (value?: any) => { SENSOR_LARGE_NUMBERS_OPTION = value; }),
        "SENSOR_LARGE_NUMBERS_OFF_VALUE": liveGlobal(() => SENSOR_LARGE_NUMBERS_OFF_VALUE, (value?: any) => { SENSOR_LARGE_NUMBERS_OFF_VALUE = value; }),
        "SENSOR_TIME_UNIT_OPTION": liveGlobal(() => SENSOR_TIME_UNIT_OPTION, (value?: any) => { SENSOR_TIME_UNIT_OPTION = value; }),
        "SENSOR_ACTIVE_COLOR_OPTION": liveGlobal(() => SENSOR_ACTIVE_COLOR_OPTION, (value?: any) => { SENSOR_ACTIVE_COLOR_OPTION = value; }),
        "SWITCH_CONFIRM_OFF_OPTION": liveGlobal(() => SWITCH_CONFIRM_OFF_OPTION, (value?: any) => { SWITCH_CONFIRM_OFF_OPTION = value; }),
        "SWITCH_CONFIRM_ON_OPTION": liveGlobal(() => SWITCH_CONFIRM_ON_OPTION, (value?: any) => { SWITCH_CONFIRM_ON_OPTION = value; }),
        "SWITCH_CONFIRM_MESSAGE_OPTION": liveGlobal(() => SWITCH_CONFIRM_MESSAGE_OPTION, (value?: any) => { SWITCH_CONFIRM_MESSAGE_OPTION = value; }),
        "SWITCH_CONFIRM_YES_OPTION": liveGlobal(() => SWITCH_CONFIRM_YES_OPTION, (value?: any) => { SWITCH_CONFIRM_YES_OPTION = value; }),
        "SWITCH_CONFIRM_NO_OPTION": liveGlobal(() => SWITCH_CONFIRM_NO_OPTION, (value?: any) => { SWITCH_CONFIRM_NO_OPTION = value; }),
        "SWITCH_CONFIRM_DEFAULT_MESSAGE": liveGlobal(() => SWITCH_CONFIRM_DEFAULT_MESSAGE, (value?: any) => { SWITCH_CONFIRM_DEFAULT_MESSAGE = value; }),
        "SWITCH_CONFIRM_ON_DEFAULT_MESSAGE": liveGlobal(() => SWITCH_CONFIRM_ON_DEFAULT_MESSAGE, (value?: any) => { SWITCH_CONFIRM_ON_DEFAULT_MESSAGE = value; }),
        "SWITCH_CONFIRM_BOTH_DEFAULT_MESSAGE": liveGlobal(() => SWITCH_CONFIRM_BOTH_DEFAULT_MESSAGE, (value?: any) => { SWITCH_CONFIRM_BOTH_DEFAULT_MESSAGE = value; }),
        "SWITCH_CONFIRM_DEFAULT_YES": liveGlobal(() => SWITCH_CONFIRM_DEFAULT_YES, (value?: any) => { SWITCH_CONFIRM_DEFAULT_YES = value; }),
        "SWITCH_CONFIRM_DEFAULT_NO": liveGlobal(() => SWITCH_CONFIRM_DEFAULT_NO, (value?: any) => { SWITCH_CONFIRM_DEFAULT_NO = value; }),
        "ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE": liveGlobal(() => ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE, (value?: any) => { ACTION_SCRIPT_CONFIRM_DEFAULT_MESSAGE = value; }),
        "ACTION_SCRIPT_FIELDS_OPTION": liveGlobal(() => ACTION_SCRIPT_FIELDS_OPTION, (value?: any) => { ACTION_SCRIPT_FIELDS_OPTION = value; }),
        "ALARM_PIN_ARM_OPTION": liveGlobal(() => ALARM_PIN_ARM_OPTION, (value?: any) => { ALARM_PIN_ARM_OPTION = value; }),
        "ALARM_PIN_DISARM_OPTION": liveGlobal(() => ALARM_PIN_DISARM_OPTION, (value?: any) => { ALARM_PIN_DISARM_OPTION = value; }),
        "ALARM_ACTIONS_OPTION": liveGlobal(() => ALARM_ACTIONS_OPTION, (value?: any) => { ALARM_ACTIONS_OPTION = value; }),
        "ALARM_ICON_DISPLAY_OPTION": liveGlobal(() => ALARM_ICON_DISPLAY_OPTION, (value?: any) => { ALARM_ICON_DISPLAY_OPTION = value; }),
        "ALARM_LABEL_DISPLAY_OPTION": liveGlobal(() => ALARM_LABEL_DISPLAY_OPTION, (value?: any) => { ALARM_LABEL_DISPLAY_OPTION = value; }),
        "GARAGE_LABEL_DISPLAY_OPTION": liveGlobal(() => GARAGE_LABEL_DISPLAY_OPTION, (value?: any) => { GARAGE_LABEL_DISPLAY_OPTION = value; }),
        "GATE_LABEL_DISPLAY_OPTION": liveGlobal(() => GATE_LABEL_DISPLAY_OPTION, (value?: any) => { GATE_LABEL_DISPLAY_OPTION = value; }),
        "CLIMATE_LABEL_DISPLAY_OPTION": liveGlobal(() => CLIMATE_LABEL_DISPLAY_OPTION, (value?: any) => { CLIMATE_LABEL_DISPLAY_OPTION = value; }),
        "CLIMATE_NUMBER_DISPLAY_OPTION": liveGlobal(() => CLIMATE_NUMBER_DISPLAY_OPTION, (value?: any) => { CLIMATE_NUMBER_DISPLAY_OPTION = value; }),
        "CLIMATE_TEMPERATURE_STEP_OPTION": liveGlobal(() => CLIMATE_TEMPERATURE_STEP_OPTION, (value?: any) => { CLIMATE_TEMPERATURE_STEP_OPTION = value; }),
        "MEDIA_VOLUME_MAX_OPTION": liveGlobal(() => MEDIA_VOLUME_MAX_OPTION, (value?: any) => { MEDIA_VOLUME_MAX_OPTION = value; }),
        "MEDIA_LABEL_DISPLAY_OPTION": liveGlobal(() => MEDIA_LABEL_DISPLAY_OPTION, (value?: any) => { MEDIA_LABEL_DISPLAY_OPTION = value; }),
        "MEDIA_NUMBER_DISPLAY_OPTION": liveGlobal(() => MEDIA_NUMBER_DISPLAY_OPTION, (value?: any) => { MEDIA_NUMBER_DISPLAY_OPTION = value; }),
        "MEDIA_PLAYLIST_CONTENT_ID_OPTION": liveGlobal(() => MEDIA_PLAYLIST_CONTENT_ID_OPTION, (value?: any) => { MEDIA_PLAYLIST_CONTENT_ID_OPTION = value; }),
        "MEDIA_PLAYLIST_CONTENT_TYPE_OPTION": liveGlobal(() => MEDIA_PLAYLIST_CONTENT_TYPE_OPTION, (value?: any) => { MEDIA_PLAYLIST_CONTENT_TYPE_OPTION = value; }),
        "MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION": liveGlobal(() => MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION, (value?: any) => { MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION = value; }),
        "MEDIA_COVER_ART_OPTION": liveGlobal(() => MEDIA_COVER_ART_OPTION, (value?: any) => { MEDIA_COVER_ART_OPTION = value; }),
        "MEDIA_COVER_ART_ACTION_OPTION": liveGlobal(() => MEDIA_COVER_ART_ACTION_OPTION, (value?: any) => { MEDIA_COVER_ART_ACTION_OPTION = value; }),
        "MEDIA_COVER_ART_DETAILS_OPTION": liveGlobal(() => MEDIA_COVER_ART_DETAILS_OPTION, (value?: any) => { MEDIA_COVER_ART_DETAILS_OPTION = value; }),
        "MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION": liveGlobal(() => MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION, (value?: any) => { MEDIA_COVER_ART_SECONDARY_ENTITY_OPTION = value; }),
        "SUBPAGE_KIND_OPTION": liveGlobal(() => SUBPAGE_KIND_OPTION, (value?: any) => { SUBPAGE_KIND_OPTION = value; }),
        "IMAGE_LABEL_OPTION": liveGlobal(() => IMAGE_LABEL_OPTION, (value?: any) => { IMAGE_LABEL_OPTION = value; }),
        "IMAGE_ICON_OPTION": liveGlobal(() => IMAGE_ICON_OPTION, (value?: any) => { IMAGE_ICON_OPTION = value; }),
        "IMAGE_MODAL_MODE_OPTION": liveGlobal(() => IMAGE_MODAL_MODE_OPTION, (value?: any) => { IMAGE_MODAL_MODE_OPTION = value; }),
        "CARD_BACKGROUND_IMAGE_OPTION": liveGlobal(() => CARD_BACKGROUND_IMAGE_OPTION, (value?: any) => { CARD_BACKGROUND_IMAGE_OPTION = value; }),
        "LIGHT_CONTROL_TABS_OPTION": liveGlobal(() => LIGHT_CONTROL_TABS_OPTION, (value?: any) => { LIGHT_CONTROL_TABS_OPTION = value; }),
        "COVER_CONTROL_TABS_OPTION": liveGlobal(() => COVER_CONTROL_TABS_OPTION, (value?: any) => { COVER_CONTROL_TABS_OPTION = value; }),
        "CLIMATE_CONTROL_TABS_OPTION": liveGlobal(() => CLIMATE_CONTROL_TABS_OPTION, (value?: any) => { CLIMATE_CONTROL_TABS_OPTION = value; }),
        "FAN_CONTROL_TABS_OPTION": liveGlobal(() => FAN_CONTROL_TABS_OPTION, (value?: any) => { FAN_CONTROL_TABS_OPTION = value; }),
        "IMAGE_SLOT_CAPACITY": liveGlobal(() => IMAGE_SLOT_CAPACITY, (value?: any) => { IMAGE_SLOT_CAPACITY = value; }),
        "CARD_BACKGROUND_IMAGE_LIMIT": liveGlobal(() => CARD_BACKGROUND_IMAGE_LIMIT, (value?: any) => { CARD_BACKGROUND_IMAGE_LIMIT = value; }),
        "largeNumbersExplicitlyDisabled": staticGlobal(largeNumbersExplicitlyDisabled),
        "copyLargeNumbersOption": staticGlobal(copyLargeNumbersOption),
        "cardBackgroundSupported": staticGlobal(cardBackgroundSupported),
        "normalizeCardBackgroundImageId": staticGlobal(normalizeCardBackgroundImageId),
        "cardBackgroundImage": staticGlobal(cardBackgroundImage),
        "setCardBackgroundImage": staticGlobal(setCardBackgroundImage),
        "copyCardBackgroundOptions": staticGlobal(copyCardBackgroundOptions),
        "cardImageUrl": staticGlobal(cardImageUrl),
        "cardContractOptionSpec": staticGlobal(cardContractOptionSpec),
        "cardContractOptionSupportedFor": staticGlobal(cardContractOptionSupportedFor),
        "cardContractOptionDefaultValue": staticGlobal(cardContractOptionDefaultValue),
    };
}
