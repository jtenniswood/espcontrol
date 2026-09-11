import { resetSession, type ResetMode } from "../api/reset_session";

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
    button.className = "sp-backup-btn";
    button.textContent = label;
    button.onclick = async () => {
      const warning = description + (description.endsWith(".") ? " " : ". ") + "Settings cannot be recovered without a backup.";
      if (mode === "factory" ? window.prompt(warning + " Type RESET to continue.") !== "RESET" : !window.confirm(warning)) return;
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
          message.textContent = "Follow the display's Wi-Fi setup instructions to reconnect, or use its network address over Ethernet. You may need to set up its Home Assistant connection again.";
        } else {
          message.textContent = "Your Wi-Fi and Home Assistant connection will be retained. This page will reload when the display is ready.";
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
    if (status.modes.includes("factory")) addAction("factory", "Complete reset", "Remove all existing configuration and reset back to first time setup.");
  }).catch(() => { /* Leave unavailable actions hidden; write transport fails closed. */ });
  return card;
}
