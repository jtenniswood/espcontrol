import {
  createNativePanelConfigClient,
  updateNativePanelConfigDocument,
  type NativePanelConfigRequest,
  type NativePanelConfigResponse,
} from "../../src/webserver/features/native_panel_config";
import { createNativePanelConfigMigrationController } from "../../src/webserver/application/native_panel_config_migration";
import { decodePanelConfig, encodePanelConfig, type PanelConfigDocument } from "../../src/webserver/model";

interface MigrationFixture {
  readonly scenarios: {
    readonly downgrade: {
      readonly native_document: PanelConfigDocument;
      readonly legacy_entities: Record<string, string>;
    };
    readonly partial_migration: {
      readonly native_document: PanelConfigDocument;
      readonly legacy_entity_update: { readonly collection: "buttons"; readonly key: number; readonly value: string };
      readonly expected_document: PanelConfigDocument;
    };
    readonly failed_legacy_mirror: {
      readonly document: PanelConfigDocument;
      readonly expected_result: "mirror-failed";
    };
  };
}

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error(`${message}: expected ${JSON.stringify(expected)}, received ${JSON.stringify(actual)}`);
  }
}

const document = encodePanelConfig({
  deviceProfile: "panel-a",
  buttons: { 1: "light.kitchen" },
  subpages: {},
  settings: { button_order: "1" },
});

function response(
  status: number,
  body: Uint8Array = new Uint8Array(),
  etag: string | null = null,
): NativePanelConfigResponse {
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: { get: (name: string) => name === "ETag" ? etag : null },
    json: async () => ({ configuration: { read: true, write: true, document_versions: [1] } }),
    arrayBuffer: async () => new Uint8Array(body).buffer as ArrayBuffer,
  };
}

export async function runNativePanelConfigTests(migrationFixture?: MigrationFixture): Promise<void> {
  const partialDocument = updateNativePanelConfigDocument({
    deviceProfile: "panel-a",
    buttons: { 1: "old-button", 2: "preserved-button" },
    subpages: { 2: "preserved-subpage" },
    settings: { button_order: "1,2", future_setting: "preserved" },
  }, "panel-a", "buttons", 1, "new-button");
  deepEqual(partialDocument, {
    deviceProfile: "panel-a",
    buttons: { 1: "new-button", 2: "preserved-button" },
    subpages: { 2: "preserved-subpage" },
    settings: { button_order: "1,2", future_setting: "preserved" },
  }, "one changed record preserves the configuration still arriving from the device");
  deepEqual(updateNativePanelConfigDocument(partialDocument, "panel-a", "buttons", 1, "").buttons,
    { 2: "preserved-button" }, "an empty record clears only that record");
  deepEqual(updateNativePanelConfigDocument(partialDocument, "panel-a", "settings", "button_on_color", "0088FF").settings,
    { button_order: "1,2", future_setting: "preserved", button_on_color: "0088FF" },
    "appearance settings use the same native document without replacing future settings");

  const requests: Array<{ path: string; request?: NativePanelConfigRequest }> = [];
  const client = createNativePanelConfigClient(async (path, request) => {
    requests.push(request ? { path, request } : { path });
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(204);
    return response(200, document, "\"7\"");
  });
  equal(await client.discover(), true, "native capabilities are detected");
  equal(await client.save((current) => ({ ...current, settings: { ...current.settings, button_order: "1d" } })), "saved", "guarded native save succeeds");
  const put = requests.find((entry) => entry.request?.method === "PUT");
  equal(put?.request?.headers?.["If-Match"], "\"7\"", "native save uses the document generation");

  let retries = 0;
  const retryClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(retries++ === 0 ? 409 : 204);
    return response(200, document, `\"${retries + 1}\"`);
  });
  equal(await retryClient.save((current) => current), "saved", "a stale generation retries once");

  const mirrorFailureClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(202);
    return response(200, document, "\"1\"");
  });
  equal(await mirrorFailureClient.save((current) => current), "mirror-failed", "a failed legacy mirror is reported");

  const legacyClient = createNativePanelConfigClient(async () => ({
    ...response(200),
    json: async () => ({ configuration: { read: false, write: false, document_versions: [] } }),
  }));
  equal(await legacyClient.save((current) => current), "unsupported", "legacy firmware stays on the entity path");

  let nativeInitializationComplete = false;
  const reconnectingClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") {
      return nativeInitializationComplete
        ? response(200)
        : { ...response(503), json: async () => ({}) };
    }
    if (request?.method === "PUT") return response(204);
    return response(200, document, "\"8\"");
  });
  equal(await reconnectingClient.discover(), false,
    "an editor reconnecting during initialization treats the retryable 503 as temporary");
  equal(reconnectingClient.retryable(), true,
    "a missing capabilities endpoint is retried after deferred initialization");
  nativeInitializationComplete = true;
  equal(await reconnectingClient.save((current) => current), "saved",
    "a later save rediscovers native configuration after initialization completes");

  let panelRestarted = false;
  const restartedClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(204);
    return panelRestarted ? { ...response(404), json: async () => ({}) } : response(200, document, "\"9\"");
  });
  equal(await restartedClient.discover(), true, "native support is available before a panel restart");
  panelRestarted = true;
  equal(await restartedClient.save((current) => current), "unsupported",
    "a missing configuration endpoint resets cached native support after restart");
  equal(restartedClient.supported(), false,
    "a restarted panel no longer uses stale native capability state");
  equal(restartedClient.retryable(), true,
    "a restarted panel retries native discovery when its endpoint is temporarily missing");

  const savedDescriptors = new Map<string, PropertyDescriptor | undefined>();
  const saveDescriptor = (name: string): void => {
    if (!savedDescriptors.has(name))
      savedDescriptors.set(name, Object.getOwnPropertyDescriptor(globalThis, name));
  };
  const setGlobal = (name: string, value: unknown): void => {
    saveDescriptor(name);
    Object.defineProperty(globalThis, name, {
      configurable: true,
      writable: true,
      value,
    });
  };
  try {
    let capabilityRequests = 0;
    let nativeSaves = 0;
    setGlobal("fetch", async (path: string, request?: NativePanelConfigRequest) => {
      if (path === "/api/v1/capabilities") {
        capabilityRequests += 1;
        return capabilityRequests === 1
          ? { ...response(404), json: async () => ({}) }
          : response(200);
      }
      if (request?.method === "PUT") {
        nativeSaves += 1;
        return response(204);
      }
      return response(200, document, "\"10\"");
    });
    setGlobal("setTimeout", (callback: () => void) => { callback(); return 0; });
    const controller = createNativePanelConfigMigrationController({
      deviceProfile: () => "panel-a",
      slotCount: () => 2,
      entityName: (name: string) => name,
      entityNameForSlot: (name: string, slot: number) => `${name}_${slot}`,
      normalizeHexColor: (value: string) => value,
      showBanner: () => undefined,
      delay: (callback: () => void) => { callback(); return 0 as any; },
    });
    await Promise.resolve();
    await Promise.resolve();
    equal(await controller.writeText("button_order", "1,2"), "saved",
      "an edit waits for deferred native setup instead of writing a stale legacy shadow");
    equal(capabilityRequests, 2,
      "a deferred edit retries capability discovery after the temporary 404");
    equal(nativeSaves, 1,
      "a deferred edit is written once the native configuration endpoint is ready");

    controller.maxDiscoveryRetries = 0;
    let permanentlyMissingCapabilityRequests = 0;
    const permanentlyMissingClient = createNativePanelConfigClient(async (path) => {
      if (path === "/api/v1/capabilities") {
        permanentlyMissingCapabilityRequests += 1;
        return { ...response(404), json: async () => ({}) };
      }
      return response(500);
    });
    await permanentlyMissingClient.discover();
    controller.client = permanentlyMissingClient;
    equal(await controller.writeText("button_order", "2,1"), "legacy-fallback",
      "a permanently absent capabilities endpoint releases the pending edit to the legacy route");
    equal(await controller.writeText("button_order", "1,2"), "legacy-fallback",
      "later queued edits reuse the older-firmware fallback");
    equal(permanentlyMissingCapabilityRequests, 2,
      "the capped older-firmware fallback avoids rediscovering native configuration for each save");
  } finally {
    for (const [name, descriptor] of savedDescriptors) {
      if (descriptor) Object.defineProperty(globalThis, name, descriptor);
      else delete (globalThis as Record<string, unknown>)[name];
    }
  }

  if (!migrationFixture) return;
  const downgrade = migrationFixture.scenarios.downgrade;
  const downgradedDocument = decodePanelConfig(encodePanelConfig(downgrade.native_document));
  equal(downgradedDocument.buttons[1], downgrade.legacy_entities.button_config_1,
    "downgrade fixture retains the first button's legacy value");
  equal(downgradedDocument.subpages[1], downgrade.legacy_entities.button_subpage_config_1,
    "downgrade fixture retains the first subpage's legacy value");
  equal(downgradedDocument.settings.button_on_color, downgrade.legacy_entities.button_on_color,
    "downgrade fixture retains the active colour");
  equal(downgradedDocument.settings.button_order, downgrade.legacy_entities.button_order,
    "downgrade fixture retains button order");

  const partialMigration = migrationFixture.scenarios.partial_migration;
  deepEqual(updateNativePanelConfigDocument(
    partialMigration.native_document,
    partialMigration.native_document.deviceProfile,
    partialMigration.legacy_entity_update.collection,
    partialMigration.legacy_entity_update.key,
    partialMigration.legacy_entity_update.value,
  ), partialMigration.expected_document, "partial migration preserves native records that legacy firmware cannot see");

  const mirrorScenario = migrationFixture.scenarios.failed_legacy_mirror;
  const mirrorScenarioDocument = encodePanelConfig(mirrorScenario.document);
  const fixtureMirrorClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(202);
    return response(200, mirrorScenarioDocument, "\"1\"");
  });
  equal(await fixtureMirrorClient.save((current) => current), mirrorScenario.expected_result,
    "failed legacy mirrors keep the native document durable but block a downgrade claim");
}
