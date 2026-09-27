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
import { configOptionEnabled, setConfigOption } from "../model/config_primitives";
import { CARD_SIZE_LARGE, CARD_SIZE_WIDE, cardSizeDefinition } from "../model/grid";

export function registerClockCardTypes(
    registry: CardRegistry,
    dateTimeOptions: ConfigDateTimeOptionsFeature,
    fields: ControlsFieldsFeature,
): void {
    const { cardLargeNumbersActiveForCardSize, cardSensorPreviewHtml } = fields;
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
            const centerClock: any = (metadata as any).centerClock || {};
            if (centerClock.supportedCardSize(b, helpers) && cardLargeNumbersActiveForCardSize(b, helpers, metadata)) {
                const centerToggle: any = helpers.toggleRow(
                    centerClock.label,
                    helpers.idPrefix + centerClock.idSuffix,
                    configOptionEnabled(b.options, "center_clock"),
                );
                panel.appendChild(centerToggle.row);
                centerToggle.input.addEventListener("change", function (this: any) {
                    b.options = setConfigOption(b.options, "center_clock", this.checked);
                    helpers.saveField("options", b.options);
                });
            }
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var time: any = dateTimeCardTimeParts();
            const cardSize = (helpers && helpers.cardSize) || 1;
            const cardDefinition = cardSizeDefinition(cardSize);
            const largeClock = cardLargeNumbersActiveForCardSize(b, helpers, metadata);
            const usesWideLayout = cardSize === CARD_SIZE_WIDE || cardDefinition.colSpan > 2;
            const centered = largeClock && usesWideLayout && configOptionEnabled(b.options, "center_clock");
            return {
                buttonClass: [
                    largeClock ? "sp-clock-large" : "",
                    largeClock && usesWideLayout ? (centered ? "sp-clock-centered" : "sp-clock-left-mid") : "",
                ].filter(Boolean).join(" ") || undefined,
                iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
                labelHtml: "",
            };
        },
    });
}
