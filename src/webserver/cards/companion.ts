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

const COMPANION_SHORTCUT_PREFIX = "shortcut.";
const COMPANION_URL_PREFIX = "url.";
export const COMPANION_MEDIA_ACTIONS = [
    { id: "media.play_pause", label: "Play / Pause", icon: "Play Pause" },
    { id: "media.previous", label: "Previous Track", icon: "Skip Previous" },
    { id: "media.next", label: "Next Track", icon: "Skip Next" },
] as const;
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
        return encoded.length <= 1024 ? COMPANION_URL_PREFIX + encoded : "";
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

export function companionMediaIcon(
    currentIcon: string,
    previousGeneratedIcon: string,
    selectedGeneratedIcon: string,
): string {
    return !currentIcon || currentIcon === "Auto" || currentIcon === previousGeneratedIcon
        ? selectedGeneratedIcon : currentIcon;
}

export function applyCompanionMediaPresentation(card: any, previousGeneratedLabel = ""): void {
    if (!card) return;
    const selected = COMPANION_MEDIA_ACTIONS[0];
    const currentLabel = typeof card.label === "string" ? card.label : "";
    const currentIcon = typeof card.icon === "string" ? card.icon : "";
    card.label = companionAppLabel(currentLabel, previousGeneratedLabel, selected.label);
    card.icon = companionMediaIcon(currentIcon, "Monitor", selected.icon);
}

const COMPANION_CARD_METADATA = {
    mode: {
        label: "Action",
        idSuffix: "companion-mode",
        options: [
            ["app", "Launch app"],
            ["shortcut", "Keyboard shortcut"],
            ["url", "Open URL"],
            ["media", "Media control"],
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
    if (COMPANION_MEDIA_ACTIONS.some((action) => action.id === entity)) return "media";
    if (sensor.startsWith(COMPANION_URL_PREFIX)) return "url";
    return "app";
}

export function resetCompanionMediaPresentation(card: any, nextMode: string): void {
    if (!card || nextMode === "media") return;
    const previous = COMPANION_MEDIA_ACTIONS.find((action) => action.id === card.entity);
    if (!previous) return;
    if (card.label === previous.label) card.label = "";
    if (card.icon === previous.icon) card.icon = "Monitor";
}

export function normalizeCompanionCard(card: any): void {
    if (!card) return;
    const urlConfig = typeof card.sensor === "string" && card.sensor.startsWith(COMPANION_URL_PREFIX)
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
                        const previousLabel = card.label;
                        const previousIcon = card.icon;
                        const previousMode = companionCardMode(card);
                        let previousGeneratedLabel = "";
                        if (previousMode === "app" || previousMode === "url") {
                            const appSelect = document.getElementById(
                                helpers.idPrefix + "companion-action") as HTMLSelectElement | null;
                            const selectedOption = appSelect?.selectedOptions[0];
                            if (selectedOption && selectedOption.value === card.entity) {
                                previousGeneratedLabel = selectedOption.textContent || "";
                            }
                        }
                        resetCompanionMediaPresentation(card, this.value);
                        card.entity = this.value === "shortcut" ? COMPANION_SHORTCUT_PREFIX
                            : this.value === "media" ? COMPANION_MEDIA_ACTIONS[0].id : "";
                        card.sensor = this.value === "url" ? COMPANION_URL_PREFIX : "";
                        if (this.value === "media") {
                            applyCompanionMediaPresentation(card, previousGeneratedLabel);
                        }
                        if (card.label !== previousLabel) helpers.saveField("label", card.label);
                        if (card.icon !== previousIcon) helpers.saveField("icon", card.icon);
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
            });

            const mediaField = document.createElement("div");
            mediaField.className = "sp-field";
            mediaField.appendChild(fieldLabel("Media Control", helpers.idPrefix + "companion-media-action"));
            const mediaSelect = document.createElement("select");
            mediaSelect.className = "sp-select";
            mediaSelect.id = helpers.idPrefix + "companion-media-action";
            COMPANION_MEDIA_ACTIONS.forEach(function (action) {
                const option = document.createElement("option");
                option.value = action.id;
                option.textContent = action.label;
                option.selected = card.entity === action.id;
                mediaSelect.appendChild(option);
            });
            mediaField.appendChild(mediaSelect);
            panel?.appendChild(mediaField);

            function syncMode(mode: string): void {
                appField.style.display = mode === "app" || mode === "url" ? "" : "none";
                appFieldLabel.textContent = mode === "url" ? "Open with" : "Mac App";
                shortcutField.style.display = mode === "shortcut" ? "" : "none";
                urlField.style.display = mode === "url" ? "" : "none";
                mediaField.style.display = mode === "media" ? "" : "none";
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

            mediaSelect.addEventListener("change", function () {
                const previous = COMPANION_MEDIA_ACTIONS.find(function (action) { return action.id === card.entity; });
                const selected = COMPANION_MEDIA_ACTIONS.find(function (action) { return action.id === mediaSelect.value; });
                if (!selected) return;
                const currentLabel = typeof card.label === "string" ? card.label : "";
                const currentIcon = typeof card.icon === "string" ? card.icon : "";
                card.entity = selected.id;
                card.label = companionAppLabel(currentLabel, previous?.label || "", selected.label);
                card.icon = companionMediaIcon(currentIcon, previous?.icon || "", selected.icon);
                helpers.saveField("entity", card.entity);
                helpers.saveField("label", card.label);
                helpers.saveField("icon", card.icon);
                renderButtonSettings();
            });

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
            let urlLabel = "";
            try { urlLabel = new URL(companionUrlValue(card.sensor || "")).hostname; } catch { /* incomplete URL */ }
            return cardBadgePreview(card, helpers, {
                label: card.label || shortcutLabel || urlLabel || card.entity || "Mac App",
                iconFallback: "Monitor",
                badge: COMPANION_CARD_METADATA.preview.badge,
            });
        },
    });
}
