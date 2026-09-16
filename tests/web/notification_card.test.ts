import {
  NOTIFICATION_LEVEL_ALERT,
  NOTIFICATION_LEVEL_INFORMATION,
  NOTIFICATION_LEVEL_WARNING,
  defaultNotificationLevelAttribute,
  normalizeNotificationLevel,
  normalizeNotificationOptions,
  notificationAckAction,
  notificationAckActionValid,
  notificationLevelAttribute,
  notificationMessageAttribute,
} from "../../src/webserver/application/config_notification_contract";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

// These expectations mirror the firmware assertions in
// scripts/check_firmware_parser.py so both sides read the same saved config.
export function runNotificationCardTests(): void {
  equal(normalizeNotificationLevel(""), NOTIFICATION_LEVEL_INFORMATION, "missing level");
  equal(normalizeNotificationLevel("information"), NOTIFICATION_LEVEL_INFORMATION, "information level");
  equal(normalizeNotificationLevel("unknown level"), NOTIFICATION_LEVEL_INFORMATION, "unrecognised level");
  for (const alias of ["  Warning ", "warn", "caution", "medium"]) {
    equal(normalizeNotificationLevel(alias), NOTIFICATION_LEVEL_WARNING, `warning alias ${alias}`);
  }
  for (const alias of ["ALERT", "critical", "error", "danger", "severe", "high"]) {
    equal(normalizeNotificationLevel(alias), NOTIFICATION_LEVEL_ALERT, `alert alias ${alias}`);
  }

  equal(defaultNotificationLevelAttribute(), "notification_level", "contract level attribute default");
  equal(notificationLevelAttribute({ options: "" }), "notification_level", "missing level attribute");
  equal(
    notificationLevelAttribute({ options: "level_attribute=severity" }),
    "severity",
    "custom level attribute",
  );
  equal(
    notificationMessageAttribute({ options: "message_attribute= detail " }),
    "detail",
    "trimmed message attribute",
  );
  equal(notificationMessageAttribute({ options: "" }), "", "message attribute defaults to the state");
  equal(notificationAckAction({ options: "ack_action=script.ack" }), "script.ack", "acknowledge action");

  if (!notificationAckActionValid("script.ack_notice")) throw new Error("domain.service must be accepted");
  for (const action of ["script", "script.ack.notice", "Script.Ack", "not-an-action", ""]) {
    if (notificationAckActionValid(action)) throw new Error(`${action}: must be rejected`);
  }

  equal(
    normalizeNotificationOptions("level_attribute=notification_level"),
    "",
    "the default level attribute is omitted",
  );
  equal(
    normalizeNotificationOptions("ack_action=Script.Ack_Notice ,message_attribute= detail "),
    "message_attribute=detail,ack_action=script.ack_notice",
    "options are reordered, trimmed, and lowercased",
  );
  equal(
    normalizeNotificationOptions("ack_action=not-an-action,unknown=1"),
    "",
    "unusable actions and unknown options are dropped",
  );
  equal(
    normalizeNotificationOptions(normalizeNotificationOptions("level_attribute=severity")),
    "level_attribute=severity",
    "normalization is stable across a second pass",
  );
}
