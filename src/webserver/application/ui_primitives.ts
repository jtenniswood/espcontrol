import {
  GENERATED_DOMAIN_ICONS,
  GENERATED_ICON_EXCEPTIONS,
  GENERATED_ICON_NAMES,
} from "../generated/icons";

export const iconOptions: readonly string[] = ["Auto"].concat(GENERATED_ICON_NAMES).sort();
export const domainIcons: Readonly<Record<string, string>> = GENERATED_DOMAIN_ICONS;

export function uniqueOptions(options?: unknown[]): string[] {
  const output: string[] = [];
  for (const option of options || []) {
    const value = String(option);
    if (output.indexOf(value) < 0) output.push(value);
  }
  return output;
}

export function setSelectValue(
  select: HTMLSelectElement | null | undefined,
  value: unknown,
  label?: unknown,
): void {
  if (!select) return;
  const normalized = String(value);
  const found = Array.from(select.options).some((option) => option.value === normalized);
  if (!found) {
    const option = select.ownerDocument.createElement("option");
    option.value = normalized;
    option.textContent = label == null ? normalized : String(label);
    select.appendChild(option);
  }
  select.value = normalized;
}

export function escHtml(value: unknown): string {
  return String(value).replace(/&/g, "&amp;").replace(/</g, "&lt;")
    .replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

export function escAttr(value: unknown): string {
  return String(value == null ? "" : value).replace(/&/g, "&amp;").replace(/"/g, "&quot;")
    .replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

// Custom icons are stored as "mdi:<hex codepoint>" (for example "mdi:F0F92").
// The firmware renders them directly once the glyph is added to the device
// font in the user's ESPHome YAML. Matches find_icon() in icons.h.
const CUSTOM_ICON_RE = /^mdi:([0-9A-Fa-f]{4,6})$/;

function isIconCodepoint(codepoint: number): boolean {
  return (codepoint >= 0xE000 && codepoint <= 0xF8FF) ||
    (codepoint >= 0xF0000 && codepoint <= 0x10FFFD);
}

export function customIconCodepoint(value: unknown): number | null {
  const match = CUSTOM_ICON_RE.exec(String(value || ""));
  if (!match || !match[1]) return null;
  const codepoint = parseInt(match[1], 16);
  return isIconCodepoint(codepoint) ? codepoint : null;
}

// Turn what someone typed in the icon picker ("F0F92", "mdi:F0F92",
// "\U000F0F92", "U+F0F92", "0xF0F92") into a saved custom icon value.
export function normalizeCustomIcon(input: unknown): string | null {
  const text = String(input || "").trim()
    .replace(/^mdi:/i, "")
    .replace(/^(?:\\U0*|U\+|0x)/i, "");
  if (!/^[0-9A-Fa-f]{4,6}$/.test(text)) return null;
  const codepoint = parseInt(text, 16);
  if (!isIconCodepoint(codepoint)) return null;
  return "mdi:" + codepoint.toString(16).toUpperCase().padStart(4, "0");
}

const customIconRules: Record<string, true> = {};
let customIconStyle: HTMLStyleElement | null = null;

// The bundled icon font contains every MDI glyph, but CSS classes exist only
// for built-in icons. Add a class for each custom codepoint the first time
// it is shown so every existing preview renders it unchanged.
function customIconSlug(codepoint: number): string {
  const hex = codepoint.toString(16);
  const slug = "custom-" + hex;
  if (!customIconRules[slug] && typeof document !== "undefined" && document.head) {
    if (!customIconStyle) {
      customIconStyle = document.createElement("style");
      document.head.appendChild(customIconStyle);
    }
    customIconStyle.appendChild(document.createTextNode(
      ".mdi-" + slug + "::before{content:'\\" + hex + "'}"));
    customIconRules[slug] = true;
  }
  return slug;
}

export function iconSlug(name: unknown): string {
  const value = String(name || "");
  const customCodepoint = customIconCodepoint(value);
  if (customCodepoint !== null) return customIconSlug(customCodepoint);
  // Malformed custom values show the cog, matching the firmware fallback.
  if (/^mdi:/i.test(value)) return "cog";
  return GENERATED_ICON_EXCEPTIONS[value] || value.toLowerCase().replace(/[^a-z0-9]/g, (character) =>
    character === " " ? "-" : "") || "cog";
}

export function mdiIcon(icon?: unknown, className?: string): HTMLSpanElement {
  const iconName = String(icon || "cog").trim();
  const span = document.createElement("span");
  span.className = className || "mdi";
  span.classList.add("mdi-" + (/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(iconName) ? iconName : iconSlug(iconName)));
  return span;
}

export function textSpan(text?: unknown, className?: string): HTMLSpanElement {
  const span = document.createElement("span");
  if (className) span.className = className;
  span.textContent = text == null ? "" : String(text);
  return span;
}
