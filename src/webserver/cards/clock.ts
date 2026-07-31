import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerClockCardTypes(): GlobalDescriptors {
    // Read-only local clock card: displays the panel's local time only.
    registerButtonType("clock", {
        label: function (this: any) { return cardContractCardLabel("clock"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("clock"); },
        pickerKey: function (this: any) { return cardContractPickerKey("clock"); },
        hidden: function (this: any) { return cardContractHidden("clock"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("clock"); },
        isAvailable: function (this: any) {
            return false;
        },
        cardMetadata: DATE_TIME_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("clock");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            b.entity = "";
            b.label = "";
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            helpers.renderCardModeSelector(panel, b, helpers, DATE_TIME_CARD_METADATA);
            helpers.renderCardLargeNumbersToggle(panel, b, helpers, DATE_TIME_CARD_METADATA);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var time: any = dateTimeCardTimeParts();
            return {
                buttonClass: cardLargeNumbersHidePreviewLabel(b, helpers, DATE_TIME_CARD_METADATA)
                    ? "sp-clock-wide-large"
                    : undefined,
                iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
                labelHtml: "",
            };
        },
    });
    return {};
}
