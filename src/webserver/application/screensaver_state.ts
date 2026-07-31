import { state } from "../state/app_instance";
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
    return {
        "getActiveScreensaverMode": staticGlobal(getActiveScreensaverMode),
    };
}
