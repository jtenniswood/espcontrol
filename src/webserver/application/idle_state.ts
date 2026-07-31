import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installIdleStateModule(): GlobalDescriptors {
    // ── Idle State ─────────────────────────────────────────────────────────
    function syncIdleUi(this: any) {
        state.homeScreenTimeout = Number(state.homeScreenTimeout) || 0;
        if (els.setHSTimeout)
            els.setHSTimeout.value = String(state.homeScreenTimeout);
        if (els.setIdleBadge) {
            els.setIdleBadge.className = "sp-card-badge" +
                (state.homeScreenTimeout > 0 ? "" : " sp-hidden");
        }
    }
    return {
        "syncIdleUi": staticGlobal(syncIdleUi),
    };
}
