import { normalizePanelName, panelHostname, readIdentityBackup } from "../../src/webserver/model/panel_identity";
import { createBackupExportController } from "../../src/webserver/features/backup_export_controller";
import { createAppTitleFeature } from "../../src/webserver/application/app_title";
import { createPanelIdentityFeature } from "../../src/webserver/application/panel_identity";

function assert(value: unknown, message: string): asserts value { if (!value) throw new Error(message); }
export async function runPanelIdentityTests() {
  assert(normalizePanelName("  Kitchen\t") === "Kitchen", "trim name");
  for (const value of ["x".repeat(121), "bad\nname", "a\0b", "a\u0085b", "\ud800"]) {
    let rejected = false;
    try { normalizePanelName(value); } catch { rejected = true; }
    assert(rejected, "reject invalid name " + JSON.stringify(value));
  }
  assert(normalizePanelName("界".repeat(40)).length === 40, "accept 120 UTF-8 bytes");
  assert(panelHostname("Kitchen", "b2c3") === "kitchen-b2c3", "hostname");
  assert(panelHostname("Küche", "cdef") === "k-che-cdef", "UTF-8 slug");
  assert(panelHostname("東京", "cdef") === "panel-cdef", "non-ASCII fallback");
  assert(panelHostname("K", "cdef") === "panel-cdef", "ASCII casing matches firmware");
  assert(panelHostname("very long hallway panel", "cdef").length <= 31, "hostname limit");
  assert(panelHostname("Kitchen", "1111") !== panelHostname("Kitchen", "2222"), "destination suffix");
  assert(readIdentityBackup(undefined) === undefined, "old backups");
  assert(readIdentityBackup({ version: 2 }) === undefined, "future metadata ignored");
  const identity = { version: 1 as const, name: "Kitchen", hostname: "kitchen-b2c3", mac_suffix: "b2c3" };
  assert(readIdentityBackup(identity)?.name === "Kitchen", "metadata");
  const exporter = createBackupExportController({ serializeButtonConfig: () => "", serializeSubpageConfig: () => "" });
  assert(exporter.fileName("4 inches", new Date(2026, 8, 11), identity) === "espcontrol-4-inch-kitchen-b2c3-2026-09-11.json", "named backup");
  assert(exporter.fileName("4 inches", new Date(2026, 8, 11)) === "espcontrol-4-inch-2026-09-11.json", "legacy filename");
  const document = { title: "" } as Document;
  let custom = "Kitchen";
  const title = createAppTitleFeature({ document, panelName: () => custom, eventStreamEnabled: () => false,
    eventSourceAvailable: () => false, createEventSource: () => { throw new Error("unused"); } });
  title.handleWebServerPingEvent({ data: '{"title":"Old model name"}' });
  assert(document.title === "EspControl — Kitchen", "server events cannot overwrite custom title");
  custom = "";
  title.handleWebServerPingEvent({ data: '{"title":"Original"}' });
  assert(String(document.title) === "Original", "default title restored");
  let requests = 0, restarted = false;
  const feature = createPanelIdentityFeature({ document,
    fetch: (async (_url: unknown, init: RequestInit) => {
      requests++;
      assert(init.credentials === "include", "authenticated requests");
      return new Response("failed", { status: 500 });
    }) as typeof fetch,
    changed: () => {}, beforeSave: async () => {}, restart: async () => { restarted = true; },
    makeCard: () => { throw new Error("unused"); },
  });
  let failed = false;
  try { await feature.saveAndRestart("Kitchen"); } catch { failed = true; }
  assert(failed && !restarted && feature.current() === null, "failed write cannot restart or publish name");
  assert(requests === 1, "one request on save");
  const old = createPanelIdentityFeature({ document, fetch: (async () => new Response("", { status: 404 })) as typeof fetch,
    changed: () => {}, beforeSave: async () => {}, restart: async () => {}, makeCard: () => { throw new Error("unused"); } });
  assert(await old.load() === null, "old firmware has no identity feature");
  assert(await old.chooseRestoreName({}) === undefined, "old backup preserves identity");
}
