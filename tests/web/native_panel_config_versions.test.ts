import { createNativePanelConfigClient, type NativePanelConfigRequest, type NativePanelConfigResponse } from "../../src/webserver/features/native_panel_config";
import { decodePanelConfig, encodePanelConfig, type PanelConfigDocument } from "../../src/webserver/model/panel_config";
import { convertNativeV1ToV2, convertNativeV2ToV1 } from "../../src/webserver/model/panel_config_conversion";
import { decodeNativeV2, encodeNativeV2, NativeV2Kind, type NativeV2Record } from "../../src/webserver/model/panel_config_v2";

function check(value: unknown, message: string): asserts value { if (!value) throw new Error(message); }
function same(actual: unknown, expected: unknown, message: string): void {
  check(JSON.stringify(actual) === JSON.stringify(expected), `${message}: ${JSON.stringify(actual)}`);
}
const initial: PanelConfigDocument = {
  deviceProfile: "panel-a", buttons: { 1: "light.one;Original", 32: "unknown.future;Dormant" },
  subpages: { 32: "B,1|sensor.hidden:Hidden:Auto:Auto:::sensor" },
  settings: { button_order: "1", future_setting: "\u0000温度🌡️" },
};
const update = (current: PanelConfigDocument): PanelConfigDocument => ({ ...current, buttons: { ...current.buttons, 1: "light.one;Edited" } });
function response(status: number, body: Uint8Array = new Uint8Array(), version: number | null = null, etag: string | null = null): NativePanelConfigResponse {
  return { ok: status >= 200 && status < 300, status,
    headers: { get: (name) => name === "ETag" ? etag : name === "X-Panel-Config-Version" && version ? String(version) : null },
    arrayBuffer: async () => new Uint8Array(body).buffer as ArrayBuffer, json: async () => ({}) };
}
function capabilities(versions = [1, 2], max = 524288): NativePanelConfigResponse {
  return { ...response(200), json: async () => ({ configuration: {
    read: true, write: true, document_versions: versions, max_record_body_bytes: 4096, max_v2_document_bytes: max,
  } }) };
}
const selected = (request?: NativePanelConfigRequest) => Number(request?.headers?.["X-Panel-Config-Version"] ?? 1);
function bytes(document: PanelConfigDocument, version: number): Uint8Array {
  const v1 = encodePanelConfig(document); return version === 2 ? convertNativeV1ToV2(v1) : v1;
}
function model(body: Uint8Array, version: number): PanelConfigDocument {
  return decodePanelConfig(version === 2 ? convertNativeV2ToV1(body) : body);
}

export async function runNativePanelConfigVersionTests(): Promise<void> {
  const requests: NativePanelConfigRequest[] = [];
  let persisted: PanelConfigDocument | undefined;
  const client = createNativePanelConfigClient(async (path, request) => {
    if (path.endsWith("capabilities")) return capabilities();
    requests.push(request!);
    check(selected(request) === 2, "editor must explicitly request v2 for both read and write");
    if (request?.method === "PUT") {
      check(request.headers?.["If-Match"] === '"v2-7"', "v2 saves use the response's v2 generation token");
      persisted = model(request.body!, 2); return response(204);
    }
    return response(200, bytes(initial, 2), 2, '"v2-7"');
  });
  same(await client.save(update), "saved", "negotiated save succeeds");
  same(persisted, update(initial), "v2 edit preserves dormant cards, child pages and opaque settings");
  same(requests.length, 2, "one negotiated read and write");

  for (const versions of [[1], [2]]) {
    const only = createNativePanelConfigClient(async (path, request) => {
      if (path.endsWith("capabilities")) return capabilities(versions);
      const version = versions[0]!;
      check(selected(request) === version, "select an advertised supported version");
      return request?.method === "PUT" ? response(204) : response(200, bytes(initial, version), version, version === 2 ? '"v2-1"' : '"1"');
    });
    same(await only.save(update), "saved", `version ${versions[0]} only firmware saves`);
    if (versions[0] === 2) {
      let failed = false; try { await only.readForBackup("panel-a"); } catch { failed = true; }
      check(failed, "v2-only firmware cannot produce a falsely compatible v1 backup");
      check(!only.canUseLegacyFallback(), "v2-only firmware never selects legacy text saves");
    }
  }

  for (const rejection of ["GET", "PUT"]) {
    const events: string[] = []; let current = initial, written: PanelConfigDocument | undefined;
    const fallback = createNativePanelConfigClient(async (path, request) => {
      if (path.endsWith("capabilities")) return capabilities();
      const version = selected(request), method = request?.method ?? "GET"; events.push(`${method}:${version}`);
      if (version === 2 && method === rejection) {
        current = { ...initial, settings: { ...initial.settings, concurrent_edit: "preserve" } };
        return response(method === "GET" ? 406 : 415);
      }
      if (method === "PUT") { written = model(request!.body!, version); return response(204); }
      return response(200, bytes(current, version), version, version === 2 ? '"v2-1"' : '"2"');
    });
    same(await fallback.save(update), "saved", `${rejection} version rejection falls back to advertised v1`);
    same(Array.from(encodePanelConfig(written!)), Array.from(encodePanelConfig(update(current))),
      "fallback reloads the current snapshot before applying the edit");
    same(events, rejection === "GET" ? ["GET:2", "GET:1", "PUT:1"] : ["GET:2", "PUT:2", "GET:1", "PUT:1"], "bounded version fallback");
  }

  const transitions: string[] = []; let v1Saves = 0;
  const fallbackConflict = createNativePanelConfigClient(async (path, request) => {
    if (path.endsWith("capabilities")) return capabilities();
    const version = selected(request), method = request?.method ?? "GET"; transitions.push(`${method}:${version}`);
    if (method === "PUT") return response(version === 2 ? 415 : ++v1Saves === 1 ? 409 : 204);
    return response(200, bytes(initial, version), version, version === 2 ? '"v2-1"' : `"${v1Saves+1}"`);
  });
  same(await fallbackConflict.save(update), "saved", "fallback does not consume the conflict retry budget");
  same(transitions, ["GET:2","PUT:2","GET:1","PUT:1","GET:1","PUT:1"], "one fallback and one stale-write retry");

  let saves = 0, reads = 0;
  const conflict = createNativePanelConfigClient(async (path, request) => {
    if (path.endsWith("capabilities")) return capabilities();
    check(selected(request) === 2, "conflicts retain v2 selection");
    if (request?.method === "PUT") return response(++saves === 1 ? 409 : 204);
    ++reads; const current = reads === 1 ? initial : { ...initial, settings: { ...initial.settings, external: "new" } };
    return response(200, bytes(current, 2), 2, `"v2-${reads}"`);
  });
  same(await conflict.save(update), "saved", "stale v2 write reloads once");
  same([reads, saves], [2, 2], "conflict retry stays bounded");

  for (const status of [202, 400, 401, 403, 409, 413, 500, 503]) {
    const versions: number[] = [];
    const failed = createNativePanelConfigClient(async (path, request) => {
      if (path.endsWith("capabilities")) return capabilities();
      versions.push(selected(request));
      return request?.method === "PUT" ? response(status) : response(200, bytes(initial, 2), 2, '"v2-1"');
    });
    const expected = status === 202 ? "mirror-failed" : status === 401 || status === 403 ? "authentication-required" :
      status === 409 ? "conflict" : status === 413 ? "too-large" : status === 503 ? "unsupported" : "failed";
    same(await failed.save(update), expected, `${status} remains its own save outcome`);
    check(versions.every((version) => version === 2), `${status} cannot cause a fallback write`);
    check(!failed.canUseLegacyFallback(), "known native failures never select entity saves");
  }
  for (const mismatch of [null, 1, 3]) {
    let writes = 0;
    const failed = createNativePanelConfigClient(async (path, request) => {
      if (path.endsWith("capabilities")) return capabilities();
      if (request?.method === "PUT") { writes++; return response(204); }
      return response(200, bytes(initial, 2), mismatch, '"v2-1"');
    });
    same(await failed.save(update), "failed", "unconfirmed response format fails closed");
    same(writes, 0, "version mismatch does not write");
  }
  for (const status of [400, 401, 403, 404, 422, 500, 503]) {
    let writes = 0;
    const failed = createNativePanelConfigClient(async (path, request) => {
      if (path.endsWith("capabilities")) return capabilities();
      check(selected(request) === 2, "read failures cannot downgrade representations");
      if (request?.method === "PUT") writes++;
      return response(status);
    });
    same(await failed.save(update), status === 401 || status === 403 ? "authentication-required" :
      status === 404 || status === 503 ? "unsupported" : "failed", `${status} read failure is preserved`);
    check(!writes && !failed.canUseLegacyFallback(), "read failures cannot write or select legacy entities");
  }
  for (const largeOnRead of [true, false]) {
    let writes = 0;
    const limited = createNativePanelConfigClient(async (path, request) => {
      if (path.endsWith("capabilities")) return capabilities([1, 2], largeOnRead ? 32 : 800);
      if (request?.method === "PUT") { writes++; return response(204); }
      return response(200, bytes(initial, 2), 2, '"v2-1"');
    });
    same(await limited.save((current) => largeOnRead ? current : { ...current, settings: { ...current.settings, more: "x".repeat(1000) } }),
      "too-large", "negotiated input/output ceiling is enforced");
    same(writes, 0, "oversize v2 cannot write");
  }
  const hardLimit = createNativePanelConfigClient(async (path) => path.endsWith("capabilities")
    ? capabilities([1, 2], Number.MAX_SAFE_INTEGER) : response(200, new Uint8Array(524289), 2, '"v2-1"'));
  same(await hardLimit.save(update), "too-large", "advertised limits cannot remove the browser's hard ceiling");
  const unknown = createNativePanelConfigClient(async () => capabilities([3]));
  same(await unknown.save(update), "unsupported", "unknown future version stays unsupported");
  check(!unknown.canUseLegacyFallback(), "unknown native versions do not select entity saves");
}

export async function runNativeNegotiatedBackupTests(): Promise<void> {
  const exact = encodePanelConfig(initial); let edits = 0; const backupVersions: number[] = [];
  const client = createNativePanelConfigClient(async (path, request) => {
    if (path.endsWith("capabilities")) return capabilities();
    if (request?.method === "PUT") { edits++; return response(204); }
    const version = selected(request); backupVersions.push(version);
    return response(200, version === 1 ? exact : bytes(initial, 2), version, version === 1 ? '"1"' : '"v2-1"');
  });
  same(await client.save(update), "saved", "editor can negotiate v2 before a backup");
  const backup = await client.readForBackup("panel-a");
  same(Array.from(backup!), Array.from(exact), "existing backups retain exact durable v1 bytes");
  same(backupVersions, [2, 1], "backup explicitly requests its compatible v1 representation");
  same(edits, 1, "backup does not write or trigger migration");

  // A structurally valid future graph which this editor cannot project must
  // never be flattened by reading a v1 view and replacing the stored graph.
  const records: NativeV2Record[] = decodeNativeV2(bytes(initial, 2));
  const folderType = 64000, owner = 64001, childPage = 64002;
  const hiddenPage = records.find((row) => row.kind === NativeV2Kind.Page && row.id !== 0)!;
  check(hiddenPage?.id, "future graph has its retained first-level page");
  const hiddenPageId = hiddenPage.id;
  records.push({ kind: NativeV2Kind.String, id: folderType, text: "subpage" });
  records.push({ kind: NativeV2Kind.Card, id: owner, first: hiddenPageId, second: 2, fieldMask: 1 << 6, fields: [0, 0, 0, 0, 0, 0, folderType, 0, 0] });
  records.push({ kind: NativeV2Kind.Page, id: childPage, first: hiddenPageId, second: owner });
  records.push({ kind: NativeV2Kind.Navigation, id: owner, first: childPage });
  const nested = encodeNativeV2(records);
  let writes = 0;
  const future = createNativePanelConfigClient(async (path, request) => {
    if (path.endsWith("capabilities")) return capabilities();
    if (request?.method === "PUT") { writes++; return response(204); }
    check(selected(request) === 2, "unprojectable content must not silently downgrade its read");
    return response(200, nested, 2, '"v2-1"');
  });
  same(await future.save(update), "failed", "unprojectable future graph rejects the edit");
  same(writes, 0, "unsupported future graph remains intact");
}
