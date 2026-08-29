import { state } from "../state/app_instance";
import { AUTO_TIMEZONE_OPTION, FALLBACK_TIMEZONE_OPTION } from "../state/app_state";
import { normalizeLanguage } from "../model/settings";
import type { VoiceServicesController } from "../features/voice_services_controller";
import type { ApplicationLayoutState } from "./application_context";

export interface EnvironmentStateFeature {
    voiceServicesSupported(): boolean;
    voiceServicesState(): { supported: boolean; enabled: boolean };
    applyVoiceServicesState(next: { enabled: boolean }): void;
    voiceServicesUiState(): ReturnType<VoiceServicesController["uiState"]>;
    setVoiceServicesEnabled(enabled: boolean): void;
    isHomeAssistantAutoTimezone(value?: any): boolean;
    effectiveTimezoneOptionForWeb(value?: any): any;
    timezoneOptionsWithFallback(options?: any, selected?: any, preserveSelectedAuto?: any): any[];
    monthNameForIndex(index?: any): string;
}

export function createEnvironmentStateFeature(
    voiceServicesController: VoiceServicesController,
    defaultTimezoneOptions: () => string[],
    layout: ApplicationLayoutState,
): EnvironmentStateFeature {
    function voiceServicesSupported() {
        return !!(layout.config.features && layout.config.features.voiceServices);
    }
    function voiceServicesState() {
        return {
            supported: voiceServicesSupported(),
            enabled: !!state.voiceServicesOn,
        };
    }
    function applyVoiceServicesState(next: { enabled: boolean }) {
        state.voiceServicesOn = next.enabled;
    }
    function voiceServicesUiState() {
        return voiceServicesController.uiState(voiceServicesState());
    }
    function setVoiceServicesEnabled(enabled: boolean) {
        applyVoiceServicesState(voiceServicesController.setEnabled(voiceServicesState(), enabled));
    }
    function isHomeAssistantAutoTimezone(value?: any) {
        return String(value || "") === AUTO_TIMEZONE_OPTION;
    }
    function effectiveTimezoneOptionForWeb(value?: any) {
        if (!isHomeAssistantAutoTimezone(value))
            return value;
        var active: any = String(state && state.activeTimezone || "").trim();
        return active && !isHomeAssistantAutoTimezone(active) ? active : FALLBACK_TIMEZONE_OPTION;
    }
    function timezoneOptionsWithFallback(options?: any, selected?: any, preserveSelectedAuto?: any) {
        var list: any = Array.isArray(options) && options.length ? options.slice() : defaultTimezoneOptions();
        var supportsAuto: any = list.indexOf(AUTO_TIMEZONE_OPTION) !== -1;
        if (selected && list.indexOf(selected) === -1 &&
            (!isHomeAssistantAutoTimezone(selected) || supportsAuto || preserveSelectedAuto)) {
            list.unshift(selected);
        }
        return list;
    }
    function monthNameForIndex(index?: any) {
        var monthIndex: any = parseInt(index, 10);
        if (!isFinite(monthIndex) || monthIndex < 0 || monthIndex > 11)
            return "Date";
        try {
            return new Intl.DateTimeFormat(normalizeLanguage(state.language), { month: "long" })
                .format(new Date(Date.UTC(2000, monthIndex, 1)));
        }
        catch (_) {
            return new Intl.DateTimeFormat("en", { month: "long" })
                .format(new Date(Date.UTC(2000, monthIndex, 1)));
        }
    }
    return {
        voiceServicesSupported,
        voiceServicesState,
        applyVoiceServicesState,
        voiceServicesUiState,
        setVoiceServicesEnabled,
        isHomeAssistantAutoTimezone,
        effectiveTimezoneOptionForWeb,
        timezoneOptionsWithFallback,
        monthNameForIndex,
    };
}
