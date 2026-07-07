// ── Button Settings Render Queue ──────────────────────────────────
// @web-module-requires: state, preview_render

// ── Render debouncing ──────────────────────────────────────────────────

var _renderPending = false;
function scheduleRender() {
  if (_renderPending) return;
  _renderPending = true;
  requestAnimationFrame(function () {
    _renderPending = false;
    renderPreview();
    if (isSettingsOpen() || isSettingsFocused()) {
      _settingsDeferred = true;
    } else {
      renderButtonSettings();
    }
  });
}

var _settingsDeferred = false;
document.addEventListener("focusout", function (e) {
  if (!_settingsDeferred) return;
  if (e.relatedTarget && els.buttonSettings && els.buttonSettings.contains(e.relatedTarget)) return;
  requestAnimationFrame(function () {
    if (isSettingsOpen()) return;
    if (!isSettingsFocused()) {
      _settingsDeferred = false;
      renderButtonSettings();
    }
  });
});

document.addEventListener("keydown", function (e) {
  if (e.key === "Escape" && els.settingsOverlay &&
      els.settingsOverlay.classList.contains("sp-visible")) {
    closeSettings();
  }
});
