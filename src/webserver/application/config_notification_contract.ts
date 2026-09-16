import { configOptionValue, setConfigOptionValue } from "../model/config_primitives";
import {
    NOTIFICATION_ACK_ACTION_OPTION,
    NOTIFICATION_LEVEL_ATTRIBUTE_OPTION,
    NOTIFICATION_MESSAGE_ATTRIBUTE_OPTION,
    cardContractOptionDefaultValue,
} from "./config_option_core";

// Notification cards show a message from a Home Assistant entity and colour
// themselves from a severity attribute on that same entity. Level names are
// matched loosely so common template sensors work without extra setup.
export const NOTIFICATION_LEVEL_INFORMATION = "information";
export const NOTIFICATION_LEVEL_WARNING = "warning";
export const NOTIFICATION_LEVEL_ALERT = "alert";

const NOTIFICATION_WARNING_ALIASES = ["warning", "warn", "caution", "medium"];
const NOTIFICATION_ALERT_ALIASES = ["alert", "critical", "error", "danger", "severe", "high"];
const NOTIFICATION_ACTION_PATTERN = /^[a-z0-9_]+\.[a-z0-9_]+$/;

function trimmed(value?: unknown): string {
    return String(value == null ? "" : value).trim();
}

export function defaultNotificationLevelAttribute(): string {
    return cardContractOptionDefaultValue(
        "notification", NOTIFICATION_LEVEL_ATTRIBUTE_OPTION, "notification_level");
}

export function normalizeNotificationLevel(value?: unknown): string {
    const level = trimmed(value).toLowerCase();
    if (NOTIFICATION_ALERT_ALIASES.indexOf(level) >= 0) return NOTIFICATION_LEVEL_ALERT;
    if (NOTIFICATION_WARNING_ALIASES.indexOf(level) >= 0) return NOTIFICATION_LEVEL_WARNING;
    return NOTIFICATION_LEVEL_INFORMATION;
}

export function notificationLevelAttribute(button?: any): string {
    const value = trimmed(configOptionValue(button && button.options, NOTIFICATION_LEVEL_ATTRIBUTE_OPTION));
    return value || defaultNotificationLevelAttribute();
}

export function notificationMessageAttribute(button?: any): string {
    return trimmed(configOptionValue(button && button.options, NOTIFICATION_MESSAGE_ATTRIBUTE_OPTION));
}

export function notificationAckAction(button?: any): string {
    return trimmed(configOptionValue(button && button.options, NOTIFICATION_ACK_ACTION_OPTION));
}

// A Home Assistant action is only callable as "domain.service".
export function notificationAckActionValid(action?: unknown): boolean {
    return NOTIFICATION_ACTION_PATTERN.test(trimmed(action));
}

export function normalizeNotificationOptions(options?: unknown): string {
    let out = "";
    const levelAttribute = trimmed(configOptionValue(options, NOTIFICATION_LEVEL_ATTRIBUTE_OPTION));
    if (levelAttribute && levelAttribute !== defaultNotificationLevelAttribute()) {
        out = setConfigOptionValue(out, NOTIFICATION_LEVEL_ATTRIBUTE_OPTION, levelAttribute);
    }
    const messageAttribute = trimmed(configOptionValue(options, NOTIFICATION_MESSAGE_ATTRIBUTE_OPTION));
    if (messageAttribute) {
        out = setConfigOptionValue(out, NOTIFICATION_MESSAGE_ATTRIBUTE_OPTION, messageAttribute);
    }
    const ackAction = trimmed(configOptionValue(options, NOTIFICATION_ACK_ACTION_OPTION)).toLowerCase();
    if (notificationAckActionValid(ackAction)) {
        out = setConfigOptionValue(out, NOTIFICATION_ACK_ACTION_OPTION, ackAction);
    }
    return out;
}

export function setNotificationLevelAttribute(button?: any, value?: unknown): string {
    if (!button) return "";
    button.options = setConfigOptionValue(
        button.options || "", NOTIFICATION_LEVEL_ATTRIBUTE_OPTION, trimmed(value));
    button.options = normalizeNotificationOptions(button.options);
    return button.options;
}

export function setNotificationMessageAttribute(button?: any, value?: unknown): string {
    if (!button) return "";
    button.options = setConfigOptionValue(
        button.options || "", NOTIFICATION_MESSAGE_ATTRIBUTE_OPTION, trimmed(value));
    button.options = normalizeNotificationOptions(button.options);
    return button.options;
}

export function setNotificationAckAction(button?: any, value?: unknown): string {
    if (!button) return "";
    button.options = setConfigOptionValue(
        button.options || "", NOTIFICATION_ACK_ACTION_OPTION, trimmed(value));
    button.options = normalizeNotificationOptions(button.options);
    return button.options;
}
