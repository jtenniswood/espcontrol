import {
  createNativePanelConfigClient,
  updateNativePanelConfigDocument,
  NATIVE_BACKUP_UPDATE_REQUIRED_MESSAGE,
  type NativePanelConfigRequest,
  type NativePanelConfigResponse,
} from "../../src/webserver/features/native_panel_config";
import { createNativePanelConfigMigrationController } from "../../src/webserver/application/native_panel_config_migration";
import { NativePanelConfigController } from "../../src/webserver/controllers/native_panel_config_controller";
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

export async function runNativePanelConfigBackupTests(): Promise<void> {
  for (const slots of [6, 9, 15, 20]) for (const support of [undefined, false, "true", true]) {
    for (const collection of ["buttons", "subpages"] as const) for (const value of ["", "retained"]) {
      const original = decodePanelConfig(document);
      const incoming: PanelConfigDocument = {
        ...original, [collection]: { ...original[collection], [slots + 1]: value },
      };
      const before = JSON.stringify(incoming);
      let current = original, puts = 0;
      const banners: string[] = [];
      const target = new NativePanelConfigController({
        fetch: async (path, request) => {
          if (path.endsWith("capabilities")) return {
            ...response(200), json: async () => ({ configuration: {
              read: true, write: true, document_versions: [1], preserves_inactive_records: support,
            } }),
          };
          if (request?.method === "PUT") {
            ++puts;current = decodePanelConfig(request.body!);return response(204);
          }
          return response(200, encodePanelConfig(current), '"1"');
        },
        deviceProfile: () => "panel-a", slotCount: () => slots,
        entityName: name => name, entityNameForSlot: (name, slot) => `${name}_${slot}`,
        normalizeHexColor: value => value, showBanner: message => { banners.push(message); },
        delay: callback => { callback(); },
      });
      equal(await target.writeDocument(incoming), support === true ? "saved" : "firmware-update-required",
        "retained backup records require explicit target support, independently of wire version");
      equal(puts, support === true ? 1 : 0, "unsupported backups never reach PUT, including empty records");
      deepEqual(current, support === true ? incoming : original, "rejection preserves the target's saved document");
      equal(JSON.stringify(incoming), before, "admission never trims the backup");
      if (support !== true) {
        deepEqual(banners, [NATIVE_BACKUP_UPDATE_REQUIRED_MESSAGE], "restore refusal explains the required update");
        equal(await target.writeText("button_order", "1d"), "saved", "a refused restore does not block later ordinary edits");
        equal(puts, 1, "ordinary in-profile edits retain v1 support");
        const withinProfile = { ...current, buttons: { ...current.buttons, [slots]: "" } };
        equal(await target.writeDocument(withinProfile), "saved", "in-profile backups remain supported without the new capability");
        deepEqual(current, withinProfile, "the final configured slot, including an empty record, remains writable");
      }
    }
  }
  const completeBytes = encodePanelConfig({
    deviceProfile: "panel-a", buttons: { 9: "inactive" }, subpages: { 9: "folder" },
    settings: { future_setting: "温度🌡️".repeat(100) },
  });
  const client = createNativePanelConfigClient(async (path) => path.endsWith("capabilities")
    ? response(200) : response(200, completeBytes, '"12"'));
  deepEqual(Array.from((await client.readForBackup("panel-a"))!), Array.from(completeBytes),
    "backup reads preserve native-only settings and inactive records byte for byte");

  const rejects = async (operation: Promise<unknown>, message: string): Promise<void> => {
    let rejected = false;
    try { await operation; } catch { rejected = true; }
    equal(rejected, true, message);
  };
  await rejects(client.readForBackup("different-panel"), "backup reads reject a changed device profile");
  for (const status of [401, 403, 404, 503, 500]) {
    const failedRead = createNativePanelConfigClient(async (path) => path.endsWith("capabilities")
      ? response(200) : response(status));
    await rejects(failedRead.readForBackup("panel-a"), `${status} config reads cannot become partial backups`);
    const failedDiscovery = createNativePanelConfigClient(async () => response(status));
    await rejects(failedDiscovery.readForBackup("panel-a"), `${status} discovery cannot become partial backups`);
  }
  const corruptRead = createNativePanelConfigClient(async (path) => path.endsWith("capabilities")
    ? response(200) : response(200, new Uint8Array([1, 2, 3])));
  await rejects(corruptRead.readForBackup("panel-a"), "corrupt native reads cannot become partial backups");
  const legacy = createNativePanelConfigClient(async () => ({
    ...response(200), json: async () => ({ configuration: { read: false, write: false, document_versions: [] } }),
  }));
  equal(await legacy.readForBackup("panel-a"), null, "confirmed older firmware retains portable backup export");

  let current = decodePanelConfig(document);
  let releaseRead: (() => void) | undefined;
  let blockRead = false;
  let failRead = false;
  const events: string[] = [];
  const controller = new NativePanelConfigController({
    fetch: async (path, request) => {
      if (path.endsWith("capabilities")) return response(200);
      if (request?.method === "PUT") {
        current = decodePanelConfig(request.body!);
        events.push(`save:${current.settings.button_order}`);
        return response(204);
      }
      if (blockRead) {
        blockRead = false;
        events.push("snapshot-start");
        await new Promise<void>((resolve) => { releaseRead = resolve; });
        events.push("snapshot-end");
      }
      return response(failRead ? 503 : 200, encodePanelConfig(current), '"12"');
    },
    deviceProfile: () => "panel-a", slotCount: () => 9,
    entityName: (name) => name, entityNameForSlot: (name, slot) => `${name}_${slot}`,
    normalizeHexColor: (value) => value, showBanner: () => undefined,
    delay: (callback) => { callback(); },
  });
  const first = controller.writeText("button_order", "1d");
  const snapshot = controller.readForBackup();
  // The snapshot is queued before the later edit, even though neither has run yet.
  await first;
  blockRead = true;
  const later = controller.writeText("button_order", "1w");
  for (let i = 0; i < 20 && !releaseRead; i += 1) await Promise.resolve();
  equal(typeof releaseRead, "function", "the snapshot reaches the transport after the preceding save");
  deepEqual(events, ["save:1d", "snapshot-start"], "later edits wait for the snapshot read");
  releaseRead!();
  equal(decodePanelConfig((await snapshot)!).settings.button_order, "1d", "the snapshot includes the preceding saved change");
  equal(await later, "saved", "later edits resume after the snapshot");
  deepEqual(events, ["save:1d", "snapshot-start", "snapshot-end", "save:1w"], "snapshot and saves remain ordered");
  failRead = true;
  await rejects(controller.readForBackup(), "a failed snapshot is reported");
  failRead = false;
  equal(await controller.writeText("button_order", "1"), "saved", "a failed snapshot does not poison later saves");
  controller.saveQueue = Promise.resolve("conflict");
  await rejects(controller.readForBackup(), "a failed preceding save blocks an incomplete backup");
  equal(await controller.saveQueue, "conflict", "a backup attempt preserves the failed save outcome");
  controller.saveQueue = Promise.resolve("mirror-failed");
  equal(decodePanelConfig((await controller.readForBackup())!).settings.button_order, "1",
    "a durable native save remains available for backup even if its legacy mirror failed");
}

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

  const authenticationRequiredClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(403);
    return response(200, document, "\"1\"");
  });
  equal(await authenticationRequiredClient.save((current) => current), "authentication-required",
    "a rejected authenticated write is reported clearly");

  const protectedReadClient = createNativePanelConfigClient(async (path) => {
    if (path === "/api/v1/capabilities") return response(200);
    return response(403);
  });
  equal(await protectedReadClient.save((current) => current), "authentication-required",
    "a protected configuration read is not treated as a generic save failure");

  const challengedWriteClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(401);
    return response(200, document, "\"1\"");
  });
  equal(await challengedWriteClient.save((current) => current), "authentication-required",
    "an authentication challenge while writing is reported clearly");

  const challengedReadClient = createNativePanelConfigClient(async (path) => {
    if (path === "/api/v1/capabilities") return response(200);
    return response(401);
  });
  equal(await challengedReadClient.save((current) => current), "authentication-required",
    "an authentication challenge while reading is reported clearly");

  const runtimeFailureClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(500);
    return response(200, document, "\"1\"");
  });
  equal(await runtimeFailureClient.save((current) => current), "failed",
    "a runtime application failure is not reported as a successful restore");

  const legacyClient = createNativePanelConfigClient(async () => ({
    ...response(200),
    json: async () => ({ configuration: { read: false, write: false, document_versions: [] } }),
  }));
  equal(await legacyClient.save((current) => current), "unsupported", "legacy firmware stays on the entity path");
  equal(legacyClient.confirmedUnsupported(), true,
    "a valid capabilities response can confirm that native configuration is unsupported");

  let confirmedLegacyRequests = 0;
  const confirmedLegacyClient = createNativePanelConfigClient(async () => {
    confirmedLegacyRequests += 1;
    if (confirmedLegacyRequests > 1) throw new Error("confirmed legacy discovery should be reused");
    return {
      ...response(200),
      json: async () => ({ configuration: { read: false, write: false, document_versions: [] } }),
    };
  });

  let discoveryAttempts = 0;
  const recoveringDiscoveryClient = createNativePanelConfigClient(async (path, request) => {
    if (path === "/api/v1/capabilities" && discoveryAttempts++ === 0)
      throw new Error("temporary network failure");
    if (path === "/api/v1/capabilities") return response(200);
    if (request?.method === "PUT") return response(204);
    return response(200, document, "\"7a\"");
  });
  equal(await recoveringDiscoveryClient.discover(), false,
    "a failed capabilities request does not claim native support");
  equal(recoveringDiscoveryClient.confirmedUnsupported(), false,
    "a failed capabilities request is not mistaken for older firmware");
  equal(await recoveringDiscoveryClient.save((current) => current), "saved",
    "native configuration discovery can recover after a temporary failure");

  const malformedCapabilitiesClient = createNativePanelConfigClient(async () => ({
    ...response(200),
    json: async () => ({}),
  }));
  equal(await malformedCapabilitiesClient.discover(), false,
    "a capabilities object without a configuration contract is rejected");
  equal(malformedCapabilitiesClient.confirmedUnsupported(), false,
    "a malformed capabilities object is not mistaken for older firmware");

  const malformedVersionClient = createNativePanelConfigClient(async () => ({
    ...response(200),
    json: async () => ({ configuration: { read: true, write: true, document_versions: ["1"] } }),
  }));
  equal(await malformedVersionClient.discover(), false,
    "a non-numeric document version is rejected");
  equal(malformedVersionClient.confirmedUnsupported(), false,
    "a malformed version list is not mistaken for older firmware");

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
    const confirmedLegacyController = new NativePanelConfigController({
      fetch: null,
      deviceProfile: () => "panel-a",
      slotCount: () => 2,
      entityName: (name: string) => name,
      entityNameForSlot: (name: string, slot: number) => `${name}_${slot}`,
      normalizeHexColor: (value: string) => value,
      showBanner: () => undefined,
      delay: (callback: () => void) => { callback(); return 0 as any; },
    });
    confirmedLegacyController.client = confirmedLegacyClient;
    equal(await confirmedLegacyController.begin(), false,
      "controller setup records a confirmed older-firmware response");
    equal(await confirmedLegacyController.writeDocument(decodePanelConfig(document)), "legacy-fallback",
      "a confirmed older-firmware result is reused without another capabilities request");
    equal(confirmedLegacyRequests, 1,
      "confirmed older firmware is discovered only once");

    let rejectInitialDiscovery: ((reason?: unknown) => void) | undefined;
    let recoveringControllerCapabilityRequests = 0;
    setGlobal("fetch", async (path: string, request?: NativePanelConfigRequest) => {
      if (path === "/api/v1/capabilities" && recoveringControllerCapabilityRequests++ === 0) {
        return new Promise<NativePanelConfigResponse>((_resolve, reject) => {
          rejectInitialDiscovery = reject;
        });
      }
      if (path === "/api/v1/capabilities") return response(200);
      if (request?.method === "PUT") return response(204);
      return response(200, document, "\"8a\"");
    });
    const recoveringController = createNativePanelConfigMigrationController({
      deviceProfile: () => "panel-a",
      slotCount: () => 2,
      entityName: (name: string) => name,
      entityNameForSlot: (name: string, slot: number) => `${name}_${slot}`,
      normalizeHexColor: (value: string) => value,
      showBanner: () => undefined,
      delay: (callback: () => void) => { callback(); return 0 as any; },
    });
    const failedDiscoveryRestore = recoveringController.writeDocument(decodePanelConfig(document));
    if (!failedDiscoveryRestore) throw new Error("failed discovery restore was not queued");
    if (!rejectInitialDiscovery) throw new Error("failing capability discovery did not start");
    rejectInitialDiscovery(new Error("temporary network failure"));
    equal(await failedDiscoveryRestore, "failed",
      "a discovery error reports a failed native restore instead of using readable legacy fields");
    equal(recoveringController.legacyFallback, false,
      "a discovery error does not permanently latch older-firmware mode");
    equal(await recoveringController.writeDocument(decodePanelConfig(document)), "saved",
      "a later native restore recovers after the capability request succeeds");

    let resolveInitialDiscovery: ((value: NativePanelConfigResponse) => void) | undefined;
    let initialDiscoverySaves = 0;
    setGlobal("fetch", async (path: string, request?: NativePanelConfigRequest) => {
      if (path === "/api/v1/capabilities") {
        return new Promise<NativePanelConfigResponse>((resolve) => {
          resolveInitialDiscovery = resolve;
        });
      }
      if (request?.method === "PUT") {
        initialDiscoverySaves += 1;
        return response(204);
      }
      return response(200, document, "\"9a\"");
    });
    const discoveringController = createNativePanelConfigMigrationController({
      deviceProfile: () => "panel-a",
      slotCount: () => 2,
      entityName: (name: string) => name,
      entityNameForSlot: (name: string, slot: number) => `${name}_${slot}`,
      normalizeHexColor: (value: string) => value,
      showBanner: () => undefined,
      delay: (callback: () => void) => { callback(); return 0 as any; },
    });
    const initialDiscoveryRestore = discoveringController.writeDocument(decodePanelConfig(document));
    if (!initialDiscoveryRestore) throw new Error("initial discovery restore was not queued");
    equal(initialDiscoverySaves, 0,
      "a restore does not fall back or write before initial capability discovery finishes");
    if (!resolveInitialDiscovery) throw new Error("initial capability discovery did not start");
    resolveInitialDiscovery(response(200));
    equal(await initialDiscoveryRestore, "saved",
      "a backup restore queued during initial discovery uses native configuration");
    equal(initialDiscoverySaves, 1,
      "a restore queued during initial discovery writes the native document once");

    let capabilityRequests = 0;
    let nativeSaves = 0;
    const savedDocuments: PanelConfigDocument[] = [];
    setGlobal("fetch", async (path: string, request?: NativePanelConfigRequest) => {
      if (path === "/api/v1/capabilities") {
        capabilityRequests += 1;
        return capabilityRequests === 1
          ? { ...response(404), json: async () => ({}) }
          : response(200);
      }
      if (request?.method === "PUT") {
        nativeSaves += 1;
        savedDocuments.push(decodePanelConfig(new Uint8Array(request.body!)));
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
    const deferredBackupDocument = decodePanelConfig(document);
    const deferredBackup = {
      ...deferredBackupDocument,
      settings: { ...deferredBackupDocument.settings, future_native_setting: "preserved" },
    };
    equal(await controller.writeDocument(deferredBackup), "saved",
      "a backup restore waits for deferred native setup instead of losing native-only settings");
    equal(savedDocuments[0]?.settings.future_native_setting, "preserved",
      "deferred backup restore writes the exact native document");
    equal(await controller.writeText("button_order", "1,2"), "saved",
      "an edit waits for deferred native setup instead of writing a stale legacy shadow");
    equal(await controller.writeButtonAndOrder(2, "combined-button", "2,1"), "saved",
      "a card edit and its grid position save in one native document update");
    equal(savedDocuments[2]?.buttons[2], "combined-button",
      "the combined native save includes the edited card");
    equal(savedDocuments[2]?.settings.button_order, "2,1",
      "the combined native save includes the edited grid order");
    equal(capabilityRequests, 2,
      "a deferred edit retries capability discovery after the temporary 404");
    equal(nativeSaves, 3,
      "deferred backup restore and edits are each written once the native endpoint is ready");

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

export async function runNativeReadinessTests(): Promise<void> {
  const base = {
    deviceProfile: "panel-a", buttons: { 1: "original", 9: "inactive" }, subpages: {},
    settings: { button_order: "1", future_setting: "preserved" },
  };
  const makeController = (fetch: (path: string, request?: NativePanelConfigRequest) => Promise<NativePanelConfigResponse>, banners: string[] = []) => {
    const controller = new NativePanelConfigController({
      fetch, deviceProfile: () => "panel-a", slotCount: () => 9,
      entityName: (name) => name, entityNameForSlot: (name, slot) => `${name}_${slot}`,
      normalizeHexColor: (value) => value, showBanner: (message) => { banners.push(message); },
      delay: (callback) => { callback(); },
    });
    controller.maxDiscoveryRetries = 0;
    return controller;
  };
  let ready = false;
  const saved: PanelConfigDocument[] = [];
  const startup = makeController(async (path, request) => {
    if (path.endsWith("capabilities")) return response(ready ? 200 : 503);
    if (request?.method === "PUT") { saved.push(decodePanelConfig(request.body!)); return response(204); }
    return response(200, encodePanelConfig(base), '"1"');
  });
  equal(await startup.writeText("button_order", "1d"), "failed", "capped startup 503 fails without a legacy write");
  equal(startup.legacyFallback, false, "busy startup does not latch older-firmware mode");
  equal(saved.length, 0, "startup failure does not write native data either");
  ready = true;
  equal(await startup.writeText("button_order", "1d"), "saved", "native edits recover after startup readiness returns");
  equal(saved[0]?.settings.future_setting, "preserved", "startup recovery preserves native-only settings");
  equal(saved[0]?.buttons[9], "inactive", "startup recovery preserves inactive cards");

  for (const status of [404, 503]) {
    let restarting = false;
    const known = makeController(async (path) => {
      if (path.endsWith("capabilities")) return response(restarting ? status : 200);
      return response(restarting ? status : 200, encodePanelConfig(base), '"1"');
    });
    equal(await known.begin(), true, "native support is established before restart");
    restarting = true;
    equal(await known.writeText("button_order", "1d"), "failed", `known-native ${status} cannot become legacy editing`);
    equal(known.legacyFallback, false, "known-native restart failure keeps its native contract");
  }

  for (const failAt of ["GET", "PUT"]) {
    const banners: string[] = [];
    const busy = makeController(async (path, request) => {
      if (path.endsWith("capabilities")) return response(200);
      if ((request?.method ?? "GET") === failAt) return response(503);
      return response(200, encodePanelConfig(base), '"1"');
    }, banners);
    equal(await busy.writeText("button_order", "1d"), "failed", `${failAt} resource 503 is a reported save failure`);
    equal(busy.legacyFallback, false, "resource pressure never invokes legacy editing");
    equal(banners.length, 1, "an exhausted native save reports one visible error");
    equal(banners[0]?.includes("Could not save"), true, "resource pressure is not silently reported as unsupported");
  }

  for (const configuration of [
    { read: true, write: true, document_versions: [3] },
    { read: true, write: false, document_versions: [1] },
    { read: false, write: false, document_versions: [2] },
    { read: false, write: false, document_versions: [1, 2] },
  ]) {
    const fetch = async () => ({ ...response(200), json: async () => ({ configuration }) });
    const newer = makeController(fetch);
    equal(await newer.writeText("button_order", "1d"), "failed", "an incompatible or readonly native contract cannot become legacy editing");
    equal(newer.client?.confirmedUnsupported(), false, "native version/write incompatibility is distinct from legacy-only firmware");
    const client = createNativePanelConfigClient(fetch);
    let rejected = false;
    try { await client.readForBackup("panel-a"); } catch { rejected = true; }
    equal(rejected, true, "an unreadable native contract cannot export a partial legacy backup");
  }

  const oldMissing = makeController(async () => response(404));
  equal(await oldMissing.readForBackup(), null, "never-native missing endpoints still permit the capped legacy backup path");
  equal(oldMissing.legacyFallback, true, "the missing old endpoint retains the existing compatibility route");
  for (const document_versions of [[], [1]]) {
    const legacy = makeController(async () => ({
      ...response(200), json: async () => ({ configuration: { read: false, write: false, document_versions } }),
    }));
    equal(await legacy.readForBackup(), null, "explicit legacy-only flags allow portable backups");
    equal(await legacy.writeText("button_order", "1d"), "legacy-fallback", "explicit legacy-only flags preserve old editing");
  }
}
