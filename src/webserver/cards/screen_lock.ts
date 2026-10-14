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
import { buildScreensaverPinSettings } from "../application/screensaver_pin";
import { configOptionEnabled, setConfigOption } from "../model/config_primitives";
const SCREEN_LOCK_CARD_METADATA: any = {
    preview: {
        badge: "lock",
    },
};

export function registerScreenLockCardTypes(
    registry: CardRegistry, fields: ControlsFieldsFeature, document: Document, request: typeof fetch,
): void {
    const { cardBadgePreview } = fields;
    // Local display card: opens the shared touchscreen PIN lock without Home Assistant.
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
        renderSettings: function (_panel: HTMLElement, button: any, _slot: number, helpers: any) {
            const behavior = helpers.disclosureSection("Screensaver", helpers.idPrefix + "screen-lock-behavior", false);
            const screensaver = fields.toggleRow("Screensaver on lock", helpers.idPrefix + "screensaver-on-lock",
                configOptionEnabled(button.options, "screensaver_on_lock"));
            screensaver.input.addEventListener("change", () => {
                button.options = setConfigOption(button.options, "screensaver_on_lock", screensaver.input.checked);
                helpers.saveField("options", button.options);
            });
            behavior.section.appendChild(screensaver.row);
            const section = helpers.disclosureSection("Pin Protection", helpers.idPrefix + "screen-lock-settings", true);
            const pinSettings = buildScreensaverPinSettings(document, request, fields, null, {
                idPrefix: helpers.idPrefix + "screen-lock-pin", requirePin: true,
            });
            section.section.appendChild(pinSettings.body);
            _panel.appendChild(section.panel);
            _panel.appendChild(behavior.panel);
            helpers.requireField(pinSettings.pinInput, "Save a PIN before saving this card.", undefined, pinSettings.hasSavedPin);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return cardBadgePreview(b, helpers, {
                label: "Screen Unlocked",
                iconFallback: "Lock Open",
                badge: SCREEN_LOCK_CARD_METADATA.preview.badge,
            });
        },
    });
}
