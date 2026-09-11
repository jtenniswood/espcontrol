import {
  decodePanelConfig,
  createPanelConfigBackupPayload,
  decodePanelConfigBackupPayload,
  encodePanelConfig,
  PanelConfigError,
  type PanelConfigDocument,
} from "../../src/webserver/model";

interface PanelConfigFixture {
  document: PanelConfigDocument;
  encoded_hex: string;
}

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}
function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}
function hex(bytes: Uint8Array): string { return Array.from(bytes, (value) => value.toString(16).padStart(2, "0")).join(""); }
function expectPanelConfigError(run: () => void, message: string): void {
  try { run(); } catch (error) {
    if (error instanceof PanelConfigError) return;
    throw error;
  }
  throw new Error(message);
}

export function runPanelConfigTests(fixture: PanelConfigFixture): void {
  const { document } = fixture;
  const encoded = encodePanelConfig(document);
  equal(hex(encoded), fixture.encoded_hex, "the browser encoder produces the shared codec fixture");
  deepEqual(decodePanelConfig(encoded), document, "document round-trips");
  const duplicateSlot = encoded.slice();
  duplicateSlot[55] = 2;
  duplicateSlot[58] = 1;
  expectPanelConfigError(() => decodePanelConfig(duplicateSlot), "duplicate slots must be rejected");
  const invalidUtf8 = encoded.slice();
  invalidUtf8[19] = 0xff;
  expectPanelConfigError(() => decodePanelConfig(invalidUtf8), "invalid UTF-8 must be rejected");
  const collidingButtonSlots: Record<string, string> = { 1: "light.kitchen", "01": "light.hall" };
  expectPanelConfigError(
    () => encodePanelConfig({ ...document, buttons: collidingButtonSlots }),
    "slot keys that normalize to the same value must be rejected",
  );
  const protoSettings = Object.create(null) as Record<string, string>;
  protoSettings.__proto__ = "preserved";
  const decodedProtoSettings = decodePanelConfig(encodePanelConfig({ ...document, settings: protoSettings }));
  equal(Object.prototype.hasOwnProperty.call(decodedProtoSettings.settings, "__proto__"), true, "reserved setting names are preserved");
  equal(decodedProtoSettings.settings.__proto__, "preserved", "reserved setting values round-trip");
  expectPanelConfigError(() => encodePanelConfig({ ...document, deviceProfile: "x".repeat(65) }), "oversized device profiles must be rejected");

  const backup = createPanelConfigBackupPayload(encoded);
  equal(backup.document_version, 1, "backup records the native document version");
  equal(backup.device_profile, document.deviceProfile, "backup records the native device profile");
  deepEqual(decodePanelConfigBackupPayload(backup), encoded, "backup payload restores the exact document bytes");
  expectPanelConfigError(
    () => decodePanelConfigBackupPayload({ ...backup, device_profile: "another-panel" }),
    "backup profile must match the encoded document",
  );
}
