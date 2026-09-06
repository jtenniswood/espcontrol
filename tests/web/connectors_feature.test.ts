import {
  connectorOnboardingComplete,
  homeAssistantConnectorStatusText,
  requestedConnectorFromSearch,
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
  if (requestedConnectorFromSearch("?tab=connectors") !== "mac_companion") {
    throw new Error("The legacy connectors pairing URL must open Mac Companion");
  }
  if (requestedConnectorFromSearch("?tab=connectors&connector=mac_companion") !== "mac_companion") {
    throw new Error("The explicit Mac Companion URL must open Mac Companion");
  }
  if (requestedConnectorFromSearch("?tab=settings") !== null) {
    throw new Error("Unrelated deep links must not select a connector");
  }
  if (connectorOnboardingComplete(status())) {
    throw new Error("An unconfigured display must remain in onboarding");
  }
  if (!connectorOnboardingComplete(status({
    home_assistant: {
      available: true,
      configured: true,
      connected: false,
      actions_confirmed: true,
    },
  }))) {
    throw new Error("A configured Home Assistant connector must complete onboarding while offline");
  }
  if (!connectorOnboardingComplete(status({
    mac_companion: {
      available: true,
      configured: true,
      paired: true,
      connected: false,
    },
  }))) {
    throw new Error("A trusted Mac pairing must complete onboarding while offline");
  }
  const permissionStatus = homeAssistantConnectorStatusText({
    available: true,
    configured: false,
    connected: true,
    actions_confirmed: false,
  });
  if (permissionStatus !== "Home Assistant connected") {
    throw new Error("Connected Home Assistant setup must not show a confirmation action");
  }
}
