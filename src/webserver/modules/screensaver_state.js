// ── Screensaver State ──────────────────────────────────────────────────
// @web-module-requires: state

function getActiveScreensaverMode() {
  if (state.screensaverMode === "sensor") return "sensor";
  if (state.screensaverMode === "timer") return "timer";
  return "disabled";
}
