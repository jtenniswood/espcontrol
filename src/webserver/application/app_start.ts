import type { AppFeature } from "./app";

const startupState = globalThis as typeof globalThis & {
    __ESPCONTROL_RELOAD_EMBEDDED__?: () => void;
    __ESPCONTROL_UI_STARTED__?: boolean;
    __ESPCONTROL_UI_STARTING__?: boolean;
};

export function startApp(app: Pick<AppFeature, "init">): void {
    // ── Start ──────────────────────────────────────────────────────────────
    function start(this: any) {
        startupState.__ESPCONTROL_UI_STARTING__ = true;
        try {
            app.init();
            startupState.__ESPCONTROL_UI_STARTED__ = true;
        }
        catch (error) {
            startupState.__ESPCONTROL_UI_STARTED__ = false;
            const reload = startupState.__ESPCONTROL_RELOAD_EMBEDDED__;
            if (typeof reload === "function") {
                reload();
                return;
            }
            throw error;
        }
        finally {
            startupState.__ESPCONTROL_UI_STARTING__ = false;
        }
    }
    if (document.readyState === "loading") {
        startupState.__ESPCONTROL_UI_STARTING__ = true;
        document.addEventListener("DOMContentLoaded", start);
    }
    else {
        start();
    }
}
