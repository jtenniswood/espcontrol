// ── Appearance State ───────────────────────────────────────────────────
// @web-module-requires: state

function normalizeTheme(value) {
  return THEME_PRESETS[value] ? value : defaultTheme();
}

function syncThemeUi() {
  if (els.root) els.root.setAttribute("data-screen-theme", normalizeTheme(state.theme).toLowerCase());
}

function syncColorUi() {
  if (els.setOnColor && els.setOnColor._syncColor) els.setOnColor._syncColor(state.onColor);
}

function resetAppearanceColors(postChanges) {
  state.onColor = DEFAULT_COLOR_PRESET.on;
  syncColorUi();
  renderPreview();
  if (postChanges) {
    postText(entityName("button_on_color"), state.onColor);
  }
}
