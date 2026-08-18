import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry } from "../application/card_registry";
import type { ConfigDateTimeOptionsFeature } from "../application/config_date_time_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerCalendarCardTypes(
    registry: CardRegistry,
    dateTimeOptions: ConfigDateTimeOptionsFeature,
    fields: ControlsFieldsFeature,
): void {
    const { cardBadgeLabelHtml, cardLargeNumbersHidePreviewLabel, cardSensorPreviewHtml } = fields;
    const {
        dateTimeCardTimeParts,
        metadata,
        monthNameForIndex,
        now,
    } = dateTimeOptions;

    registry.register("calendar", {
        label: function () { return cardContractCardLabel("calendar"); },
        allowInSubpage: function () { return cardContractAllowInSubpage("calendar"); },
        pickerKey: function () { return cardContractPickerKey("calendar"); },
        hidden: function () { return cardContractHidden("calendar"); },
        hideLabel: true,
        defaultConfig: function () { return cardContractDefaultConfig("calendar"); },
        cardMetadata: metadata,
        onSelect: function (button?: any) {
            const defaults: any = cardContractDefaultConfig("calendar");
            Object.keys(defaults).forEach(function (key) { button[key] = defaults[key]; });
            button.precision = button.precision === "datetime" ? "datetime" : "";
        },
        renderSettings: function (panel?: any, button?: any, _slot?: any, helpers?: any) {
            if (!button.entity) button.entity = "sensor.date";
            if (button.precision !== "datetime") button.precision = "";
            helpers.renderCardModeSelector(panel, button, helpers, metadata);
            helpers.renderCardLargeNumbersToggle(panel, button, helpers, metadata);
        },
        renderPreview: function (button?: any, helpers?: any) {
            const current = now();
            const isDateTime = button.precision === "datetime";
            const hideLabel = cardLargeNumbersHidePreviewLabel(button, helpers, metadata);
            const buttonClass = hideLabel
                ? (isDateTime ? "sp-clock-wide-large" : "sp-date-time-wide-large")
                : undefined;
            const day = String(current.getUTCDate());
            const month = monthNameForIndex(current.getUTCMonth());
            if (isDateTime) {
                const time = dateTimeCardTimeParts();
                return {
                    buttonClass,
                    iconHtml: cardSensorPreviewHtml(button, helpers, time.value, time.unit),
                    labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, day + " " + month, metadata.preview.dateBadge),
                };
            }
            return {
                buttonClass,
                iconHtml: cardSensorPreviewHtml(button, helpers, day, null),
                labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, month, metadata.preview.dateBadge),
            };
        },
    });
}
