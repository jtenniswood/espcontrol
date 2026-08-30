import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";
import type { DeviceConfig } from "../state/types";

/**
 * Companion cards deliberately have no free-form command field. The selected
 * identifier is supplied by the paired Mac and is opaque to the panel UI.
 */
export function registerCompanionCardTypes(
    registry: CardRegistry,
    fields: ControlsFieldsFeature,
    deviceConfig: DeviceConfig,
): void {
    const { cardBadgePreview } = fields;

    function companionSupported() {
        return !!deviceConfig.features?.companion;
    }

    function normalizeCompanionCard(card?: any) {
        if (!card) return;
        card.type = "companion";
        card.sensor = "";
        card.unit = "";
        card.precision = "";
        card.options = "";
        card.icon_on = "Auto";
        if (!card.icon || card.icon === "Auto") card.icon = "Monitor";
    }

    function fetchActions() {
        return fetch("/companion/actions", { cache: "no-store" })
            .then(function (response: Response) {
                if (!response.ok) throw new Error("HTTP " + response.status);
                return response.json();
            });
    }

    const metadata: any = {
        icon: {
            pickerIdSuffix: "icon-picker",
            idSuffix: "icon",
            field: "icon",
            fallback: "Monitor",
        },
        preview: { badge: "monitor" },
    };

    registry.register("companion", {
        label: function () { return cardContractCardLabel("companion"); },
        allowInSubpage: function () { return cardContractAllowInSubpage("companion"); },
        pickerKey: function () { return cardContractPickerKey("companion"); },
        hidden: function () { return cardContractHidden("companion"); },
        labelPlaceholder: "e.g. Safari",
        defaultConfig: function () { return cardContractDefaultConfig("companion"); },
        cardMetadata: metadata,
        isAvailable: companionSupported,
        onSelect: function (card?: any) {
            const defaults: any = cardContractDefaultConfig("companion");
            Object.keys(defaults).forEach(function (key) { card[key] = defaults[key]; });
        },
        renderSettings: function (panel?: any, card?: any, _slot?: any, helpers?: any) {
            normalizeCompanionCard(card);
            const field: any = document.createElement("div");
            field.className = "sp-field";
            field.appendChild(helpers.fieldLabel("Mac App", helpers.idPrefix + "companion-action"));
            const select: any = document.createElement("select");
            select.className = "sp-select";
            select.id = helpers.idPrefix + "companion-action";
            select.disabled = true;
            const loading = document.createElement("option");
            loading.value = "";
            loading.textContent = "Loading approved apps…";
            select.appendChild(loading);
            field.appendChild(select);
            panel.appendChild(field);

            fetchActions().then(function (actions: any) {
                select.innerHTML = "";
                const placeholder = document.createElement("option");
                placeholder.value = "";
                placeholder.textContent = actions.length ? "Choose an approved app…" : "No Mac companion is connected";
                select.appendChild(placeholder);
                actions.forEach(function (action: any) {
                    const option = document.createElement("option");
                    option.value = action.id;
                    option.textContent = action.label;
                    option.selected = action.id === card.entity;
                    select.appendChild(option);
                });
                if (card.entity && !actions.some(function (action: any) { return action.id === card.entity; })) {
                    const unavailable = document.createElement("option");
                    unavailable.value = card.entity;
                    unavailable.textContent = "Unavailable (" + card.entity + ")";
                    unavailable.selected = true;
                    select.appendChild(unavailable);
                }
                select.disabled = !actions.length;
            }).catch(function () {
                select.innerHTML = "";
                const unavailable = document.createElement("option");
                unavailable.value = card.entity || "";
                unavailable.textContent = card.entity ? "Unavailable (companion offline)" : "Mac companion unavailable";
                unavailable.selected = true;
                select.appendChild(unavailable);
            });
            select.addEventListener("change", function (this: HTMLSelectElement) {
                card.entity = this.value;
                helpers.saveField("entity", card.entity);
            });
            helpers.renderBasicCardFields(panel, card, helpers, metadata, { entity: false });
        },
        renderPreview: function (card?: any, helpers?: any) {
            return cardBadgePreview(card, helpers, {
                label: card.label || card.entity || "Mac App",
                iconFallback: "Monitor",
                badge: metadata.preview.badge,
            });
        },
    });
}
