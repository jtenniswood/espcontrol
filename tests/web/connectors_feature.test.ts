import {
  connectorOnboardingComplete,
  homeAssistantConnectorStatusText,
  type ConnectorsStatus,
} from "../../src/webserver/application/connectors_page";

function status(overrides: Partial<ConnectorsStatus> = {}): ConnectorsStatus {
  return {
    onboarding_complete: false,
    home_assistant: {
      available: true,
      configured: false,
      connected: false,
      actions_confirmed: false,
    },
    mac_companion: {
      available: true,
      configured: false,
      paired: false,
      connected: false,
    },
    ...overrides,
  };
}

export function runConnectorsFeatureTests(): void {
  if (connectorOnboardingComplete(status())) {
    throw new Error("An unconfigured display must remain incomplete");
  }
  if (!connectorOnboardingComplete(status({
    home_assistant: {
      available: true,
      configured: true,
      connected: false,
      actions_confirmed: true,
    },
  }))) {
    throw new Error("A configured Home Assistant connector must be complete while offline");
  }
  if (!connectorOnboardingComplete(status({
    mac_companion: {
      available: true,
      configured: true,
      paired: true,
      connected: false,
    },
  }))) {
    throw new Error("A trusted Mac pairing must complete setup while offline");
  }
  const permissionStatus = homeAssistantConnectorStatusText({
    available: true,
    configured: false,
    connected: true,
    actions_confirmed: false,
  });
  if (!permissionStatus.includes("confirm action permission")) {
    throw new Error("Connected Home Assistant setup must still request action permission");
  }
}
