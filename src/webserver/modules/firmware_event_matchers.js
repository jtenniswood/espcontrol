// Firmware update entity detection helpers.

function firmwareEventFields(id, d) {
  d = d || {};
  return {
    id: String(id || "").toLowerCase(),
    nameId: String(d.name_id || "").toLowerCase(),
    domain: String(d.domain || "").toLowerCase(),
    name: String(d.name || "").toLowerCase()
  };
}

function isFirmwareVersionEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "text_sensor/firmware: version" ||
    (fields.domain === "text_sensor" && fields.name === "firmware: version") ||
    (fields.id.indexOf("text_sensor-") === 0 && fields.id.indexOf("firmware") !== -1 && fields.id.indexOf("version") !== -1);
}

function isFirmwareUpdateEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "update/firmware: update" ||
    (fields.domain === "update" && fields.name === "firmware: update") ||
    (fields.id.indexOf("update-") === 0 && fields.id.indexOf("firmware") !== -1 && fields.id.indexOf("update") !== -1);
}

function isFirmwareCheckButtonEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "button/firmware: check for update" ||
    (fields.domain === "button" && fields.name === "firmware: check for update") ||
    (fields.id.indexOf("button-") === 0 && fields.id.indexOf("firmware") !== -1 &&
      fields.id.indexOf("check") !== -1 && fields.id.indexOf("update") !== -1);
}

function isFirmwareInstallButtonEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "button/firmware: install update" ||
    (fields.domain === "button" && fields.name === "firmware: install update") ||
    (fields.id.indexOf("button-") === 0 && fields.id.indexOf("firmware") !== -1 &&
      fields.id.indexOf("install") !== -1 && fields.id.indexOf("update") !== -1);
}

function isC6FirmwareCurrentEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "text_sensor/esp32-c6: current firmware" ||
    (fields.domain === "text_sensor" && fields.name === "esp32-c6: current firmware") ||
    (fields.id.indexOf("text_sensor-") === 0 && fields.id.indexOf("c6") !== -1 &&
      fields.id.indexOf("current") !== -1 && fields.id.indexOf("firmware") !== -1);
}

function isC6FirmwareLatestEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "text_sensor/esp32-c6: latest firmware" ||
    (fields.domain === "text_sensor" && fields.name === "esp32-c6: latest firmware") ||
    (fields.id.indexOf("text_sensor-") === 0 && fields.id.indexOf("c6") !== -1 &&
      fields.id.indexOf("latest") !== -1 && fields.id.indexOf("firmware") !== -1);
}

function isC6FirmwareUpdateAvailableEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "text_sensor/esp32-c6: update available" ||
    (fields.domain === "text_sensor" && fields.name === "esp32-c6: update available") ||
    (fields.id.indexOf("text_sensor-") === 0 && fields.id.indexOf("c6") !== -1 &&
      fields.id.indexOf("update") !== -1 && fields.id.indexOf("available") !== -1);
}

function isC6FirmwareCheckButtonEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "button/firmware esp32-c6: check for update" ||
    (fields.domain === "button" && fields.name === "firmware esp32-c6: check for update") ||
    (fields.id.indexOf("button-") === 0 && fields.id.indexOf("c6") !== -1 &&
      fields.id.indexOf("check") !== -1 && fields.id.indexOf("update") !== -1);
}

function isC6FirmwareInstallButtonEvent(id, d) {
  var fields = firmwareEventFields(id, d);
  return fields.nameId === "button/firmware esp32-c6: install update" ||
    (fields.domain === "button" && fields.name === "firmware esp32-c6: install update") ||
    (fields.id.indexOf("button-") === 0 && fields.id.indexOf("c6") !== -1 &&
      fields.id.indexOf("install") !== -1 && fields.id.indexOf("update") !== -1);
}
