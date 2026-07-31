import { state } from "../state/app_instance";
import { staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppearanceStateModule(): GlobalDescriptors {
    // ── Appearance State ───────────────────────────────────────────────────
    function syncColorUi(this: any) {
        if (els.setOnColor && els.setOnColor._syncColor)
            els.setOnColor._syncColor(state.onColor);
    }
    function resetAppearanceColors(this: any, postChanges?: any) {
        state.onColor = WEB_UI_COLORS.primary;
        syncColorUi();
        renderPreview();
        if (postChanges) {
            postText(entityName("button_on_color"), state.onColor);
        }
    }
    return {
        "syncColorUi": staticGlobal(syncColorUi),
        "resetAppearanceColors": staticGlobal(resetAppearanceColors),
    };
}
