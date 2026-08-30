import {
  createNativePanelConfigClient,
  updateNativePanelConfigDocument,
  type NativePanelConfigClient,
  type NativePanelConfigFetch,
  type NativePanelConfigSaveResult,
} from "../features/native_panel_config";
import type { PanelConfigDocument } from "../model";

export type NativePanelConfigUpdate = (document: PanelConfigDocument) => PanelConfigDocument;
export type NativePanelConfigSaveOutcome = NativePanelConfigSaveResult | "legacy-fallback";

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
  private saveQueue_: Promise<NativePanelConfigSaveOutcome> = Promise.resolve("saved");
  private legacyFallback_ = false;
  private retryDelayMs_ = 250;
  // P4 panels can still be completing their deferred native-configuration
  // setup when the web editor first loads. Allow enough time for that API to
  // become ready before deciding that the firmware only supports legacy
  // entity configuration.
  private maxDiscoveryRetries_ = 40;

  constructor(private readonly dependencies: NativePanelConfigControllerDependencies) {
    this.client_ = dependencies.fetch ? createNativePanelConfigClient(dependencies.fetch) : null;
  }

  get client(): NativePanelConfigClient | null { return this.client_; }
  set client(value: NativePanelConfigClient | null) {
    this.client_ = value;
    this.legacyFallback_ = false;
  }
  get saveQueue(): Promise<NativePanelConfigSaveOutcome> { return this.saveQueue_; }
  set saveQueue(value: Promise<NativePanelConfigSaveOutcome>) { this.saveQueue_ = value; }
  get legacyFallback(): boolean { return this.legacyFallback_; }
  set legacyFallback(value: boolean) { this.legacyFallback_ = value; }
  get retryDelayMs(): number { return this.retryDelayMs_; }
  set retryDelayMs(value: number) { this.retryDelayMs_ = value; }
  get maxDiscoveryRetries(): number { return this.maxDiscoveryRetries_; }
  set maxDiscoveryRetries(value: number) { this.maxDiscoveryRetries_ = value; }

  begin(): Promise<boolean> {
    return this.client_ ? this.client_.discover() : Promise.resolve(false);
  }

  supported(): boolean {
    return this.client_?.supported() ?? false;
  }

  private report(result: NativePanelConfigSaveOutcome): NativePanelConfigSaveOutcome {
    if (result === "conflict") {
      this.dependencies.showBanner("Configuration changed in another browser. Reload before saving again.", "error");
    } else if (result === "mirror-failed") {
      this.dependencies.showBanner("The configuration saved, but its older-firmware copy did not. Do not downgrade this panel yet.", "error");
    } else if (result === "authentication-required") {
      this.dependencies.showBanner("Wifi Sharing passwords require web authentication. Enable the web_server_auth package before saving this card.", "error");
    } else if (result === "failed") {
      this.dependencies.showBanner("Could not save the configuration. Check the connection and try again.", "error");
    }
    return result;
  }

  async waitForDiscovery(attempts = 0): Promise<boolean | "legacy-fallback" | "failed"> {
    if (this.legacyFallback_) return "legacy-fallback";
    if (this.client_?.confirmedUnsupported()) {
      this.legacyFallback_ = true;
      return "legacy-fallback";
    }
    const supported = await this.begin();
    if (supported) {
      this.legacyFallback_ = false;
      return true;
    }
    if (this.client_?.confirmedUnsupported()) {
      // Discovery has completed and confirmed that the native contract is not
      // available. Callers can now safely use the legacy entity path.
      this.legacyFallback_ = true;
      return "legacy-fallback";
    }
    if (!this.client_?.retryable()) return "failed";
    if (attempts >= this.maxDiscoveryRetries_) {
      // Older firmware never exposes the native endpoints. Preserve this
      // capped decision so queued saves use their legacy paths immediately.
      this.legacyFallback_ = true;
      return "legacy-fallback";
    }
    await new Promise<void>((resolve) => { this.dependencies.delay(resolve, this.retryDelayMs_); });
    return this.waitForDiscovery(attempts + 1);
  }

  schedule(update: NativePanelConfigUpdate): Promise<NativePanelConfigSaveOutcome> | null {
    const client = this.client_;
    if (!client) return null;
    const save = this.saveQueue_
      .then(async () => this.supported() || await this.waitForDiscovery())
      .then(async (supported) => supported === "legacy-fallback" || supported === "failed"
        ? supported
        : supported ? client.save(update) : "unsupported" as const)
      .then(async (result) => {
        if (result !== "unsupported" || !client.retryable()) return result;
        const supported = await this.waitForDiscovery();
        if (supported === "legacy-fallback" || supported === "failed") return supported;
        return supported ? client.save(update) : result;
      })
      .then((result) => this.report(result), () => this.report("failed"));
    this.saveQueue_ = save;
    return save;
  }

  writeText(name: string, value: string): Promise<NativePanelConfigSaveOutcome> | null | false {
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

  writeButtonAndOrder(slot: number, value: string, order: string): Promise<NativePanelConfigSaveOutcome> | null | false {
    if (!Number.isInteger(slot) || slot < 1 || slot > this.dependencies.slotCount()) return false;
    return this.schedule((current) => {
      const withButton = updateNativePanelConfigDocument(
        current, this.dependencies.deviceProfile(), "buttons", slot, value,
      );
      return updateNativePanelConfigDocument(
        withButton, this.dependencies.deviceProfile(), "settings", "button_order", order,
      );
    });
  }

  writeDocument(document: PanelConfigDocument): Promise<NativePanelConfigSaveOutcome> | null {
    if (!this.client_) return null;
    return this.schedule((current) => {
      if (current.deviceProfile !== document.deviceProfile) {
        throw new Error("The backup targets a different device profile.");
      }
      return document;
    });
  }

  writeSubpage(slot: number, value: string): Promise<NativePanelConfigSaveOutcome> | false | null {
    if (!Number.isInteger(slot) || slot < 1 || slot > this.dependencies.slotCount()) return false;
    return this.schedule((current) => updateNativePanelConfigDocument(
      current, this.dependencies.deviceProfile(), "subpages", slot, value,
    ));
  }
}
