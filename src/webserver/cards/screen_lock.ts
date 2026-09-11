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
const SCREEN_LOCK_CARD_METADATA: any = {
    preview: {
        badge: "lock",
    },
};

export function registerScreenLockCardTypes(registry: CardRegistry, fields: ControlsFieldsFeature): void {
    const { cardBadgePreview } = fields;
    // Local display card: toggles screen lock on the device without Home Assistant.
    registry.register("screen_lock", {
        label: function (this: any) { return cardContractCardLabel("screen_lock"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("screen_lock"); },
        pickerKey: function (this: any) { return cardContractPickerKey("screen_lock"); },
        hidden: function (this: any) { return cardContractHidden("screen_lock"); },
        hideLabel: true,
        labelPlaceholder: "e.g. Screen Lock",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("screen_lock"); },
        cardMetadata: SCREEN_LOCK_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("screen_lock");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
        },
        // Screen Lock is entirely local and uses fixed translated labels/icons.
        // Defining an empty renderer prevents the generic Switch settings fallback.
        renderSettings: function () {},
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return cardBadgePreview(b, helpers, {
                label: "Screen Unlocked",
                iconFallback: "Lock Open",
                badge: SCREEN_LOCK_CARD_METADATA.preview.badge,
            });
        },
    });
}
