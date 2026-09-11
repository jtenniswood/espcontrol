import { normalizePanelName, panelHostname, readIdentityBackup, type PanelIdentityInfo, type PanelIdentityBackup } from "../model/panel_identity";

export interface PanelIdentityFeature {
  load(): Promise<PanelIdentityInfo | null>;
  current(): PanelIdentityInfo | null;
  backup(): PanelIdentityBackup | undefined;
  buildCard(): HTMLElement;
  chooseRestoreName(data: unknown): Promise<string | undefined | null>;
  saveAndRestart(name: string): Promise<void>;
}

export interface PanelIdentityDependencies {
  document: Document;
  fetch: typeof fetch;
  changed: () => void;
  restart: () => Promise<void>;
  beforeSave: () => Promise<void>;
  makeCard: (title: string, body: HTMLElement) => HTMLElement;
  infoPanel: (id: string, text: string) => HTMLElement;
}

export function createPanelIdentityFeature(deps: PanelIdentityDependencies): PanelIdentityFeature {
  let info: PanelIdentityInfo | null = null;
  let loading: Promise<PanelIdentityInfo | null> | null = null;
  let saving = false;
  const document = deps.document;
  async function request(name?: string): Promise<PanelIdentityInfo | null> {
    const response = await deps.fetch("/api/v1/identity", {
      method: name === undefined ? "GET" : "POST", credentials: "include", cache: "no-store",
      ...(name === undefined ? {} : { headers: { "Content-Type": "application/json" }, body: JSON.stringify({ name }) }),
    });
    if (name === undefined && response.status === 404) return null;
    if (!response.ok) throw new Error(name === undefined
      ? "Could not read the panel name. Check the connection and try again."
      : "Could not save the panel name. Check the connection and try again.");
    const value = await response.json() as PanelIdentityInfo;
    if (!value || typeof value.name !== "string" || typeof value.friendly_name !== "string"
        || typeof value.hostname !== "string" || !/^[a-z0-9_-]{1,31}$/.test(value.hostname)
        || typeof value.mac_suffix !== "string" || !/^(?:[a-f0-9]{4}|[a-f0-9]{6})$/.test(value.mac_suffix)
        || typeof value.ip_address !== "string" || typeof value.restart_required !== "boolean") {
      throw new Error("Invalid panel identity response.");
    }
    info = value;
    deps.changed();
    return info;
  }
  function load(): Promise<PanelIdentityInfo | null> {
    if (!loading) loading = (async () => {
      // Discover support before requesting identity so older firmware stays quiet.
      for (let attempt = 0; attempt < 4; attempt++) {
        const response = await deps.fetch("/api/v1/capabilities", { credentials: "include", cache: "no-store" });
        if (response.status === 404) return null;
        if (response.status === 503 && attempt < 3) {
          await new Promise(resolve => setTimeout(resolve, 2000));
          continue;
        }
        if (!response.ok) throw new Error("Could not read panel naming support. Try again.");
        const capabilities = await response.json();
        return capabilities?.identity?.version === 1 ? request() : null;
      }
      return null;
    })().catch(error => { loading = null; throw error; });
    return loading;
  }
  function backup(): PanelIdentityBackup | undefined {
    return info ? { version: 1, name: info.name, hostname: info.hostname, mac_suffix: info.mac_suffix } : undefined;
  }
  function reconnectDialog(value: PanelIdentityInfo): HTMLElement {
    const dialog = document.createElement("dialog");
    dialog.className = "sp-identity-dialog";
    const title = document.createElement("h3");
    title.textContent = "Panel name saved";
    dialog.append(title);
    const note = document.createElement("p");
    note.textContent = "The panel is restarting. Reopen it at the new address. Home Assistant action names may need updating.";
    dialog.append(note);
    const address = document.createElement("a");
    address.href = `http://${value.hostname}.local${location.port ? ":" + location.port : ""}/`;
    address.textContent = `${value.hostname}.local`;
    dialog.append(address);
    // Never build navigation URLs from unvalidated backup or server strings.
    if (/^(?:\d{1,3}\.){3}\d{1,3}$/.test(value.ip_address)) {
      const ip = document.createElement("a");
      ip.href = `http://${value.ip_address}${location.port ? ":" + location.port : ""}/`;
      ip.textContent = value.ip_address;
      dialog.append(document.createElement("br"), ip);
    }
    const close = document.createElement("button");
    close.className = "sp-fw-btn";
    close.textContent = "Close";
    close.onclick = () => dialog.close();
    dialog.append(document.createElement("br"), close);
    dialog.addEventListener("close", () => dialog.remove());
    document.body.append(dialog);
    dialog.showModal();
    return note;
  }
  async function saveAndRestart(value: string): Promise<void> {
    if (saving) throw new Error("A panel name change is already in progress.");
    const name = normalizePanelName(value);
    saving = true;
    try {
      await deps.beforeSave();
      const saved = await request(name);
      if (!saved) throw new Error("Panel naming is unavailable.");
      loading = Promise.resolve(saved);
      if (saved.restart_required) {
        const message = reconnectDialog(saved);
        try { await deps.restart(); }
        catch (error) {
          message.textContent = "Name saved, but the restart failed. Reopen Settings and choose Save and restart to retry.";
          throw error;
        }
      }
    } finally { saving = false; }
  }
  function buildCard(): HTMLElement {
    const body = document.createElement("div");
    const card = deps.makeCard("Panel name", body);
    card.hidden = true;
    const input = document.createElement("input");
    input.className = "sp-input";
    input.id = "sp-panel-name";
    input.placeholder = "e.g. Kitchen";
    const label = document.createElement("label");
    label.className = "sp-field-label";
    label.htmlFor = input.id;
    label.textContent = "Panel name";
    const preview = deps.infoPanel("sp-panel-name-info", "");
    const previewText = preview.lastElementChild!;
    const error = document.createElement("p");
    error.setAttribute("role", "status");
    const button = document.createElement("button");
    button.className = "sp-fw-btn";
    button.textContent = "Save and restart";
    function sync() {
      try {
        const name = normalizePanelName(input.value);
        error.textContent = "";
        const hostname = name && info ? panelHostname(name, info.mac_suffix)
          : info && !info.name ? info.hostname : null;
        if (hostname) {
          const address = document.createElement("code");
          address.textContent = `${hostname}.local`;
          previewText.replaceChildren("Your device will show as ", address, " on your network");
        } else {
          previewText.textContent = "Your device will use its original firmware name and address on your network";
        }
        button.disabled = saving || !info || (name === info.name && !info.restart_required);
      } catch (e) { error.textContent = (e as Error).message; button.disabled = true; }
    }
    input.oninput = sync;
    button.onclick = async () => {
      button.disabled = true;
      input.disabled = true;
      try { await saveAndRestart(input.value); sync(); }
      catch (e) { sync(); error.textContent = (e as Error).message; }
      finally { input.disabled = false; }
    };
    body.append(label, input, button, error, preview);
    function loadCard() {
      void load().then(value => {
        card.hidden = !value;
        if (value) {
          body.replaceChildren(label, input, button, error, preview);
          input.value = value.name;
          sync();
        }
      }).catch(() => {
        card.hidden = false;
        const message = document.createElement("p");
        message.textContent = "Could not read the panel name. Check the connection and try again.";
        const retry = document.createElement("button");
        retry.className = "sp-fw-btn";
        retry.textContent = "Try again";
        retry.onclick = () => { retry.disabled = true; loadCard(); };
        body.replaceChildren(message, retry);
      });
    }
    loadCard();
    return card;
  }
  async function chooseRestoreName(data: unknown): Promise<string | undefined | null> {
    const identity = readIdentityBackup((data as { identity?: unknown } | null)?.identity);
    if (!identity) return undefined;
    let target: PanelIdentityInfo | null;
    try { target = await load(); }
    catch { return undefined; } // Naming is optional; preserve it and restore configuration.
    if (!target) return undefined;
    return new Promise(resolve => {
      const dialog = document.createElement("dialog");
      dialog.className = "sp-identity-dialog";
      const title = document.createElement("h3");
      title.textContent = "Restore backup";
      const label = document.createElement("label");
      const checkbox = document.createElement("input");
      checkbox.type = "checkbox";
      checkbox.checked = false;
      label.append(checkbox, document.createTextNode(" Also restore panel name"));
      const note = document.createElement("p");
      note.textContent = identity.name
        ? `Restore the name “${identity.name}” with this panel's address ${panelHostname(identity.name, target.mac_suffix)}.local. This restarts the panel and changes ESPHome action names.`
        : "Restore this panel's original firmware name and address. This may require a restart.";
      const apply = document.createElement("button");
      apply.className = "sp-fw-btn";
      apply.textContent = "Restore";
      apply.onclick = () => { resolve(checkbox.checked ? identity.name : undefined); dialog.close(); };
      const cancel = document.createElement("button");
      cancel.className = "sp-fw-btn";
      cancel.textContent = "Cancel";
      cancel.onclick = () => dialog.close();
      dialog.addEventListener("close", () => { resolve(null); dialog.remove(); });
      dialog.append(title, label, note, apply, cancel);
      document.body.append(dialog);
      dialog.showModal();
    });
  }
  return { load, current: () => info, backup, buildCard, chooseRestoreName, saveAndRestart };
}
