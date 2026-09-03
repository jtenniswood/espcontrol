import type { ApplicationDomServices } from "./application_context";
import type { ControlsFieldsFeature } from "./controls_fields";
import type { ControlsShellFeature } from "./controls_shell";
import type {
    CompanionPairingState,
    SettingsCompanionSectionFeature,
} from "./settings_companion_section";

export interface ConnectorConnectionState {
    available: boolean;
    configured: boolean;
    connected: boolean;
}

export interface HomeAssistantConnectorState extends ConnectorConnectionState {
    actions_confirmed: boolean;
}

export interface MacCompanionConnectorState extends ConnectorConnectionState {
    paired: boolean;
}

export interface ConnectorsStatus {
    onboarding_complete: boolean;
    home_assistant: HomeAssistantConnectorState;
    mac_companion: MacCompanionConnectorState;
}

export interface ConnectorsPageFeature {
    buildPage(parent: HTMLElement): void;
    start(): void;
}

export function homeAssistantConnectorStatusText(state: HomeAssistantConnectorState): string {
    if (state.connected && state.actions_confirmed) return "Home Assistant connected and actions confirmed";
    if (state.connected) return "Home Assistant connected — confirm action permission below";
    if (state.configured) return "Home Assistant configured, but currently offline";
    return "Waiting for Home Assistant";
}

export function connectorOnboardingComplete(status: ConnectorsStatus): boolean {
    return !!(status.home_assistant.configured || status.mac_companion.paired);
}

export function createConnectorsPageFeature(
    dom: Pick<ApplicationDomServices, "document" | "window" | "fetch">,
    shell: Pick<ControlsShellFeature, "createActionButton" | "showBanner" | "setOnboardingComplete">,
    fields: Pick<ControlsFieldsFeature, "makeCollapsibleCard">,
    companionSection: SettingsCompanionSectionFeature,
    companionSupported: boolean,
): ConnectorsPageFeature {
    const { document, window, fetch } = dom;
    let heading: HTMLElement | null = null;
    let intro: HTMLElement | null = null;
    let homeAssistantStatus: HTMLElement | null = null;
    let confirmButton: HTMLButtonElement | null = null;
    let current: ConnectorsStatus | null = null;
    let timer: number | null = null;
    let refreshInProgress = false;

    function fallbackStatus(): ConnectorsStatus {
        return {
            // Older firmware has no connector endpoint. Keep its established
            // configurator usable rather than trapping it in an unfinishable
            // onboarding screen.
            onboarding_complete: true,
            home_assistant: {
                available: true,
                configured: true,
                connected: false,
                actions_confirmed: true,
            },
            mac_companion: {
                available: companionSupported,
                configured: false,
                paired: false,
                connected: false,
            },
        };
    }

    async function requestStatus(): Promise<ConnectorsStatus> {
        const response = await fetch("/connectors/status", {
            cache: "no-store",
            headers: { Accept: "application/json" },
        });
        if (!response.ok) throw new Error("Connector status is unavailable");
        return await response.json() as ConnectorsStatus;
    }

    function applyStatus(value: ConnectorsStatus): void {
        value.onboarding_complete = connectorOnboardingComplete(value);
        const previous = current;
        const wasComplete = previous?.onboarding_complete === true;
        const announceCompletion = !!previous && !wasComplete && value.onboarding_complete;
        current = value;
        if (homeAssistantStatus) {
            homeAssistantStatus.textContent = homeAssistantConnectorStatusText(value.home_assistant);
            homeAssistantStatus.classList.toggle(
                "sp-connector-status-connected", value.home_assistant.connected);
        }
        if (confirmButton) {
            confirmButton.disabled = !value.home_assistant.connected ||
                value.home_assistant.actions_confirmed;
            confirmButton.textContent = value.home_assistant.actions_confirmed
                ? "Actions confirmed"
                : "I've enabled actions";
        }
        if (heading) {
            heading.textContent = value.onboarding_complete ? "Connectors" : "Connect EspControl";
        }
        if (intro) {
            intro.textContent = value.onboarding_complete
                ? "Manage the services that provide data and actions for this display."
                : companionSupported
                    ? "Choose Home Assistant or Mac Companion to finish setting up your display. You can add the other connector later."
                    : "Connect Home Assistant to finish setting up your display.";
        }
        shell.setOnboardingComplete(value.onboarding_complete, announceCompletion);
    }

    async function refreshStatus(): Promise<void> {
        if (refreshInProgress) return;
        refreshInProgress = true;
        try {
            applyStatus(await requestStatus());
        } catch {
            if (!current) applyStatus(fallbackStatus());
        } finally {
            refreshInProgress = false;
        }
    }

    function buildHomeAssistantCard(): HTMLElement {
        const body = document.createElement("div");
        const note = document.createElement("p");
        note.className = "sp-setting-note";
        note.textContent = "Add this display in Home Assistant, then allow it to perform Home Assistant actions.";
        body.appendChild(note);

        homeAssistantStatus = document.createElement("div");
        homeAssistantStatus.className = "sp-connector-status";
        homeAssistantStatus.setAttribute("role", "status");
        homeAssistantStatus.setAttribute("aria-live", "polite");
        homeAssistantStatus.textContent = "Checking Home Assistant status…";
        body.appendChild(homeAssistantStatus);

        const steps = document.createElement("ol");
        steps.className = "sp-connector-steps";
        [
            "In Home Assistant, open Settings → Devices & services.",
            "Add the discovered EspControl device. If it is not shown, add ESPHome and enter " + window.location.hostname + ".",
            "Open the device configuration and enable ‘Allow the device to perform Home Assistant actions’.",
        ].forEach(function (text) {
            const item = document.createElement("li");
            item.textContent = text;
            steps.appendChild(item);
        });
        body.appendChild(steps);

        confirmButton = shell.createActionButton(
            "sp-backup-btn", "I've enabled actions", "check-circle");
        confirmButton.disabled = true;
        confirmButton.addEventListener("click", async function () {
            if (!confirmButton) return;
            confirmButton.disabled = true;
            try {
                const response = await fetch("/connectors/home-assistant/complete", {
                    method: "POST",
                    cache: "no-store",
                    headers: { Accept: "application/json" },
                    body: "",
                });
                if (!response.ok) throw new Error("Home Assistant must be connected first");
                applyStatus(await response.json() as ConnectorsStatus);
                shell.showBanner("Home Assistant connector configured.", "success");
            } catch (error) {
                shell.showBanner(error instanceof Error ? error.message : "Could not finish Home Assistant setup", "error");
                await refreshStatus();
            }
        });
        const actions = document.createElement("div");
        actions.className = "sp-backup-btns";
        actions.appendChild(confirmButton);
        body.appendChild(actions);
        return fields.makeCollapsibleCard("Home Assistant", body, true);
    }

    function applyCompanionStatus(value: CompanionPairingState): void {
        if (!current) return;
        applyStatus({
            ...current,
            mac_companion: {
                available: value.available,
                configured: value.paired,
                paired: value.paired,
                connected: value.connected,
            },
        });
    }

    function buildPage(parent: HTMLElement): void {
        const page = document.createElement("div");
        page.id = "sp-connectors";
        page.className = "sp-page";
        const config = document.createElement("div");
        config.className = "sp-config sp-connectors-config fade-in";
        heading = document.createElement("h1");
        heading.className = "sp-connectors-heading";
        heading.textContent = "Connect EspControl";
        intro = document.createElement("p");
        intro.className = "sp-connectors-intro";
        intro.textContent = companionSupported
            ? "Choose Home Assistant or Mac Companion to finish setting up your display."
            : "Connect Home Assistant to finish setting up your display.";
        config.appendChild(heading);
        config.appendChild(intro);
        config.appendChild(buildHomeAssistantCard());
        if (companionSupported) {
            config.appendChild(companionSection.buildCompanionSettingsCard(applyCompanionStatus));
        }
        page.appendChild(config);
        parent.appendChild(page);
    }

    function start(): void {
        void refreshStatus();
        if (timer !== null) window.clearInterval(timer);
        timer = window.setInterval(function () { void refreshStatus(); }, 2000);
    }

    return { buildPage, start };
}
