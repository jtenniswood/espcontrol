import { state } from "../state/app_instance";
import {
    cardContractAllowInSubpage,
    cardContractCard,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";

const PUSH_CARD_METADATA: any = {
    icon: {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        fallback: "Auto",
    },
    preview: {
        badge: "gesture-tap",
    },
};

function pushActionSpec() {
    var card: any = cardContractCard("push");
    return card && card.behavior && card.behavior.pushAction || {};
}

export function pushDefaultIcon() {
    return pushActionSpec().defaultIcon || "Gesture Tap";
}

export function pushDefaultIconOn() {
    return pushActionSpec().defaultIconOn || "Auto";
}

export function registerPushCardTypes(registry: CardRegistry, fields: ControlsFieldsFeature): void {
    const { cardBadgePreview } = fields;
    // Momentary trigger card: stored as "push" for config compatibility.
    // Fires an esphome.push_button_pressed event with no toggle state.
    registry.register("push", {
        label: function (this: any) { return cardContractCardLabel("push"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("push"); },
        pickerKey: function (this: any) { return cardContractPickerKey("push"); },
        hidden: function (this: any) { return cardContractHidden("push"); },
        labelPlaceholder: "e.g. Doorbell",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("push"); },
        cardMetadata: PUSH_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("push");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            helpers.renderBasicCardFields(panel, b, helpers, PUSH_CARD_METADATA);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || "Trigger";
            return cardBadgePreview(b, helpers, {
                label: label,
                iconFallback: pushDefaultIcon(),
                badge: PUSH_CARD_METADATA.preview.badge,
            });
        },
    });
}
