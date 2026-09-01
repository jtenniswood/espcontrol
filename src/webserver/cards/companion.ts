import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";

interface CompanionAction {
    readonly id: string;
    readonly label: string;
}

interface CompanionWindowAction {
    readonly id: string;
    readonly label: string;
    readonly group: string;
}

const COMPANION_SHORTCUT_PREFIX = "shortcut.";
const COMPANION_URL_PREFIX = "url.";
const COMPANION_WINDOW_PREFIX = "window.";
export const COMPANION_WINDOW_ACTIONS: readonly CompanionWindowAction[] = [
    { id: "window.close", label: "Close", group: "Window" },
    { id: "window.minimize", label: "Minimise", group: "Window" },
    { id: "window.hide", label: "Hide App", group: "Window" },
    { id: "window.fullscreen", label: "Full Screen", group: "Window" },
    { id: "window.fill", label: "Fill Desktop", group: "Move & Resize" },
    { id: "window.center", label: "Centre", group: "Move & Resize" },
    { id: "window.left", label: "Left", group: "Move & Resize" },
    { id: "window.right", label: "Right", group: "Move & Resize" },
    { id: "window.top", label: "Top", group: "Move & Resize" },
    { id: "window.bottom", label: "Bottom", group: "Move & Resize" },
    { id: "window.restore", label: "Return to Previous Size", group: "Move & Resize" },
    { id: "window.arrange.left-right", label: "Left & Right", group: "Arrange Windows" },
    { id: "window.arrange.right-left", label: "Right & Left", group: "Arrange Windows" },
    { id: "window.arrange.top-bottom", label: "Top & Bottom", group: "Arrange Windows" },
    { id: "window.arrange.bottom-top", label: "Bottom & Top", group: "Arrange Windows" },
    { id: "window.arrange.left-quarters", label: "Left & Quarters", group: "Arrange Windows" },
    { id: "window.arrange.right-quarters", label: "Right & Quarters", group: "Arrange Windows" },
    { id: "window.arrange.top-quarters", label: "Top & Quarters", group: "Arrange Windows" },
    { id: "window.arrange.bottom-quarters", label: "Bottom & Quarters", group: "Arrange Windows" },
];
const COMPANION_SHORTCUT_MODIFIERS = ["command", "control", "option", "shift"] as const;
const COMPANION_SHORTCUT_KEYS: Readonly<Record<string, string>> = {
    Space: "space", Enter: "enter", Tab: "tab", Escape: "escape",
    Backspace: "delete", Delete: "forwarddelete",
    ArrowLeft: "left", ArrowRight: "right", ArrowUp: "up", ArrowDown: "down",
    Home: "home", End: "end", PageUp: "pageup", PageDown: "pagedown",
    Comma: "keycomma", Period: "keyperiod", Slash: "keyslash", Semicolon: "keysemicolon",
    Quote: "keyquote", Backslash: "keybackslash", Minus: "keyminus", Equal: "keyequal",
    BracketLeft: "keybracketleft", BracketRight: "keybracketright", Backquote: "keybackquote",
};
const COMPANION_SHORTCUT_KEY_LABELS: Readonly<Record<string, string>> = {
    space: "Space", enter: "Return", tab: "Tab", escape: "Esc",
    delete: "Delete", forwarddelete: "Forward Delete",
    left: "←", right: "→", up: "↑", down: "↓",
    home: "Home", end: "End", pageup: "Page Up", pagedown: "Page Down",
    keycomma: ",", keyperiod: ".", keyslash: "/", keysemicolon: ";", keyquote: "'",
    keybackslash: "\\", keyminus: "-", keyequal: "=", keybracketleft: "[",
    keybracketright: "]", keybackquote: "`",
};

function companionShortcutKey(code: string): string {
    if (/^Key[A-Z]$/.test(code)) return code.slice(3).toLowerCase();
    if (/^Digit[0-9]$/.test(code)) return code.slice(5);
    if (/^F(?:[1-9]|1[0-9]|20)$/.test(code)) return code.toLowerCase();
    return COMPANION_SHORTCUT_KEYS[code] || "";
}

export function companionShortcutActionId(event: Pick<KeyboardEvent,
    "code" | "metaKey" | "ctrlKey" | "altKey" | "shiftKey">): string {
    const key = companionShortcutKey(event.code);
    if (!key || (!event.metaKey && !event.ctrlKey && !event.altKey)) return "";
    const parts: string[] = [];
    if (event.metaKey) parts.push("command");
    if (event.ctrlKey) parts.push("control");
    if (event.altKey) parts.push("option");
    if (event.shiftKey) parts.push("shift");
    parts.push(key);
    return COMPANION_SHORTCUT_PREFIX + parts.join("+");
}

export function formatCompanionShortcutActionId(actionId: string): string {
    if (!actionId.startsWith(COMPANION_SHORTCUT_PREFIX)) return "";
    const parts = actionId.slice(COMPANION_SHORTCUT_PREFIX.length).split("+");
    const key = parts.pop() || "";
    if (!key || !parts.length || parts.some((part) =>
        !(COMPANION_SHORTCUT_MODIFIERS as readonly string[]).includes(part))) return "";
    const symbols: Readonly<Record<string, string>> = {
        command: "⌘", control: "⌃", option: "⌥", shift: "⇧",
    };
    const keyLabel = COMPANION_SHORTCUT_KEY_LABELS[key]
        || (/^[a-z]$/.test(key) ? key.toUpperCase() : key.toUpperCase());
    return parts.map((part) => symbols[part]).join("") + keyLabel;
}

export function companionUrlConfig(rawValue: string): string {
    const value = rawValue.trim();
    if (!value) return "";
    try {
        const url = new URL(value);
        if ((url.protocol !== "http:" && url.protocol !== "https:") || !url.hostname ||
            url.username || url.password) return "";
        const encoded = encodeURIComponent(url.href);
        return encoded.length <= 128 ? COMPANION_URL_PREFIX + encoded : "";
    } catch {
        return "";
    }
}

export function companionUrlValue(sensor: string): string {
    if (!sensor.startsWith(COMPANION_URL_PREFIX)) return "";
    try {
        return decodeURIComponent(sensor.slice(COMPANION_URL_PREFIX.length));
    } catch {
        return "";
    }
}

export function companionAppLabel(
    currentLabel: string,
    previousAppLabel: string,
    selectedAppLabel: string,
): string {
    if (!selectedAppLabel) return currentLabel;
    const trimmedLabel = currentLabel.trim();
    return !trimmedLabel || trimmedLabel === previousAppLabel ? selectedAppLabel : currentLabel;
}

export function companionWindowActionLabel(actionId: string): string {
    return COMPANION_WINDOW_ACTIONS.find((action) => action.id === actionId)?.label || "";
}

const COMPANION_CARD_METADATA = {
    mode: {
        label: "Action",
        idSuffix: "companion-mode",
        options: [
            ["app", "Launch app"],
            ["shortcut", "Keyboard shortcut"],
            ["url", "Open URL"],
            ["window", "Window controls"],
        ],
        value: companionCardMode,
    },
    icon: {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        fallback: "Monitor",
    },
    preview: { badge: "monitor" },
};

export function companionCardMode(card: any): string {
    const entity = typeof card?.entity === "string" ? card.entity : "";
    const sensor = typeof card?.sensor === "string" ? card.sensor : "";
    if (entity.startsWith(COMPANION_SHORTCUT_PREFIX)) return "shortcut";
    if (entity.startsWith(COMPANION_WINDOW_PREFIX)) return "window";
    if (sensor.startsWith(COMPANION_URL_PREFIX)) return "url";
    return "app";
}

export function normalizeCompanionCard(card: any): void {
    if (!card) return;
    const windowAction = typeof card.entity === "string" && card.entity.startsWith(COMPANION_WINDOW_PREFIX);
    const urlConfig = !windowAction && typeof card.sensor === "string" && card.sensor.startsWith(COMPANION_URL_PREFIX)
        ? card.sensor : "";
    card.type = "companion";
    card.sensor = urlConfig;
    card.unit = "";
    card.precision = "";
    card.options = "";
    card.icon_on = "Auto";
    if (!card.icon || card.icon === "Auto") card.icon = "Monitor";
}

async function fetchCompanionActions(fetchImpl: typeof fetch): Promise<readonly CompanionAction[]> {
    const response = await fetchImpl("/companion/actions", { cache: "no-store" });
    if (!response.ok) throw new Error("HTTP " + response.status);
    const value: unknown = await response.json();
    if (!Array.isArray(value)) return [];
    return value.filter((action): action is CompanionAction =>
        !!action && typeof action.id === "string" && typeof action.label === "string",
    );
}

export function registerCompanionCardTypes(
    registry: CardRegistry,
    supported: boolean,
    document: Document,
    fetchImpl: typeof fetch,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { cardBadgePreview, fieldLabel } = fields;
    const { renderButtonSettings } = cardUi;

    registry.register("companion", {
        label: function () { return cardContractCardLabel("companion"); },
        allowInSubpage: function () { return cardContractAllowInSubpage("companion"); },
        pickerKey: function () { return cardContractPickerKey("companion"); },
        hidden: function () { return cardContractHidden("companion"); },
        labelPlaceholder: "e.g. Safari or Select all",
        defaultConfig: function () { return cardContractDefaultConfig("companion"); },
        cardMetadata: COMPANION_CARD_METADATA,
        isAvailable: function () { return supported; },
        onSelect: function (card?: any) {
            const defaults: any = cardContractDefaultConfig("companion");
            Object.keys(defaults).forEach(function (key) { card[key] = defaults[key]; });
        },
        renderSettingsBeforeLabel: function (panel?: HTMLElement, card?: any, _slot?: any, helpers?: any) {
            normalizeCompanionCard(card);
            helpers.renderCardModeSelector(panel, card, helpers, {
                mode: {
                    ...COMPANION_CARD_METADATA.mode,
                    onChange: function (this: HTMLSelectElement) {
                        card.entity = this.value === "shortcut" ? COMPANION_SHORTCUT_PREFIX
                            : this.value === "window" ? (COMPANION_WINDOW_ACTIONS[0]?.id || "window.close") : "";
                        card.sensor = this.value === "url" ? COMPANION_URL_PREFIX : "";
                        helpers.saveField("entity", card.entity);
                        helpers.saveField("sensor", card.sensor);
                        renderButtonSettings();
                    },
                },
            });
        },
        renderSettings: function (panel?: HTMLElement, card?: any, _slot?: any, helpers?: any) {
            normalizeCompanionCard(card);
            const currentEntity = typeof card.entity === "string" ? card.entity : "";
            card.entity = currentEntity;
            const initialMode = companionCardMode(card);

            const appField = document.createElement("div");
            appField.className = "sp-field";
            const appFieldLabel = fieldLabel("Mac App", helpers.idPrefix + "companion-action");
            appField.appendChild(appFieldLabel);

            const select = document.createElement("select");
            select.className = "sp-select";
            select.id = helpers.idPrefix + "companion-action";
            select.disabled = true;
            const loading = document.createElement("option");
            loading.value = "";
            loading.textContent = "Loading Mac apps…";
            select.appendChild(loading);
            appField.appendChild(select);
            panel?.appendChild(appField);

            const shortcutField = document.createElement("div");
            shortcutField.className = "sp-field";
            shortcutField.appendChild(fieldLabel("Shortcut", helpers.idPrefix + "companion-shortcut"));
            const shortcutInput = document.createElement("input");
            shortcutInput.className = "sp-input";
            shortcutInput.id = helpers.idPrefix + "companion-shortcut";
            shortcutInput.readOnly = true;
            shortcutInput.placeholder = "Click, then press a shortcut such as ⌘A";
            shortcutInput.value = formatCompanionShortcutActionId(card.entity);
            shortcutInput.setAttribute("aria-label", "Keyboard shortcut");
            shortcutField.appendChild(shortcutInput);
            const shortcutNote = document.createElement("div");
            shortcutNote.className = "sp-field-info-text";
            shortcutNote.textContent = "Use Command, Control, or Option with a key. The shortcut is replayed on the active Mac app.";
            shortcutField.appendChild(shortcutNote);
            panel?.appendChild(shortcutField);

            const windowField = document.createElement("div");
            windowField.className = "sp-field";
            windowField.appendChild(fieldLabel("Window action", helpers.idPrefix + "companion-window-action"));
            const windowSelect = document.createElement("select");
            windowSelect.className = "sp-select";
            windowSelect.id = helpers.idPrefix + "companion-window-action";
            const groups = new Map<string, HTMLOptGroupElement>();
            COMPANION_WINDOW_ACTIONS.forEach(function (action) {
                let group = groups.get(action.group);
                if (!group) {
                    group = document.createElement("optgroup");
                    group.label = action.group;
                    groups.set(action.group, group);
                    windowSelect.appendChild(group);
                }
                const option = document.createElement("option");
                option.value = action.id;
                option.textContent = action.label;
                option.selected = action.id === card.entity;
                group.appendChild(option);
            });
            windowField.appendChild(windowSelect);
            const windowNote = document.createElement("div");
            windowNote.className = "sp-field-info-text sp-visible";
            windowNote.textContent = "Controls the active Mac window. Tiling actions require macOS 15 or later.";
            windowField.appendChild(windowNote);
            panel?.appendChild(windowField);

            const urlField = document.createElement("div");
            urlField.className = "sp-field";
            urlField.appendChild(fieldLabel("URL", helpers.idPrefix + "companion-url"));
            const urlInput = document.createElement("input");
            urlInput.className = "sp-input";
            urlInput.id = helpers.idPrefix + "companion-url";
            urlInput.type = "url";
            urlInput.inputMode = "url";
            urlInput.autocomplete = "off";
            urlInput.spellcheck = false;
            urlInput.maxLength = 700;
            urlInput.placeholder = "https://example.com";
            urlInput.value = companionUrlValue(card.sensor);
            urlField.appendChild(urlInput);
            const urlNote = document.createElement("div");
            urlNote.className = "sp-field-info-text";
            urlNote.textContent = "Only http:// and https:// addresses are supported.";
            urlField.appendChild(urlNote);
            panel?.appendChild(urlField);
            helpers.requireField(urlInput, "Enter an http:// or https:// address before saving.", function () {
                return initialMode === "url";
            }, function (value: string) {
                return Boolean(companionUrlConfig(value));
            });

            function syncMode(mode: string): void {
                appField.style.display = mode === "app" || mode === "url" ? "" : "none";
                appFieldLabel.textContent = mode === "url" ? "Open with" : "Mac App";
                shortcutField.style.display = mode === "shortcut" ? "" : "none";
                windowField.style.display = mode === "window" ? "" : "none";
                urlField.style.display = mode === "url" ? "" : "none";
            }
            syncMode(initialMode);

            shortcutInput.addEventListener("keydown", function (event) {
                event.preventDefault();
                event.stopPropagation();
                if (["MetaLeft", "MetaRight", "ControlLeft", "ControlRight", "AltLeft", "AltRight", "ShiftLeft", "ShiftRight"]
                    .includes(event.code)) return;
                const actionId = companionShortcutActionId(event);
                if (!actionId) {
                    shortcutInput.value = "Use ⌘, ⌃, or ⌥ with a supported key";
                    return;
                }
                card.entity = actionId;
                shortcutInput.value = formatCompanionShortcutActionId(actionId);
                helpers.saveField("entity", card.entity);
            });

            windowSelect.addEventListener("change", function () {
                const currentLabel = typeof card.label === "string" ? card.label : "";
                const previousLabel = companionWindowActionLabel(card.entity);
                const nextLabel = companionWindowActionLabel(windowSelect.value);
                card.entity = windowSelect.value;
                helpers.saveField("entity", card.entity);
                const updatedLabel = companionAppLabel(currentLabel, previousLabel, nextLabel);
                if (updatedLabel !== currentLabel) {
                    card.label = updatedLabel;
                    const labelInput = document.getElementById(helpers.idPrefix + "label") as HTMLInputElement | null;
                    if (labelInput) labelInput.value = updatedLabel;
                    helpers.saveField("label", updatedLabel);
                }
            });

            function saveUrl(): void {
                const config = companionUrlConfig(urlInput.value);
                card.sensor = config || COMPANION_URL_PREFIX;
                urlNote.textContent = urlInput.value && !config
                    ? "Enter a complete http:// or https:// address."
                    : "Only http:// and https:// addresses are supported.";
                helpers.saveField("sensor", card.sensor);
            }
            urlInput.addEventListener("input", saveUrl);
            urlInput.addEventListener("change", saveUrl);

            let companionActions: readonly CompanionAction[] = [];
            fetchCompanionActions(fetchImpl).then(function (actions) {
                companionActions = actions;
                select.replaceChildren();
                const placeholder = document.createElement("option");
                placeholder.value = "";
                placeholder.textContent = actions.length ? "Choose a Mac app…" : "No Mac companion is connected";
                select.appendChild(placeholder);
                actions.forEach(function (action) {
                    const option = document.createElement("option");
                    option.value = action.id;
                    option.textContent = action.label;
                    option.selected = action.id === card.entity;
                    select.appendChild(option);
                });
                if (card.entity && !actions.some(function (action) { return action.id === card.entity; })) {
                    const unavailable = document.createElement("option");
                    unavailable.value = card.entity;
                    unavailable.textContent = "Unavailable (" + card.entity + ")";
                    unavailable.selected = true;
                    select.appendChild(unavailable);
                }
                select.disabled = actions.length === 0;
            }).catch(function () {
                select.replaceChildren();
                const unavailable = document.createElement("option");
                unavailable.value = card.entity || "";
                unavailable.textContent = card.entity ? "Unavailable (companion offline)" : "Mac companion unavailable";
                unavailable.selected = true;
                select.appendChild(unavailable);
            });
            select.addEventListener("change", function () {
                const previousAction = companionActions.find(function (action) { return action.id === card.entity; });
                const selectedAction = companionActions.find(function (action) { return action.id === select.value; });
                const currentLabel = typeof card.label === "string" ? card.label : "";
                const nextLabel = companionAppLabel(
                    currentLabel,
                    previousAction?.label || "",
                    selectedAction?.label || "",
                );
                card.entity = select.value;
                helpers.saveField("entity", card.entity);
                if (nextLabel !== currentLabel) {
                    card.label = nextLabel;
                    const labelInput = document.getElementById(helpers.idPrefix + "label") as HTMLInputElement | null;
                    if (labelInput) labelInput.value = nextLabel;
                    helpers.saveField("label", nextLabel);
                }
            });
            helpers.renderBasicCardFields(panel, card, helpers, COMPANION_CARD_METADATA, { entity: false });
        },
        renderPreview: function (card?: any, helpers?: any) {
            const shortcutLabel = formatCompanionShortcutActionId(card.entity);
            const windowLabel = companionWindowActionLabel(card.entity);
            let urlLabel = "";
            try { urlLabel = new URL(companionUrlValue(card.sensor || "")).hostname; } catch { /* incomplete URL */ }
            return cardBadgePreview(card, helpers, {
                label: card.label || windowLabel || shortcutLabel || urlLabel || card.entity || "Mac App",
                iconFallback: "Monitor",
                badge: COMPANION_CARD_METADATA.preview.badge,
            });
        },
    });
}
