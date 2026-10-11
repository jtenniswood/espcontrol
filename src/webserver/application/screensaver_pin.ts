import type { ControlsFieldsFeature } from "./controls_fields";

export interface ScreensaverPinInfo {
  enabled: boolean;
  has_pin: boolean;
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
  return { enabled: info.enabled, has_pin: info.has_pin };
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
  const note = document.createElement("p");
  note.className = "sp-note";
  note.textContent = "Protects local touchscreen controls. The PIN is not included in backups. Network and web access use separate security settings.";
  body.append(toggle.row, label, row, status, note);
  let info: ScreensaverPinInfo | null = null;
  let busy = false;
  function sync(): void {
    if (!busy) enabled.checked = !!info?.enabled;
    enabled.disabled = save.disabled = clear.disabled = pin.disabled = busy;
    clear.disabled = busy || !info?.has_pin;
    pin.placeholder = info?.has_pin ? "••••••••" : "1–16 digits";
    statusMessage.textContent = info?.has_pin ? "PIN saved. Leave the field blank to keep it." : "No PIN set. Touchscreen PIN protection is inactive.";
  }
  async function update(settings: { enabled: boolean; pin?: string }): Promise<void> {
    if (busy) return;
    busy = true;
    sync();
    let errorMessage = "";
    try { info = await screensaverPinRequest(request, settings); }
    catch (error) { errorMessage = (error as Error).message; }
    finally {
      pin.value = "";
      busy = false;
      sync();
      if (errorMessage) statusMessage.textContent = errorMessage;
    }
  }
  enabled.addEventListener("change", () => { void update({ enabled: enabled.checked }); });
  save.addEventListener("click", () => {
    if (!/^[0-9]{1,16}$/.test(pin.value)) {
      statusMessage.textContent = "Enter a PIN containing 1–16 digits.";
      return;
    }
    void update({ enabled: enabled.checked, pin: pin.value });
  });
  clear.addEventListener("click", () => { void update({ enabled: false, pin: "" }); });
  async function refresh(): Promise<void> {
    try {
      info = await loadScreensaverPinSettings(request);
      body.hidden = info === null;
      pin.value = "";
      sync();
    } catch (error) {
      body.hidden = false;
      enabled.disabled = save.disabled = clear.disabled = pin.disabled = true;
      statusMessage.textContent = (error as Error).message;
    }
  }
  void refresh();
  return { body, refresh };
}
