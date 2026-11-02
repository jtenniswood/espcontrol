import type { ControlsFieldsFeature } from "./controls_fields";

export interface GpioRelayControl { pin: number; name: string; inverted: boolean }
export interface GpioRelayInfo { enabled: boolean; controls: GpioRelayControl[]; allowed_pins: number[] }
let current: GpioRelayInfo | null = null;
let revision = 0;
export function gpioRelayProfileSupported(profile: string): boolean {
  return profile === "guition-esp32-p4-jc1060p470" || profile === "guition-esp32-p4-jc1060p470-v2";
}
export function gpioRelayOptions(): { key: string; label: string }[] {
  if (!current?.enabled) return [];
  return current.controls.flatMap((control, index) => control.pin < 0 ? [] : [{
    key: "gpio_relay_" + (index + 1), label: control.name || "GPIO Control " + (index + 1),
  }]);
}
export function validateGpioRelaySettings(info: GpioRelayInfo): string {
  const pins = new Set<number>();
  for (const control of info.controls) {
    if (new TextEncoder().encode(control.name).length > 32) return "Control names must use no more than 32 bytes.";
    if (control.pin === -1) continue;
    if (!info.allowed_pins.includes(control.pin)) return "Choose a supported GPIO pin for each control.";
    if (pins.has(control.pin)) return "Each GPIO pin can only be assigned to one control.";
    pins.add(control.pin);
  }
  return info.enabled && !pins.size ? "Assign a GPIO pin to at least one control before enabling GPIO relays." : "";
}
export async function gpioRelayRequest(request: typeof fetch, settings?: GpioRelayInfo): Promise<GpioRelayInfo | null> {
  const readRevision = ++revision;
  if (settings) {
    const error = validateGpioRelaySettings(settings);
    if (error) throw new Error(error);
  }
  const response = await request("/api/v1/gpio-relays", {
    method: settings ? "POST" : "GET", credentials: "include", cache: "no-store",
    ...(settings ? { headers: { "Content-Type": "application/json" }, body: JSON.stringify(settings) } : {}),
  });
  if (!settings && response.status === 404) { if (readRevision === revision) current = null; return null; }
  if (!response.ok) throw new Error("Could not read or save GPIO relay settings. Check the connection and try again.");
  const info = await response.json();
  if (typeof info?.enabled !== "boolean" || !Array.isArray(info.controls) || info.controls.length !== 3 ||
      !Array.isArray(info.allowed_pins) || !info.allowed_pins.every((pin: unknown) => Number.isInteger(pin) && Number(pin) >= 1 && Number(pin) <= 3) ||
      !info.controls.every((control: any) => control && Number.isInteger(control.pin) &&
        typeof control.name === "string" && typeof control.inverted === "boolean") || validateGpioRelaySettings(info)) {
    throw new Error("Invalid GPIO relay settings response.");
  }
  if (readRevision === revision) current = info;
  return info;
}

export function buildGpioRelaySettings(
  document: Document, request: typeof fetch,
  fields: Pick<ControlsFieldsFeature, "toggleRow" | "fieldLabel" | "textInput">,
  onVisibilityChange: (visible: boolean) => void,
): HTMLElement {
  const body = document.createElement("div");
  const enabled = fields.toggleRow("Enable GPIO relay controls", "sp-gpio-enabled", false);
  const introduction = document.createElement("p");
  introduction.textContent = "Configure external relay control signals for this display. Pins are never assigned automatically. Use a compatible relay driver or module; GPIO pins cannot power a relay coil directly.";
  const controls = document.createElement("div");
  const save = document.createElement("button");
  save.type = "button"; save.className = "sp-fw-btn"; save.textContent = "Save GPIO settings";
  const status = document.createElement("p");
  status.setAttribute("role", "status");
  body.append(introduction, enabled.row, controls, save, status);
  let info: GpioRelayInfo | null = null;
  let busy = true;
  const inputs: { pin: HTMLSelectElement; name: HTMLInputElement; inverted: HTMLInputElement }[] = [];
  function sync(): void {
    enabled.input.disabled = save.disabled = busy || !info;
    controls.hidden = !enabled.input.checked;
    for (const input of inputs) {
      input.pin.disabled = input.name.disabled = input.inverted.disabled = busy || !enabled.input.checked;
    }
  }
  enabled.input.addEventListener("change", sync);
  save.addEventListener("click", async () => {
    if (busy || !info) return;
    const settings: GpioRelayInfo = { enabled: enabled.input.checked, allowed_pins: info.allowed_pins,
      controls: inputs.map(input => ({ pin: Number(input.pin.value), name: input.name.value.trim(), inverted: input.inverted.checked })) };
    const error = validateGpioRelaySettings(settings);
    if (error) { status.textContent = error; return; }
    busy = true; sync();
    try {
      info = await gpioRelayRequest(request, settings);
      status.textContent = "Saved. GPIO outputs are off. Add a GPIO Relay card to control an assigned pin.";
    } catch (error) { status.textContent = (error as Error).message; }
    finally { busy = false; sync(); }
  });
  sync();
  void gpioRelayRequest(request).then(loaded => {
    info = loaded;
    onVisibilityChange(info !== null);
    if (!info) return;
    enabled.input.checked = info.enabled;
    info.controls.forEach((control, index) => {
      const prefix = "sp-gpio-control-" + (index + 1);
      const section = document.createElement("div"); section.className = "sp-field";
      const title = document.createElement("h4"); title.textContent = "Control " + (index + 1);
      const pin = document.createElement("select"); pin.id = prefix + "-pin"; pin.className = "sp-select";
      for (const value of [-1, ...info!.allowed_pins]) {
        const option = document.createElement("option"); option.value = String(value);
        option.textContent = value < 0 ? "Not configured" : "GPIO " + value; pin.append(option);
      }
      pin.value = String(control.pin);
      const name = fields.textInput(prefix + "-name", control.name, "e.g. Porch Light") as HTMLInputElement;
      name.maxLength = 32;
      const inverted = fields.toggleRow("Active low", prefix + "-inverted", control.inverted);
      section.append(title, fields.fieldLabel("Name", name.id), name,
        fields.fieldLabel("GPIO pin", pin.id), pin, inverted.row);
      controls.append(section); inputs.push({ pin, name, inverted: inverted.input });
    });
  }).catch(error => { onVisibilityChange(true); status.textContent = (error as Error).message; })
    .finally(() => { busy = false; sync(); });
  return body;
}
