import {
  companionPairingStatusText,
} from "../../src/webserver/application/settings_companion_section";

export function runCompanionPairingFeatureTests(): void {
  const openStatus = companionPairingStatusText({
    available: true,
    active: true,
    paired: false,
    connected: false,
    expires_in_seconds: 900,
    pairing_code: "",
  });
  if (openStatus !== "Pairing is open for about 15 minutes.") {
    throw new Error("Companion setup should describe the extended pairing window");
  }

  const connectedStatus = companionPairingStatusText({
    available: true,
    active: false,
    paired: true,
    connected: true,
    expires_in_seconds: 0,
    pairing_code: "",
  });
  if (connectedStatus !== "Mac Companion connected") {
    throw new Error("Connected Companion status must be clear on the settings page");
  }
}
