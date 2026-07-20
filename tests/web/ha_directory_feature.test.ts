import { haEntityNamesFromStates, haWebSocketUrl, HA_DEFAULT_URL } from "../../src/webserver/features/ha_directory";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runHaDirectoryFeatureTests(): void {
  equal(HA_DEFAULT_URL, "http://homeassistant.local:8123", "default URL points at the standard HA address");
  equal(haWebSocketUrl(""), "ws://homeassistant.local:8123/api/websocket", "empty URL falls back to the default");
  equal(haWebSocketUrl("homeassistant.local"), "ws://homeassistant.local:8123/api/websocket", "bare host gains ws scheme and default port");
  equal(haWebSocketUrl("http://ha.local:8123/"), "ws://ha.local:8123/api/websocket", "trailing slash is stripped");
  equal(haWebSocketUrl("https://ha.example.com"), "wss://ha.example.com/api/websocket", "https maps to wss without a forced port");
  equal(haWebSocketUrl("https://ha.example.com:9443"), "wss://ha.example.com:9443/api/websocket", "explicit port is preserved");
  equal(haWebSocketUrl("ws://ha.local"), "ws://ha.local:8123/api/websocket", "ws scheme passes through with default port");
  equal(haWebSocketUrl("http://ha.local:8123/lovelace/home"), "ws://ha.local:8123/api/websocket", "paths are replaced with the websocket endpoint");
  equal(haWebSocketUrl("ftp://ha.local"), null, "unsupported schemes are rejected");
  equal(haWebSocketUrl("http://"), null, "URL without a host is rejected");

  const names = haEntityNamesFromStates([
    { entity_id: "light.kitchen", attributes: { friendly_name: "Kitchen Light" } },
    { entity_id: "sensor.outside_temp", attributes: {} },
    { entity_id: "no_dot", attributes: { friendly_name: "Ignored" } },
    { attributes: { friendly_name: "No id" } },
    null,
    "garbage",
  ]);
  equal(Object.keys(names).length, 2, "only entries with dotted entity ids are kept");
  equal(names["light.kitchen"], "Kitchen Light", "friendly name is mapped");
  equal(names["sensor.outside_temp"], "", "missing friendly name maps to empty string");
  equal(Object.keys(haEntityNamesFromStates(null)).length, 0, "non-array input yields an empty map");
  equal(Object.keys(haEntityNamesFromStates({})).length, 0, "object input yields an empty map");
}
