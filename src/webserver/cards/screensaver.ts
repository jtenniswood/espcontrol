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
const SCREENSAVER_CARD_METADATA: any = {
    preview: {
        badge: "screensaver",
    },
};

export function registerScreensaverCardTypes(registry: CardRegistry, fields: ControlsFieldsFeature): void {
    const { cardBadgePreview } = fields;
    // Local display card: starts the configured screensaver on the device without Home Assistant.
    registry.register("screensaver", {
        label: function (this: any) { return cardContractCardLabel("screensaver"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("screensaver"); },
        pickerKey: function (this: any) { return cardContractPickerKey("screensaver"); },
        hidden: function (this: any) { return cardContractHidden("screensaver"); },
        hideLabel: true,
        labelPlaceholder: "e.g. Screensaver",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("screensaver"); },
        cardMetadata: SCREENSAVER_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("screensaver");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
        },
        // Screensaver is entirely local and uses fixed translated labels/icons.
        // Defining an empty renderer prevents the generic Switch settings fallback.
        renderSettings: function () {},
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return cardBadgePreview(b, helpers, {
                label: "Screensaver",
                iconFallback: "Monitor",
                badge: SCREENSAVER_CARD_METADATA.preview.badge,
            });
        },
    });
}
