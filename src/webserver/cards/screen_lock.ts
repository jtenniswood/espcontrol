import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerScreenLockCardTypes(): GlobalDescriptors {
    // Local display card: toggles screen lock on the device without Home Assistant.
    var SCREEN_LOCK_CARD_METADATA: any = {
        preview: {
            badge: "lock",
        },
    };
    var SCREENSAVER_CARD_METADATA: any = {
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
    registerButtonType("screen_lock", {
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
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return cardBadgePreview(b, helpers, {
                label: "Screen Unlocked",
                iconFallback: "Lock Open",
                badge: SCREEN_LOCK_CARD_METADATA.preview.badge,
            });
        },
    });
    registerButtonType("screensaver", {
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
                label: b.label || "Screensaver",
                iconFallback: "Power",
                badge: "local",
            });
        },
    });
    return {
        "SCREEN_LOCK_CARD_METADATA": liveGlobal(() => SCREEN_LOCK_CARD_METADATA, (value?: any) => { SCREEN_LOCK_CARD_METADATA = value; }),
        "SCREENSAVER_CARD_METADATA": liveGlobal(() => SCREENSAVER_CARD_METADATA, (value?: any) => { SCREENSAVER_CARD_METADATA = value; }),
    };
}
