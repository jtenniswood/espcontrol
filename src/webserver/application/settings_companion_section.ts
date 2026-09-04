import type { ApplicationDomServices } from "./application_context";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { ControlsShellFeature } from "./controls_shell";

export interface CompanionPairingState {
    available: boolean;
    active: boolean;
    paired: boolean;
    connected: boolean;
    expires_in_seconds: number;
    port?: number;
    pairing_code: string;
    mdns_name?: string;
}

export interface SettingsCompanionSectionFeature {
    buildCompanionSettingsCard(onStatus?: (state: CompanionPairingState) => void): HTMLElement;
}

function setHidden(element: HTMLElement, hidden: boolean): void {
    element.hidden = hidden;
    element.classList.toggle("sp-hidden", hidden);
}

export function companionPairingStatusText(state: CompanionPairingState): string {
    if (state.active) {
        const hours = Math.ceil(state.expires_in_seconds / 3600);
        const minutes = Math.max(1, Math.ceil(state.expires_in_seconds / 60));
        const duration = hours >= 2
            ? hours + (hours === 1 ? " hour" : " hours")
            : minutes + (minutes === 1 ? " minute" : " minutes");
        const pairing = "Pairing is open for about " + duration + ".";
        return state.connected ? "Mac Companion connected. " + pairing : pairing;
    }
    if (state.connected) return "Mac Companion connected";
    return state.paired ? "Mac paired, but not connected" : "No Mac paired";
}

export function createSettingsCompanionSectionFeature(
    dom: Pick<ApplicationDomServices, "document" | "window" | "fetch">,
    _shell: Pick<ControlsShellFeature, "createActionButton" | "showBanner">,
    fields: Pick<ControlsFieldsFeature, "makeCollapsibleCard">,
): SettingsCompanionSectionFeature {
    const { document, window, fetch } = dom;

    async function requestPairing(): Promise<CompanionPairingState> {
        const options: RequestInit = {
            method: "GET",
            cache: "no-store",
            headers: { Accept: "application/json" },
        };
        const response = await fetch("/companion/pairing", options);
        if (!response.ok) throw new Error("Companion pairing is not available on this panel");
        return await response.json() as CompanionPairingState;
    }

    function buildCompanionSettingsCard(onStatus?: (state: CompanionPairingState) => void): HTMLElement {
        const body = document.createElement("div");
        const instructions = document.createElement("div");
        instructions.className = "sp-connector-instructions";
        const note = document.createElement("p");
        note.className = "sp-setting-note sp-companion-note";
        note.textContent = "Pairing requires physical access to the display. The setup code expires after 15 minutes or as soon as the Mac connects; the trusted pairing remains saved across reboots.";
        instructions.appendChild(note);

        const steps = document.createElement("ol");
        steps.className = "sp-connector-steps";
        [
            "Press and hold the Wi-Fi icon on the display to show a pairing code.",
            "Open EspControl Companion on your Mac and select the Device tab.",
            "Enter the display address and the code shown on the display, then select Pair.",
        ].forEach(function (text) {
            const item = document.createElement("li");
            item.textContent = text;
            steps.appendChild(item);
        });
        instructions.appendChild(steps);
        body.appendChild(instructions);

        const status = document.createElement("div");
        status.className = "sp-companion-status";
        status.textContent = "Checking Companion status…";
        status.setAttribute("role", "status");
        status.setAttribute("aria-live", "polite");
        body.appendChild(status);

        const badge = document.createElement("span");
        badge.className = "sp-card-badge sp-hidden";
        const badgeDot = document.createElement("span");
        badgeDot.className = "sp-card-badge-dot";
        badge.appendChild(badgeDot);
        badge.appendChild(document.createTextNode("ON"));

        function render(value: CompanionPairingState): void {
            if (onStatus) onStatus(value);
            status.textContent = companionPairingStatusText(value);
            status.classList.toggle("sp-companion-status-connected", value.connected);
            setHidden(instructions, value.connected);
            setHidden(badge, !value.paired);
        }

        async function refreshStatus(): Promise<void> {
            try {
                render(await requestPairing());
            } catch {
                status.textContent = "Companion connection status unavailable";
                status.classList.remove("sp-companion-status-connected");
            }
        }

        requestPairing().then(render).catch(function () {
            status.textContent = "Companion pairing is unavailable";
        });
        const card = fields.makeCollapsibleCard("Mac Companion", body, true, badge);
        let refreshInProgress = false;
        const refreshTimer = window.setInterval(async function () {
            if (!card.isConnected) {
                window.clearInterval(refreshTimer);
                return;
            }
            if (refreshInProgress) return;
            refreshInProgress = true;
            try {
                await refreshStatus();
            } finally {
                refreshInProgress = false;
            }
        }, 2000);
        return card;
    }

    return { buildCompanionSettingsCard };
}
