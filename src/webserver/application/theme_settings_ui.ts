import type { AppState } from "../state/types";
import type { UiRuntimeState } from "./state";
import { previewEffectiveTheme } from "../state/preview_theme";

export function syncThemeSettingsUi(state: AppState, runtime: UiRuntimeState): void {
  const els = runtime.els;
  const buttons = els.setThemeModeButtons || {};
  for (const mode of ["Dark", "Light", "Auto"]) {
    const button = buttons[mode];
    if (button) button.classList.toggle("active", state.themeMode === mode);
  }
  const auto = state.themeMode === "Auto";
  if (els.setThemeAutoFields)
    els.setThemeAutoFields.className = "sp-cond-field" + (auto ? " sp-visible" : "");
  const methodButtons = els.setThemeAutoMethodButtons || {};
  for (const method of ["Time", "Sunrise / Sunset"]) {
    const button = methodButtons[method];
    if (button) button.classList.toggle("active", state.themeAutoMethod === method);
  }
  if (els.setThemeScheduleFields)
    els.setThemeScheduleFields.className =
      "sp-cond-field" + (auto && state.themeAutoMethod === "Time" ? " sp-visible" : "");
  if (els.setThemeSunFields)
    els.setThemeSunFields.className =
      "sp-cond-field" + (auto && state.themeAutoMethod === "Sunrise / Sunset" ? " sp-visible" : "");
  if (els.setThemeLightStart) els.setThemeLightStart.value = state.themeLightStart;
  if (els.setThemeDarkStart) els.setThemeDarkStart.value = state.themeDarkStart;
  if (els.setThemeSunriseOffset) els.setThemeSunriseOffset.value = String(state.themeSunriseOffset);
  if (els.setThemeSunsetOffset) els.setThemeSunsetOffset.value = String(state.themeSunsetOffset);
  if (els.previewScreen)
    els.previewScreen.classList.toggle("sp-theme-light", previewEffectiveTheme(state) === "Light");
}
