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
import type { ConfigDateTimeOptionsFeature } from "../application/config_date_time_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerClockCardTypes(
    registry: CardRegistry,
    dateTimeOptions: ConfigDateTimeOptionsFeature,
    fields: ControlsFieldsFeature,
): void {
    const { cardLargeNumbersHidePreviewLabel, cardSensorPreviewHtml } = fields;
    const { dateTimeCardTimeParts, metadata } = dateTimeOptions;
    // Read-only local clock card: displays the panel's local time only.
    registry.register("clock", {
        label: function (this: any) { return cardContractCardLabel("clock"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("clock"); },
        pickerKey: function (this: any) { return cardContractPickerKey("clock"); },
        hidden: function (this: any) { return cardContractHidden("clock"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("clock"); },
        isAvailable: function (this: any) {
            return false;
        },
        cardMetadata: metadata,
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
            helpers.renderCardModeSelector(panel, b, helpers, metadata);
            helpers.renderCardLargeNumbersToggle(panel, b, helpers, metadata);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var time: any = dateTimeCardTimeParts();
            return {
                buttonClass: cardLargeNumbersHidePreviewLabel(b, helpers, metadata)
                    ? "sp-clock-wide-large"
                    : undefined,
                iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
                labelHtml: "",
            };
        },
    });
}
