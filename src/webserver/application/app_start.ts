import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppStartModule(): GlobalDescriptors {
    // ── Start ──────────────────────────────────────────────────────────────
    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    }
    else {
        init();
    }
    return {};
}
