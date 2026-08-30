import type { UiRuntimeState } from "./state";

export interface ButtonSettingsRenderQueueFeature {
    schedule(): void;
    clearDeferred(): void;
}

export interface ButtonSettingsRenderQueueDependencies {
    readonly document: Document;
    readonly requestFrame: (callback: FrameRequestCallback) => number;
    readonly renderPreview: () => void;
    readonly renderButtonSettings: () => void;
    readonly closeSettings: () => void;
}

export function createButtonSettingsRenderQueueFeature(
    runtime: UiRuntimeState,
    dependencies: ButtonSettingsRenderQueueDependencies,
): ButtonSettingsRenderQueueFeature {
    const els = runtime.els;
    let renderPending = false;
    let settingsDeferred = false;
    const schedule = (): void => {
        if (renderPending) return;
        renderPending = true;
        dependencies.requestFrame(() => {
            renderPending = false;
            dependencies.renderPreview();
            if (runtime.isSettingsOpen() || runtime.isSettingsFocused()) {
                settingsDeferred = true;
            } else {
                dependencies.renderButtonSettings();
            }
        });
    };
    dependencies.document.addEventListener("focusout", (event: FocusEvent) => {
        if (!settingsDeferred) return;
        if (event.relatedTarget && els.buttonSettings?.contains(event.relatedTarget as Node)) return;
        dependencies.requestFrame(() => {
            if (runtime.isSettingsOpen()) return;
            if (!runtime.isSettingsFocused()) {
                settingsDeferred = false;
                dependencies.renderButtonSettings();
            }
        });
    });
    dependencies.document.addEventListener("keydown", (event: KeyboardEvent) => {
        if (event.key === "Escape" && els.settingsOverlay?.classList.contains("sp-visible")) {
            dependencies.closeSettings();
        }
    });
    return {
        schedule,
        clearDeferred: () => { settingsDeferred = false; },
    };
}
