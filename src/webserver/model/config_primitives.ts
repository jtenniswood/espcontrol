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
