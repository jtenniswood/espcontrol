import {
  companionPairingStatusText,
  formatCompanionPairingDetails,
} from "../../src/webserver/application/settings_companion_section";

export function runCompanionPairingFeatureTests(): void {
  const details = formatCompanionPairingDetails("192.168.6.100", {
    available: true,
    active: true,
    paired: false,
    connected: false,
    expires_in_seconds: 300,
    pairing_code: "ABCD-EFGH",
    verification_code: "1234-5678-90AB",
  });
  const expected = [
    "EspControl Companion pairing",
    "Panel: 192.168.6.100",
    "Pairing code: ABCD-EFGH",
    "Verify code: 1234-5678-90AB",
  ].join("\n");
  if (details !== expected) throw new Error("Companion pairing details must match the Mac paste format");

  const connectedStatus = companionPairingStatusText({
    available: true,
    active: false,
    paired: true,
    connected: true,
    expires_in_seconds: 0,
    pairing_code: "",
    verification_code: "",
  });
  if (connectedStatus !== "Mac Companion connected") {
    throw new Error("Connected Companion status must be clear on the settings page");
  }
}
