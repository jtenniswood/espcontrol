import { state } from "../state/app_instance";
import { WEB_UI_COLORS } from "../state/ui_tokens";
import type { UiRuntimeState } from "./state";

export interface AppearanceFeature {
    syncColorUi(): void;
    resetColors(postChanges?: boolean): void;
}

export function createAppearanceFeature(
    runtime: UiRuntimeState,
    dependencies: {
        renderPreview(): void;
        postOnColor(value: string): void;
    },
): AppearanceFeature {
    const els = runtime.els;
    // ── Appearance State ───────────────────────────────────────────────────
    function syncColorUi() {
        if (els.setOnColor && els.setOnColor._syncColor)
            els.setOnColor._syncColor(state.onColor);
    }
    function resetColors(postChanges?: boolean) {
        state.onColor = WEB_UI_COLORS.primary;
        syncColorUi();
        dependencies.renderPreview();
        if (postChanges) {
            dependencies.postOnColor(state.onColor);
        }
    }
    return {
        syncColorUi,
        resetColors,
    };
}
