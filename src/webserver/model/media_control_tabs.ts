export const MEDIA_CONTROL_TABS = ["controls", "progress", "volume", "speakers", "power"] as const;
export const MEDIA_CONTROL_DEFAULT_TABS = ["controls", "progress", "volume", "speakers"] as const;
export type MediaControlTab = typeof MEDIA_CONTROL_TABS[number];

export function normalizeMediaControlTabs(value: string, powerAvailable = false): MediaControlTab[] {
  const raw = String(value || "").trim();
  const parts = raw ? raw.split("|") : [...MEDIA_CONTROL_DEFAULT_TABS, ...(powerAvailable ? ["power"] : [])];
  const tabs: MediaControlTab[] = [];
  for (const part of parts) {
    const tab = part.trim() as MediaControlTab;
    if (MEDIA_CONTROL_TABS.includes(tab) && (tab !== "power" || powerAvailable) && !tabs.includes(tab)) tabs.push(tab);
  }
  return tabs.length ? tabs : ["controls"];
}
