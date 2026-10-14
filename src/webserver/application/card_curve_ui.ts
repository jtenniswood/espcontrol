import type { AppState } from "../state/types";
import type { UiRuntimeState } from "./state";

export function syncCardCurveUi(state: AppState, runtime: UiRuntimeState): void {
  const els = runtime.els;
  const buttons = els.setCardCurveButtons || {};
  for (const preset of ["Small", "Medium", "Large"]) {
    const button = buttons[preset];
    if (!button) continue;
    const active = state.cardCurve === preset;
    button.classList.toggle("active", active);
    button.setAttribute("aria-pressed", String(active));
    button.disabled = !state.cardCurveSupported;
  }
}
