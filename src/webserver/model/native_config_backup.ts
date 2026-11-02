import { createPanelConfigBackupPayload, decodePanelConfigBackupPayload, decodePanelConfig,
  encodeBase64, decodeBase64, PanelConfigError, type PanelConfigBackupPayload, type PanelConfigDocument } from "./panel_config";
import { decodePanelPageDocument } from "./panel_config_conversion";

const MAX_GRAPH_BYTES = 512 * 1024;
export function nativeConfigDocumentVersion(bytes: Uint8Array): 1 | 2 {
  if (bytes.length < 16) throw new PanelConfigError("Native backup document is incomplete");
  const version = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint16(4, true);
  if (version !== 1 && version !== 2) throw new PanelConfigError("Native backup document version is unsupported");
  return version;
}
export function decodeNativeConfigDocument(bytes: Uint8Array): PanelConfigDocument {
  if (nativeConfigDocumentVersion(bytes) === 1) return decodePanelConfig(bytes);
  if (bytes.length > MAX_GRAPH_BYTES) throw new PanelConfigError("Native backup exceeds the graph byte limit");
  return decodePanelPageDocument(bytes);
}
/** Preserve original bytes, including graph identities and record order. */
export function createNativeConfigBackupPayload(bytes: Uint8Array): PanelConfigBackupPayload {
  if (nativeConfigDocumentVersion(bytes) === 1) return createPanelConfigBackupPayload(bytes);
  const document = decodeNativeConfigDocument(bytes);
  return { document_version: 2, device_profile: document.deviceProfile, payload: encodeBase64(bytes) };
}
export function decodeNativeConfigBackupPayload(value: unknown): Uint8Array {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new PanelConfigError("Native backup section is invalid");
  const payload = value as Partial<PanelConfigBackupPayload>;
  if (payload.document_version === 1) return decodePanelConfigBackupPayload(value);
  if (payload.document_version !== 2 || typeof payload.device_profile !== "string" ||
      typeof payload.payload !== "string" || payload.payload.length > 4 * Math.ceil(MAX_GRAPH_BYTES / 3))
    throw new PanelConfigError("Native backup version, profile or byte limit is invalid");
  const bytes = decodeBase64(payload.payload);
  if (nativeConfigDocumentVersion(bytes) !== 2) throw new PanelConfigError("Native backup version does not match its document");
  const document = decodeNativeConfigDocument(bytes);
  if (document.deviceProfile !== payload.device_profile) throw new PanelConfigError("Native backup device profile does not match its document");
  return bytes;
}
