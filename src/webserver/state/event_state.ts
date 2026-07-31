import type { AppState, EntityEvent } from "./types";

function uniquePush(list: string[], value: string): void {
  if (value && !list.includes(value)) list.push(value);
}

function objectId(value: string): string {
  return value.replace(/./g, (character) => {
    if (character === " ") return "_";
    const lower = character.toLowerCase();
    return /[a-z0-9_-]/.test(lower) ? lower : "_";
  });
}

function parsedEntityKey(value: unknown): { domain: string; name?: string; objectId: string } | null {
  const id = String(value || "");
  if (!id) return null;
  if (id.includes("/")) {
    const parts = id.split("/");
    const domain = parts[0];
    const name = parts[parts.length - 1];
    if (!domain || !name) return null;
    return { domain, name, objectId: objectId(name) };
  }
  const dash = id.indexOf("-");
  return dash > 0 ? { domain: id.substring(0, dash), objectId: id.substring(dash + 1) } : null;
}

export function entityStateKeys(event: EntityEvent): string[] {
  const keys: string[] = [];
  for (const rawId of [event.id, event.name_id]) {
    const id = String(rawId || "");
    const parsed = parsedEntityKey(id);
    uniquePush(keys, id);
    if (parsed) uniquePush(keys, `${parsed.domain}-${parsed.objectId}`);
    if (parsed?.name) uniquePush(keys, `${parsed.domain}:${parsed.name}`);
  }
  return keys;
}

export function applyClockBarStateValue(state: AppState, value: string, event: EntityEvent, matchedKey?: string): boolean {
  const keys = entityStateKeys(event);
  uniquePush(keys, matchedKey || "");
  const nextOn = event.value === true || value === "ON";
  const sourceKey = matchedKey || keys[0] || "clock_bar";
  state._clockBarStateValues[sourceKey] = nextOn;
  const previous = state.clockBarOn;
  state.clockBarOn = Object.values(state._clockBarStateValues).some(Boolean);
  return state.clockBarOn !== previous;
}

export function isRemovedLegacyStateEvent(id: string, event: EntityEvent): boolean {
  const keys = entityStateKeys(event);
  uniquePush(keys, id);
  return [
    "text-screen_saver__cover_art_fallback_server",
    "text-screen_saver_cover_art_fallback_server",
    "text-cover_art_fallback_server",
  ].some((key) => keys.includes(key));
}

export function resetStateForConnection(state: AppState): void {
  state.selectedSlots = [];
  state.lastClickedSlot = -1;
  state.editingSubpage = null;
  state.subpageSelectedSlots = [];
  state.subpageLastClicked = -1;
}

export function parseEntityEventData(data: unknown): EntityEvent | null {
  if (typeof data !== "string") return null;
  try {
    const parsed: unknown = JSON.parse(data);
    return parsed !== null && typeof parsed === "object" && !Array.isArray(parsed) ? parsed as EntityEvent : null;
  } catch {
    return null;
  }
}
