import type { ApplicationDomServices } from "./application_context";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { ControlsShellFeature } from "./controls_shell";

export interface CompanionPairingState {
    available: boolean;
    active: boolean;
    paired: boolean;
    connected: boolean;
    expires_in_seconds: number;
    pairing_code: string;
    verification_code: string;
}

export interface SettingsCompanionSectionFeature {
    buildCompanionSettingsCard(): HTMLElement;
}

export function formatCompanionPairingDetails(host: string, state: CompanionPairingState): string {
    return [
        "EspControl Companion pairing",
        "Panel: " + host,
        "Pairing code: " + state.pairing_code,
        "Verify code: " + state.verification_code,
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

    function buildCompanionSettingsCard(): HTMLElement {
        const body = document.createElement("div");
        const note = document.createElement("p");
        note.className = "sp-setting-note sp-companion-note";
        note.textContent = "Start a five-minute pairing session, then copy the details into EspControl Companion on your Mac.";
        body.appendChild(note);

        const status = document.createElement("div");
        status.className = "sp-companion-status";
        status.textContent = "Checking Companion status…";
        body.appendChild(status);

        const details = document.createElement("div");
        details.className = "sp-companion-details sp-hidden";
        const pairingRow = document.createElement("div");
        pairingRow.className = "sp-companion-code-row";
        pairingRow.innerHTML = '<span>Pairing code</span><strong class="sp-companion-code"></strong>';
        const verificationRow = document.createElement("div");
        verificationRow.className = "sp-companion-code-row";
        verificationRow.innerHTML = '<span>Verify code</span><strong class="sp-companion-code"></strong>';
        details.appendChild(pairingRow);
        details.appendChild(verificationRow);
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
        const verificationValue = verificationRow.querySelector("strong") as HTMLElement;

        function render(value: CompanionPairingState): void {
            current = value;
            if (value.active && value.pairing_code && value.verification_code) {
                const minutes = Math.max(1, Math.ceil(value.expires_in_seconds / 60));
                status.textContent = "Pairing is open for about " + minutes + (minutes === 1 ? " minute." : " minutes.");
                pairingValue.textContent = value.pairing_code;
                verificationValue.textContent = value.verification_code;
                details.classList.remove("sp-hidden");
                copyButton.classList.remove("sp-hidden");
                startButton.textContent = "Generate new codes";
                return;
            }
            details.classList.add("sp-hidden");
            copyButton.classList.add("sp-hidden");
            startButton.textContent = "Start pairing";
            status.textContent = value.connected ? "Mac connected" : value.paired ? "Mac paired, but not connected" : "No Mac paired";
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

        requestPairing("GET").then(render).catch(function () {
            status.textContent = "Companion pairing is unavailable";
            startButton.disabled = true;
        });
        return fields.makeCollapsibleCard("Mac Companion", body, true);
    }

    return { buildCompanionSettingsCard };
}
