import { normalizePowerMode, normalizeScreensaverAction } from "../model/settings";

export interface ScreensaverControlState {
  readonly mode: string;
  readonly clockVisible: boolean;
  readonly dimVisible: boolean;
  readonly dayBrightnessLabel: string;
  readonly nightBrightnessLabel: string;
  readonly dimBrightnessLabel: string;
}

export interface PowerModeControlState {
  readonly mode: string;
  readonly disabled: boolean;
  readonly description: string;
}

export function powerModeControlState(mode: unknown, supported: boolean): PowerModeControlState {
  const normalized = normalizePowerMode(mode);
  if (!supported) {
    return {
      mode: normalized,
      disabled: true,
      description: "Update the device firmware to use Power Mode.",
    };
  }
  if (normalized === "Battery Saver") {
    return {
      mode: normalized,
      disabled: false,
      description: "Wi-Fi light sleep and reduced display-only work are active. For the best battery life, select Display Off below. Brightness, voice services and relays remain under your control.",
    };
  }
  return {
    mode: normalized,
    disabled: false,
    description: "Normal keeps the current responsive Wi-Fi behaviour. Changing this mode never changes your brightness, screensaver, voice or relay settings.",
  };
}

export interface SettingsUiDependencies {
  readonly document: Document;
  readonly textSpan: (text: string, className?: string) => HTMLElement;
  readonly createDisclosureChevron: (className: string) => HTMLElement;
}

export interface SettingsUiFeature {
  settingsStatusHeader(title: string): HTMLElement;
  appendSettingsSection(parent: HTMLElement, title: string, cards: readonly (HTMLElement | null | undefined)[]): void;
  infoPanel(id: string, text: string): HTMLElement;
  statusBadge(label: string, text?: string): HTMLElement;
  disclosureBadge(text: string, label?: string): HTMLElement;
  inlineDisclosure(title: string, bodyElement: HTMLElement, defaultOpen: boolean, badgeElement?: HTMLElement): HTMLElement;
}

export function screensaverControlState(
  action: unknown,
  dayBrightness: number,
  nightBrightness: number,
  dimBrightness: number,
): ScreensaverControlState {
  const mode = normalizeScreensaverAction(action);
  return {
    mode,
    clockVisible: mode === "clock",
    dimVisible: mode === "dim",
    dayBrightnessLabel: `${Math.round(dayBrightness)}%`,
    nightBrightnessLabel: `${Math.round(nightBrightness)}%`,
    dimBrightnessLabel: `${Math.round(dimBrightness)}%`,
  };
}

export function timedSettingLabel(value: unknown, formatDuration: (seconds: number) => string): string {
  const seconds = Number(value);
  if (seconds < 0) return "Always";
  if (seconds > 0) return formatDuration(seconds);
  return "Never";
}

export function createSettingsUiFeature(dependencies: SettingsUiDependencies): SettingsUiFeature {
  const { document, textSpan, createDisclosureChevron } = dependencies;

  function settingsStatusHeader(title: string): HTMLElement {
    const header = document.createElement("div");
    header.className = "sp-settings-status-header";
    const label = document.createElement("div");
    label.className = "sp-settings-status-title";
    label.textContent = title;
    header.appendChild(label);
    return header;
  }

  return {
    settingsStatusHeader,
    appendSettingsSection(parent, title, cards) {
      const visibleCards = cards.filter((card): card is HTMLElement => !!card);
      if (!visibleCards.length) return;
      parent.appendChild(settingsStatusHeader(title));
      visibleCards.forEach((card) => parent.appendChild(card));
    },
    infoPanel(id, text) {
      const panel = document.createElement("div");
      panel.className = "sp-info-panel";
      panel.id = id;
      panel.setAttribute("role", "note");
      const icon = document.createElement("span");
      icon.className = "mdi mdi-information-outline";
      icon.setAttribute("aria-hidden", "true");
      const message = document.createElement("span");
      message.textContent = text;
      panel.appendChild(icon);
      panel.appendChild(message);
      return panel;
    },
    statusBadge(label, text = "ON") {
      const badge = document.createElement("span");
      badge.setAttribute("aria-label", label);
      badge.appendChild(textSpan("", "sp-card-badge-dot"));
      badge.appendChild(textSpan(text));
      return badge;
    },
    disclosureBadge(text, label) {
      const badge = document.createElement("span");
      badge.className = "sp-disclosure-badge";
      badge.setAttribute("aria-label", label || text);
      badge.appendChild(textSpan("", "sp-disclosure-badge-dot"));
      badge.appendChild(textSpan(text));
      return badge;
    },
    inlineDisclosure(title, bodyElement, defaultOpen, badgeElement) {
      const panel = document.createElement("div");
      panel.className = `sp-disclosure${defaultOpen ? " sp-open" : ""}`;
      const button = document.createElement("button");
      button.type = "button";
      button.className = "sp-disclosure-button";
      button.setAttribute("aria-expanded", defaultOpen ? "true" : "false");
      const label = document.createElement("span");
      label.textContent = title;
      const rightWrap = document.createElement("span");
      rightWrap.className = "sp-disclosure-header-right";
      const chevron = createDisclosureChevron("sp-disclosure-chevron");
      button.appendChild(label);
      if (badgeElement) rightWrap.appendChild(badgeElement);
      rightWrap.appendChild(chevron);
      button.appendChild(rightWrap);
      const body = document.createElement("div");
      body.className = "sp-disclosure-body";
      body.appendChild(bodyElement);
      button.addEventListener("click", () => {
        const open = !panel.classList.contains("sp-open");
        panel.classList.toggle("sp-open", open);
        button.setAttribute("aria-expanded", open ? "true" : "false");
      });
      panel.appendChild(button);
      panel.appendChild(body);
      return panel;
    },
  };
}
