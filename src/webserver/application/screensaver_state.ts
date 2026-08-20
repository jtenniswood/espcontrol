import { state } from "../state/app_instance";
export function getActiveScreensaverMode() {
        if (state.screensaverMode === "sensor")
            return "sensor";
        if (state.screensaverMode === "timer")
            return "timer";
        return "disabled";
}

export function normalizePin(value?: unknown): string {
    return String(value == null ? "" : value).replace(/\D+/g, "").slice(0, 16);
}
