// ── Firmware Version State ─────────────────────────────────────────────
// @web-module-requires: state, firmware_metadata

var FIRMWARE_CHECKING_VERSION_LABEL = "Checking version...";
var FIRMWARE_DEV_VERSION_LABEL = "Dev build";
var FIRMWARE_UNKNOWN_VERSION_LABEL = "Version unknown";

function renderFirmwareVersion() {
  if (!els.fwVersionLabel) return;
  els.fwVersionLabel.innerHTML = '<span class="sp-fw-label">Installed </span>' +
    escHtml(firmwareVersionLabel());
}

function setFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return;
  if (isSpecificFirmwareVersion(state.firmwareVersion) && !isSpecificFirmwareVersion(version)) return;
  state.firmwareVersion = displayFirmwareVersion(version);
  renderFirmwareVersion();
  syncFirmwareVersionSelect();
  renderFirmwareUpdateStatus();
  stopFirmwareInstallRefreshIfComplete();
}

function displayFirmwareVersion(version) {
  version = String(version == null ? "" : version).trim();
  if (!version) return FIRMWARE_UNKNOWN_VERSION_LABEL;
  if (version === FIRMWARE_UNKNOWN_VERSION_LABEL) return FIRMWARE_UNKNOWN_VERSION_LABEL;
  return isSpecificFirmwareVersion(version) ? version : FIRMWARE_DEV_VERSION_LABEL;
}

function firmwareVersionLabel() {
  if (!state.firmwareVersion && state.firmwareVersionRefreshPending) {
    return FIRMWARE_CHECKING_VERSION_LABEL;
  }
  return displayFirmwareVersion(state.firmwareVersion);
}
