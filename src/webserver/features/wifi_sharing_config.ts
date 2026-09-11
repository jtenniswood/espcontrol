import type { PanelConfigDocument } from "../model";
import { parseRawButtonConfig } from "../model/card";
import { parseRawSubpageConfig } from "../model/subpage";
import { cardContractSubpageTypeFromCode } from "../generated/card_contract";

function isWifiSharingType(type: string): boolean {
  return type === "wifi_qr" || type === "wifi_qr_card";
}

function serializedButtonContainsWifiSharing(value: unknown): boolean {
  return isWifiSharingType(parseRawButtonConfig(String(value || "")).type);
}

function serializedSubpageContainsWifiSharing(value: unknown): boolean {
  return parseRawSubpageConfig(String(value || ""), cardContractSubpageTypeFromCode)
    .buttons.some((button) => isWifiSharingType(button.type));
}

export function serializedConfigContainsWifiSharing(value: unknown): boolean {
  const serialized = String(value || "");
  if (!serialized) return false;
  return serializedButtonContainsWifiSharing(serialized) ||
    (serialized.includes("|") && serializedSubpageContainsWifiSharing(serialized));
}

export function panelConfigDocumentContainsWifiSharing(document: PanelConfigDocument): boolean {
  return Object.values(document.buttons).some(serializedButtonContainsWifiSharing) ||
    Object.values(document.subpages).some(serializedSubpageContainsWifiSharing);
}
