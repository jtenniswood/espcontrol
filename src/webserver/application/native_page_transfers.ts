import { state } from "../state/app_instance";
import { createClipboardEntry } from "../features/clipboard";
import { capturePageCards, planPageCardDelete, planPageCardPaste, PageTransferConflict,
  type PageCardClipboard, type PageTransferCard, type PageTransferOrigin } from "../model/page_card_transfer";
import type { PanelConfigDocument } from "../model/panel_config";
import type { NativePanelConfigController } from "../controllers/native_panel_config_controller";
import type { NativePageEditorLoader } from "../features/page_editor";
import type { ApplicationLayoutState } from "./application_context";
import type { ApplicationApiFeature } from "./api";
import type { GridFeature } from "./grid";
import type { ControlsShellFeature } from "./controls_shell";

export interface NativePageTransfersFeature {
  available(): boolean;
  capture(slots: readonly number[]): Promise<PageCardClipboard | null>;
  copy(slots: readonly number[], cut?: boolean): Promise<boolean>;
  paste(position: number, page?: number, entries?: readonly PageTransferCard[]): Promise<{ ok: boolean; resized: number }>;
  duplicate(slot: number): Promise<boolean>;
  delete(slots: readonly number[]): Promise<boolean>;
}
/** Publish the complete native transaction before changing the visible editor. */
export function createNativePageTransfersFeature(options: {
  native: NativePanelConfigController;
  editor: () => NativePageEditorLoader;
  layout: ApplicationLayoutState;
  api: Pick<ApplicationApiFeature, "postQueueIdle" | "postQueueHadError">;
  grid: Pick<GridFeature, "flushMainGridSave" | "ctx">;
  shell: Pick<ControlsShellFeature, "isConfigLocked" | "syncConfigLockUi" | "showBanner">;
  validate(entries: readonly PageTransferCard[], page: number, moving: boolean): void;
}): NativePageTransfersFeature {
  const currentLayout = () => ({ slots: options.layout.previewSlots, columns: options.layout.gridCols });
  let busy = false;
  let unappliedSaveAttempt: number | null = null;
  const available = () => options.native.writableNavigationDepth() === 2;
  const page = () => state.editingSubpage ?? 0;
  async function run(action: (layout: ReturnType<typeof currentLayout>) => Promise<void>): Promise<boolean> {
    if (!available() || busy || options.shell.isConfigLocked()) return false;
    if (state.settingsDraft?.dirty || state.settingsDraft?.isNew) {
      options.shell.showBanner("Save or cancel the open card before transferring cards.", "warning"); return false;
    }
    busy = true;
    options.editor().invalidate();
    options.native.markLocalEdit();
    state.nativeConfigLoading = true;
    options.shell.syncConfigLockUi();
    try {
      options.grid.flushMainGridSave();
      await options.api.postQueueIdle();
      const previous = await options.native.saveQueue;
      if (options.api.postQueueHadError() || (previous !== "saved" && previous !== "mirror-failed" && options.native.saveAttemptId !== unappliedSaveAttempt))
        throw new Error("The previous change did not save. Retry it or reload before transferring cards.");
      if (state.configLocked) throw new Error("The connection changed. Try again after reconnecting.");
      await action(currentLayout());
      return true;
    } catch (error) {
      options.shell.showBanner(error instanceof Error ? error.message : "The cards could not be transferred.", "error");
      return false;
    } finally {
      busy = false; state.nativeConfigLoading = false; options.shell.syncConfigLockUi();
    }
  }
  function savedDocument(): PanelConfigDocument {
    const document = options.native.client?.lastDocument();
    if (!document) throw new Error("Reload the saved pages before transferring cards.");
    return document;
  }
  function unchanged(current: PanelConfigDocument, origin: PageTransferOrigin, layout: ReturnType<typeof currentLayout>): void {
    if (capturePageCards(current, origin.page, origin.slots, layout).origin.fingerprint !== origin.fingerprint)
      throw new PageTransferConflict("The selected cards changed. Reload before trying again.");
  }
  async function save(plan: (current: PanelConfigDocument) => PanelConfigDocument): Promise<void> {
    let committed: PanelConfigDocument | null = null;
    let planningFailed = false, planningError: unknown;
    const pending = options.native.schedule(current => {
      try { committed = plan(current); return committed; }
      catch (error) { planningFailed = true; planningError = error; throw error; }
    });
    const attempt = options.native.saveAttemptId;
    const result = await pending;
    // A failed connection may lose the acknowledgement after publication.
    // Local planning errors occur before PUT; explicit server rejections also
    // leave storage unchanged. Reads preserve eligibility for that attempt.
    unappliedSaveAttempt = pending && (planningFailed || result !== "failed" && result !== "legacy-fallback") ? attempt : null;
    if (planningFailed) throw planningError;
    if (result === "failed") throw new Error("Could not confirm the transfer. Reload the saved pages before trying again.");
    if ((result !== "saved" && result !== "mirror-failed") || !committed)
      throw new Error(result === "conflict" ? "The saved pages changed. Reload before trying again." : "The cards did not save. No transfer was applied.");
    unappliedSaveAttempt = null;
    // A disconnected panel will refresh itself on reconnect. Do not overwrite
    // its connection state with the candidate from this now-obsolete session.
    if (!state.configLocked) options.editor().replace(committed);
  }
  async function capture(slots: readonly number[]): Promise<PageCardClipboard | null> {
    const sourcePage = page();
    let captured: PageCardClipboard | null = null;
    await run(async layout => {
      const expected = capturePageCards(savedDocument(), sourcePage, slots, layout).origin;
      const snapshot = await options.native.readSnapshot();
      if (!snapshot) throw new Error("Could not read the saved cards.");
      unchanged(snapshot.document, expected, layout);
      captured = capturePageCards(snapshot.document, sourcePage, slots, layout);
    });
    return captured;
  }
  async function copy(slots: readonly number[], cut = false): Promise<boolean> {
    const captured = await capture(slots);
    if (!captured) return false;
    state.clipboard = { buttons: captured.entries.map(entry => createClipboardEntry(entry.button, entry.size, entry.subtree?.configuration ?? null)),
      native: { ...captured, cut } };
    return true;
  }
  async function paste(position: number, target = page(), supplied?: readonly PageTransferCard[]): Promise<{ ok: boolean; resized: number }> {
    const clipboard = state.clipboard;
    const entries = supplied ?? clipboard?.native?.entries ?? clipboard?.buttons.map(entry => ({ button: entry, size: entry.size,
      subtree: entry.subpageConfig === null ? null : { configuration: entry.subpageConfig, children: {} } }));
    if (!entries) return { ok: false, resized: 0 };
    const move = supplied ? undefined : clipboard?.native?.cut ? clipboard.native.origin : undefined;
    let resized = 0;
    const ok = await run(async layout => {
      options.validate(entries, target, !!move);
      await save(current => {
        const plan = planPageCardPaste(current, target, position, entries, layout, move);
        resized = plan.resized; return plan.document;
      });
      if (!supplied && state.clipboard === clipboard) state.clipboard = null;
    });
    if (ok && resized && !supplied) options.shell.showBanner("Cards pasted. Some were resized to fit.", "warning");
    return { ok, resized: ok ? resized : 0 };
  }
  async function duplicate(slot: number): Promise<boolean> {
    const sourcePage = page(), pos = options.grid.ctx().grid.indexOf(slot);
    return run(async layout => {
      const captured = capturePageCards(savedDocument(), sourcePage, [slot], layout);
      options.validate(captured.entries, sourcePage, false);
      await save(current => {
        unchanged(current, captured.origin, layout);
        return planPageCardPaste(current, sourcePage, (pos + 1) % layout.slots, captured.entries, layout).document;
      });
    });
  }
  async function remove(slots: readonly number[]): Promise<boolean> {
    const sourcePage = page();
    return run(async layout => {
      const origin = capturePageCards(savedDocument(), sourcePage, slots, layout).origin;
      await save(current => planPageCardDelete(current, origin, layout));
    });
  }
  return { available, capture, copy, paste, duplicate, delete: remove };
}
