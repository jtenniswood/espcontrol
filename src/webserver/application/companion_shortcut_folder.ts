import { configOptionEnabled, setConfigOption } from "../model/config_primitives";
import { cardTransferOwnsSubpage } from "../model/card_transfer";

export const COMPANION_APP_SHORTCUTS_OPTION = "app_shortcuts";
export const SAFARI_BUNDLE_ID = "com.apple.Safari";
export const COMPANION_SHORTCUT_PREFIX = "shortcut.";
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

export function companionAppShortcutFolderEnabled(card: any): boolean {
    return !!card && card.type === "companion" && card.entity === SAFARI_BUNDLE_ID &&
        !card.sensor &&
        configOptionEnabled(card.options, COMPANION_APP_SHORTCUTS_OPTION);
}

export function normalizeCompanionAppShortcutOptions(card: any): string {
    if (!card || card.type !== "companion" || card.entity !== SAFARI_BUNDLE_ID || card.sensor) return "";
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
        enabled && card.type === "companion" && card.entity === SAFARI_BUNDLE_ID && !card.sensor,
    );
}

export function cardOwnsSubpage(card: any): boolean {
    return !!card && cardTransferOwnsSubpage(card);
}

export function companionShortcutFolderEditorAvailable(draftCard: any, savedCard: any): boolean {
    return companionAppShortcutFolderEnabled(draftCard) && companionAppShortcutFolderEnabled(savedCard);
}

export function companionShortcutFolderParent(buttons: readonly any[], homeSlot: number | null): any | null {
    if (!homeSlot || homeSlot < 1) return null;
    const parent = buttons[homeSlot - 1];
    return companionAppShortcutFolderEnabled(parent) ? parent : null;
}

export function companionShortcutFolderCardAllowed(card: any): boolean {
    return !!card && card.type === "companion" && companionShortcutActionIdValid(card.entity);
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

export function createSafariShortcutSubpage(): any {
    return {
        order: ["B", "1", "2", "3", "4", "5"],
        buttons: safariShortcutPresetCards(),
        grid: [],
        sizes: {},
        backLabel: "Back",
    };
}
