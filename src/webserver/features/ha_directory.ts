export const HA_DEFAULT_URL = "http://homeassistant.local:8123";

const HA_WS_SCHEMES: Record<string, string> = {
  "http:": "ws:",
  "https:": "wss:",
  "ws:": "ws:",
  "wss:": "wss:",
};

export function haWebSocketUrl(raw: unknown): string | null {
  let text = String(raw == null ? "" : raw).trim();
  if (!text) text = HA_DEFAULT_URL;
  if (!/^[a-zA-Z][a-zA-Z0-9+.-]*:\/\//.test(text)) text = "http://" + text;
  let parsed: URL;
  try {
    parsed = new URL(text);
  } catch (err) {
    return null;
  }
  const scheme = HA_WS_SCHEMES[parsed.protocol];
  if (!scheme || !parsed.hostname) return null;
  let port = parsed.port;
  if (!port && scheme === "ws:") port = "8123";
  return scheme + "//" + parsed.hostname + (port ? ":" + port : "") + "/api/websocket";
}

export function haEntityNamesFromStates(states: unknown): Record<string, string> {
  const names: Record<string, string> = {};
  if (!Array.isArray(states)) return names;
  for (const item of states) {
    if (!item || typeof item !== "object") continue;
    const entityId = (item as { entity_id?: unknown }).entity_id;
    if (typeof entityId !== "string" || entityId.indexOf(".") <= 0) continue;
    const attributes = (item as { attributes?: unknown }).attributes;
    const friendlyName = attributes && typeof attributes === "object"
      ? (attributes as { friendly_name?: unknown }).friendly_name
      : "";
    names[entityId] = typeof friendlyName === "string" ? friendlyName : "";
  }
  return names;
}
