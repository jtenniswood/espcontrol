import { resetSession, type ResetMode } from "../api/reset_session";

export function buildResetSettings(exportBackup: () => void, makeCard: (title: string, body: HTMLElement, collapsed: boolean) => HTMLElement): HTMLElement {
  const body = document.createElement("div");
  const card = makeCard("Reset", body, true);
  card.hidden = true;
  const note = document.createElement("p");
  note.className = "sp-setting-note";
  note.textContent = "Save a backup first if you want to restore your customization later. Backups do not include network credentials. Both options keep the installed firmware and any defaults compiled into it.";
  body.append(note);
  const backup = document.createElement("button");
  backup.type = "button";
  backup.className = "sp-backup-btn";
  backup.textContent = "Export backup";
  backup.onclick = exportBackup;
  body.append(backup);
  const session = resetSession();
  function addAction(mode: ResetMode, label: string, description: string): void {
    const row = document.createElement("div");
    row.className = "sp-field";
    const text = document.createElement("p");
    text.className = "sp-setting-note";
    text.textContent = description;
    const button = document.createElement("button");
    button.type = "button";
    button.className = "sp-backup-btn";
    button.textContent = label;
    button.onclick = async () => {
      const warning = description + " Settings cannot be recovered without a backup.";
      if (mode === "factory" ? window.prompt(warning + " Type RESET to continue.") !== "RESET" : !window.confirm(warning)) return;
      const dialog = document.createElement("dialog");
      dialog.className = "sp-reset-dialog";
      dialog.setAttribute("aria-label", label);
      dialog.addEventListener("cancel", event => event.preventDefault());
      const message = document.createElement("p");
      message.textContent = "Requesting reset…";
      dialog.append(message);
      document.body.append(dialog);
      dialog.showModal();
      try {
        await session.reset(mode);
        if (mode === "factory") {
          message.textContent = "Factory reset requested. The display is restarting. Follow its Wi-Fi setup instructions to reconnect, or use its network address over Ethernet. You may need to set up its Home Assistant connection again.";
        } else {
          message.textContent = "Restarting… Your Wi-Fi and Home Assistant connection will be retained. This page will reload when the display is ready.";
          const started = Date.now();
          const poll = async () => {
            try { if (await session.restarted()) { window.location.reload(); return; } } catch (_) { /* Restart disconnects HTTP. */ }
            if (Date.now() - started < 120000) window.setTimeout(poll, 2000);
            else message.textContent = "The display has not reconnected yet. Check its screen and reload this page when it is ready. Do not restore a backup until reset has completed.";
          };
          window.setTimeout(poll, 2000);
        }
      } catch (error) {
        message.textContent = String((error as Error).message) + " Check the display: the reset may already be restarting it. Reload this page before making further changes.";
      }
    };
    row.append(text, button);
    body.append(row);
  }
  void session.discover().then(status => {
    if (!status) return;
    card.hidden = false;
    if (status.pending) {
      note.textContent = "A reset is pending. Wait for the display to restart before making changes.";
      backup.disabled = true;
      return;
    }
    if (status.modes.includes("customization")) addAction("customization", "Reset customization", "Remove all cards and device preferences, keeping Wi-Fi and Home Assistant connected. The display will restart into card setup.");
    if (status.modes.includes("factory")) addAction("factory", "Factory reset", "Remove all customization and saved Wi-Fi and Home Assistant credentials. The display will restart into first-time setup.");
  }).catch(() => { /* Leave unavailable actions hidden; write transport fails closed. */ });
  return card;
}
