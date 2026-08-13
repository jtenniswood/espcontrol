import type { ApplicationLayoutState } from "./application_context";

export interface UiRuntimeState {
    els: any;
    dragSrcPos: any;
    didDrag: any;
    previewPlaceholder: any;
    previewDropIdx: any;
    dragRafPending: any;
    dragSrcEl: any;
    dragEnterCount: any;
    orderReceived: boolean;
    migrationTimer: number | null;
    sliderMigrationTimer: number | null;
    pendingSliderSubpageMigrations: Record<string, true>;
    eventSource: any;
    isSettingsFocused(): boolean;
    isSettingsOpen(): boolean;
}

export function createUiRuntimeState(
    layout: ApplicationLayoutState,
    document: Document,
): UiRuntimeState {
    const runtime: UiRuntimeState = {
        els: {},
        dragSrcPos: -1,
        didDrag: false,
        previewPlaceholder: null,
        previewDropIdx: -1,
        dragRafPending: layout.config.dragAnimation ? false : null,
        dragSrcEl: null,
        dragEnterCount: 0,
        orderReceived: false,
        migrationTimer: null,
        sliderMigrationTimer: null,
        pendingSliderSubpageMigrations: {},
        eventSource: null,
        isSettingsFocused() {
            const activeElement = document.activeElement;
            return !!(activeElement && runtime.els.buttonSettings && runtime.els.buttonSettings.contains(activeElement));
        },
        isSettingsOpen() {
            return !!(runtime.els.settingsOverlay && runtime.els.settingsOverlay.classList.contains("sp-visible"));
        },
    };
    return runtime;
}
