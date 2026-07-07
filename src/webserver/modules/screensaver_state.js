// ── Screensaver State ──────────────────────────────────────────────────
// @web-module-requires: state

function getActiveScreensaverMode() {
  if (state.screensaverMode === "sensor") return "sensor";
  if (state.screensaverMode === "timer") return "timer";
  return "disabled";
}

function normalizeScreensaverAction(value) {
  return EspControlModel.normalizeScreensaverAction(value);
}

function screensaverActionOption(value) {
  return EspControlModel.screensaverActionOption(value);
}

function normalizeClockBrightness(value, fallback) {
  return EspControlModel.normalizeClockBrightness(value, fallback);
}

function normalizeScreensaverDimmedBrightness(value) {
  return EspControlModel.normalizeScreensaverDimmedBrightness(value);
}
