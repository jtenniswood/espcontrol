export interface PanelIdentityInfo {
  name: string;
  friendly_name: string;
  hostname: string;
  mac_suffix: string;
  ip_address: string;
  restart_required: boolean;
}

export interface PanelIdentityBackup {
  version: 1;
  name: string;
  hostname: string;
  mac_suffix: string;
}

export function normalizePanelName(value: unknown): string {
  if (typeof value !== "string") throw new Error("Panel name must be text.");
  const name = value.replace(/^[ \t\r\n\f\v]+|[ \t\r\n\f\v]+$/g, "");
  if (new TextEncoder().encode(name).length > 120 || /[\u0000-\u001f\u007f-\u009f]/.test(name)
      || /[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/.test(name)) {
    throw new Error("Use up to 120 UTF-8 bytes with no control characters.");
  }
  return name;
}

export function panelHostname(name: string, suffix: string): string {
  const slug = name.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-+|-+$/g, "") || "panel";
  return `espcontrol-${slug.slice(0, 13).replace(/-+$/, "")}-${suffix}`;
}

export function readIdentityBackup(value: unknown): PanelIdentityBackup | undefined {
  if (!value || typeof value !== "object" || Array.isArray(value)) return undefined;
  const data = value as Record<string, unknown>;
  if (data.version !== 1) return undefined;
  const name = normalizePanelName(data.name);
  if (typeof data.mac_suffix !== "string" || !/^[a-f0-9]{6}$/.test(data.mac_suffix)
      || typeof data.hostname !== "string" || !/^[a-z0-9-]{1,31}$/.test(data.hostname)) {
    throw new Error("Invalid panel identity in backup.");
  }
  return { version: 1, name, hostname: data.hostname, mac_suffix: data.mac_suffix };
}
