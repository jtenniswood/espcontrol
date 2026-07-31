import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installButtonSettingsRenderQueueModule(): GlobalDescriptors {
    // ── Button Settings Render Queue ──────────────────────────────────
    // ── Render debouncing ──────────────────────────────────────────────────
    var _renderPending: any = false;
    function scheduleRender(this: any) {
        if (_renderPending)
            return;
        _renderPending = true;
        requestAnimationFrame(function (this: any) {
            _renderPending = false;
            renderPreview();
            if (isSettingsOpen() || isSettingsFocused()) {
                _settingsDeferred = true;
            }
            else {
                renderButtonSettings();
            }
        });
    }
    var _settingsDeferred: any = false;
    document.addEventListener("focusout", function (this: any, e?: any) {
        if (!_settingsDeferred)
            return;
        if (e.relatedTarget && els.buttonSettings && els.buttonSettings.contains(e.relatedTarget))
            return;
        requestAnimationFrame(function (this: any) {
            if (isSettingsOpen())
                return;
            if (!isSettingsFocused()) {
                _settingsDeferred = false;
                renderButtonSettings();
            }
        });
    });
    document.addEventListener("keydown", function (this: any, e?: any) {
        if (e.key === "Escape" && els.settingsOverlay &&
            els.settingsOverlay.classList.contains("sp-visible")) {
            closeSettings();
        }
    });
    return {
        "_renderPending": liveGlobal(() => _renderPending, (value?: any) => { _renderPending = value; }),
        "scheduleRender": staticGlobal(scheduleRender),
        "_settingsDeferred": liveGlobal(() => _settingsDeferred, (value?: any) => { _settingsDeferred = value; }),
    };
}
