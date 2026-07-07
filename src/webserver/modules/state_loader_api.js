// ── State Loader API ──────────────────────────────────────────────────
// @web-module-requires: state, firmware_metadata, firmware_version_state, entity_catalog, entity_state, firmware_update_state, screensaver_timeout, c6_firmware_ui, config_post_api

function eventStreamEnabled() {
  try {
    return new URLSearchParams(window.location.search).get("events") === "1";
  } catch (_) {
    return false;
  }
}

function cardStateEntities() {
  return entityStateItems(ENTITY_CATALOG.groups.card)
    .concat(entityStateItemsForSlots(ENTITY_CATALOG.groups.card_slot));
}

function settingsStateEntities() {
  var items = entityStateItems(ENTITY_CATALOG.groups.settings);

  if (CFG.features && CFG.features.screenRotation) {
    items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_optional));
  }
  if (CFG.features && CFG.features.voiceServices) {
    items = items.concat(entityStateItems(ENTITY_CATALOG.groups.settings_voice));
  }

  return items;
}

function subpageStateEntities() {
  return entityStateItemsForSlots(subpageEntityKeys());
}

function loadStateItems(items, handleState, concurrency) {
  var index = 0;
  var active = 0;
  var loadedCount = 0;
  var limit = Math.max(1, concurrency || 1);

  return new Promise(function (resolve) {
    function done() {
      active--;
      run();
    }

    function run() {
      if (index >= items.length && active === 0) {
        resolve(loadedCount);
        return;
      }

      while (active < limit && index < items.length) {
        var item = items[index++];
        active++;
        getJsonQuietly(entityDetailPath(item[0], item[1], entityInitialDetail(item[0]))).then(function (data) {
          if (data) {
            loadedCount++;
            handleState(data);
          }
        }).then(done, done);
      }
    }

    run();
  });
}

function loadInitialState(handleState, onLoaded) {
  loadStateItems(cardStateEntities(), handleState, 4).then(function (loadedCount) {
    if (loadedCount === 0) {
      setConfigLocked(true, "Reconnecting to device\u2026");
      showBanner("Reconnecting to device\u2026", "offline");
      setTimeout(connectEvents, 5000);
      return;
    }
    if (onLoaded) onLoaded();
    clearTimeout(migrationTimer);
    migrationTimer = setTimeout(scheduleMigration, 5000);
    clearTimeout(sliderMigrationTimer);
    pendingSliderSubpageMigrations = {};

    loadStateItems(settingsStateEntities(), handleState, 2).then(function () {
      loadStateItems(subpageStateEntities(), handleState, 2);
    });
  });
}

function refreshFirmwareVersion() {
  var pending = 12;
  if (!state.firmwareVersion) {
    state.firmwareVersionRefreshPending = true;
    renderFirmwareVersion();
  }
  function finishFirmwareVersionRefresh() {
    pending--;
    if (pending > 0) return;
    state.firmwareVersionRefreshPending = false;
    renderFirmwareVersion();
  }

  getJsonQuietly(FIRMWARE_VERSION_METADATA_PATH, function (d) {
    setFirmwareVersion(firmwareVersionFromMetadata(d));
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonQuietly(publicFirmwareManifestUrl(), function (d) {
    setPublicFirmwareInfo(firmwareInfoFromPublicManifest(d));
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonQuietly(publicFirmwareVersionsUrl(), function (d) {
    setPublicFirmwareVersions(firmwareInfosFromPublicVersions(d));
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("firmware_version")), function (d) {
    setFirmwareVersion(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("update", entityLookupNames("firmware_update")), function (d) {
    rememberEntityPostPath(d);
    setFirmwareUpdateInfo(d);
  }).then(function (data) {
    if (!data && state.firmwareUpdateControlsSupported !== true) {
      state.firmwareUpdateControlsSupported = false;
      syncFirmwareUpdateUi();
    }
    finishFirmwareVersionRefresh();
  }, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("firmware_install_update")), function (d) {
    rememberEntityPostPath(d);
    state.firmwareUpdateControlsSupported = true;
    state.firmwareInstallControlsSupported = true;
    renderFirmwareUpdateStatus();
    syncFirmwareUpdateUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("firmware_check_for_update")), function (d) {
    rememberEntityPostPath(d);
    state.firmwareUpdateControlsSupported = true;
    renderFirmwareUpdateStatus();
    syncFirmwareUpdateUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_current_firmware")), function (d) {
    rememberEntityPostPath(d);
    setC6FirmwareCurrentVersion(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_latest_firmware")), function (d) {
    rememberEntityPostPath(d);
    setC6FirmwareLatestVersion(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("text_sensor", entityLookupNames("esp32_c6_update_available")), function (d) {
    rememberEntityPostPath(d);
    setC6FirmwareUpdateAvailable(d.state || d.value);
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("esp32_c6_install_update")), function (d) {
    rememberEntityPostPath(d);
    state.c6FirmwareUpdateControlsSupported = true;
    state.c6FirmwareInstallControlsSupported = true;
    syncC6FirmwareUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
  getJsonFirst(entityDetailPaths("button", entityLookupNames("esp32_c6_check_for_update")), function (d) {
    rememberEntityPostPath(d);
    state.c6FirmwareUpdateControlsSupported = true;
    syncC6FirmwareUi();
  }).then(finishFirmwareVersionRefresh, finishFirmwareVersionRefresh);
}

function refreshScreensaverTimeout() {
  getJsonQuietly("/number/" + encodeURIComponent(entityName("screensaver_timeout")) + "?detail=all", applyScreensaverTimeoutState)
    .then(function (data) {
      if (!data) {
        getJsonQuietly("/number/" + encodeURIComponent(entityObjectIds("screensaver_timeout")[0]) + "?detail=all", applyScreensaverTimeoutState);
      }
    });
}

function waitForReboot() {
  if (_eventSource) { _eventSource.close(); _eventSource = null; }
  setConfigLocked(true, "Restarting device\u2026");
  showBanner("Restarting device\u2026", "offline");
  setTimeout(function () {
    connectEvents();
  }, 15000);
}
