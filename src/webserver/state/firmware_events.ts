import type { EntityEvent } from "./types";

interface FirmwareEventFields {
  id: string;
  nameId: string;
  domain: string;
  name: string;
}

function firmwareEventFields(id: string | null | undefined, event: EntityEvent | null | undefined): FirmwareEventFields {
  return {
    id: String(id || "").toLowerCase(),
    nameId: String(event?.name_id || "").toLowerCase(),
    domain: String(event?.domain || "").toLowerCase(),
    name: String(event?.name || "").toLowerCase(),
  };
}

function matches(fields: FirmwareEventFields, nameId: string, domain: string, name: string, idWords: readonly string[]): boolean {
  return fields.nameId === nameId || (fields.domain === domain && fields.name === name) ||
    (fields.id.startsWith(`${domain}-`) && idWords.every((word) => fields.id.includes(word)));
}

export function isFirmwareVersionEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "text_sensor/firmware: version", "text_sensor", "firmware: version", ["firmware", "version"]);
}
export function isFirmwareUpdateEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "update/firmware: update", "update", "firmware: update", ["firmware", "update"]);
}
export function isFirmwareCheckButtonEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "button/firmware: check for update", "button", "firmware: check for update", ["firmware", "check", "update"]);
}
export function isFirmwareInstallButtonEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "button/firmware: install update", "button", "firmware: install update", ["firmware", "install", "update"]);
}
export function isC6FirmwareCurrentEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "text_sensor/esp32-c6: current firmware", "text_sensor", "esp32-c6: current firmware", ["c6", "current", "firmware"]);
}
export function isC6FirmwareLatestEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "text_sensor/esp32-c6: latest firmware", "text_sensor", "esp32-c6: latest firmware", ["c6", "latest", "firmware"]);
}
export function isC6FirmwareUpdateAvailableEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "text_sensor/esp32-c6: update available", "text_sensor", "esp32-c6: update available", ["c6", "update", "available"]);
}
export function isC6FirmwareCheckButtonEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "button/firmware esp32-c6: check for update", "button", "firmware esp32-c6: check for update", ["c6", "check", "update"]);
}
export function isC6FirmwareInstallButtonEvent(id: string, event: EntityEvent): boolean {
  return matches(firmwareEventFields(id, event), "button/firmware esp32-c6: install update", "button", "firmware esp32-c6: install update", ["c6", "install", "update"]);
}
