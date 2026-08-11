import {
  createNativePanelConfigClient,
  updateNativePanelConfigDocument,
  type NativePanelConfigClient,
  type NativePanelConfigFetch,
  type NativePanelConfigSaveResult,
} from "../features/native_panel_config";
import type { PanelConfigDocument } from "../model";

export type NativePanelConfigUpdate = (document: PanelConfigDocument) => PanelConfigDocument;

export interface NativePanelConfigControllerDependencies {
  readonly fetch: NativePanelConfigFetch | null;
  readonly deviceProfile: () => string;
  readonly slotCount: () => number;
  readonly entityName: (name: string) => string;
  readonly entityNameForSlot: (name: string, slot: number) => string;
  readonly normalizeHexColor: (value: string, fallback: string) => string;
  readonly showBanner: (message: string, level: "error") => void;
  readonly delay: (callback: () => void, milliseconds: number) => unknown;
}

/** Owns native configuration discovery and serialised document writes. */
export class NativePanelConfigController {
  private client_: NativePanelConfigClient | null;
  private saveQueue_: Promise<NativePanelConfigSaveResult> = Promise.resolve("saved");
  private retryDelayMs_ = 250;

  constructor(private readonly dependencies: NativePanelConfigControllerDependencies) {
    this.client_ = dependencies.fetch ? createNativePanelConfigClient(dependencies.fetch) : null;
  }

  get client(): NativePanelConfigClient | null { return this.client_; }
  set client(value: NativePanelConfigClient | null) { this.client_ = value; }
  get saveQueue(): Promise<NativePanelConfigSaveResult> { return this.saveQueue_; }
  set saveQueue(value: Promise<NativePanelConfigSaveResult>) { this.saveQueue_ = value; }
  get retryDelayMs(): number { return this.retryDelayMs_; }
  set retryDelayMs(value: number) { this.retryDelayMs_ = value; }

  begin(): Promise<boolean> {
    return this.client_ ? this.client_.discover() : Promise.resolve(false);
  }

  supported(): boolean {
    return this.client_?.supported() ?? false;
  }

  private report(result: NativePanelConfigSaveResult): NativePanelConfigSaveResult {
    if (result === "conflict") {
      this.dependencies.showBanner("Configuration changed in another browser. Reload before saving again.", "error");
    } else if (result === "mirror-failed") {
      this.dependencies.showBanner("The configuration saved, but its older-firmware copy did not. Do not downgrade this panel yet.", "error");
    } else if (result === "failed") {
      this.dependencies.showBanner("Could not save the configuration. Check the connection and try again.", "error");
    }
    return result;
  }

  async waitForDiscovery(): Promise<boolean> {
    const supported = await this.begin();
    if (supported || !this.client_?.retryable()) return supported;
    await new Promise<void>((resolve) => { this.dependencies.delay(resolve, this.retryDelayMs_); });
    return this.waitForDiscovery();
  }

  schedule(update: NativePanelConfigUpdate): Promise<NativePanelConfigSaveResult> | null {
    const client = this.client_;
    if (!client) return null;
    if (!this.supported() && !client.retryable()) {
      // A restarting panel can serve the editor before deferred endpoints are ready.
      void this.begin();
      return null;
    }
    const save = this.saveQueue_
      .then(async () => this.supported() || await this.waitForDiscovery())
      .then(async (supported) => supported ? client.save(update) : "unsupported" as const)
      .then(async (result) => {
        if (result !== "unsupported" || !client.retryable()) return result;
        return await this.waitForDiscovery() ? client.save(update) : result;
      })
      .then((result) => this.report(result), () => this.report("failed"));
    this.saveQueue_ = save;
    return save;
  }

  writeText(name: string, value: string): Promise<NativePanelConfigSaveResult> | null | false {
    if (name === this.dependencies.entityName("button_order")) {
      return this.schedule((current) => updateNativePanelConfigDocument(
        current, this.dependencies.deviceProfile(), "settings", "button_order", value,
      ));
    }
    if (name === this.dependencies.entityName("button_on_color")) {
      const color = this.dependencies.normalizeHexColor(value, "");
      if (!color) return false;
      return this.schedule((current) => updateNativePanelConfigDocument(
        current, this.dependencies.deviceProfile(), "settings", "button_on_color", color,
      ));
    }
    for (let slot = 1; slot <= this.dependencies.slotCount(); slot += 1) {
      if (name === this.dependencies.entityNameForSlot("button_config", slot)) {
        return this.schedule((current) => updateNativePanelConfigDocument(
          current, this.dependencies.deviceProfile(), "buttons", slot, value,
        ));
      }
    }
    return false;
  }

  writeSubpage(slot: number, value: string): Promise<NativePanelConfigSaveResult> | false | null {
    if (!Number.isInteger(slot) || slot < 1 || slot > this.dependencies.slotCount()) return false;
    return this.schedule((current) => updateNativePanelConfigDocument(
      current, this.dependencies.deviceProfile(), "subpages", slot, value,
    ));
  }
}
