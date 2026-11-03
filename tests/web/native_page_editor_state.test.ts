import { initializeAppState, state } from "../../src/webserver/state/app_instance";
import { initializeDeviceConfig } from "../../src/webserver/device_config";
import { createNativePageEditorFeature } from "../../src/webserver/application/native_page_editor";
import { createAppConfigEventsFeature } from "../../src/webserver/application/app_config_events";
import { createControlsShellFeature } from "../../src/webserver/application/controls_shell";
import { createPreviewInteractionsFeature } from "../../src/webserver/application/preview_interactions";
import { createAppStateEventHandlersFeature } from "../../src/webserver/application/app_state_event_handlers";
import { createAppEventsFeature } from "../../src/webserver/application/app_events";
import { parseRawButtonConfig, parseRawSubpageConfig, buildSubpageGrid } from "../../src/webserver/model";

function check(value: unknown, message: string): void { if (!value) throw new Error(message); }
function initialize(): void {
  const globals = globalThis as any;
  globals.__ESPCONTROL_DEFAULT_DEVICE_ID__ = "test";
  globals.__ESPCONTROL_DEVICE_PROFILES__ = { test: { slots: 9 } };
  globals.__ESPCONTROL_TIMEZONE_OPTIONS__ = [];
  initializeDeviceConfig(); initializeAppState();
}

export function runNativeRootSettingsEventTests(): void {
  initialize();
  const noop = new Proxy({}, { get: () => () => undefined }) as any;
  for (const snapshotLoaded of [false, true]) {
    state.nativePageSnapshotLoaded = snapshotLoaded;
    state.nativeHasNestedPages = false;
    state.nativePanelDocument = { deviceProfile: "test", buttons: {}, subpages: {}, settings: {} };
    state.onColor = "112233";
    const orders: string[] = [];
    let refreshes = 0, connected: any;
    const runtime = { els: {} } as any;
    const events = createAppStateEventHandlersFeature(runtime, noop, noop, noop, noop, noop, noop,
      noop, noop, noop, noop, noop, { applyButtonOrderValue: (value: string) => orders.push(value) } as any, noop, noop);
    createAppEventsFeature({ connect: (value: unknown) => { connected = value; } } as any, events,
      { patterns: () => [] }, runtime, noop, noop, noop, noop, noop, noop,
      { eventStreamEnabled: () => false } as any, noop,
      { invalidate() {}, refresh: async () => false, refreshAfterChange() { ++refreshes; } }).connect();
    connected.onState({ id: "text-button_order", state: "2" });
    connected.onState({ id: "text/Button On Color", state: "AA0000" });
    check(orders.length === (snapshotLoaded ? 0 : 1), "only a complete snapshot supersedes legacy root order");
    check(state.onColor === (snapshotLoaded ? "112233" : "AA0000"), "late legacy accent cannot override a flat native snapshot");
    check(refreshes === (snapshotLoaded ? 2 : 0), "named root events refresh native state, including normalized entity aliases");
  }
}
export function runNativeEditorStateTests(): void {
  initialize();
  const layout = { numSlots: 9, totalSlots: 9, previewSlots: 9, gridCols: 3 } as any;
  const runtime = { dragSrcPos: -1, pendingSliderSubpageMigrations: {}, els: {} } as any;
  let appliedLegacy = 0;
  const codec = {
    parseButtonConfig: parseRawButtonConfig,
    parseSubpageConfig: (value: string) => parseRawSubpageConfig(value, code => code),
    buildSubpageGrid: (page: any) => Object.assign(page, buildSubpageGrid(page, 9, 3)),
    buttonConfigNeedsMigration: () => false,
    applySubpageRaw: () => { ++appliedLegacy; },
  } as any;
  const events = createAppConfigEventsFeature({} as any, codec, layout, { schedule() {}, clearDeferred() {} });
  state.configLocked = false;
  events.applySubpageConfigStateEvent(1, "main", "~B,1|,light.old,Old");
  check(appliedLegacy === 1, "legacy-only loading still accepts chunks");
  // Depth-one native writers cache their successful saves too, but do not
  // load the editor through the graph snapshot path.
  state.nativePanelDocument = { deviceProfile: "test", buttons: {}, subpages: {}, settings: {} };
  events.applySubpageConfigStateEvent(1, "ext", "old-reader extension");
  check(appliedLegacy === 2, "a depth-one native save cannot disable legacy state updates");
  const editor = createNativePageEditorFeature({ native: { client: { acceptSnapshot() {} }, writableNavigationDepth: () => 1 } as any,
    codec, layout, runtime, document: { activeElement: null } as any,
    shell: { showBanner() {}, syncConfigLockUi() {} }, closeSettings() {}, render() {}, canApply: () => true });
  const raw = "~B,1|,light.current," + "Current ".repeat(45);
  editor.replace({ deviceProfile: "test", buttons: { 1: ";Folder;;;;;subpage" }, subpages: { 1: raw }, settings: {} });
  const complete = JSON.stringify(state.subpages);
  for (const [key, value] of [["ext", ""], ["ext2", "stale fragment"], ["main", "~B,1|,light.old,Old"]])
    events.applySubpageConfigStateEvent(1, key!, value!);
  events.applyButtonConfigStateEvent(1, ";Obsolete;;;;;subpage");
  check(JSON.stringify(state.subpages) === complete && appliedLegacy === 2, "late empty/nonempty legacy chunks cannot replace complete native pages");
  check(state.buttons[0]!.label === "Folder", "late root events cannot overwrite the native transaction");

  const classes: Record<string, boolean> = {}, attributes: Record<string, string> = {}, apply = { disabled: false, textContent: "" };
  runtime.els = { root: { classList: { toggle(key: string, enabled: boolean) { classes[key] = enabled; } }, querySelectorAll: () => [apply] },
    previewMain: { setAttribute(key: string, value: string) { attributes[key] = value; } } };
  const shell = createControlsShellFeature(runtime, { state, document: {}, updatePreviewHint() {} } as any);
  state.nativeConfigReadOnly = true; state.nativeConfigLoading = false;
  shell.syncConfigLockUi();
  check(shell.isConfigLocked() && apply.disabled, "read-only graphs still forbid configuration writes");
  check(!classes["sp-config-locked"] && attributes["aria-disabled"] === "false", "read-only preview remains reachable");
  const listeners: Record<string, (event: any) => void> = {};
  runtime.els.previewMain.addEventListener = (name: string, callback: (event: any) => void) => { listeners[name] = callback; };
  let entered = 0, exited = 0;
  createPreviewInteractionsFeature({ runtime, shell, window: {}, cardEditorDraft: {}, configPersistence: {}, layout, imageOptions: {},
    codec: { enterChildSubpage: () => { ++entered; }, exitSubpage: () => { ++exited; } }, entityState: {},
    grid: { ctx: () => ({ buttons: state.buttons }) }, selection: {}, placement: {}, contextMenu: {}, requestApi: {},
  } as any).setup();
  const click = (slot: number) => listeners.click!({ preventDefault() {}, stopPropagation() {},
    target: { closest: () => ({ getAttribute: () => String(slot) }) } });
  click(1); click(-2); click(2);
  check(entered === 1 && exited === 1, "read-only preview permits folder and Back navigation only");
  state.configLocked = true; shell.syncConfigLockUi(); click(1);
  check(entered === 1 && classes["sp-config-locked"], "disconnect blocks even read-only navigation");
  state.configLocked = false; state.nativeConfigLoading = true; shell.syncConfigLockUi(); click(-2);
  check(exited === 1 && classes["sp-config-locked"], "incomplete loading blocks navigation");
  state.nativeConfigLoading = false; state.nativePanelDocument = null; shell.syncConfigLockUi(); click(1);
  check(entered === 1 && classes["sp-config-locked"], "failed native loading cannot expose a partial preview");
}
