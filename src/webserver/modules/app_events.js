// ── SSE ────────────────────────────────────────────────────────────────
// @web-module-requires: state, firmware_version_state, firmware_update_state, screensaver_timeout, c6_firmware_ui, api, state_loader_api, config_codec, app_backup, app_status_preview, app_title, app_config_events, app_state_event_handlers

function connectEvents() {
  if (_eventSource) { _eventSource.close(); _eventSource = null; }

  function markConnected() {
    resetStateForConnection(state);
    orderReceived = false;
    setConfigLocked(false);
    if (els.banner) els.banner.className = "sp-banner";
    els.root.querySelectorAll(".sp-apply-btn").forEach(function (btn) {
      btn.disabled = false;
      btn.textContent = "Apply Configuration";
    });
    clearTimeout(migrationTimer);
    migrationTimer = setTimeout(scheduleMigration, 5000);
    clearTimeout(sliderMigrationTimer);
    pendingSliderSubpageMigrations = {};
    refreshFirmwareVersion();
    refreshScreensaverTimeout();
  }

  function handleDisconnected(source) {
    setConfigLocked(true, "Reconnecting to device\u2026");
    showBanner("Reconnecting to device\u2026", "offline");
    if (source.readyState === 2) {
      source.close();
      _eventSource = null;
      setTimeout(connectEvents, 5000);
    }
  }

  var sseHandlers = createSseHandlers();

  applySseHandlerAliases(sseHandlers);

  var ssePatterns = configEventPatterns();

  function handleState(d) {
    rememberEntityPostPath(d);
    var keys = entityStateKeys(d);
    var id = keys[0] || d.id;
    var val = d.state != null ? String(d.state) : "";

    for (var ki = 0; ki < keys.length; ki++) {
      if (sseHandlers[keys[ki]]) { sseHandlers[keys[ki]](val, d, keys[ki]); return; }
    }
    if (isFirmwareVersionEvent(id, d)) {
      setFirmwareVersion(val);
      return;
    }
    if (isFirmwareUpdateEvent(id, d)) {
      setFirmwareUpdateInfo(d);
      return;
    }
    if (isFirmwareInstallButtonEvent(id, d)) {
      state.firmwareUpdateControlsSupported = true;
      state.firmwareInstallControlsSupported = true;
      renderFirmwareUpdateStatus();
      return;
    }
    if (isFirmwareCheckButtonEvent(id, d)) {
      state.firmwareUpdateControlsSupported = true;
      renderFirmwareUpdateStatus();
      return;
    }
    if (isC6FirmwareCurrentEvent(id, d)) {
      setC6FirmwareCurrentVersion(val);
      return;
    }
    if (isC6FirmwareLatestEvent(id, d)) {
      setC6FirmwareLatestVersion(val);
      return;
    }
    if (isC6FirmwareUpdateAvailableEvent(id, d)) {
      setC6FirmwareUpdateAvailable(val);
      return;
    }
    if (isC6FirmwareInstallButtonEvent(id, d)) {
      state.c6FirmwareUpdateControlsSupported = true;
      state.c6FirmwareInstallControlsSupported = true;
      syncC6FirmwareUi();
      return;
    }
    if (isC6FirmwareCheckButtonEvent(id, d)) {
      state.c6FirmwareUpdateControlsSupported = true;
      syncC6FirmwareUi();
      return;
    }
    if (isRemovedLegacyStateEvent(id, d)) return;

    for (var i = 0; i < ssePatterns.length; i++) {
      for (var pk = 0; pk < keys.length; pk++) {
        var m = keys[pk].match(ssePatterns[i].re);
        if (m) { ssePatterns[i].fn(m, val, d); return; }
      }
    }

    console.log("[state] unhandled:", id, val);
  }

  if (!eventStreamEnabled()) {
    loadInitialState(handleState, markConnected);
    return;
  }

  var source = new EventSource("/events");
  _eventSource = source;

  source.addEventListener("open", markConnected);
  source.addEventListener("error", function () {
    handleDisconnected(source);
  });
  source.addEventListener("ping", handleWebServerPingEvent);
  source.addEventListener("state", function (e) {
    var d = parseEntityEventData(e.data);
    if (!d) return;
    handleState(d);
  });

}
