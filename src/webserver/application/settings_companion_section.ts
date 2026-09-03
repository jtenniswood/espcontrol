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

export function companionPairingStatusText(state: CompanionPairingState): string {
    if (state.active && state.pairing_code) {
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

export function formatCompanionPairingDetails(host: string, state: CompanionPairingState): string {
    const mdnsName = String(state.mdns_name || "").trim().replace(/\.$/, "");
    const panelHost = mdnsName || host;
    const panel = state.port && state.port !== 8443 ? panelHost + ":" + state.port : panelHost;
    return [
        "EspControl Companion pairing",
        "Panel: " + panel,
        "Pairing code: " + state.pairing_code,
    ].join("\n");
}

export function createSettingsCompanionSectionFeature(
    dom: Pick<ApplicationDomServices, "document" | "window" | "fetch">,
    shell: Pick<ControlsShellFeature, "createActionButton" | "showBanner">,
    fields: Pick<ControlsFieldsFeature, "makeCollapsibleCard">,
): SettingsCompanionSectionFeature {
    const { document, window, fetch } = dom;

    async function requestPairing(method: "GET" | "POST"): Promise<CompanionPairingState> {
        const options: RequestInit = {
            method,
            cache: "no-store",
            headers: { Accept: "application/json" },
        };
        if (method === "POST") options.body = "";
        const response = await fetch("/companion/pairing", options);
        if (!response.ok) throw new Error("Companion pairing is not available on this panel");
        return await response.json() as CompanionPairingState;
    }

    async function copyText(value: string): Promise<void> {
        if (window.navigator.clipboard && window.isSecureContext) {
            await window.navigator.clipboard.writeText(value);
            return;
        }
        const textarea = document.createElement("textarea");
        textarea.value = value;
        textarea.setAttribute("readonly", "");
        textarea.style.position = "fixed";
        textarea.style.opacity = "0";
        document.body.appendChild(textarea);
        textarea.select();
        const copied = document.execCommand("copy");
        textarea.remove();
        if (!copied) throw new Error("Copy was blocked by the browser");
    }

    function buildCompanionSettingsCard(onStatus?: (state: CompanionPairingState) => void): HTMLElement {
        const body = document.createElement("div");
        const note = document.createElement("p");
        note.className = "sp-setting-note sp-companion-note";
        note.textContent = "Pair this display with EspControl Companion on a trusted local network. The setup code expires after 15 minutes or as soon as the Mac connects; the trusted pairing remains saved across reboots.";
        body.appendChild(note);

        const steps = document.createElement("ol");
        steps.className = "sp-connector-steps";
        [
            "Open EspControl Companion on your Mac and select the Device tab.",
            "Select Start pairing below, then copy the pairing details.",
            "In the Mac app, select Paste pairing details and then Pair.",
        ].forEach(function (text) {
            const item = document.createElement("li");
            item.textContent = text;
            steps.appendChild(item);
        });
        body.appendChild(steps);

        const status = document.createElement("div");
        status.className = "sp-companion-status";
        status.textContent = "Checking Companion status…";
        status.setAttribute("role", "status");
        status.setAttribute("aria-live", "polite");
        body.appendChild(status);

        const details = document.createElement("div");
        details.className = "sp-companion-details sp-hidden";
        const pairingRow = document.createElement("div");
        pairingRow.className = "sp-companion-code-row";
        pairingRow.innerHTML = '<span>Pairing code</span><strong class="sp-companion-code"></strong>';
        details.appendChild(pairingRow);
        body.appendChild(details);

        const actions = document.createElement("div");
        actions.className = "sp-backup-btns sp-companion-actions";
        const startButton = shell.createActionButton("sp-backup-btn", "Start pairing", "link");
        const copyButton = shell.createActionButton("sp-backup-btn", "Copy pairing details", "copy");
        copyButton.classList.add("sp-hidden");
        actions.appendChild(startButton);
        actions.appendChild(copyButton);
        body.appendChild(actions);

        let current: CompanionPairingState | null = null;
        const pairingValue = pairingRow.querySelector("strong") as HTMLElement;

        function render(value: CompanionPairingState): void {
            current = value;
            if (onStatus) onStatus(value);
            status.textContent = companionPairingStatusText(value);
            status.classList.toggle("sp-companion-status-connected", value.connected);
            if (value.active && value.pairing_code) {
                pairingValue.textContent = value.pairing_code;
                details.classList.remove("sp-hidden");
                copyButton.classList.remove("sp-hidden");
                startButton.textContent = "Generate new code";
                return;
            }
            details.classList.add("sp-hidden");
            copyButton.classList.add("sp-hidden");
            startButton.textContent = "Start pairing";
        }

        startButton.addEventListener("click", async function () {
            startButton.disabled = true;
            try {
                render(await requestPairing("POST"));
            } catch (error) {
                shell.showBanner(error instanceof Error ? error.message : "Could not start pairing", "error");
            } finally {
                startButton.disabled = false;
            }
        });
        copyButton.addEventListener("click", async function () {
            if (!current || !current.active) return;
            try {
                await copyText(formatCompanionPairingDetails(window.location.hostname, current));
                shell.showBanner("Pairing details copied. Paste them into the Mac app.", "success");
            } catch (error) {
                shell.showBanner(error instanceof Error ? error.message : "Could not copy pairing details", "error");
            }
        });

        async function refreshStatus(): Promise<void> {
            try {
                render(await requestPairing("GET"));
            } catch {
                status.textContent = "Companion connection status unavailable";
                status.classList.remove("sp-companion-status-connected");
            }
        }

        requestPairing("GET").then(render).catch(function () {
            status.textContent = "Companion pairing is unavailable";
            startButton.disabled = true;
        });
        const card = fields.makeCollapsibleCard("Mac Companion", body, true);
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
