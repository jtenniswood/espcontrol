import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installScreensaverStateModule(): GlobalDescriptors {
    // ── Screensaver State ──────────────────────────────────────────────────
    function getActiveScreensaverMode(this: any) {
        if (state.screensaverMode === "sensor")
            return "sensor";
        if (state.screensaverMode === "timer")
            return "timer";
        return "disabled";
    }
    function normalizePin(this: any, value?: any) {
        return String(value == null ? "" : value).replace(/\D+/g, "").slice(0, 16);
    }
    return {
        "getActiveScreensaverMode": staticGlobal(getActiveScreensaverMode),
        "normalizePin": staticGlobal(normalizePin),
    };
}
