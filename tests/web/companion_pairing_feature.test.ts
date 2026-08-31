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
    expires_in_seconds: 900,
    pairing_code: "ABCD-EFGH",
    mdns_name: "espcontrol-4inch-s3.local",
  });
  const expected = [
    "EspControl Companion pairing",
    "Panel: espcontrol-4inch-s3.local",
    "Pairing code: ABCD-EFGH",
  ].join("\n");
  if (details !== expected) throw new Error("Companion pairing details must match the Mac paste format");

  const openStatus = companionPairingStatusText({
    available: true,
    active: true,
    paired: false,
    connected: false,
    expires_in_seconds: 900,
    pairing_code: "ABCD-EFGH",
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
