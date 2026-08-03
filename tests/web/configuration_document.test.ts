import {
  decodeConfigurationDocument,
  encodeConfigurationDocument,
  type ConfigurationDocument,
} from "../../src/webserver/api/configuration_document";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function deepEqual(actual: unknown, expected: unknown, message: string): void {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) throw new Error(`${message}: expected ${expectedText}, received ${actualText}`);
}

function throws(action: () => unknown, pattern: RegExp, message: string): void {
  try {
    action();
  } catch (error) {
    if (pattern.test(String(error))) return;
    throw new Error(`${message}: received ${String(error)}`);
  }
  throw new Error(`${message}: expected an error`);
}

export function runConfigurationDocumentTests(): void {
  const document: ConfigurationDocument = {
    records: [
      { domain: "text", objectId: "button_1_config", value: "light.kitchen;Kitchen" },
      { domain: "switch", objectId: "screen__clock_bar", value: "1" },
      { domain: "select", objectId: "screen__language", value: "fr" },
      { domain: "number", objectId: "screensaver_timeout", value: "300" },
    ],
  };
  deepEqual(decodeConfigurationDocument(encodeConfigurationDocument(document)), document, "entity document round-trip");

  const unicode: ConfigurationDocument = {
    records: [{ domain: "text", objectId: "button_1_config", value: "Lumière 🌤️" }],
  };
  deepEqual(decodeConfigurationDocument(encodeConfigurationDocument(unicode)), unicode, "UTF-8 value round-trip");

  const trailing = new Uint8Array(encodeConfigurationDocument(document).byteLength + 1);
  trailing.set(encodeConfigurationDocument(document));
  throws(() => decodeConfigurationDocument(trailing), /trailing/, "trailing bytes are rejected");

  const duplicate: ConfigurationDocument = {
    records: [
      { domain: "text", objectId: "button_order", value: "1" },
      { domain: "text", objectId: "button_order", value: "2" },
    ],
  };
  throws(() => decodeConfigurationDocument(encodeConfigurationDocument(duplicate)), /duplicate/, "duplicates are rejected");
  equal(encodeConfigurationDocument({ records: [] }).byteLength, 8, "empty document keeps its header");
}
