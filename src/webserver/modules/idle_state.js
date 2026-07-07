// ── Idle State ─────────────────────────────────────────────────────────
// @web-module-requires: state

function syncIdleUi() {
  state.homeScreenTimeout = parseFloat(state.homeScreenTimeout) || 0;
  if (els.setHSTimeout) els.setHSTimeout.value = String(state.homeScreenTimeout);
  if (els.setIdleBadge) {
    els.setIdleBadge.className = "sp-card-badge" +
      (state.homeScreenTimeout > 0 ? "" : " sp-hidden");
  }
}
