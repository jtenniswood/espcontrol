import type { PanelConfigDocument } from "../model";

const WIFI_SHARING_TYPE = /(?:^|[;:|,])wifi_qr(?:_card)?(?=$|[;:|,])/;

export function serializedConfigContainsWifiSharing(value: unknown): boolean {
  return WIFI_SHARING_TYPE.test(String(value || ""));
}

export function panelConfigDocumentContainsWifiSharing(document: PanelConfigDocument): boolean {
  return [...Object.values(document.buttons), ...Object.values(document.subpages)]
    .some(serializedConfigContainsWifiSharing);
}
