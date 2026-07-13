// ── NTP State ──────────────────────────────────────────────────────────
// @web-module-requires: state

function hasCustomNtpServers() {
  return normalizeNtpServer(state.ntpServer1, NTP_SERVER_DEFAULTS[0]) !== NTP_SERVER_DEFAULTS[0] ||
    normalizeNtpServer(state.ntpServer2, NTP_SERVER_DEFAULTS[1]) !== NTP_SERVER_DEFAULTS[1] ||
    normalizeNtpServer(state.ntpServer3, NTP_SERVER_DEFAULTS[2]) !== NTP_SERVER_DEFAULTS[2];
}

function resetNtpServersToDefaults() {
  state.ntpServer1 = NTP_SERVER_DEFAULTS[0];
  state.ntpServer2 = NTP_SERVER_DEFAULTS[1];
  state.ntpServer3 = NTP_SERVER_DEFAULTS[2];
}

function syncNtpServerUi() {
  if (els.setCustomNtpServersToggle) {
    els.setCustomNtpServersToggle.checked = !!state.customNtpServers;
  }
  if (els.setNtpServerFields) {
    els.setNtpServerFields.className =
      "sp-field-stack" + (state.customNtpServers ? "" : " sp-hidden");
  }
  syncInput(els.setNtpServer1, state.ntpServer1);
  syncInput(els.setNtpServer2, state.ntpServer2);
  syncInput(els.setNtpServer3, state.ntpServer3);
}
