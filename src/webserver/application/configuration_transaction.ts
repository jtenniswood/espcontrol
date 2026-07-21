import {
  configurationRecordKey,
  decodeConfigurationDocument,
  encodeConfigurationDocument,
  type ConfigurationDocument,
  type ConfigurationDomain,
  type ConfigurationRecord,
} from "../api/configuration_document";
import { createTransactionId } from "../api/transaction_id";
import { state } from "../state/app_instance";

const SNAPSHOT_PATH = "/espcontrol/configuration";
const BEGIN_PATH = "/espcontrol/configuration/begin";
const CHUNK_PATH = "/espcontrol/configuration/chunk";
const COMMIT_PATH = "/espcontrol/configuration/commit";
const UPLOAD_CHUNK_SIZE = 384;
const SAVE_DEBOUNCE_MS = 150;
const RETRY_DELAY_MS = 1500;

type FallbackPost = () => unknown;

interface CommitResult {
  ok: boolean;
  conflict: boolean;
  revision: number;
}

let supported: boolean | null = null;
let revision = 0;
let documentVersion = 1;
let records: ConfigurationRecord[] = [];
let recordByKey = new Map<string, ConfigurationRecord>();
const pendingValues = new Map<string, string>();
let initialization: Promise<boolean> | null = null;
let saveTimer: number | null = null;
let saveActive = false;
let waiters: Array<{ resolve: (value: unknown) => void; reject: (reason: unknown) => void }> = [];

function validUnsignedHeader(value: string | null, maximum: number): number | null {
  if (!value || !/^\d+$/.test(value)) return null;
  const parsed = Number(value);
  return Number.isSafeInteger(parsed) && parsed >= 0 && parsed <= maximum ? parsed : null;
}

function rebuildRecordIndex(document: ConfigurationDocument): void {
  records = document.records;
  recordByKey = new Map(records.map((record) => [configurationRecordKey(record.domain, record.objectId), record]));
}

async function loadSnapshot(): Promise<boolean> {
  const response = await fetch(SNAPSHOT_PATH, { cache: "no-store", credentials: "same-origin" });
  if (response.status === 404 || response.status === 204) {
    supported = false;
    return false;
  }
  supported = true;
  if (!response.ok) throw new Error(`Configuration snapshot unavailable (${response.status})`);
  const nextRevision = validUnsignedHeader(response.headers.get("X-EspControl-Revision"), 0xffffffff);
  const nextVersion = validUnsignedHeader(response.headers.get("X-EspControl-Document-Version"), 0xffff);
  if (nextRevision === null || nextVersion === null) throw new Error("Configuration snapshot is missing revision metadata");
  const document = decodeConfigurationDocument(await response.arrayBuffer());
  revision = nextRevision;
  documentVersion = nextVersion;
  rebuildRecordIndex(document);
  for (const [key, value] of pendingValues) {
    const record = recordByKey.get(key);
    if (record) record.value = value;
  }
  return true;
}

export function initializeConfigurationTransaction(): Promise<boolean> {
  if (!initialization) {
    initialization = loadSnapshot().catch((error: unknown) => {
      if (supported === true) throw error;
      supported = false;
      return false;
    });
  }
  return initialization;
}

export function refreshConfigurationSnapshot(): void {
  if (saveActive || pendingValues.size > 0) return;
  initialization = loadSnapshot().catch((error: unknown) => {
    console.warn("Unable to refresh configuration revision", error);
    return supported === true;
  });
}

function rememberedObjectId(domain: ConfigurationDomain, name: string): string | null {
  const keys = [`${domain}:${name}`, `${domain}-${esphomeObjectId(name)}`];
  for (const key of keys) {
    const path = state.entityPostPaths[key];
    if (!path) continue;
    const parts = path.split("/").filter(Boolean);
    if (parts.length < 2 || decodeURIComponent(parts[0] || "") !== domain) continue;
    const objectId = decodeURIComponent(parts[parts.length - 1] || "");
    if (objectId) return objectId;
  }
  return null;
}

function findRecord(domain: ConfigurationDomain, name: string, objectIds: string[]): ConfigurationRecord | null {
  const candidates = objectIds.slice();
  const remembered = rememberedObjectId(domain, name);
  if (remembered) candidates.push(remembered);
  candidates.push(esphomeObjectId(name));
  for (const objectId of candidates) {
    const record = recordByKey.get(configurationRecordKey(domain, objectId));
    if (record) return record;
  }
  return null;
}

async function formPost(path: string, values: Record<string, string>): Promise<Response> {
  return fetch(path, {
    method: "POST",
    credentials: "same-origin",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: new URLSearchParams(values).toString(),
  });
}

function bytesToHex(bytes: Uint8Array): string {
  let result = "";
  for (const value of bytes) result += value.toString(16).padStart(2, "0");
  return result;
}

async function uploadCandidate(document: Uint8Array): Promise<CommitResult> {
  const transaction = createTransactionId();
  const begin = await formPost(BEGIN_PATH, {
    transaction: String(transaction),
    expected_revision: String(revision),
    document_version: String(documentVersion),
    size: String(document.byteLength),
  });
  if (!begin.ok) throw new Error(`Configuration upload could not start (${begin.status})`);

  for (let offset = 0; offset < document.byteLength; offset += UPLOAD_CHUNK_SIZE) {
    const chunk = document.subarray(offset, Math.min(offset + UPLOAD_CHUNK_SIZE, document.byteLength));
    const response = await formPost(CHUNK_PATH, {
      transaction: String(transaction),
      offset: String(offset),
      data: bytesToHex(chunk),
    });
    if (!response.ok) throw new Error(`Configuration chunk was rejected (${response.status})`);
  }

  const committed = await formPost(COMMIT_PATH, { transaction: String(transaction) });
  let payload: { revision?: unknown } = {};
  try {
    payload = await committed.json() as { revision?: unknown };
  } catch {
    // Status handling below still reports a useful error for an empty body.
  }
  const nextRevision = typeof payload.revision === "number" ? payload.revision : revision;
  if (committed.status === 409) return { ok: false, conflict: true, revision: nextRevision };
  if (!committed.ok) throw new Error(`Configuration commit was rejected (${committed.status})`);
  return { ok: true, conflict: false, revision: nextRevision };
}

function resolveWaiters(value: unknown): void {
  const settled = waiters;
  waiters = [];
  for (const waiter of settled) waiter.resolve(value);
}

function rejectWaiters(reason: unknown): void {
  const settled = waiters;
  waiters = [];
  for (const waiter of settled) waiter.reject(reason);
}

function scheduleSave(delay = SAVE_DEBOUNCE_MS): void {
  if (saveTimer !== null || saveActive) return;
  saveTimer = window.setTimeout(() => {
    saveTimer = null;
    void flushConfiguration();
  }, delay);
}

async function flushConfiguration(): Promise<void> {
  if (saveActive || pendingValues.size === 0 || supported !== true) return;
  saveActive = true;
  let retryDelay = SAVE_DEBOUNCE_MS;
  try {
    let committed = false;
    for (let conflictAttempt = 0; conflictAttempt < 3; conflictAttempt += 1) {
      for (const [key, value] of pendingValues) {
        const record = recordByKey.get(key);
        if (!record) throw new Error(`Configuration entity ${key} is no longer available`);
        record.value = value;
      }
      const captured = new Map(pendingValues);
      const result = await uploadCandidate(encodeConfigurationDocument({ records }));
      if (result.conflict) {
        await loadSnapshot();
        continue;
      }
      revision = result.revision;
      for (const [key, value] of captured) {
        if (pendingValues.get(key) === value) pendingValues.delete(key);
      }
      if (pendingValues.size === 0) resolveWaiters({ revision });
      committed = true;
      break;
    }
    if (!committed) throw new Error("Configuration changed repeatedly while saving");
  } catch (error) {
    console.warn("Configuration save will be retried", error);
    showBanner("Waiting to save configuration…", "offline");
    retryDelay = RETRY_DELAY_MS;
  } finally {
    saveActive = false;
    if (pendingValues.size > 0) scheduleSave(retryDelay);
  }
}

export function postConfigurationValue(
  domain: ConfigurationDomain,
  name: string,
  objectIds: string[],
  value: string,
  fallback: FallbackPost,
): Promise<unknown> {
  return initializeConfigurationTransaction().then((available) => {
    if (!available) return fallback();
    const record = findRecord(domain, name, objectIds);
    if (!record) return fallback();
    const key = configurationRecordKey(record.domain, record.objectId);
    record.value = value;
    pendingValues.set(key, value);
    scheduleSave();
    return new Promise((resolve, reject) => {
      waiters.push({ resolve, reject });
    });
  }).catch((error: unknown) => {
    if (supported !== true) return fallback();
    rejectWaiters(error);
    throw error;
  });
}
