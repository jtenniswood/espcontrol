// ── Firmware Update Post API ──────────────────────────────────────────
// @web-module-requires: entity_state, api

function postFirmwareUpdateInstall() {
  var urls = [];
  rememberedPostUrls("button", entityName("firmware_install_update"), entityObjectIds("firmware_install_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("firmware_install_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  rememberedPostUrls("update", entityName("firmware_update"), entityObjectIds("firmware_update"), "install")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("firmware_update").forEach(function (name) {
    uniquePush(urls, "/update/" + encodeURIComponent(name) + "/install");
  });

  post(urls, null, "Could not start firmware update.");
}

function postFirmwareUpdateCheck() {
  var urls = [];
  rememberedPostUrls("button", entityName("firmware_check_for_update"), entityObjectIds("firmware_check_for_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("firmware_check_for_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  post(urls, null, "Could not check for firmware update.");
}

function postC6FirmwareUpdateInstall() {
  var urls = [];
  rememberedPostUrls("button", entityName("esp32_c6_install_update"), entityObjectIds("esp32_c6_install_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("esp32_c6_install_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  post(urls, null, "Could not start WiFi firmware update.");
}

function postC6FirmwareUpdateCheck() {
  var urls = [];
  rememberedPostUrls("button", entityName("esp32_c6_check_for_update"), entityObjectIds("esp32_c6_check_for_update"), "press")
    .forEach(function (url) { uniquePush(urls, url); });
  entityLookupNames("esp32_c6_check_for_update").forEach(function (name) {
    uniquePush(urls, "/button/" + encodeURIComponent(name) + "/press");
  });

  post(urls, null, "Could not check WiFi firmware update.");
}
