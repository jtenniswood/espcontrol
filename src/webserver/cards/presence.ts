import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerPresenceCardTypes(): GlobalDescriptors {
    // Read-only presence card: shows a sensor where Detected is active and Clear is inactive.
    var PRESENCE_CARD_METADATA: any = {
        entity: {
            label: "Sensor Entity",
            idSuffix: "sensor",
            placeholder: "e.g. binary_sensor.living_room_presence",
            domains: function (this: any) { return cardContractDomains("presence"); },
            bindName: "sensor",
            rerender: true,
            requiredMessage: "Add a presence sensor before saving.",
        },
        labelField: {
            label: "Label",
            idSuffix: "label",
            field: "label",
            placeholder: "e.g. Living Room",
            rerender: true,
        },
        iconOff: {
            pickerIdSuffix: "clear-icon-picker",
            idSuffix: "icon",
            field: "icon",
            label: "Clear Icon",
            fallback: "Motion Sensor Off",
        },
        iconOn: {
            pickerIdSuffix: "detected-icon-picker",
            idSuffix: "icon-on",
            field: "icon_on",
            label: "Detected Icon",
            fallback: "Motion Sensor",
        },
        activeColor: {
            label: "Lit When Detected",
            idSuffix: "presence-active-color",
            checked: presenceActiveColorEnabled,
        },
    };
    registerButtonType("presence", {
        label: function (this: any) { return cardContractCardLabel("presence"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("presence"); },
        pickerKey: function (this: any) { return cardContractPickerKey("presence"); },
        hidden: function (this: any) { return cardContractHidden("presence"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("presence"); },
        cardMetadata: PRESENCE_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("presence");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            b.entity = "";
            b.unit = "";
            b.precision = "";
            b.options = normalizePresenceOptions(b.options);
            if (!b.icon || b.icon === "Auto")
                b.icon = "Motion Sensor Off";
            if (!b.icon_on || b.icon_on === "Auto")
                b.icon_on = "Motion Sensor";
            helpers.renderBasicCardFields(panel, b, helpers, PRESENCE_CARD_METADATA);
            helpers.renderCardActiveColorToggle(panel, b, helpers, PRESENCE_CARD_METADATA.activeColor, setPresenceActiveColorEnabled);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || b.sensor || "Presence";
            return cardBadgePreview(b, helpers, {
                label: label,
                iconFallback: "Motion Sensor Off",
                badge: "motion-sensor",
            });
        },
    });
    return {
        "PRESENCE_CARD_METADATA": liveGlobal(() => PRESENCE_CARD_METADATA, (value?: any) => { PRESENCE_CARD_METADATA = value; }),
    };
}
