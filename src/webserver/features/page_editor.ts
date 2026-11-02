import type { CardConfig } from "../contracts/types";
import type { RuntimeSubpage } from "../state/types";
import type { PanelConfigDocument } from "../model/panel_config";
import type { NativePanelConfigSnapshot } from "./native_panel_config";
import { pageDepth, pageOwnerSlot, parentPageKey } from "../model/page_keys";
import { panelPageDocumentDepth } from "../model/panel_config_conversion";

export interface PageEditorDocument {
  readonly buttons: CardConfig[];
  readonly subpages: Record<number, RuntimeSubpage>;
  readonly source: PanelConfigDocument;
  readonly nested: boolean;
}
export interface PageEditorCodec {
  parseButtonConfig(value: string): CardConfig;
  parseSubpageConfig(value: string): RuntimeSubpage;
  buildSubpageGrid(page: RuntimeSubpage): unknown;
}

/** Parse everything privately before replacing any live editor state. */
export function preparePageEditorDocument(document: PanelConfigDocument, slots: number, codec: PageEditorCodec): PageEditorDocument {
  const buttons = Array.from({ length: slots }, (_, index) => codec.parseButtonConfig(document.buttons[index + 1] ?? ""));
  const subpages: Record<number, RuntimeSubpage> = Object.create(null) as Record<number, RuntimeSubpage>;
  for (const [key, value] of Object.entries(document.subpages)) {
    if (!/^[1-9][0-9]*$/.test(key) || !pageDepth(Number(key))) throw new Error("Invalid page path");
    const page = codec.parseSubpageConfig(value);
    codec.buildSubpageGrid(page);
    subpages[Number(key)] = page;
  }
  return { buttons, subpages, nested: panelPageDocumentDepth(document) > 1,
    source: { ...document, buttons: { ...document.buttons }, subpages: { ...document.subpages }, settings: { ...document.settings } } };
}

export function editorPageOwner(key: number, buttons: readonly CardConfig[], subpages: Readonly<Record<number, RuntimeSubpage>>): CardConfig | undefined {
  const parent = parentPageKey(key), slot = pageOwnerSlot(key);
  return (parent === 0 ? buttons : subpages[parent]?.buttons)?.[slot - 1];
}
export function restoredEditorPage(key: number | null, document: PageEditorDocument): number | null {
  if (key === null || key === 0) return null;
  for (let page = key; page !== 0; page = parentPageKey(page))
    if (editorPageOwner(page, document.buttons, document.subpages)?.type !== "subpage") return null;
  return key;
}

export interface NativePageEditorLoaderOptions {
  discover(): Promise<boolean | "legacy-fallback" | "failed">;
  readDepth(): number;
  revision(): number;
  read(signal: AbortSignal): Promise<NativePanelConfigSnapshot | null>;
  canApply(): boolean;
  apply(document: PanelConfigDocument): void;
  failed(error: unknown): void;
  loading?(active: boolean): void;
}
/** Ignore disconnected, superseded and locally edited snapshots. */
export class NativePageEditorLoader {
  private epoch = 0;
  private abort: AbortController | null = null;
  private pending: Promise<boolean> | null = null;
  constructor(private readonly options: NativePageEditorLoaderOptions) {}
  invalidate(): void { ++this.epoch; this.abort?.abort(); this.abort = null; this.pending = null; }
  refresh(): Promise<boolean> {
    if (this.pending) return this.pending;
    const abort = new AbortController();
    this.abort = abort;
    this.options.loading?.(true);
    const epoch = this.epoch, revision = this.options.revision();
    const task = (async () => {
      try {
        if (await this.options.discover() !== true || this.options.readDepth() < 2) return false;
        const snapshot = await this.options.read(abort.signal);
        if (!snapshot || epoch !== this.epoch || revision !== this.options.revision() || !this.options.canApply()) return false;
        this.options.apply(snapshot.document);
        return true;
      } catch (error) {
        if (epoch === this.epoch) this.options.failed(error);
        return false;
      } finally {
        if (epoch === this.epoch) { this.pending = null; this.abort = null; this.options.loading?.(false); }
      }
    })();
    this.pending = task;
    return task;
  }
}
