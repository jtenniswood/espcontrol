import { configOptionEnabled, setConfigOption } from "../model/config_primitives";
import { cardTransferOwnsSubpage } from "../model/card_transfer";

export const COMPANION_APP_SHORTCUTS_OPTION = "app_shortcuts";
export const SAFARI_BUNDLE_ID = "com.apple.Safari";
export const CODEX_BUNDLE_ID = "com.openai.codex";
export const SLACK_BUNDLE_ID = "com.tinyspeck.slackmacgap";
export const COMPANION_SHORTCUT_PREFIX = "shortcut.";
const COMPANION_SHORTCUT_FOLDER_APPS: Readonly<Record<string, string>> = {
    [SAFARI_BUNDLE_ID]: "Safari",
    [CODEX_BUNDLE_ID]: "Codex",
    [SLACK_BUNDLE_ID]: "Slack",
};
const COMPANION_SHORTCUT_MODIFIERS = new Set(["command", "control", "option", "shift"]);
const COMPANION_SHORTCUT_KEYS = new Set([
    "space", "enter", "tab", "escape", "delete", "forwarddelete",
    "left", "right", "up", "down", "home", "end", "pageup", "pagedown",
    "keycomma", "keyperiod", "keyslash", "keysemicolon", "keyquote", "keybackslash",
    "keyminus", "keyequal", "keybracketleft", "keybracketright", "keybackquote",
]);

export interface CompanionShortcutPresetCard {
    entity: string;
    label: string;
    icon: string;
    icon_on: string;
    sensor: string;
    unit: string;
    type: string;
    precision: string;
    options: string;
}

export function companionShortcutFolderAppLabel(bundleIdentifier: unknown): string {
    return typeof bundleIdentifier === "string"
        ? COMPANION_SHORTCUT_FOLDER_APPS[bundleIdentifier] || ""
        : "";
}

export function companionAppShortcutFolderEnabled(card: any): boolean {
    return !!card && card.type === "companion" && !!companionShortcutFolderAppLabel(card.entity) &&
        !card.sensor &&
        configOptionEnabled(card.options, COMPANION_APP_SHORTCUTS_OPTION);
}

export function normalizeCompanionAppShortcutOptions(card: any): string {
    if (!card || card.type !== "companion" || !companionShortcutFolderAppLabel(card.entity) || card.sensor) return "";
    return setConfigOption(
        "",
        COMPANION_APP_SHORTCUTS_OPTION,
        configOptionEnabled(card.options, COMPANION_APP_SHORTCUTS_OPTION),
    );
}

export function setCompanionAppShortcutFolderEnabled(card: any, enabled: boolean): void {
    if (!card) return;
    card.options = setConfigOption(
        card.options,
        COMPANION_APP_SHORTCUTS_OPTION,
        enabled && card.type === "companion" && !!companionShortcutFolderAppLabel(card.entity) && !card.sensor,
    );
}

export function cardOwnsSubpage(card: any): boolean {
    return !!card && cardTransferOwnsSubpage(card);
}

export function companionShortcutFolderEditorAvailable(draftCard: any, savedCard: any): boolean {
    return companionAppShortcutFolderEnabled(draftCard) && companionAppShortcutFolderEnabled(savedCard);
}

export function companionShortcutActionIdValid(actionId: unknown): boolean {
    if (typeof actionId !== "string" || !actionId.startsWith(COMPANION_SHORTCUT_PREFIX)) return false;
    const parts = actionId.slice(COMPANION_SHORTCUT_PREFIX.length).split("+");
    if (parts.length < 2 || parts.length > 5) return false;
    const key = parts.pop() || "";
    const modifiers = parts;
    if (new Set(modifiers).size !== modifiers.length ||
        modifiers.some((modifier) => !COMPANION_SHORTCUT_MODIFIERS.has(modifier)) ||
        !modifiers.some((modifier) => modifier === "command" || modifier === "control" || modifier === "option")) {
        return false;
    }
    if (/^[a-z0-9]$/.test(key) || COMPANION_SHORTCUT_KEYS.has(key)) return true;
    return /^f(?:[1-9]|1[0-9]|20)$/.test(key);
}

function shortcutCard(entity: string, label: string, icon: string): CompanionShortcutPresetCard {
    return {
        entity,
        label,
        icon,
        icon_on: "Auto",
        sensor: "",
        unit: "",
        type: "companion",
        precision: "",
        options: "",
    };
}

export function safariShortcutPresetCards(): CompanionShortcutPresetCard[] {
    return [
        shortcutCard("shortcut.command+keybracketleft", "Back", "Chevron Left"),
        shortcutCard("shortcut.command+keybracketright", "Forward", "Chevron Right"),
        shortcutCard("shortcut.command+r", "Reload", "Repeat"),
        shortcutCard("shortcut.command+t", "New Tab", "Plus"),
        shortcutCard("shortcut.command+w", "Close Tab", "Close"),
    ];
}

export function codexShortcutPresetCards(): CompanionShortcutPresetCard[] {
    return [
        shortcutCard("shortcut.command+k", "Command Menu", "Application"),
        shortcutCard("shortcut.command+o", "Open Folder", "Folder Outline"),
        shortcutCard("shortcut.command+b", "Toggle Sidebar", "View Headline"),
        shortcutCard("shortcut.command+j", "Toggle Bottom Panel", "Monitor"),
        shortcutCard("shortcut.control+keybackquote", "Toggle Terminal", "Application"),
    ];
}

export function slackShortcutPresetCards(): CompanionShortcutPresetCard[] {
    return [
        shortcutCard("shortcut.command+n", "Compose Message", "Message Video"),
        shortcutCard("shortcut.command+g", "Search", "Spotlight"),
        shortcutCard("shortcut.command+shift+k", "Browse DMs", "Account"),
        shortcutCard("shortcut.command+j", "Jump to Unread", "Bell"),
        shortcutCard("shortcut.command+shift+a", "All Unreads", "View Headline"),
    ];
}

export function companionShortcutPresetCards(bundleIdentifier: string): CompanionShortcutPresetCard[] {
    if (bundleIdentifier === SAFARI_BUNDLE_ID) return safariShortcutPresetCards();
    if (bundleIdentifier === CODEX_BUNDLE_ID) return codexShortcutPresetCards();
    if (bundleIdentifier === SLACK_BUNDLE_ID) return slackShortcutPresetCards();
    return [];
}

export function createCompanionShortcutSubpage(bundleIdentifier: string): any {
    return {
        order: ["B", "1", "2", "3", "4", "5"],
        buttons: companionShortcutPresetCards(bundleIdentifier),
        grid: [],
        sizes: {},
        backLabel: "Back",
    };
}

export function createSafariShortcutSubpage(): any {
    return createCompanionShortcutSubpage(SAFARI_BUNDLE_ID);
}

export function createCodexShortcutSubpage(): any {
    return createCompanionShortcutSubpage(CODEX_BUNDLE_ID);
}

export function createSlackShortcutSubpage(): any {
    return createCompanionShortcutSubpage(SLACK_BUNDLE_ID);
}
