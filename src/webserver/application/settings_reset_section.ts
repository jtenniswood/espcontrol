import { resetSession, type ResetMode } from "../api/reset_session";

function confirmCompleteReset(): Promise<boolean> {
  return new Promise(resolve => {
    const dialog = document.createElement("dialog");
    dialog.className = "sp-reset-dialog sp-reset-confirm";
    dialog.setAttribute("aria-labelledby", "sp-reset-confirm-title");
    dialog.setAttribute("aria-describedby", "sp-reset-confirm-message");
    const heading = document.createElement("h2");
    heading.id = "sp-reset-confirm-title";
    heading.textContent = "Complete reset";
    const message = document.createElement("p");
    message.id = "sp-reset-confirm-message";
    message.textContent = "Remove all existing configuration and return this display to first-time setup.";
    const warning = document.createElement("div");
    warning.className = "sp-reset-warning";
    const icon = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    icon.setAttribute("viewBox", "0 0 24 24");
    icon.setAttribute("aria-hidden", "true");
    icon.innerHTML = '<circle cx="12" cy="12" r="9"/><path d="M12 7v6m0 4h.01"/>';
    const warningText = document.createElement("p");
    warningText.id = "sp-reset-confirm-warning";
    warningText.textContent = "Saved Wi-Fi credentials and the Home Assistant API key will be erased. Wi-Fi compiled into the firmware will stay disabled. Use Wi-Fi setup to save your network again.";
    warning.append(icon, warningText);
    const backupNote = document.createElement("p");
    backupNote.textContent = "Save a backup first. This reset cannot be undone.";
    dialog.setAttribute("aria-describedby", "sp-reset-confirm-message sp-reset-confirm-warning");
    const form = document.createElement("form");
    const field = document.createElement("div");
    field.className = "sp-field";
    const label = document.createElement("label");
    label.className = "sp-field-label";
    label.htmlFor = "sp-reset-confirm-input";
    label.textContent = "Type RESET to confirm";
    const input = document.createElement("input");
    input.id = label.htmlFor;
    input.className = "sp-input";
    input.type = "text";
    input.autocomplete = "off";
    input.setAttribute("autocapitalize", "off");
    input.spellcheck = false;
    input.autofocus = true;
    field.append(label, input);
    const actions = document.createElement("div");
    actions.className = "sp-btn-row sp-reset-confirm-actions";
    const cancel = document.createElement("button");
    cancel.type = "button";
    cancel.className = "sp-action-btn sp-cancel-btn";
    cancel.textContent = "Cancel";
    cancel.onclick = () => dialog.close("cancel");
    const confirm = document.createElement("button");
    confirm.type = "submit";
    confirm.className = "sp-action-btn sp-reset-danger";
    confirm.textContent = "Complete reset";
    confirm.disabled = true;
    input.addEventListener("input", () => { confirm.disabled = input.value !== "RESET"; });
    form.addEventListener("submit", event => {
      event.preventDefault();
      if (input.value === "RESET") dialog.close("confirm");
    });
    dialog.addEventListener("close", () => {
      dialog.remove();
      resolve(dialog.returnValue === "confirm" && input.value === "RESET");
    }, { once: true });
    actions.append(cancel, confirm);
    form.append(field, actions);
    dialog.append(heading, message, warning, backupNote, form);
    document.body.append(dialog);
    dialog.showModal();
  });
}

export function buildResetSettings(exportBackup: () => void, makeCard: (title: string, body: HTMLElement, collapsed: boolean) => HTMLElement, infoPanel: (id: string, text: string) => HTMLElement): HTMLElement {
  const body = document.createElement("div");
  const card = makeCard("Factory Reset", body, true);
  card.hidden = true;
  const banner = infoPanel("sp-reset-backup-info", "Backup your device before resetting");
  banner.classList.add("sp-reset-backup-info");
  const note = banner.lastElementChild as HTMLElement;
  const backup = document.createElement("button");
  backup.type = "button";
  backup.className = "sp-action-btn sp-save-btn";
  backup.textContent = "Save backup";
  backup.onclick = exportBackup;
  banner.append(backup);
  body.append(banner);
  const options = document.createElement("div");
  options.className = "sp-reset-options";
  body.append(options);
  const session = resetSession();
  function addAction(mode: ResetMode, label: string, description: string): void {
    const panel = document.createElement("section");
    panel.className = "sp-panel sp-reset-option";
    const title = document.createElement("h4");
    title.id = "sp-reset-option-" + mode;
    title.textContent = label;
    panel.setAttribute("aria-labelledby", title.id);
    const text = document.createElement("p");
    text.textContent = description;
    const button = document.createElement("button");
    button.type = "button";
    button.className = mode === "factory" ? "sp-backup-btn sp-reset-danger" : "sp-backup-btn";
    button.textContent = label;
    button.onclick = async () => {
      const warning = description + (description.endsWith(".") ? " " : ". ") + "Settings cannot be recovered without a backup.";
      if (mode === "factory" ? !await confirmCompleteReset() : !window.confirm(warning)) return;
      const dialog = document.createElement("dialog");
      dialog.className = "sp-reset-dialog";
      dialog.setAttribute("aria-labelledby", "sp-reset-status-title");
      dialog.setAttribute("aria-describedby", "sp-reset-status-message");
      dialog.addEventListener("cancel", event => event.preventDefault());
      const heading = document.createElement("h2");
      heading.id = "sp-reset-status-title";
      heading.textContent = "Requesting reset…";
      const message = document.createElement("p");
      message.id = "sp-reset-status-message";
      message.setAttribute("role", "status");
      message.textContent = "Waiting for the display to accept the reset.";
      dialog.append(heading, message);
      document.body.append(dialog);
      dialog.showModal();
      try {
        await session.reset(mode);
        heading.textContent = "Restarting…";
        if (mode === "factory") {
          message.textContent = "Follow the display's Wifi setup instructions to reconnect. You may need to set up its Home Assistant connection again.";
        } else {
          message.textContent = "Your Wifi and Home Assistant connection will be retained. This page will reload when the display is ready.";
          const started = Date.now();
          const poll = async () => {
            try { if (await session.restarted()) { window.location.reload(); return; } } catch (_) { /* Restart disconnects HTTP. */ }
            if (Date.now() - started < 120000) window.setTimeout(poll, 2000);
            else {
              heading.textContent = "Waiting for the display";
              message.textContent = "The display has not reconnected yet. Check its screen and reload this page when it is ready. Do not restore a backup until reset has completed.";
            }
          };
          window.setTimeout(poll, 2000);
        }
      } catch (error) {
        heading.textContent = "Check the display";
        message.textContent = String((error as Error).message) + " Check the display: the reset may already be restarting it. Reload this page before making further changes.";
      }
    };
    panel.append(title, text, button);
    options.append(panel);
  }
  void session.discover().then(status => {
    if (!status) return;
    card.hidden = false;
    if (status.pending) {
      note.textContent = "A reset is pending. Wait for the display to restart before making changes.";
      backup.disabled = true;
      return;
    }
    if (status.modes.includes("customization")) addAction("customization", "Partial reset", "Reset cards and preferences. Retains your configuration for Wifi and Home Assistant");
    if (status.modes.includes("factory")) addAction("factory", "Complete reset", "Remove all existing configuration and return this display to first-time setup.");
  }).catch(() => { /* Leave unavailable actions hidden; write transport fails closed. */ });
  return card;
}
