import { state } from "../state/app_instance";
import type { UiRuntimeState } from "./state";
export function syncIdleUi(runtime: UiRuntimeState) {
    const els = runtime.els;
        state.homeScreenTimeout = Number(state.homeScreenTimeout) || 0;
        if (els.setHSTimeout)
            els.setHSTimeout.value = String(state.homeScreenTimeout);
        if (els.setIdleBadge) {
            els.setIdleBadge.className = "sp-card-badge" +
                (state.homeScreenTimeout > 0 ? "" : " sp-hidden");
        }
}
