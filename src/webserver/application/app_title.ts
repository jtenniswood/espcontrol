import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppTitleModule(): GlobalDescriptors {
    // ── Page title ─────────────────────────────────────────────────────────
    function applyPageTitle(this: any, title?: any) {
        var text: any = typeof title === "string" ? title.trim() : "";
        document.title = text || "EspControl";
    }
    function handleWebServerPingEvent(this: any, e?: any) {
        var data: any = null;
        try {
            data = e && e.data ? JSON.parse(e.data) : null;
        }
        catch (_) {
            applyPageTitle("");
            return;
        }
        if (data && Object.prototype.hasOwnProperty.call(data, "title")) {
            applyPageTitle(data.title);
        }
    }
    function loadPageTitleFromEventStream(this: any) {
        if (eventStreamEnabled() || typeof EventSource !== "function")
            return;
        var source: any = new EventSource("/events");
        var closeTimer: any = setTimeout(function (this: any) {
            source.close();
        }, 5000);
        source.addEventListener("ping", function (this: any, e?: any) {
            handleWebServerPingEvent(e);
            clearTimeout(closeTimer);
            source.close();
        });
        source.addEventListener("error", function (this: any) {
            clearTimeout(closeTimer);
            source.close();
        });
    }
    return {
        "applyPageTitle": staticGlobal(applyPageTitle),
        "handleWebServerPingEvent": staticGlobal(handleWebServerPingEvent),
        "loadPageTitleFromEventStream": staticGlobal(loadPageTitleFromEventStream),
    };
}
