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
const SCREENSAVER_CARD_METADATA: any = {
    icon: {
        pickerIdSuffix: "icon-picker",
        idSuffix: "icon",
        field: "icon",
        fallback: "Power",
    },
    preview: {
        badge: "local",
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
    registry.register("screensaver", {
        label: function (this: any) { return cardContractCardLabel("screensaver"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("screensaver"); },
        pickerKey: function (this: any) { return cardContractPickerKey("screensaver"); },
        hidden: function (this: any) { return cardContractHidden("screensaver"); },
        labelPlaceholder: "e.g. Screen Off",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("screensaver"); },
        cardMetadata: SCREENSAVER_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("screensaver");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            helpers.renderBasicCardFields(panel, b, helpers, SCREENSAVER_CARD_METADATA);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return cardBadgePreview(b, helpers, {
                label: b.label || "Screensaver", iconFallback: "Power",
                badge: SCREENSAVER_CARD_METADATA.preview.badge,
            });
        },
    });
}
