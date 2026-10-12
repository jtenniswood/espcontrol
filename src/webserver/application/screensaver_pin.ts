import type { ControlsFieldsFeature } from "./controls_fields";

export interface ScreensaverPinInfo {
  enabled: boolean;
  has_pin: boolean;
  pin_length: number;
  randomize_keypad?: boolean;
}

interface ScreensaverPinSettings {
  enabled: boolean;
  pin?: string;
  randomize_keypad?: boolean;
}

export async function screensaverPinRequest(
  request: typeof fetch,
  settings?: ScreensaverPinSettings,
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
  return { enabled: info.enabled, has_pin: info.has_pin, pin_length: pinLength,
    ...(typeof info.randomize_keypad === "boolean" ? { randomize_keypad: info.randomize_keypad } : {}) };
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
    if ((response.status === 404 || response.status === 503) && attempt < 3) {
      await new Promise(resolve => setTimeout(resolve, 2000));
      continue;
    }
    if (response.status === 404) return null;
    if (!response.ok) throw new Error("Could not read PIN support. Check the connection and try again.");
    const capabilities = await response.json();
    return capabilities?.screensaver_pin?.version === 1 ? screensaverPinRequest(request) : null;
  }
  return null;
}

export function buildScreensaverPinSettings(
  document: Document, request: typeof fetch,
  fields: Pick<ControlsFieldsFeature, "toggleRow" | "fieldLabel" | "textInput">,
  badge: HTMLElement | null,
  options: { idPrefix?: string; requirePin?: boolean; onVisibilityChange?: (visible: boolean) => void } = {},
): { body: HTMLElement; refresh: () => Promise<void>; pinInput: HTMLInputElement; hasSavedPin: () => boolean } {
  const idPrefix = options.idPrefix ?? "sp-set-screensaver-pin";
  const body = document.createElement("div");
  body.hidden = !options.requirePin;
  body.className = "sp-field";
  body.setAttribute("data-sp-pin-settings", "");
  const toggle = fields.toggleRow("Require PIN after wake", idPrefix + "-enabled", false);
  const enabled = toggle.input as HTMLInputElement;
  const controls = document.createElement("div");
  controls.className = "sp-pin-controls";
  controls.hidden = true;
  const randomizeToggle = fields.toggleRow("Randomize keypad layout", idPrefix + "-randomize", false);
  const randomize = randomizeToggle.input as HTMLInputElement;
  const label = fields.fieldLabel("PIN", idPrefix);
  const pin = fields.textInput(idPrefix, "", "1–16 digits") as HTMLInputElement;
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
  actions.append(save);
  if (!options.requirePin) actions.append(clear);
  row.append(pin, actions);
  const errorNotice = document.createElement("div");
  errorNotice.className = "sp-field-error";
  errorNotice.setAttribute("role", "alert");
  errorNotice.hidden = true;
  const pinField = document.createElement("div");
  pinField.className = "sp-field";
  pinField.append(label, row);
  controls.append(pinField, randomizeToggle.row);
  if (!options.requirePin) body.append(toggle.row);
  body.append(controls, errorNotice);
  let info: ScreensaverPinInfo | null = null;
  let busy = false;
  let loading = true;
  let pendingSetup = false;
  let revision = 0;
  function setVisible(visible: boolean): void {
    body.hidden = !visible;
    options.onVisibilityChange?.(visible);
  }
  function showError(message: string): void {
    errorNotice.textContent = message;
    errorNotice.hidden = !message;
  }
  function sync(): void {
    badge?.classList.toggle("sp-hidden", !(info?.enabled && info.has_pin));
    if (!busy) enabled.checked = pendingSetup || !!(info?.enabled && info.has_pin);
    if (!busy && !pendingSetup) randomize.checked = info?.randomize_keypad ?? false;
    randomizeToggle.row.hidden = typeof info?.randomize_keypad !== "boolean";
    enabled.disabled = randomize.disabled = save.disabled = clear.disabled = pin.disabled = busy || loading || !info;
    clear.disabled = busy || loading || !info?.has_pin;
    pin.placeholder = info?.has_pin ? "•".repeat(info.pin_length) : "1–16 digits";
    controls.hidden = !enabled.checked && !options.requirePin;
    showError("");
  }
  async function update(settings: ScreensaverPinSettings): Promise<void> {
    if (busy || loading || !info) return;
    busy = true;
    sync();
    let errorMessage = "";
    try {
      info = await screensaverPinRequest(request, settings);
      revision++;
      pendingSetup = false;
      if (settings.pin !== undefined) pin.value = "";
      // Both editors manage one device setting. Notify connected controls only;
      // discarded card drafts retain no document-level listeners or PIN copies.
      for (const node of document.querySelectorAll("[data-sp-pin-settings]")) {
        if (node !== body) node.dispatchEvent(new CustomEvent("sp-pin-settings-changed", { detail: info }));
      }
    }
    catch (error) { errorMessage = (error as Error).message; }
    finally {
      busy = false;
      sync();
      pin.dispatchEvent(new Event("change"));
      if (errorMessage) showError(errorMessage);
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
  randomize.addEventListener("change", () => {
    if (!info?.has_pin) return; // Keep the layout choice local until the first PIN is saved.
    void update({ enabled: enabled.checked, randomize_keypad: randomize.checked });
  });
  save.addEventListener("click", () => {
    if (!/^[0-9]{1,16}$/.test(pin.value)) {
      showError(pin.value ? "Enter a PIN containing 1–16 digits." : "Enter a PIN before saving.");
      pin.focus();
      return;
    }
    void update({ enabled: enabled.checked, pin: pin.value,
      ...(typeof info?.randomize_keypad === "boolean" ? { randomize_keypad: randomize.checked } : {}) });
  });
  clear.addEventListener("click", () => { void update({ enabled: false, pin: "" }); });
  async function refresh(): Promise<void> {
    const readRevision = ++revision;
    loading = true;
    try {
      const loadedInfo = await loadScreensaverPinSettings(request);
      if (readRevision !== revision) return;
      info = loadedInfo;
      loading = false;
      pendingSetup = false;
      setVisible(info !== null || !!options.requirePin);
      pin.value = "";
      sync();
      pin.dispatchEvent(new Event("change"));
      if (!info && options.requirePin) showError("Update the panel firmware to set up a Screen Lock PIN.");
    } catch (error) {
      if (readRevision !== revision) return;
      loading = false;
      badge?.classList.add("sp-hidden");
      setVisible(true);
      enabled.disabled = randomize.disabled = save.disabled = clear.disabled = pin.disabled = true;
      showError((error as Error).message);
    }
  }
  body.addEventListener("sp-pin-settings-changed", event => {
    info = (event as CustomEvent<ScreensaverPinInfo>).detail;
    revision++;
    loading = false;
    pendingSetup = false;
    setVisible(true);
    sync();
    pin.dispatchEvent(new Event("change"));
  });
  sync();
  void refresh();
  return { body, refresh, pinInput: pin, hasSavedPin: () => !loading && !busy && !!info?.has_pin };
}
