/** Shared shell for card and badge settings, including their content margins. */
export function createSettingsModal(document: Document, close: () => void, closeLabel = "Close settings") {
  const overlay = document.createElement("div");
  overlay.className = "sp-settings-overlay";
  const modal = document.createElement("div");
  modal.className = "sp-settings-modal";
  const closeButton = document.createElement("button");
  closeButton.type = "button";
  closeButton.className = "sp-settings-close";
  closeButton.innerHTML = '<svg class="sp-settings-close-icon" viewBox="0 0 24 24" aria-hidden="true" focusable="false">' +
    '<path d="M19 6.41 17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"></path></svg>';
  closeButton.setAttribute("aria-label", closeLabel);
  closeButton.addEventListener("click", close);
  const config = document.createElement("div");
  config.className = "sp-config";
  modal.append(closeButton, config);
  overlay.appendChild(modal);
  return { overlay, modal, config };
}
