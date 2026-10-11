import type { ControlsFieldsFeature } from "./controls_fields";

export interface ScreensaverPinInfo {
  enabled: boolean;
  has_pin: boolean;
  pin_length: number;
}

export async function screensaverPinRequest(
  request: typeof fetch,
  settings?: { enabled: boolean; pin?: string },
): Promise<ScreensaverPinInfo | null> {
  const response = await request("/api/v1/screensaver-pin", {
    method: settings ? "POST" : "GET", credentials: "include", cache: "no-store",
    ...(settings ? { headers: { "Content-Type": "application/json" }, body: JSON.stringify(settings) } : {}),
  });
  if (!settings && response.status === 404) return null;
  if (!response.ok) throw new Error("Could not save or read PIN settings. Check the connection and try again.");
  const info = await response.json();
  if (typeof info?.enabled !== "boolean" || typeof info?.has_pin !== "boolean") {
    throw new Error("Invalid PIN settings response.");
  }
  // Fall back for firmware that supports PIN settings but predates pin_length.
  const pinLength = Number.isInteger(info.pin_length) && info.pin_length >= 0 && info.pin_length <= 16
    ? info.pin_length
    : info.has_pin ? 8 : 0;
  return { enabled: info.enabled, has_pin: info.has_pin, pin_length: pinLength };
}

// Backups never carry the secret. Import also clears the destination's old PIN,
// so a restored panel always requires a fresh setup through its own web UI.
export async function clearScreensaverPinForRestore(request: typeof fetch): Promise<void> {
  if (await loadScreensaverPinSettings(request)) {
    await screensaverPinRequest(request, { enabled: false, pin: "" });
  }
}

export async function loadScreensaverPinSettings(request: typeof fetch): Promise<ScreensaverPinInfo | null> {
  for (let attempt = 0; attempt < 4; attempt++) {
    const response = await request("/api/v1/capabilities", { credentials: "include", cache: "no-store" });
    if (response.status === 404) return null;
    if (response.status === 503 && attempt < 3) {
      await new Promise(resolve => setTimeout(resolve, 2000));
      continue;
    }
    if (!response.ok) throw new Error("Could not read PIN support. Check the connection and try again.");
    const capabilities = await response.json();
    return capabilities?.screensaver_pin?.version === 1 ? screensaverPinRequest(request) : null;
  }
  return null;
}

export function buildScreensaverPinSettings(
  document: Document, request: typeof fetch,
  fields: Pick<ControlsFieldsFeature, "toggleRow" | "fieldLabel" | "textInput">,
): { body: HTMLElement; refresh: () => Promise<void> } {
  const body = document.createElement("div");
  body.hidden = true;
  body.className = "sp-field";
  const toggle = fields.toggleRow("Require PIN after wake", "sp-set-screensaver-pin-enabled", false);
  const enabled = toggle.input as HTMLInputElement;
  const controls = document.createElement("div");
  controls.className = "sp-pin-controls";
  controls.hidden = true;
  const label = fields.fieldLabel("PIN", "sp-set-screensaver-pin");
  const pin = fields.textInput("sp-set-screensaver-pin", "", "1–16 digits") as HTMLInputElement;
  pin.type = "password";
  pin.inputMode = "numeric";
  pin.maxLength = 16;
  pin.autocomplete = "new-password";
  pin.placeholder = "1–16 digits";
  const save = document.createElement("button");
  save.type = "button";
  save.className = "sp-fw-btn";
  save.textContent = "Save";
  const clear = document.createElement("button");
  clear.type = "button";
  clear.className = "sp-fw-btn";
  clear.textContent = "Clear";
  const row = document.createElement("div");
  row.className = "sp-pin-row";
  const actions = document.createElement("div");
  actions.className = "sp-pin-actions";
  actions.append(save, clear);
  row.append(pin, actions);
  const status = document.createElement("div");
  status.className = "sp-info-panel";
  status.setAttribute("role", "status");
  const statusIcon = document.createElement("span");
  statusIcon.className = "mdi mdi-information-outline";
  statusIcon.setAttribute("aria-hidden", "true");
  const statusMessage = document.createElement("span");
  status.append(statusIcon, statusMessage);
  controls.append(label, row, status);
  body.append(toggle.row, controls);
  let info: ScreensaverPinInfo | null = null;
  let busy = false;
  let pendingSetup = false;
  function showStatus(message: string, error = false): void {
    status.setAttribute("role", error ? "alert" : "status");
    statusMessage.textContent = message;
  }
  function sync(): void {
    if (!busy) enabled.checked = pendingSetup || !!(info?.enabled && info.has_pin);
    enabled.disabled = save.disabled = clear.disabled = pin.disabled = busy;
    clear.disabled = busy || !info?.has_pin;
    pin.placeholder = info?.has_pin ? "•".repeat(info.pin_length) : "1–16 digits";
    controls.hidden = !enabled.checked;
    showStatus(pendingSetup ? "Enter and save a PIN to enable protection." :
      info?.has_pin ? "PIN saved." : "No PIN set. Touchscreen PIN protection is inactive.");
  }
  async function update(settings: { enabled: boolean; pin?: string }): Promise<void> {
    if (busy) return;
    busy = true;
    sync();
    let errorMessage = "";
    try {
      info = await screensaverPinRequest(request, settings);
      pendingSetup = false;
      pin.value = "";
    }
    catch (error) { errorMessage = (error as Error).message; }
    finally {
      busy = false;
      sync();
      if (errorMessage) showStatus(errorMessage, true);
    }
  }
  enabled.addEventListener("change", () => {
    if (!info?.has_pin) {
      pendingSetup = enabled.checked;
      sync();
      if (pendingSetup) pin.focus();
      else pin.value = "";
      return;
    }
    void update({ enabled: enabled.checked });
  });
  save.addEventListener("click", () => {
    if (!/^[0-9]{1,16}$/.test(pin.value)) {
      showStatus(pin.value ? "Enter a PIN containing 1–16 digits." : "Enter a PIN before saving.", true);
      pin.focus();
      return;
    }
    void update({ enabled: enabled.checked, pin: pin.value });
  });
  clear.addEventListener("click", () => { void update({ enabled: false, pin: "" }); });
  async function refresh(): Promise<void> {
    try {
      info = await loadScreensaverPinSettings(request);
      pendingSetup = false;
      body.hidden = info === null;
      pin.value = "";
      sync();
    } catch (error) {
      body.hidden = false;
      enabled.disabled = save.disabled = clear.disabled = pin.disabled = true;
      showStatus((error as Error).message, true);
    }
  }
  void refresh();
  return { body, refresh };
}
