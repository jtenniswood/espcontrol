import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export interface CompanionAction {
    readonly id: string;
    readonly label: string;
}

const COMPANION_SHORTCUT_PREFIX = "shortcut.";
const COMPANION_URL_PREFIX = "url.";
export const COMPANION_FOLDER_PREFIX = "folder.";
const COMPANION_FINDER_ID = "com.apple.finder";
const COMPANION_STATS_MODES = "stats,processor,memory_usage,storage,battery".split(",");
export const COMPANION_SUBTYPE_DEFAULT_ICONS = {
    app: "Monitor",
    shortcut: "Shortcut Command",
    url: "Web",
    folder: "Folder Outline",
    stats: "Gauge",
} as const;
export const COMPANION_MEDIA_ACTIONS = [
    { id: "media.play_pause", label: "Play / Pause", icon: "Play Pause" },
    { id: "media.previous", label: "Previous Track", icon: "Skip Previous" },
    { id: "media.next", label: "Next Track", icon: "Skip Next" },
] as const;
export const COMPANION_SYSTEM_METRICS = [
    { mode: "processor", id: "stat.cpu", label: "Processor", unit: "%" },
    { mode: "memory_usage", id: "stat.memory", label: "Memory", unit: "%" },
    { mode: "storage", id: "stat.storage", label: "Storage", unit: "%" },
    { mode: "battery", id: "stat.battery", label: "Battery", unit: "%" },
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

export function companionMediaIcon(
    currentIcon: string,
    previousGeneratedIcon: string,
    selectedGeneratedIcon: string,
): string {
    return !currentIcon || currentIcon === "Auto" || currentIcon === previousGeneratedIcon
        ? selectedGeneratedIcon : currentIcon;
}

export function companionSubtypeDefaultIcon(mode: string, entity = ""): string {
    if (mode === "media") {
        return COMPANION_MEDIA_ACTIONS.find((action) => action.id === entity)?.icon
            || COMPANION_MEDIA_ACTIONS[0].icon;
    }
    if (COMPANION_STATS_MODES.includes(mode)) {
        return COMPANION_SUBTYPE_DEFAULT_ICONS.stats;
    }
    return COMPANION_SUBTYPE_DEFAULT_ICONS[mode as keyof typeof COMPANION_SUBTYPE_DEFAULT_ICONS]
        || COMPANION_SUBTYPE_DEFAULT_ICONS.app;
}

export function companionSubtypeIcon(
    currentIcon: string,
    previousMode: string,
    nextMode: string,
    previousEntity = "",
    nextEntity = "",
): string {
    const previousGeneratedIcon = companionSubtypeDefaultIcon(previousMode, previousEntity);
    const legacyFolderIcon = previousMode === "folder" && currentIcon === "Folder";
    return !currentIcon || currentIcon === "Auto" || currentIcon === "Monitor" ||
        currentIcon === previousGeneratedIcon || legacyFolderIcon
        ? companionSubtypeDefaultIcon(nextMode, nextEntity) : currentIcon;
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
        label: "Type",
        idSuffix: "companion-mode",
        options: [
            ["app", "Launch app"],
            ["shortcut", "Keyboard shortcut"],
            ["url", "Open URL"],
            ["folder", "Open folder"],
            ["media", "Media control"],
            ["processor", "Processor usage"],
            ["memory_usage", "Memory usage"],
            ["storage", "Storage usage"],
            ["battery", "Battery level"],
        ],
        value: companionCardMode,
    },
    icon: {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        fallback: "Monitor",
    },
    largeNumbers: {
        label: "Large Sensor Numbers",
        idSuffix: "large-companion-numbers",
        supported: companionCardIsMetric,
    },
    preview: { badge: "monitor" },
};

export function companionCardIsMetric(card: any): boolean {
    return COMPANION_SYSTEM_METRICS.some((metric) => metric.id === card?.entity);
}

export function companionLabelPlaceholder(card: any): string {
    const metric = COMPANION_SYSTEM_METRICS.find((candidate) => candidate.id === card?.entity);
    return metric ? `e.g. ${metric.label}` : "e.g. Safari or Select all";
}

export function companionCardMode(card: any): string {
    const entity = typeof card?.entity === "string" ? card.entity : "";
    const sensor = typeof card?.sensor === "string" ? card.sensor : "";
    if (entity.startsWith(COMPANION_SHORTCUT_PREFIX)) return "shortcut";
    if (entity === COMPANION_FINDER_ID || entity.startsWith(COMPANION_FOLDER_PREFIX)) return "folder";
    if (COMPANION_MEDIA_ACTIONS.some((action) => action.id === entity)) return "media";
    const metric = COMPANION_SYSTEM_METRICS.find((candidate) => candidate.id === entity);
    if (metric) return metric.mode;
    if (sensor.startsWith(COMPANION_URL_PREFIX)) return "url";
    return "app";
}

export function companionEntityForMode(mode: string): string {
    if (mode === "shortcut") return COMPANION_SHORTCUT_PREFIX;
    if (mode === "folder") return COMPANION_FOLDER_PREFIX;
    if (mode === "media") return COMPANION_MEDIA_ACTIONS[0].id;
    return COMPANION_SYSTEM_METRICS.find((metric) => metric.mode === mode)?.id || "";
}

export function companionApplicationActions(actions: readonly CompanionAction[]): readonly CompanionAction[] {
    return actions.filter((action) =>
        action.id !== COMPANION_FINDER_ID && !action.id.startsWith(COMPANION_FOLDER_PREFIX));
}

export function companionFolderActions(actions: readonly CompanionAction[]): readonly CompanionAction[] {
    return actions.filter((action) => action.id.startsWith(COMPANION_FOLDER_PREFIX));
}

export function resetCompanionMediaPresentation(card: any, nextMode: string): void {
    if (!card || nextMode === "media") return;
    const previous = COMPANION_MEDIA_ACTIONS.find((action) => action.id === card.entity);
    if (!previous) return;
    if (card.label === previous.label) card.label = "";
    if (card.icon === previous.icon) card.icon = "Monitor";
}

export function resetCompanionMetricPresentation(card: any, nextMode: string): void {
    if (!card) return;
    const previous = COMPANION_SYSTEM_METRICS.find((metric) => metric.id === card.entity);
    if (!previous || previous.mode === nextMode) return;
    if (card.label === previous.label) card.label = "";
    card.unit = "";
    card.precision = "";
    card.options = "";
}

export function normalizeCompanionCard(card: any): void {
    if (!card) return;
    const metric = COMPANION_SYSTEM_METRICS.find((candidate) => candidate.id === card.entity);
    if (metric) {
        card.type = "companion";
        card.sensor = "";
        card.label = card.label || metric.label;
        card.unit = card.unit || metric.unit;
        card.precision = card.precision === "0" || card.precision === "2" ? card.precision : "1";
        card.options = String(card.options || "").split(",").filter((option) =>
            option === "large_numbers" || option === "large_numbers=off").join(",");
        card.icon_on = "Auto";
        if (!card.icon || card.icon === "Auto" || card.icon === "Monitor") {
            card.icon = companionSubtypeDefaultIcon(metric.mode, card.entity);
        }
        return;
    }
    const urlConfig = typeof card.sensor === "string" && card.sensor.startsWith(COMPANION_URL_PREFIX)
        ? card.sensor : "";
    card.type = "companion";
    card.sensor = urlConfig;
    card.unit = "";
    card.precision = "";
    card.options = "";
    card.icon_on = "Auto";
    const mode = companionCardMode(card);
    if (!card.icon || card.icon === "Auto" ||
        (card.icon === "Monitor" && mode !== "app") ||
        (card.icon === "Folder" && mode === "folder")) {
        card.icon = companionSubtypeDefaultIcon(mode, card.entity);
    }
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
    const { cardBadgePreview, cardBadgeLabelHtml, cardSensorPreviewHtml, fieldLabel } = fields;
    const { renderButtonSettings } = cardUi;

    registry.register("companion", {
        label: function () { return cardContractCardLabel("companion"); },
        allowInSubpage: function () { return cardContractAllowInSubpage("companion"); },
        pickerKey: function () { return cardContractPickerKey("companion"); },
        hidden: function () { return cardContractHidden("companion"); },
        hideLabel: true,
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
                        const previousEntity = card.entity;
                        let previousGeneratedLabel = "";
                        if (previousMode === "app" || previousMode === "url") {
                            const appSelect = document.getElementById(
                                helpers.idPrefix + "companion-action") as HTMLSelectElement | null;
                            const selectedOption = appSelect?.selectedOptions[0];
                            if (selectedOption && selectedOption.value === card.entity) {
                                previousGeneratedLabel = selectedOption.textContent || "";
                            }
                        } else if (previousMode === "folder") {
                            const folderSelect = document.getElementById(
                                helpers.idPrefix + "companion-folder") as HTMLSelectElement | null;
                            const selectedOption = folderSelect?.selectedOptions[0];
                            if (selectedOption && selectedOption.value === card.entity) {
                                previousGeneratedLabel = selectedOption.textContent || "";
                            }
                            if (card.label === previousGeneratedLabel) card.label = "";
                        }
                        if (previousGeneratedLabel && card.label === previousGeneratedLabel) {
                            card.label = "";
                        }
                        resetCompanionMediaPresentation(card, this.value);
                        resetCompanionMetricPresentation(card, this.value);
                        const selectedMetric = COMPANION_SYSTEM_METRICS.find((metric) => metric.mode === this.value);
                        card.entity = companionEntityForMode(this.value);
                        card.sensor = this.value === "url" ? COMPANION_URL_PREFIX : "";
                        if (this.value === "media") {
                            applyCompanionMediaPresentation(card, previousGeneratedLabel);
                        } else if (selectedMetric) {
                            if (!card.label) card.label = selectedMetric.label;
                            card.unit = selectedMetric.unit;
                            card.precision = "1";
                            card.options = "";
                        }
                        card.icon = companionSubtypeIcon(
                            card.icon, previousMode, this.value, previousEntity, card.entity);
                        if (card.label !== previousLabel) helpers.saveField("label", card.label);
                        if (card.icon !== previousIcon) helpers.saveField("icon", card.icon);
                        helpers.saveField("entity", card.entity);
                        helpers.saveField("sensor", card.sensor);
                        helpers.saveField("unit", card.unit);
                        helpers.saveField("precision", card.precision);
                        helpers.saveField("options", card.options);
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

            helpers.renderCardTextField(panel, card, helpers, {
                label: "Label", idSuffix: "label", field: "label",
                placeholder: companionLabelPlaceholder(card), rerender: true,
            });

            if (companionCardIsMetric(card)) {
                helpers.renderCardTextField(panel, card, helpers, {
                    label: "Unit", idSuffix: "unit", field: "unit",
                    placeholder: "%", rerender: true,
                });
                const precision = helpers.precisionField(
                    helpers.idPrefix + "precision", card.precision || "1", function (this: HTMLSelectElement) {
                        card.precision = this.value;
                        helpers.saveField("precision", card.precision);
                    });
                panel?.appendChild(precision.field);
                helpers.renderCardLargeNumbersToggle(panel, card, helpers, COMPANION_CARD_METADATA);
                return;
            }

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

            const folderField = document.createElement("div");
            folderField.className = "sp-field";
            folderField.appendChild(fieldLabel("Folder", helpers.idPrefix + "companion-folder"));
            const folderSelect = document.createElement("select");
            folderSelect.className = "sp-select";
            folderSelect.id = helpers.idPrefix + "companion-folder";
            folderSelect.disabled = true;
            const folderLoading = document.createElement("option");
            folderLoading.value = "";
            folderLoading.textContent = "Loading approved folders…";
            folderSelect.appendChild(folderLoading);
            folderField.appendChild(folderSelect);
            const folderNote = document.createElement("div");
            folderNote.className = "sp-field-info-text";
            folderNote.textContent = "Add folders from the Folders tab in the EspControl Companion app.";
            folderField.appendChild(folderNote);
            panel?.appendChild(folderField);
            helpers.requireField(folderSelect, "Choose a folder before saving.", function () {
                return initialMode === "folder";
            }, function (value: string) {
                return value.startsWith(COMPANION_FOLDER_PREFIX);
            });

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
            }, function (value: string) {
                return Boolean(companionUrlConfig(value));
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
                folderField.style.display = mode === "folder" ? "" : "none";
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
                const applicationActions = companionApplicationActions(actions);
                const folderActions = companionFolderActions(actions);
                select.replaceChildren();
                const placeholder = document.createElement("option");
                placeholder.value = "";
                placeholder.textContent = applicationActions.length ? "Choose a Mac app…" : "No Mac apps are available";
                select.appendChild(placeholder);
                applicationActions.forEach(function (action) {
                    const option = document.createElement("option");
                    option.value = action.id;
                    option.textContent = action.label;
                    option.selected = action.id === card.entity;
                    select.appendChild(option);
                });
                if ((initialMode === "app" || initialMode === "url") && card.entity &&
                    !applicationActions.some(function (action) { return action.id === card.entity; })) {
                    const unavailable = document.createElement("option");
                    unavailable.value = card.entity;
                    unavailable.textContent = "Unavailable (" + card.entity + ")";
                    unavailable.selected = true;
                    select.appendChild(unavailable);
                }
                select.disabled = applicationActions.length === 0;

                folderSelect.replaceChildren();
                const folderPlaceholder = document.createElement("option");
                folderPlaceholder.value = "";
                folderPlaceholder.textContent = folderActions.length
                    ? "Choose a folder…" : "Add a folder in the Companion app";
                folderSelect.appendChild(folderPlaceholder);
                folderActions.forEach(function (action) {
                    const option = document.createElement("option");
                    option.value = action.id;
                    option.textContent = action.label;
                    option.selected = action.id === card.entity;
                    folderSelect.appendChild(option);
                });
                if (initialMode === "folder" && card.entity &&
                    !folderActions.some(function (action) { return action.id === card.entity; })) {
                    const unavailable = document.createElement("option");
                    unavailable.value = card.entity;
                    unavailable.textContent = "Unavailable folder";
                    unavailable.selected = true;
                    folderSelect.appendChild(unavailable);
                }
                folderSelect.disabled = folderActions.length === 0;
            }).catch(function () {
                select.replaceChildren();
                const unavailable = document.createElement("option");
                unavailable.value = card.entity || "";
                unavailable.textContent = card.entity ? "Unavailable (companion offline)" : "Mac companion unavailable";
                unavailable.selected = true;
                select.appendChild(unavailable);
                folderSelect.replaceChildren();
                const folderUnavailable = document.createElement("option");
                folderUnavailable.value = card.entity.startsWith(COMPANION_FOLDER_PREFIX) ? card.entity : "";
                folderUnavailable.textContent = card.entity.startsWith(COMPANION_FOLDER_PREFIX)
                    ? "Unavailable folder" : "Mac companion unavailable";
                folderUnavailable.selected = true;
                folderSelect.appendChild(folderUnavailable);
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
            folderSelect.addEventListener("change", function () {
                const previousAction = companionActions.find(function (action) { return action.id === card.entity; });
                const selectedAction = companionActions.find(function (action) { return action.id === folderSelect.value; });
                const currentLabel = typeof card.label === "string" ? card.label : "";
                const nextLabel = companionAppLabel(
                    currentLabel,
                    previousAction?.label || "",
                    selectedAction?.label || "",
                );
                card.entity = folderSelect.value;
                card.icon = companionSubtypeIcon(card.icon, "folder", "folder", card.entity, card.entity);
                helpers.saveField("entity", card.entity);
                helpers.saveField("icon", card.icon);
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
            const mode = companionCardMode(card);
            if (companionCardIsMetric(card)) {
                const metric = COMPANION_SYSTEM_METRICS.find((candidate) => candidate.id === card.entity);
                return {
                    iconHtml: cardSensorPreviewHtml(card, helpers, "42.0", card.unit || metric?.unit || "%"),
                    labelHtml: cardBadgeLabelHtml(helpers, card.label || metric?.label || "Mac"),
                };
            }
            const shortcutLabel = formatCompanionShortcutActionId(card.entity);
            let urlLabel = "";
            try { urlLabel = new URL(companionUrlValue(card.sensor || "")).hostname; } catch { /* incomplete URL */ }
            return cardBadgePreview(card, helpers, {
                label: card.label || shortcutLabel || urlLabel || card.entity || (mode === "folder" ? "Folder" : "Mac App"),
                iconFallback: companionSubtypeDefaultIcon(mode, card.entity),
                badge: COMPANION_CARD_METADATA.preview.badge,
            });
        },
    });
}
