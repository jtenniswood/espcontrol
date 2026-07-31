import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerPushCardTypes(): GlobalDescriptors {
    // Momentary trigger card: stored as "push" for config compatibility.
    // Fires an esphome.push_button_pressed event with no toggle state.
    var PUSH_CARD_METADATA: any = {
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
    function pushActionSpec(this: any) {
        var card: any = cardContractCard("push");
        return card && card.behavior && card.behavior.pushAction || {};
    }
    function pushDefaultIcon(this: any) {
        return pushActionSpec().defaultIcon || "Gesture Tap";
    }
    function pushDefaultIconOn(this: any) {
        return pushActionSpec().defaultIconOn || "Auto";
    }
    registerButtonType("push", {
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
    return {
        "PUSH_CARD_METADATA": liveGlobal(() => PUSH_CARD_METADATA, (value?: any) => { PUSH_CARD_METADATA = value; }),
        "pushActionSpec": staticGlobal(pushActionSpec),
        "pushDefaultIcon": staticGlobal(pushDefaultIcon),
        "pushDefaultIconOn": staticGlobal(pushDefaultIconOn),
    };
}
