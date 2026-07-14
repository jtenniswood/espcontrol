import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppearanceStateModule(): GlobalDescriptors {
    // ── Appearance State ───────────────────────────────────────────────────
    function normalizeTheme(this: any, value?: any) {
        return THEME_PRESETS[value] ? value : defaultTheme();
    }
    function syncThemeUi(this: any) {
        if (els.root)
            els.root.setAttribute("data-screen-theme", normalizeTheme(state.theme).toLowerCase());
    }
    function syncColorUi(this: any) {
        if (els.setOnColor && els.setOnColor._syncColor)
            els.setOnColor._syncColor(state.onColor);
    }
    function resetAppearanceColors(this: any, postChanges?: any) {
        state.onColor = DEFAULT_COLOR_PRESET.on;
        syncColorUi();
        renderPreview();
        if (postChanges) {
            postText(entityName("button_on_color"), state.onColor);
        }
    }
    return {
        "normalizeTheme": staticGlobal(normalizeTheme),
        "syncThemeUi": staticGlobal(syncThemeUi),
        "syncColorUi": staticGlobal(syncColorUi),
        "resetAppearanceColors": staticGlobal(resetAppearanceColors),
    };
}
