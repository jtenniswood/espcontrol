// ── Artwork State ──────────────────────────────────────────────────────
// @web-module-requires: state

function normalizeHomeAssistantArtworkPort(value) {
  var port = parseInt(value, 10);
  if (!isFinite(port)) return 8123;
  if (port < 1) return 1;
  if (port > 65535) return 65535;
  return port;
}

function normalizeHomeAssistantArtworkProtocol(value) {
  value = String(value == null ? "" : value).trim().toLowerCase();
  return value === "https" ? "https" : "http";
}
