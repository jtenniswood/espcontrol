import { state } from "../state/app_instance";
import { NTP_SERVER_DEFAULTS } from "../state/app_state";
import { normalizeNtpServer } from "../model/settings";
import type { UiRuntimeState } from "./state";
export function hasCustomNtpServers() {
        return normalizeNtpServer(state.ntpServer1, NTP_SERVER_DEFAULTS[0]) !== NTP_SERVER_DEFAULTS[0] ||
            normalizeNtpServer(state.ntpServer2, NTP_SERVER_DEFAULTS[1]) !== NTP_SERVER_DEFAULTS[1] ||
            normalizeNtpServer(state.ntpServer3, NTP_SERVER_DEFAULTS[2]) !== NTP_SERVER_DEFAULTS[2];
}
export function resetNtpServersToDefaults() {
        state.ntpServer1 = NTP_SERVER_DEFAULTS[0];
        state.ntpServer2 = NTP_SERVER_DEFAULTS[1];
        state.ntpServer3 = NTP_SERVER_DEFAULTS[2];
}
export function syncNtpServerUi(runtime: UiRuntimeState, syncInput: (element: any, value: any) => void) {
        const els = runtime.els;
        if (els.setCustomNtpServersToggle) {
            els.setCustomNtpServersToggle.checked = !!state.customNtpServers;
        }
        if (els.setNtpServerFields) {
            els.setNtpServerFields.className =
                "sp-field-stack" + (state.customNtpServers ? "" : " sp-hidden");
        }
        syncInput(els.setNtpServer1, state.ntpServer1);
        syncInput(els.setNtpServer2, state.ntpServer2);
        syncInput(els.setNtpServer3, state.ntpServer3);
}
