import {
  PANEL_CONFIG_DOCUMENT_VERSION,
  decodePanelConfig,
  encodePanelConfig,
  type PanelConfigDocument,
} from "../model";

export interface NativePanelConfigResponse {
  readonly ok: boolean;
  readonly status: number;
  readonly headers: { get(name: string): string | null };
  json(): Promise<unknown>;
  arrayBuffer(): Promise<ArrayBuffer>;
}

export interface NativePanelConfigRequest {
  readonly method?: "GET" | "PUT";
  readonly cache?: "no-store";
  readonly headers?: Record<string, string>;
  readonly body?: Uint8Array;
}

export type NativePanelConfigFetch = (
  path: string,
  request?: NativePanelConfigRequest,
) => Promise<NativePanelConfigResponse>;

export type NativePanelConfigSaveResult = "saved" | "unsupported" | "conflict" | "mirror-failed" | "authentication-required" | "failed";
export type NativePanelConfigCollection = "buttons" | "subpages" | "settings";

interface Capabilities {
  configuration?: {
    read?: unknown;
    write?: unknown;
    document_versions?: unknown;
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
    configuration.document_versions.includes(PANEL_CONFIG_DOCUMENT_VERSION);
}

export class NativePanelConfigClient {
  private supported_ = false;
  private retryable_ = false;
  private confirmedUnsupported_ = false;
  private discovery_: Promise<boolean> | null = null;

  constructor(private readonly fetch_: NativePanelConfigFetch) {}

  supported(): boolean { return this.supported_; }
  retryable(): boolean { return this.retryable_; }
  confirmedUnsupported(): boolean { return this.confirmedUnsupported_; }

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
        this.confirmedUnsupported_ = false;
        if (!response.ok) return false;
        const supported = supportedCapabilities(await response.json());
        this.confirmedUnsupported_ = supported === false;
        return supported === true;
      })
      .catch(() => {
        this.retryable_ = false;
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

  async save(update: (document: PanelConfigDocument) => PanelConfigDocument): Promise<NativePanelConfigSaveResult> {
    if (!await this.discover()) return "unsupported";
    for (let attempt = 0; attempt < 2; attempt += 1) {
      try {
        const current = await this.fetch_("/api/v1/config", { cache: "no-store" });
        if (!current.ok) {
          if (current.status === 403) return "authentication-required";
          if (current.status === 404 || current.status === 503) {
            this.retryDiscovery();
            return "unsupported";
          }
          return "failed";
        }
        const generation = current.headers.get("ETag");
        if (!generation) return "failed";
        const currentDocument = decodePanelConfig(new Uint8Array(await current.arrayBuffer()));
        const next = await this.fetch_("/api/v1/config", {
          method: "PUT",
          cache: "no-store",
          headers: {
            "Content-Type": "application/vnd.espcontrol.panel-config",
            "If-Match": generation,
          },
          body: encodePanelConfig(update(currentDocument)),
        });
        // A 202 means the document itself is safe, but firmware could not
        // update its legacy entity mirror. Treat it as a failed compatibility
        // save so callers do not claim that an older firmware can restore it.
        if (next.status === 202) return "mirror-failed";
        if (next.ok) return "saved";
        if (next.status === 403) return "authentication-required";
        if (next.status === 404 || next.status === 503) {
          this.retryDiscovery();
          return "unsupported";
        }
        if (next.status !== 409) return "failed";
      } catch {
        return "failed";
      }
    }
    return "conflict";
  }
}

export function createNativePanelConfigClient(fetch: NativePanelConfigFetch): NativePanelConfigClient {
  return new NativePanelConfigClient(fetch);
}
