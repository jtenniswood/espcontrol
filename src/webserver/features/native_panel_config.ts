import {
  PANEL_CONFIG_DOCUMENT_VERSION,
  PANEL_CONFIG_MAX_RECORD_BODY_BYTES,
  decodePanelConfig,
  encodePanelConfig,
  type PanelConfigDocument,
} from "../model";
import { convertNativeV1ToV2, convertNativeV2ToV1, decodePanelPageDocument, encodePanelPageDocument, panelPageDocumentDepth } from "../model/panel_config_conversion";
import { PageTransferConflict } from "../model/page_card_transfer";
import { decodeNativeConfigDocument, nativeConfigDocumentVersion } from "../model/native_config_backup";
import { pageDepth, pageOwnerSlot, parentPageKey } from "../model/page_keys";

const V2_DOCUMENT_VERSION = 2;
const MAX_V2_DOCUMENT_BYTES = 512 * 1024;

export interface NativePanelConfigResponse {
  readonly ok: boolean;
  readonly status: number;
  readonly headers: { get(name: string): string | null };
  json(): Promise<unknown>;
  arrayBuffer(): Promise<ArrayBuffer>;
}

export interface NativePanelConfigSnapshot {
  readonly document: PanelConfigDocument;
  readonly etag: string;
  readonly version: number;
}

export interface NativePanelConfigRequest {
  readonly method?: "GET" | "PUT";
  readonly cache?: "no-store";
  readonly credentials?: "include";
  readonly headers?: Record<string, string>;
  readonly body?: Uint8Array;
  readonly signal?: AbortSignal;
}

export type NativePanelConfigFetch = (
  path: string,
  request?: NativePanelConfigRequest,
) => Promise<NativePanelConfigResponse>;

export type NativePanelConfigSaveResult = "saved" | "unsupported" | "conflict" | "mirror-failed" | "authentication-required" | "too-large" | "firmware-update-required" | "nested-pages-unavailable" | "failed";
export const NATIVE_BACKUP_UPDATE_REQUIRED_MESSAGE = "This backup needs newer panel firmware to restore all saved cards and settings. Update the panel before importing it. No backup settings were changed.";
export class NativePanelConfigCompatibilityError extends Error {}
export function nativeBackupHasInactiveRecords(document: PanelConfigDocument, slotCount: number): boolean {
  return Object.keys(document.buttons).some(slot => Number(slot) > slotCount) || Object.keys(document.subpages).some(key => {
    const page = Number(key);
    return pageOwnerSlot(page) > slotCount || (pageDepth(page) === 2 && parentPageKey(page) > slotCount);
  });
}
export type NativePanelConfigCollection = "buttons" | "subpages" | "settings";

interface Capabilities {
  configuration?: {
    read?: unknown;
    write?: unknown;
    document_versions?: unknown;
    max_record_body_bytes?: unknown;
    max_v2_document_bytes?: unknown;
    max_durable_document_bytes?: unknown;
    max_navigation_depth?: unknown;
    writable_navigation_depth?: unknown;
    durable_document_version?: unknown;
    preserves_inactive_records?: unknown;
  };
}

/** Replaces one native record without discarding cards that have not loaded yet. */
export function updateNativePanelConfigDocument(
  current: PanelConfigDocument,
  deviceProfile: string,
  collection: NativePanelConfigCollection,
  key: number | string,
  value: string,
): PanelConfigDocument {
  if (current.deviceProfile !== deviceProfile) {
    throw new Error("The device configuration profile changed. Reload this page before saving.");
  }
  const values: Record<string, string> = { ...current[collection] };
  const recordKey = String(key);
  if (value) values[recordKey] = value;
  else delete values[recordKey];
  return { ...current, [collection]: values };
}

function supportedCapabilities(value: unknown): boolean | null {
  if (!value || typeof value !== "object" || Array.isArray(value)) return null;
  const capabilities = value as Capabilities;
  if (!("configuration" in capabilities)) return null;
  const configuration = capabilities.configuration;
  if (!configuration || typeof configuration !== "object" ||
      typeof configuration.read !== "boolean" ||
      typeof configuration.write !== "boolean" ||
      !Array.isArray(configuration.document_versions)) return null;
  if (!configuration.document_versions.every((version) =>
    Number.isInteger(version) && Number(version) > 0)) return null;
  return configuration.read && configuration.write &&
    configuration.document_versions.some((version) => version === PANEL_CONFIG_DOCUMENT_VERSION || version === V2_DOCUMENT_VERSION);
}

export class NativePanelConfigClient {
  private supported_ = false;
  private knownNative_ = false;
  private capabilitiesMissing_ = false;
  private retryable_ = false;
  private confirmedUnsupported_ = false;
  private discovery_: Promise<boolean> | null = null;
  private maxRecordBodyBytes_: number | null = null;
  private documentVersion_ = PANEL_CONFIG_DOCUMENT_VERSION;
  private v1Available_ = false;
  private maxV2DocumentBytes_ = MAX_V2_DOCUMENT_BYTES;
  private maxDurableDocumentBytes_: number | null = null;
  private preservesInactiveRecords_ = false;
  private readDepth_ = 1;
  private writeDepth_ = 1;
  private durableVersion_ = 1;
  private lastDocument_: PanelConfigDocument | null = null;

  constructor(private readonly fetch_: NativePanelConfigFetch) {}

  supported(): boolean { return this.supported_; }
  lastDocument(): PanelConfigDocument | null { return this.lastDocument_; }
  acceptSnapshot(document: PanelConfigDocument): void { this.lastDocument_ = document; }
  preservesInactiveRecords(): boolean { return this.supported_ && this.preservesInactiveRecords_; }
  readNavigationDepth(): number {
    return this.supported_ && this.documentVersion_ === V2_DOCUMENT_VERSION ? this.readDepth_ : 1;
  }
  writableNavigationDepth(): number {
    return this.supported_ && this.documentVersion_ === V2_DOCUMENT_VERSION ? this.writeDepth_ : 1;
  }
  maxConfigValueBytes(): number | null {
    return this.supported_ && this.maxRecordBodyBytes_ !== null
      ? this.maxRecordBodyBytes_ - 1 : null;
  }
  retryable(): boolean { return this.retryable_; }
  confirmedUnsupported(): boolean { return this.confirmedUnsupported_; }
  canUseLegacyFallback(): boolean {
    return this.confirmedUnsupported_ || (this.capabilitiesMissing_ && !this.knownNative_);
  }

  private retryDiscovery(): void {
    this.supported_ = false;
    this.retryable_ = true;
    this.confirmedUnsupported_ = false;
    this.discovery_ = null;
  }

  async discover(): Promise<boolean> {
    if (this.discovery_) return this.discovery_;
    this.discovery_ = this.fetch_("/api/v1/capabilities", { cache: "no-store" })
      .then(async (response) => {
        // A 404 or 503 can occur during the short deferred firmware setup.
        // A valid legacy capabilities response, however, must remain on the
        // entity fallback path without waiting for another native request.
        this.retryable_ = response.status === 404 || response.status === 503;
        this.capabilitiesMissing_ = response.status === 404;
        this.confirmedUnsupported_ = false;
        if (!response.ok) return false;
        const capabilities = await response.json();
        const supported = supportedCapabilities(capabilities);
        const limit = (capabilities as Capabilities | null)?.configuration?.max_record_body_bytes;
        this.maxRecordBodyBytes_ = supported && Number.isInteger(limit) && Number(limit) > 1
          ? Math.min(Number(limit), PANEL_CONFIG_MAX_RECORD_BODY_BYTES) : null;
        const configuration = (capabilities as Capabilities | null)?.configuration;
        this.preservesInactiveRecords_ = supported === true && configuration?.preserves_inactive_records === true;
        // A readable/readonly/newer native contract must not be mistaken for
        // firmware that only stores legacy text entities.
        const versions = Array.isArray(configuration?.document_versions) ? configuration.document_versions : [];
        this.v1Available_ = supported === true && versions.includes(PANEL_CONFIG_DOCUMENT_VERSION);
        this.documentVersion_ = supported === true && versions.includes(V2_DOCUMENT_VERSION)
          ? V2_DOCUMENT_VERSION : PANEL_CONFIG_DOCUMENT_VERSION;
        this.readDepth_ = configuration?.max_navigation_depth === 2 ? 2 : 1;
        this.durableVersion_ = configuration?.durable_document_version === 2 ? 2 : 1;
        this.writeDepth_ = this.readDepth_ === 2 && configuration?.writable_navigation_depth === 2 &&
          configuration?.durable_document_version === V2_DOCUMENT_VERSION ? 2 : 1;
        const v2Limit = configuration?.max_v2_document_bytes;
        this.maxV2DocumentBytes_ = Number.isInteger(v2Limit) && Number(v2Limit) > 0
          ? Math.min(Number(v2Limit), MAX_V2_DOCUMENT_BYTES) : MAX_V2_DOCUMENT_BYTES;
        const durableLimit = configuration?.max_durable_document_bytes;
        this.maxDurableDocumentBytes_ = Number.isSafeInteger(durableLimit) && Number(durableLimit) >= 0
          ? Number(durableLimit) : null;
        if (supported !== null && (configuration?.read || configuration?.write || versions.some((version) => version !== PANEL_CONFIG_DOCUMENT_VERSION))) this.knownNative_ = true;
        this.confirmedUnsupported_ = supported === false && configuration?.read === false && configuration?.write === false &&
          versions.every((version) => version === PANEL_CONFIG_DOCUMENT_VERSION);
        return supported === true;
      })
      .catch(() => {
        this.retryable_ = false;
        this.capabilitiesMissing_ = false;
        this.confirmedUnsupported_ = false;
        return false;
      })
      .then((supported) => {
        this.supported_ = supported;
        // A device can be between its ESPHome setup and its deferred native
        // configuration initialization. Do not cache that temporary negative
        // result for the lifetime of a reconnecting editor.
        if (!supported) this.discovery_ = null;
        return supported;
      });
    return this.discovery_;
  }

  private representationHeaders(version: number): Record<string, string> {
    // Keep old firmware's default-v1 request contract unchanged.
    return version === V2_DOCUMENT_VERSION ? { "X-Panel-Config-Version": String(version) } : {};
  }

  private fallbackToV1(version: number): boolean {
    if (version !== V2_DOCUMENT_VERSION || !this.v1Available_) return false;
    this.documentVersion_ = PANEL_CONFIG_DOCUMENT_VERSION;
    return true;
  }

  private decodeDocument(input: Uint8Array, version: number): PanelConfigDocument {
    return version === V2_DOCUMENT_VERSION
      ? this.readNavigationDepth() === 2 ? decodePanelPageDocument(input) : decodePanelConfig(convertNativeV2ToV1(input))
      : decodePanelConfig(input);
  }

  /** Read the authoritative editable graph; no legacy entity reconstruction. */
  async readSnapshot(deviceProfile: string, signal?: AbortSignal): Promise<NativePanelConfigSnapshot | null> {
    if (!await this.discover()) {
      if (this.confirmedUnsupported_) return null;
      throw new Error("Configuration discovery is unavailable.");
    }
    for (let attempt = 0; attempt < 2; ++attempt) {
      if (signal?.aborted) throw new Error("Configuration load was cancelled.");
      const version = this.documentVersion_;
      const response = await this.fetch_("/api/v1/config", {
        cache: "no-store", headers: this.representationHeaders(version), ...(signal ? { signal } : {}),
      });
      if (signal?.aborted) throw new Error("Configuration load was cancelled.");
      if (response.status === 406 && this.fallbackToV1(version)) continue;
      if (!response.ok) throw new Error("Could not read the saved configuration.");
      const returnedVersion = response.headers.get("X-Panel-Config-Version");
      const etag = response.headers.get("ETag");
      if (!etag || (returnedVersion !== null && returnedVersion !== String(version)) ||
          (version === V2_DOCUMENT_VERSION && returnedVersion !== String(version)))
        throw new Error("Configuration snapshot version or generation is unavailable.");
      const input = new Uint8Array(await response.arrayBuffer());
      if (version === V2_DOCUMENT_VERSION && input.length > this.maxV2DocumentBytes_)
        throw new Error("Configuration exceeds the advertised byte limit.");
      if (signal?.aborted) throw new Error("Configuration load was cancelled.");
      const document = this.decodeDocument(input, version);
      if (document.deviceProfile !== deviceProfile) throw new Error("The device configuration profile changed.");
      return { document, etag, version };
    }
    throw new Error("A compatible configuration representation is unavailable.");
  }

  /** Backups must retain records that this editor does not understand. */
  async readForBackup(deviceProfile: string): Promise<Uint8Array | null> {
    if (!await this.discover()) {
      if (this.confirmedUnsupported_) return null;
      throw new Error("Configuration discovery is unavailable.");
    }
    // Keep exact v1 backups on existing writers. A graph writer, or a reader
    // whose stored graph cannot project to v1, requires an exact v2 snapshot.
    const graphReadable = this.readNavigationDepth() === 2;
    let version = graphReadable && this.durableVersion_ === 2 ? 2 : this.v1Available_ ? 1 : graphReadable ? 2 : 0;
    if (!version) throw new Error("A compatible backup representation is unavailable.");
    for (let attempt = 0; attempt < 2; ++attempt) {
      const response = await this.fetch_("/api/v1/config", {
        cache: "no-store", headers: { "X-Panel-Config-Version": String(version) },
      });
      // A supported v1 representation can still be unavailable for this
      // stored graph: firmware rejects lossy v2-to-v1 export with HTTP 422.
      if ((response.status === 406 || response.status === 422) && version === 1 && graphReadable) {
        version = 2;
        continue;
      }
      if (!response.ok) throw new Error("Could not read the saved configuration.");
      const returnedVersion = response.headers.get("X-Panel-Config-Version");
      if ((version === 2 && returnedVersion !== "2") || (returnedVersion !== null && returnedVersion !== String(version)))
        throw new Error("The backup configuration version is unavailable.");
      const bytes = new Uint8Array(await response.arrayBuffer());
      if (version === 2 && bytes.length > this.maxV2DocumentBytes_) throw new Error("The backup exceeds the advertised byte limit.");
      if (nativeConfigDocumentVersion(bytes) !== version) throw new Error("The backup document does not match its negotiated version.");
      if (decodeNativeConfigDocument(bytes).deviceProfile !== deviceProfile)
        throw new Error("The device configuration profile changed.");
      return bytes;
    }
    throw new Error("A compatible backup representation is unavailable.");
  }

  async save(update: (document: PanelConfigDocument) => PanelConfigDocument): Promise<NativePanelConfigSaveResult> {
    if (!await this.discover()) return "unsupported";
    let conflicts = 0;
    while (conflicts < 2) {
      try {
        const version = this.documentVersion_;
        const representationHeaders = this.representationHeaders(version);
        const current = await this.fetch_("/api/v1/config", { cache: "no-store", headers: representationHeaders });
        if (!current.ok) {
          // Only an explicit pre-write version rejection permits a negotiated
          // fallback. Corrupt data, capacity and transaction failures do not.
          if (current.status === 406 && this.fallbackToV1(version)) continue;
          if (current.status === 401 || current.status === 403) return "authentication-required";
          if (current.status === 404 || current.status === 503) {
            this.retryDiscovery();
            return "unsupported";
          }
          return "failed";
        }
        const generation = current.headers.get("ETag");
        if (!generation) return "failed";
        const responseVersion = current.headers.get("X-Panel-Config-Version");
        if ((version === V2_DOCUMENT_VERSION && responseVersion !== String(version)) ||
            (responseVersion !== null && responseVersion !== String(version))) return "failed";
        const input = new Uint8Array(await current.arrayBuffer());
        if (version === V2_DOCUMENT_VERSION && input.length > this.maxV2DocumentBytes_) return "too-large";
        const currentDocument = this.decodeDocument(input, version);
        const currentDepth = panelPageDocumentDepth(currentDocument);
        if (currentDepth > this.writableNavigationDepth()) return "nested-pages-unavailable";
        const updated = update(currentDocument);
        const updatedDepth = panelPageDocumentDepth(updated);
        if (updatedDepth > this.writableNavigationDepth()) return "nested-pages-unavailable";
        const limit = this.maxRecordBodyBytes_ ?? 2048;
        const byteLength = (value: string) => new TextEncoder().encode(value).length;
        if (Object.values(updated.buttons).some((value) => byteLength(value) + 1 > limit) ||
            Object.values(updated.subpages).some((value) => byteLength(value) + 1 > limit) ||
            Object.entries(updated.settings).some(([key, value]) => byteLength(key) + byteLength(value) + 1 > limit)) {
          return "too-large";
        }
        const body = version === V2_DOCUMENT_VERSION && this.writableNavigationDepth() === 2
          ? encodePanelPageDocument(updated)
          : version === V2_DOCUMENT_VERSION ? convertNativeV1ToV2(encodePanelConfig(updated)) : encodePanelConfig(updated);
        if (version === V2_DOCUMENT_VERSION && body.length > this.maxV2DocumentBytes_) return "too-large";
        // A converted transport representation can exceed the saved-byte
        // limit while its durable form still fits. Firmware checks that case
        // after lossless conversion; the editor can check identical formats.
        if (version === this.durableVersion_ && this.maxDurableDocumentBytes_ !== null &&
            body.length > this.maxDurableDocumentBytes_) return "too-large";
        const next = await this.fetch_("/api/v1/config", {
          method: "PUT",
          cache: "no-store",
          headers: {
            ...representationHeaders,
            "Content-Type": "application/vnd.espcontrol.panel-config",
            "If-Match": generation,
          },
          body,
        });
        // Reload before applying the edit through v1, so the fallback never
        // reuses a v2 ETag or overwrites a concurrent change from an old read.
        if (next.status === 415 && (currentDepth > 1 || updatedDepth > 1)) return "nested-pages-unavailable";
        if (next.status === 415 && this.fallbackToV1(version)) continue;
        if (next.status === 422) return "nested-pages-unavailable";
        // A 202 means the document itself is safe, but firmware could not
        // update its legacy entity mirror. Treat it as a failed compatibility
        // save so callers do not claim that an older firmware can restore it.
        if (next.status === 413) return "too-large";
        if (next.ok) {
          this.lastDocument_ = updated;
          return next.status === 202 ? "mirror-failed" : "saved";
        }
        if (next.status === 401 || next.status === 403) return "authentication-required";
        if (next.status === 404 || next.status === 503) {
          this.retryDiscovery();
          return "unsupported";
        }
        if (next.status !== 409) return "failed";
        conflicts += 1;
      } catch (error) {
        if (error instanceof PageTransferConflict) return "conflict";
        if (error instanceof NativePanelConfigCompatibilityError) return "firmware-update-required";
        return "failed";
      }
    }
    return "conflict";
  }
}

export function createNativePanelConfigClient(fetch: NativePanelConfigFetch): NativePanelConfigClient {
  return new NativePanelConfigClient(fetch);
}
