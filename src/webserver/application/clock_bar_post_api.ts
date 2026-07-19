import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installClockBarPostApiModule(): GlobalDescriptors {
    // ── Clock Bar Post API ────────────────────────────────────────────────
    function postClockBrightnessDay(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_daytime_clock_brightness"), entityObjectIds("screen_saver_daytime_clock_brightness"), value);
    }
    function postClockBrightnessNight(this: any, value?: any) {
        postNumberWithObjectIds(entityName("screen_saver_nighttime_clock_brightness"), entityObjectIds("screen_saver_nighttime_clock_brightness"), value);
    }
    function postClockScreensaver(this: any, on?: any) {
        return postSwitchWithObjectIds(entityName("screen_saver_clock"), entityObjectIds("screen_saver_clock"), on);
    }
    var CLOCK_BAR_UNAVAILABLE: any = "Clock bar setting is not available on this firmware. Update the device firmware, then reload this page.";
    function postClockBar(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_clock_bar"), entityObjectIds("screen_clock_bar"), on, CLOCK_BAR_UNAVAILABLE);
    }
    function postClockBarTemperatureEntities(this: any, value?: any) {
        var name: any = entityName("clock_bar_temperature_entities");
        var objectIds: any = entityObjectIds("clock_bar_temperature_entities");
        return postOptional(entityPostUrls("text", name, objectIds, "set?value=" + encodeURIComponent(value)));
    }
    var CLOCK_BAR_TIME_UNAVAILABLE: any = "Clock bar time setting is not available on this firmware. Update the device firmware, then reload this page.";
    function postClockBarTime(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_clock_bar_time"), entityObjectIds("screen_clock_bar_time"), on, CLOCK_BAR_TIME_UNAVAILABLE);
    }
    var NETWORK_STATUS_ICON_UNAVAILABLE: any = "Network status icon setting is not available on this firmware. Update the device firmware, then reload this page.";
    function postNetworkStatusIcon(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_network_status_icon"), entityObjectIds("screen_network_status_icon"), on, NETWORK_STATUS_ICON_UNAVAILABLE);
    }
    var VOICE_SERVICES_UNAVAILABLE: any = "Voice services setting is not available on this firmware. Update the device firmware, then reload this page.";
    function voiceServicesPostUrls(this: any, on?: any) {
        return entityPostUrls("switch", entityName("voice_services"), entityObjectIds("voice_services"), on ? "turn_on" : "turn_off");
    }
    function postVoiceServices(this: any, on?: any) {
        post(voiceServicesPostUrls(on), null, VOICE_SERVICES_UNAVAILABLE);
    }
    function postAlarmDelayAudio(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("alarm_delay_audio"), entityObjectIds("alarm_delay_audio"), on);
    }
    function postAlarmDelayTts(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("alarm_delay_tts"), entityObjectIds("alarm_delay_tts"), on);
    }
    function postAlarmDelayEntryAnnouncement(this: any, value?: any) {
        postTextWithObjectIds(entityName("alarm_delay_entry_announcement"), entityObjectIds("alarm_delay_entry_announcement"), value);
    }
    function postAlarmDelayExitAnnouncement(this: any, value?: any) {
        postTextWithObjectIds(entityName("alarm_delay_exit_announcement"), entityObjectIds("alarm_delay_exit_announcement"), value);
    }
    function postAlarmDelayBeepVolume(this: any, value?: any) {
        postNumberWithObjectIds(entityName("alarm_delay_beep_volume"), entityObjectIds("alarm_delay_beep_volume"), value);
    }
    function postAlarmDelayFinalCountdown(this: any, value?: any) {
        postNumberWithObjectIds(entityName("alarm_delay_final_countdown"), entityObjectIds("alarm_delay_final_countdown"), value);
    }
    var TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE: any = "Temperature degree symbol setting is not available on this firmware. Update the device firmware, then reload this page.";
    function postTemperatureDegreeSymbol(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_temperature_degree_symbol"), entityObjectIds("screen_temperature_degree_symbol"), on, TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE);
    }
    var SUBPAGE_CHEVRON_UNAVAILABLE: any = "Subpage chevron setting is not available on this firmware. Update the device firmware, then reload this page.";
    function postSubpageChevron(this: any, on?: any) {
        postSwitchWithObjectIds(entityName("screen_subpage_chevron"), entityObjectIds("screen_subpage_chevron"), on, SUBPAGE_CHEVRON_UNAVAILABLE);
    }
    return {
        "postClockBrightnessDay": staticGlobal(postClockBrightnessDay),
        "postClockBrightnessNight": staticGlobal(postClockBrightnessNight),
        "postClockScreensaver": staticGlobal(postClockScreensaver),
        "CLOCK_BAR_UNAVAILABLE": liveGlobal(() => CLOCK_BAR_UNAVAILABLE, (value?: any) => { CLOCK_BAR_UNAVAILABLE = value; }),
        "postClockBar": staticGlobal(postClockBar),
        "postClockBarTemperatureEntities": staticGlobal(postClockBarTemperatureEntities),
        "CLOCK_BAR_TIME_UNAVAILABLE": liveGlobal(() => CLOCK_BAR_TIME_UNAVAILABLE, (value?: any) => { CLOCK_BAR_TIME_UNAVAILABLE = value; }),
        "postClockBarTime": staticGlobal(postClockBarTime),
        "NETWORK_STATUS_ICON_UNAVAILABLE": liveGlobal(() => NETWORK_STATUS_ICON_UNAVAILABLE, (value?: any) => { NETWORK_STATUS_ICON_UNAVAILABLE = value; }),
        "postNetworkStatusIcon": staticGlobal(postNetworkStatusIcon),
        "VOICE_SERVICES_UNAVAILABLE": liveGlobal(() => VOICE_SERVICES_UNAVAILABLE, (value?: any) => { VOICE_SERVICES_UNAVAILABLE = value; }),
        "voiceServicesPostUrls": staticGlobal(voiceServicesPostUrls),
        "postVoiceServices": staticGlobal(postVoiceServices),
        "postAlarmDelayAudio": staticGlobal(postAlarmDelayAudio),
        "postAlarmDelayTts": staticGlobal(postAlarmDelayTts),
        "postAlarmDelayEntryAnnouncement": staticGlobal(postAlarmDelayEntryAnnouncement),
        "postAlarmDelayExitAnnouncement": staticGlobal(postAlarmDelayExitAnnouncement),
        "postAlarmDelayBeepVolume": staticGlobal(postAlarmDelayBeepVolume),
        "postAlarmDelayFinalCountdown": staticGlobal(postAlarmDelayFinalCountdown),
        "TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE": liveGlobal(() => TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE, (value?: any) => { TEMPERATURE_DEGREE_SYMBOL_UNAVAILABLE = value; }),
        "postTemperatureDegreeSymbol": staticGlobal(postTemperatureDegreeSymbol),
        "SUBPAGE_CHEVRON_UNAVAILABLE": liveGlobal(() => SUBPAGE_CHEVRON_UNAVAILABLE, (value?: any) => { SUBPAGE_CHEVRON_UNAVAILABLE = value; }),
        "postSubpageChevron": staticGlobal(postSubpageChevron),
    };
}
