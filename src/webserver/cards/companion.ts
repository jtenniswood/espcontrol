import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";

interface CompanionAction {
    readonly id: string;
    readonly label: string;
}

const COMPANION_CARD_METADATA = {
    icon: {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        fallback: "Monitor",
    },
    preview: { badge: "monitor" },
};

export function normalizeCompanionCard(card: any): void {
    if (!card) return;
    card.type = "companion";
    card.sensor = "";
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
): void {
    const { cardBadgePreview, fieldLabel } = fields;

    registry.register("companion", {
        label: function () { return cardContractCardLabel("companion"); },
        allowInSubpage: function () { return cardContractAllowInSubpage("companion"); },
        pickerKey: function () { return cardContractPickerKey("companion"); },
        hidden: function () { return cardContractHidden("companion"); },
        labelPlaceholder: "e.g. Safari",
        defaultConfig: function () { return cardContractDefaultConfig("companion"); },
        cardMetadata: COMPANION_CARD_METADATA,
        isAvailable: function () { return supported; },
        onSelect: function (card?: any) {
            const defaults: any = cardContractDefaultConfig("companion");
            Object.keys(defaults).forEach(function (key) { card[key] = defaults[key]; });
        },
        renderSettings: function (panel?: HTMLElement, card?: any, _slot?: any, helpers?: any) {
            normalizeCompanionCard(card);
            const field = document.createElement("div");
            field.className = "sp-field";
            field.appendChild(fieldLabel("Mac App", helpers.idPrefix + "companion-action"));

            const select = document.createElement("select");
            select.className = "sp-select";
            select.id = helpers.idPrefix + "companion-action";
            select.disabled = true;
            const loading = document.createElement("option");
            loading.value = "";
            loading.textContent = "Loading approved apps…";
            select.appendChild(loading);
            field.appendChild(select);
            panel?.appendChild(field);

            fetchCompanionActions(fetchImpl).then(function (actions) {
                select.replaceChildren();
                const placeholder = document.createElement("option");
                placeholder.value = "";
                placeholder.textContent = actions.length ? "Choose an approved app…" : "No Mac companion is connected";
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
                card.entity = select.value;
                helpers.saveField("entity", card.entity);
            });
            helpers.renderBasicCardFields(panel, card, helpers, COMPANION_CARD_METADATA, { entity: false });
        },
        renderPreview: function (card?: any, helpers?: any) {
            return cardBadgePreview(card, helpers, {
                label: card.label || card.entity || "Mac App",
                iconFallback: "Monitor",
                badge: COMPANION_CARD_METADATA.preview.badge,
            });
        },
    });
}
