import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";
import type { ConfigCodecFeature } from "../application/config_codec";
import type { ButtonSettingsSelectionFeature } from "../application/button_settings_selection";
import { state } from "../state/app_instance";
import {
    COMPANION_SHORTCUT_PREFIX,
    SAFARI_BUNDLE_ID,
    companionAppShortcutFolderEnabled,
    companionShortcutActionIdValid,
    companionShortcutFolderEditorAvailable,
    createSafariShortcutSubpage,
    normalizeCompanionAppShortcutOptions,
    setCompanionAppShortcutFolderEnabled,
} from "../application/companion_shortcut_folder";

export interface CompanionAction {
    readonly id: string;
    readonly label: string;
}

const COMPANION_URL_PREFIX = "url.";
export const COMPANION_FOLDER_PREFIX = "folder.";
const COMPANION_FINDER_ID = "com.apple.finder";
const COMPANION_STATS_MODES = "stats,processor,memory_usage,storage,battery,network_throughput".split(",");
export const COMPANION_SUBTYPE_DEFAULT_ICONS = {
    app: "Monitor",
    shortcut: "Shortcut Command",
    url: "Web",
    folder: "Folder Outline",
    stats: "Gauge",
} as const;
export const COMPANION_MEDIA_PLAY_PAUSE_ACTION = "media.play_pause";
export const COMPANION_MEDIA_ACTIONS = [
    { id: COMPANION_MEDIA_PLAY_PAUSE_ACTION, label: "Play / Pause", icon: "Play Pause" },
    { id: "media.previous", label: "Previous Track", icon: "Skip Previous" },
    { id: "media.next", label: "Next Track", icon: "Skip Next" },
] as const;
interface CompanionSystemMetric {
    readonly mode: string;
    readonly id: string;
    readonly label: string;
    readonly unit: string;
    readonly freeId?: string;
}

export const COMPANION_SYSTEM_METRICS: readonly CompanionSystemMetric[] = [
    { mode: "processor", id: "stat.cpu", label: "Processor", unit: "%" },
    { mode: "memory_usage", id: "stat.memory", freeId: "stat.memory_free", label: "Memory", unit: "%" },
    { mode: "storage", id: "stat.storage", freeId: "stat.storage_free", label: "Storage", unit: "%" },
    { mode: "battery", id: "stat.battery", label: "Battery", unit: "%" },
    { mode: "network_throughput", id: "stat.network_throughput", label: "Network Throughput", unit: "KB/s" },
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
    if (!companionShortcutActionIdValid(actionId)) return "";
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
            ["network_throughput", "Network throughput"],
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
    return !!companionMetricForEntity(card?.entity);
}

function companionMetricForEntity(entity: unknown): CompanionSystemMetric | undefined {
    return COMPANION_SYSTEM_METRICS.find((metric) =>
        metric.id === entity || metric.freeId === entity);
}

export function companionMetricDisplayMode(card: any): "used" | "free" {
    const metric = companionMetricForEntity(card?.entity);
    return metric?.freeId === card?.entity ? "free" : "used";
}

export function companionLabelPlaceholder(card: any): string {
    const metric = companionMetricForEntity(card?.entity);
    return metric ? `e.g. ${metric.label}` : "e.g. Safari or Select all";
}

export function companionMetricPreviewValue(precision: unknown, sample = Math.random()): string {
    const parsed = Number.parseInt(String(precision ?? "0"), 10);
    const digits = parsed >= 0 && parsed <= 2 ? parsed : 0;
    const normalizedSample = Number.isFinite(sample) ? Math.min(1, Math.max(0, sample)) : 0.5;
    return (10 + normalizedSample * 80).toFixed(digits);
}

export function companionCardMode(card: any): string {
    const entity = typeof card?.entity === "string" ? card.entity : "";
    const sensor = typeof card?.sensor === "string" ? card.sensor : "";
    if (entity.startsWith(COMPANION_SHORTCUT_PREFIX)) return "shortcut";
    if (entity === COMPANION_FINDER_ID || entity.startsWith(COMPANION_FOLDER_PREFIX)) return "folder";
    if (COMPANION_MEDIA_ACTIONS.some((action) => action.id === entity)) return "media";
    const metric = companionMetricForEntity(entity);
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
        action.id !== COMPANION_FINDER_ID && !action.id.startsWith(COMPANION_FOLDER_PREFIX) &&
        !COMPANION_MEDIA_ACTIONS.some((mediaAction) => mediaAction.id === action.id));
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
    const previous = companionMetricForEntity(card.entity);
    const nextMetric = COMPANION_SYSTEM_METRICS.some((metric) => metric.mode === nextMode);
    if (!previous || previous.mode === nextMode || nextMetric) return;
    if (card.label === previous.label) card.label = "";
    card.unit = "";
    card.precision = "";
    card.options = "";
}

export function normalizeCompanionCard(card: any): void {
    if (!card) return;
    const metric = companionMetricForEntity(card.entity);
    if (metric) {
        card.type = "companion";
        card.sensor = "";
        card.unit = card.unit || metric.unit;
        card.precision = card.precision === "0" || card.precision === "1" || card.precision === "2"
            ? card.precision : "0";
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
    card.options = normalizeCompanionAppShortcutOptions(card);
    card.icon_on = "Auto";
    const mode = companionCardMode(card);
    if (!card.icon || card.icon === "Auto" ||
        (card.icon === "Monitor" && mode !== "app" && mode !== "media") ||
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
    codec: Pick<ConfigCodecFeature, "buildSubpageGrid" | "enterSubpage" | "saveSubpageConfig">,
    selection: Pick<ButtonSettingsSelectionFeature, "closeSettings">,
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
        normalizeConfig: normalizeCompanionCard,
        onSelect: function (card?: any) {
            const defaults: any = cardContractDefaultConfig("companion");
            Object.keys(defaults).forEach(function (key) { card[key] = defaults[key]; });
        },
        renderSettingsBeforeLabel: function (panel?: HTMLElement, card?: any, _slot?: any, helpers?: any) {
            normalizeCompanionCard(card);
            const shortcutOnly = !!helpers.shortcutOnly;
            helpers.renderCardModeSelector(panel, card, helpers, {
                mode: {
                    ...COMPANION_CARD_METADATA.mode,
                    options: shortcutOnly
                        ? [["shortcut", "Keyboard shortcut"]]
                        : COMPANION_CARD_METADATA.mode.options,
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
                        card.options = "";
                        if (this.value === "media") {
                            applyCompanionMediaPresentation(card, previousGeneratedLabel);
                        } else if (selectedMetric) {
                            if (!companionMetricForEntity(previousEntity)) {
                                card.unit = selectedMetric.unit;
                                card.precision = "0";
                                card.options = "";
                            }
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
        renderSettings: function (panel?: HTMLElement, card?: any, slot?: any, helpers?: any) {
            normalizeCompanionCard(card);
            const currentEntity = typeof card.entity === "string" ? card.entity : "";
            card.entity = currentEntity;
            const initialMode = companionCardMode(card);
            const shortcutOnly = !!helpers.shortcutOnly;

            helpers.renderCardTextField(panel, card, helpers, {
                label: "Label", idSuffix: "label", field: "label",
                placeholder: companionLabelPlaceholder(card), rerender: true,
            });

            if (companionCardIsMetric(card)) {
                const metric = companionMetricForEntity(card.entity);
                if (metric?.freeId) {
                    const displayField = document.createElement("div");
                    displayField.className = "sp-field";
                    displayField.appendChild(fieldLabel("Show", helpers.idPrefix + "metric-display"));
                    const displaySelect = document.createElement("select");
                    displaySelect.className = "sp-select";
                    displaySelect.id = helpers.idPrefix + "metric-display";
                    [{ value: "used", label: "Used" }, { value: "free", label: "Free" }].forEach((item) => {
                        const option = document.createElement("option");
                        option.value = item.value;
                        option.textContent = item.label;
                        displaySelect.appendChild(option);
                    });
                    displaySelect.value = companionMetricDisplayMode(card);
                    displaySelect.addEventListener("change", function () {
                        card.entity = this.value === "free" ? metric.freeId : metric.id;
                        helpers.saveField("entity", card.entity);
                        renderButtonSettings();
                    });
                    displayField.appendChild(displaySelect);
                    panel?.appendChild(displayField);
                }
                helpers.renderCardTextField(panel, card, helpers, {
                    label: "Unit", idSuffix: "unit", field: "unit",
                    placeholder: "%", rerender: true,
                });
                const precision = helpers.precisionField(
                    helpers.idPrefix + "precision", card.precision || "0", function (this: HTMLSelectElement) {
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
            helpers.requireField(shortcutInput, "Capture a valid keyboard shortcut before saving.", function () {
                return initialMode === "shortcut";
            }, function () {
                return companionShortcutActionIdValid(card.entity);
            });

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

            const safariShortcutField = document.createElement("div");
            safariShortcutField.className = "sp-field";
            const folderToggle = helpers.toggleRow(
                "Add shortcut folder",
                helpers.idPrefix + "companion-app-shortcuts",
                companionAppShortcutFolderEnabled(card),
            );
            safariShortcutField.appendChild(folderToggle.row);
            const safariShortcutNote = document.createElement("div");
            safariShortcutNote.className = "sp-field-info-text";
            safariShortcutNote.textContent = "Launch Safari, then open an editable folder of Safari keyboard shortcuts.";
            safariShortcutField.appendChild(safariShortcutNote);
            panel?.appendChild(safariShortcutField);
            folderToggle.input.addEventListener("change", function () {
                setCompanionAppShortcutFolderEnabled(card, folderToggle.input.checked);
                helpers.saveField("options", card.options);
                renderButtonSettings();
            });

            function syncMode(mode: string): void {
                appField.style.display = mode === "app" || mode === "url" ? "" : "none";
                appFieldLabel.textContent = mode === "url" ? "Open with" : "Mac App";
                folderField.style.display = mode === "folder" ? "" : "none";
                shortcutField.style.display = mode === "shortcut" ? "" : "none";
                urlField.style.display = mode === "url" ? "" : "none";
                mediaField.style.display = mode === "media" ? "" : "none";
                safariShortcutField.style.display = !shortcutOnly && mode === "app" && card.entity === SAFARI_BUNDLE_ID ? "" : "none";
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
                helpers.clearFieldError(shortcutInput);
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
                card.options = normalizeCompanionAppShortcutOptions(card);
                helpers.saveField("entity", card.entity);
                helpers.saveField("options", card.options);
                if (nextLabel !== currentLabel) {
                    card.label = nextLabel;
                    const labelInput = document.getElementById(helpers.idPrefix + "label") as HTMLInputElement | null;
                    if (labelInput) labelInput.value = nextLabel;
                    helpers.saveField("label", nextLabel);
                }
                renderButtonSettings();
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
            const savedParent = !helpers.isSub && slot ? state.buttons[slot - 1] : null;
            if (companionShortcutFolderEditorAvailable(card, savedParent)) {
                const editButton = document.createElement("button");
                editButton.className = "sp-action-btn sp-edit-subpage-btn";
                editButton.textContent = "Edit Safari Shortcuts";
                editButton.addEventListener("click", function () {
                    selection.closeSettings();
                    codec.enterSubpage(slot);
                });
                panel?.appendChild(editButton);
            }
        },
        renderPreview: function (card?: any, helpers?: any) {
            const mode = companionCardMode(card);
            if (companionCardIsMetric(card)) {
                const metric = companionMetricForEntity(card.entity);
                return {
                    iconHtml: cardSensorPreviewHtml(
                        card, helpers, companionMetricPreviewValue(card.precision),
                        card.unit || metric?.unit || "%",
                    ),
                    labelHtml: cardBadgeLabelHtml(helpers, card.label || metric?.label || "Mac"),
                };
            }
            const shortcutLabel = formatCompanionShortcutActionId(card.entity);
            let urlLabel = "";
            try { urlLabel = new URL(companionUrlValue(card.sensor || "")).hostname; } catch { /* incomplete URL */ }
            const preview = cardBadgePreview(card, helpers, {
                label: card.label || shortcutLabel || urlLabel || card.entity || (mode === "folder" ? "Folder" : "Mac App"),
                iconFallback: companionSubtypeDefaultIcon(mode, card.entity),
                badge: COMPANION_CARD_METADATA.preview.badge,
            });
            if (companionAppShortcutFolderEnabled(card)) {
                const label = card.label || card.entity || "Safari";
                preview.labelHtml = '<span class="sp-btn-label-row"><span class="sp-btn-label">' +
                    helpers.escHtml(label) +
                    '</span><span class="sp-subpage-badge mdi mdi-chevron-right"></span></span>';
            }
            return preview;
        },
        contextMenuItems: function (slot?: any, card?: any, helpers?: any) {
            if (!companionAppShortcutFolderEnabled(card)) return;
            helpers.addCtxItem("cog", "Edit Safari Shortcuts", function () { codec.enterSubpage(slot); });
        },
        afterSave: function (card?: any, slot?: any, context?: any) {
            if (context?.isSub || !companionAppShortcutFolderEnabled(card) || state.subpages[slot]) return;
            const subpage = createSafariShortcutSubpage();
            codec.buildSubpageGrid(subpage);
            state.subpages[slot] = subpage;
            codec.saveSubpageConfig(slot);
        },
    });
}
