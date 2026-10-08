export function trimConfigFields(fields: string[]): string[] {
  while (fields.length > 1 && !fields[fields.length - 1]) fields.pop();
  return fields;
}

export function encodeConfigField(value: string | number | boolean | null | undefined): string {
  return String(value || "").replace(/[%,;|:]/g, (ch) => {
    const hex = ch.charCodeAt(0).toString(16).toUpperCase();
    return "%" + (hex.length < 2 ? "0" : "") + hex;
  });
}

export function decodeConfigField(value: string | number | boolean | null | undefined): string {
  const str = String(value || "");
  // Decode complete UTF-8 runs so multi-byte sequences such as %C2%B0 become °.
  return str.replace(/(%[0-9a-fA-F]{2})+/g, (run) => {
    try {
      return decodeURIComponent(run);
    } catch {
      return run;
    }
  });
}

export function legacyButtonConfigSafe(fields: readonly string[]): boolean {
  return fields.join(";").charAt(0) !== "~" && fields.every((field) => {
    return String(field || "").indexOf(";") < 0;
  });
}

export function configOptionEnabled(options: unknown, name: string): boolean {
  return String(options || "").split(",").some((part) => part === name);
}

export function setConfigOption(options: unknown, name: string, enabled: boolean): string {
  const out: string[] = [];
  let found = false;
  for (const part of String(options || "").split(",")) {
    if (!part) continue;
    if (part === name) {
      found = true;
      if (enabled) out.push(part);
    } else if (out.indexOf(part) < 0) {
      out.push(part);
    }
  }
  if (enabled && !found) out.push(name);
  return out.join(",");
}

export function configOptionValue(options: unknown, name: string): string {
  const prefix = name + "=";
  for (const part of String(options || "").split(",")) {
    if (part.indexOf(prefix) === 0) return decodeConfigField(part.substring(prefix.length));
  }
  return "";
}

export function setConfigOptionValue(options: unknown, name: string, value: unknown): string {
  const prefix = name + "=";
  const out: string[] = [];
  for (const part of String(options || "").split(",")) {
    if (!part || part.indexOf(prefix) === 0) continue;
    if (out.indexOf(part) < 0) out.push(part);
  }
  const text = String(value || "").trim();
  if (text) out.push(prefix + encodeConfigField(text));
  return out.join(",");
}

const CARD_COLOR_OPTIONS = ["card_on_color", "card_off_color"] as const;

export function normalizedCardColorOptions(options: unknown): string {
  // Prefer the card colour; migrate old active-only customisations to the single colour.
  for (const name of ["card_off_color", "card_on_color"]) {
    const value = configOptionValue(options, name).replace(/^#/, "").toUpperCase();
    if (/^[0-9A-F]{6}$/.test(value)) return "card_off_color=" + value;
  }
  return "";
}

export function withoutCardColorOptions(options: unknown): string {
  return String(options || "").split(",").filter((part) =>
    !CARD_COLOR_OPTIONS.some((name) => part.indexOf(name + "=") === 0),
  ).filter(Boolean).join(",");
}

export function preserveCardColorOptions(options: unknown, normalized: unknown): string {
  return [withoutCardColorOptions(normalized), normalizedCardColorOptions(options)].filter(Boolean).join(",");
}

export function setCardColor(options: unknown, value: unknown): string {
  const cleaned = String(value || "").replace(/^#/, "").trim().toUpperCase();
  return setConfigOptionValue(withoutCardColorOptions(options), "card_off_color", /^[0-9A-F]{6}$/.test(cleaned) ? cleaned : "");
}

export function lighterCardColor(color: string): string {
  const hex = color.replace(/^#/, "");
  if (!/^[0-9A-F]{6}$/i.test(hex)) return "";
  // Match lighter_card_color in the firmware, including round-to-nearest channels.
  return [0, 2, 4].map((offset) => {
    const channel = parseInt(hex.slice(offset, offset + 2), 16);
    const lighter = channel + Math.floor(((255 - channel) * 30 + 50) / 100);
    return ("0" + lighter.toString(16)).slice(-2).toUpperCase();
  }).join("");
}
