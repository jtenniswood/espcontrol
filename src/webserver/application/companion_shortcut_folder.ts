import {
    configOptionEnabled,
    configOptionValue,
    setConfigOption,
    setConfigOptionValue,
} from "../model/config_primitives";
import { cardTransferOwnsSubpage } from "../model/card_transfer";
import { isBackOrderToken } from "../model/subpage";

export const COMPANION_APP_SHORTCUTS_OPTION = "app_shortcuts";
export const COMPANION_APP_SHORTCUTS_AUTO_SWITCH_OPTION = "app_shortcuts_auto_switch";
export const COMPANION_APP_SHORTCUTS_TABS_OPTION = "app_shortcuts_tabs";
export const COMPANION_SHORTCUT_PRESET_OPTION = "app_shortcut_preset";
export const SAFARI_BUNDLE_ID = "com.apple.Safari";
export const CODEX_BUNDLE_ID = "com.openai.codex";
export const SLACK_BUNDLE_ID = "com.tinyspeck.slackmacgap";
export const COMPANION_SHORTCUT_PREFIX = "shortcut.";
const COMPANION_SHORTCUT_FOLDER_APPS: Readonly<Record<string, string>> = {
    [SAFARI_BUNDLE_ID]: "Safari",
    [CODEX_BUNDLE_ID]: "Codex",
    [SLACK_BUNDLE_ID]: "Slack",
};
const COMPANION_SHORTCUT_PRESET_COUNTS: Readonly<Record<string, number>> = {
    [SAFARI_BUNDLE_ID]: 5,
    [CODEX_BUNDLE_ID]: 7,
    [SLACK_BUNDLE_ID]: 5,
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

export interface CompanionShortcutTabDefinition {
    value: string;
    label: string;
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

export function companionAppShortcutAutoSwitchEnabled(card: any): boolean {
    return companionAppShortcutFolderEnabled(card) &&
        configOptionEnabled(card.options, COMPANION_APP_SHORTCUTS_AUTO_SWITCH_OPTION);
}

export function normalizeCompanionAppShortcutOptions(card: any): string {
    if (!card || card.type !== "companion") return "";
    const presetIdentity = companionShortcutPresetIdentity(card);
    if (presetIdentity && companionShortcutActionIdValid(card.entity)) {
        return setConfigOptionValue("", COMPANION_SHORTCUT_PRESET_OPTION, presetIdentity);
    }
    if (!companionShortcutFolderAppLabel(card.entity) || card.sensor) return "";
    const options = setConfigOption(
        "",
        COMPANION_APP_SHORTCUTS_OPTION,
        configOptionEnabled(card.options, COMPANION_APP_SHORTCUTS_OPTION),
    );
    const flags = setConfigOption(
        options,
        COMPANION_APP_SHORTCUTS_AUTO_SWITCH_OPTION,
        configOptionEnabled(card.options, COMPANION_APP_SHORTCUTS_OPTION) &&
            configOptionEnabled(card.options, COMPANION_APP_SHORTCUTS_AUTO_SWITCH_OPTION),
    );
    if (!configOptionEnabled(flags, COMPANION_APP_SHORTCUTS_OPTION)) return flags;
    const tabs = companionShortcutTabs(card);
    const defaults = companionShortcutDefaultTabs(card.entity);
    const value = tabs.length === 0 ? "none" :
        tabs.join("|") === defaults.join("|") ? "" : tabs.join("|");
    return setConfigOptionValue(flags, COMPANION_APP_SHORTCUTS_TABS_OPTION, value);
}

export function setCompanionAppShortcutFolderEnabled(card: any, enabled: boolean): void {
    if (!card) return;
    const valid = enabled && card.type === "companion" &&
        !!companionShortcutFolderAppLabel(card.entity) && !card.sensor;
    let options = setConfigOption(
        card.options,
        COMPANION_APP_SHORTCUTS_OPTION,
        valid,
    );
    if (!valid) {
        options = setConfigOption(options, COMPANION_APP_SHORTCUTS_AUTO_SWITCH_OPTION, false);
        options = setConfigOptionValue(options, COMPANION_APP_SHORTCUTS_TABS_OPTION, "");
    }
    card.options = options;
}

export function setCompanionAppShortcutAutoSwitchEnabled(card: any, enabled: boolean): void {
    if (!card) return;
    card.options = setConfigOption(
        card.options,
        COMPANION_APP_SHORTCUTS_AUTO_SWITCH_OPTION,
        enabled && companionAppShortcutFolderEnabled(card),
    );
}

export function companionShortcutTabDefinitions(bundleIdentifier: string): CompanionShortcutTabDefinition[] {
    return companionShortcutPresetCards(bundleIdentifier).map(function (card, index) {
        return { value: String(index), label: card.label };
    });
}

export function companionShortcutDefaultTabs(bundleIdentifier: string): string[] {
    return companionShortcutTabDefinitions(bundleIdentifier).map(function (definition) {
        return definition.value;
    });
}

export function companionShortcutTabs(card: any): string[] {
    const defaults = companionShortcutDefaultTabs(card?.entity || "");
    const raw = configOptionValue(card?.options, COMPANION_APP_SHORTCUTS_TABS_OPTION);
    if (!raw) return defaults;
    if (raw === "none") return [];
    const valid = new Set(defaults);
    const tabs: string[] = [];
    raw.split("|").forEach(function (value) {
        if (valid.has(value) && tabs.indexOf(value) < 0) tabs.push(value);
    });
    return tabs;
}

export function setCompanionShortcutTabs(card: any, tabs: readonly string[]): string {
    if (!card) return "";
    const defaults = companionShortcutDefaultTabs(card.entity);
    const valid = new Set(defaults);
    const normalized: string[] = [];
    (tabs || []).forEach(function (value) {
        if (valid.has(value) && normalized.indexOf(value) < 0) normalized.push(value);
    });
    const value = normalized.length === 0 ? "none" :
        normalized.join("|") === defaults.join("|") ? "" : normalized.join("|");
    card.options = setConfigOptionValue(card.options, COMPANION_APP_SHORTCUTS_TABS_OPTION, value);
    return card.options;
}

export function resetCompanionShortcutTabs(card: any): void {
    if (!card) return;
    card.options = setConfigOptionValue(card.options, COMPANION_APP_SHORTCUTS_TABS_OPTION, "");
}

export function cardOwnsSubpage(card: any): boolean {
    return !!card && cardTransferOwnsSubpage(card);
}

export function companionShortcutFolderEditorAvailable(draftCard: any, savedCard: any): boolean {
    return companionAppShortcutFolderEnabled(draftCard) && companionAppShortcutFolderEnabled(savedCard);
}

export function companionShortcutSelectionMatchesSavedParent(draftCard: any, savedCard: any): boolean {
    return companionAppShortcutFolderEnabled(draftCard) &&
        !!savedCard && savedCard.type === "companion" && !savedCard.sensor &&
        draftCard.entity === savedCard.entity;
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

function companionShortcutPresetKey(bundleIdentifier: string, index: number): string {
    return bundleIdentifier + ":" + String(index);
}

function markCompanionShortcutPresets(
    bundleIdentifier: string,
    cards: CompanionShortcutPresetCard[],
): CompanionShortcutPresetCard[] {
    return cards.map(function (card, index) {
        return {
            ...card,
            options: setConfigOptionValue(
                card.options,
                COMPANION_SHORTCUT_PRESET_OPTION,
                companionShortcutPresetKey(bundleIdentifier, index),
            ),
        };
    });
}

function companionShortcutPresetIdentity(card: any): string {
    const raw = configOptionValue(card?.options, COMPANION_SHORTCUT_PRESET_OPTION);
    const match = raw.match(/^(.*):(\d+)$/);
    if (!match) return "";
    const bundleIdentifier = match[1] || "";
    const index = Number.parseInt(match[2] || "", 10);
    const count = COMPANION_SHORTCUT_PRESET_COUNTS[bundleIdentifier] || 0;
    return Number.isInteger(index) && index >= 0 && index < count
        ? companionShortcutPresetKey(bundleIdentifier, index)
        : "";
}

export function safariShortcutPresetCards(): CompanionShortcutPresetCard[] {
    return markCompanionShortcutPresets(SAFARI_BUNDLE_ID, [
        shortcutCard("shortcut.command+keybracketleft", "Back", "Chevron Left"),
        shortcutCard("shortcut.command+keybracketright", "Forward", "Chevron Right"),
        shortcutCard("shortcut.command+r", "Reload", "Repeat"),
        shortcutCard("shortcut.command+t", "New Tab", "Plus"),
        shortcutCard("shortcut.command+w", "Close Tab", "Close"),
    ]);
}

export function codexShortcutPresetCards(): CompanionShortcutPresetCard[] {
    return markCompanionShortcutPresets(CODEX_BUNDLE_ID, [
        shortcutCard("shortcut.command+k", "Command", "Application"),
        shortcutCard("shortcut.command+enter", "Approve", "Check"),
        shortcutCard("shortcut.command+t", "Browser", "Web"),
        shortcutCard("shortcut.command+b", "Sidebar", "View Headline"),
        shortcutCard("shortcut.option+command+b", "Side panel", "Tab"),
        shortcutCard("shortcut.command+j", "Terminal", "Application"),
        shortcutCard("shortcut.control+keybackquote", "Terminal", "Application"),
    ]);
}

export function slackShortcutPresetCards(): CompanionShortcutPresetCard[] {
    return markCompanionShortcutPresets(SLACK_BUNDLE_ID, [
        shortcutCard("shortcut.command+n", "Compose", "Message Video"),
        shortcutCard("shortcut.command+g", "Search", "Spotlight"),
        shortcutCard("shortcut.command+shift+k", "DMs", "Account"),
        shortcutCard("shortcut.command+j", "Unread", "Bell"),
        shortcutCard("shortcut.command+shift+a", "All Unread", "View Headline"),
    ]);
}

export function companionShortcutPresetCards(bundleIdentifier: string): CompanionShortcutPresetCard[] {
    if (bundleIdentifier === SAFARI_BUNDLE_ID) return safariShortcutPresetCards();
    if (bundleIdentifier === CODEX_BUNDLE_ID) return codexShortcutPresetCards();
    if (bundleIdentifier === SLACK_BUNDLE_ID) return slackShortcutPresetCards();
    return [];
}

export function createCompanionShortcutSubpage(bundleIdentifier: string, tabs?: readonly string[]): any {
    const presets = companionShortcutPresetCards(bundleIdentifier);
    const selected = tabs == null ? companionShortcutDefaultTabs(bundleIdentifier) : tabs;
    const buttons = selected.map(function (value) {
        return presets[Number.parseInt(value, 10)];
    }).filter(Boolean);
    return {
        order: ["B"].concat(buttons.map((_button, index) => String(index + 1))),
        buttons,
        grid: [],
        sizes: {},
        backLabel: "Back",
    };
}

function subpageOrderButtonIndex(token: unknown): number {
    const match = String(token || "").match(/^(\d+)/);
    return match ? Number.parseInt(match[1] || "0", 10) - 1 : -1;
}

function subpageOrderButtonSuffix(token: unknown): string {
    const match = String(token || "").match(/^\d+(.*)$/);
    return match ? (match[1] || "") : "";
}

function cardHasContent(card: any): boolean {
    return !!card && ["entity", "label", "sensor", "unit", "type", "precision", "options"].some(function (field) {
        return !!card[field];
    });
}

export function companionShortcutTabsFromSubpage(
    bundleIdentifier: string,
    subpage: any,
): string[] {
    const presets = companionShortcutPresetCards(bundleIdentifier);
    const presetIndex = new Map(presets.map(function (card, index) {
        return [companionShortcutPresetKey(bundleIdentifier, index), String(index)] as const;
    }));
    const legacyPresetIndex = new Map(presets.map(function (card, index) {
        return [card.entity, String(index)] as const;
    }));
    const tabs: string[] = [];
    const visited = new Set<number>();
    (subpage?.order || []).forEach(function (token: unknown) {
        const index = subpageOrderButtonIndex(token);
        if (index < 0 || visited.has(index)) return;
        visited.add(index);
        const card = subpage?.buttons?.[index];
        const identity = companionShortcutPresetIdentity(card);
        const value = identity ? presetIndex.get(identity) : legacyPresetIndex.get(card?.entity);
        if (value != null && tabs.indexOf(value) < 0) tabs.push(value);
    });
    (subpage?.buttons || []).forEach(function (card: any, index: number) {
        if (visited.has(index)) return;
        const identity = companionShortcutPresetIdentity(card);
        const value = identity ? presetIndex.get(identity) : legacyPresetIndex.get(card?.entity);
        if (value != null && tabs.indexOf(value) < 0) tabs.push(value);
    });
    return tabs;
}

export function syncCompanionShortcutSubpage(
    bundleIdentifier: string,
    tabs: readonly string[],
    subpage: any,
): any {
    if (!subpage) return createCompanionShortcutSubpage(bundleIdentifier, tabs);
    const presets = companionShortcutPresetCards(bundleIdentifier);
    const presetKeyByEntity = new Map(presets.map(function (card, index) {
        return [card.entity, companionShortcutPresetKey(bundleIdentifier, index)] as const;
    }));
    function presetKey(card: any): string {
        const identity = companionShortcutPresetIdentity(card);
        if (identity) {
            return identity.startsWith(bundleIdentifier + ":") ? identity : "";
        }
        return presetKeyByEntity.get(card?.entity) || "";
    }
    const existingByKey = new Map<string, any>();
    (subpage.buttons || []).forEach(function (card: any) {
        const key = presetKey(card);
        if (key && !existingByKey.has(key)) {
            card.options = setConfigOptionValue(card.options, COMPANION_SHORTCUT_PRESET_OPTION, key);
            existingByKey.set(key, card);
        }
    });
    function managedPresetKey(card: any): string {
        const key = presetKey(card);
        return key && existingByKey.get(key) === card ? key : "";
    }
    const desired = tabs.map(function (value) {
        const index = Number.parseInt(value, 10);
        const preset = presets[index];
        const key = companionShortcutPresetKey(bundleIdentifier, index);
        return preset ? { key, card: existingByKey.get(key) || preset } : null;
    }).filter(Boolean);
    const suffixByKey = new Map<string, string>();
    (subpage.order || []).forEach(function (token: unknown) {
        const index = subpageOrderButtonIndex(token);
        const key = index >= 0 ? managedPresetKey(subpage.buttons?.[index]) : "";
        if (key && !suffixByKey.has(key)) suffixByKey.set(key, subpageOrderButtonSuffix(token));
    });
    const newButtons: any[] = [];
    const newOrder: string[] = [];
    const visited = new Set<number>();
    let desiredIndex = 0;
    (subpage.order || []).forEach(function (token: unknown) {
        const rawToken = String(token || "");
        if (isBackOrderToken(rawToken) || !rawToken) {
            newOrder.push(rawToken);
            return;
        }
        const index = subpageOrderButtonIndex(token);
        if (index < 0) {
            newOrder.push(rawToken);
            return;
        }
        visited.add(index);
        const card = subpage.buttons?.[index];
        if (!cardHasContent(card)) {
            newOrder.push("");
            return;
        }
        if (managedPresetKey(card)) {
            if (desiredIndex < desired.length) {
                const entry: any = desired[desiredIndex++];
                newButtons.push(entry.card);
                newOrder.push(String(newButtons.length) + (suffixByKey.get(entry.key) || ""));
            } else {
                newOrder.push("");
            }
            return;
        }
        newButtons.push(card);
        newOrder.push(String(newButtons.length) + subpageOrderButtonSuffix(token));
    });
    (subpage.buttons || []).forEach(function (card: any, index: number) {
        if (visited.has(index) || !cardHasContent(card) || managedPresetKey(card)) return;
        newButtons.push(card);
        newOrder.push(String(newButtons.length));
    });
    for (let index = 0; index < newOrder.length && desiredIndex < desired.length; index += 1) {
        if (newOrder[index]) continue;
        const entry: any = desired[desiredIndex++];
        newButtons.push(entry.card);
        newOrder[index] = String(newButtons.length) + (suffixByKey.get(entry.key) || "");
    }
    while (desiredIndex < desired.length) {
        const entry: any = desired[desiredIndex++];
        newButtons.push(entry.card);
        newOrder.push(String(newButtons.length) + (suffixByKey.get(entry.key) || ""));
    }
    if (!newOrder.some(function (token) { return isBackOrderToken(token); })) newOrder.unshift("B");
    subpage.buttons = newButtons;
    subpage.order = newOrder;
    subpage.sizes = {};
    return subpage;
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
