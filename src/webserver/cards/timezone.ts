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

export function registerTimezoneCardTypes(
    registry: CardRegistry,
    dateTimeOptions: ConfigDateTimeOptionsFeature,
    documentService: Document,
    fields: ControlsFieldsFeature,
): void {
    const { cardBadgeLabelHtml, cardLargeNumbersHidePreviewLabel, cardSensorPreviewHtml } = fields;
    const {
        appendTimezoneOption,
        defaultTimezoneCardEntity,
        metadata,
        timezoneCardCityLabel,
        timezoneCardTimeParts,
        timezoneOptionsFor,
    } = dateTimeOptions;
    // Read-only world clock card: displays local time for a selected city.
    registry.register("timezone", {
        label: function (this: any) { return cardContractCardLabel("timezone"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("timezone"); },
        pickerKey: function (this: any) { return cardContractPickerKey("timezone"); },
        hidden: function (this: any) { return cardContractHidden("timezone"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("timezone"); },
        isAvailable: function (this: any) {
            return false;
        },
        cardMetadata: metadata,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("timezone");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
            b.entity = defaultTimezoneCardEntity();
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            if (!b.entity)
                b.entity = defaultTimezoneCardEntity();
            if (b.label) {
                b.label = "";
                helpers.saveField("label", "");
            }
            helpers.renderCardModeSelector(panel, b, helpers, metadata);
            helpers.renderCardLargeNumbersToggle(panel, b, helpers, metadata);
            var tzSelect: any = documentService.createElement("select");
            tzSelect.className = "sp-select";
            tzSelect.id = helpers.idPrefix + "timezone";
            var options: any = timezoneOptionsFor(b.entity);
            options.forEach(function (this: any, opt?: any) {
                appendTimezoneOption(tzSelect, opt);
            });
            tzSelect.value = b.entity;
            tzSelect.addEventListener("change", function (this: any) {
                b.entity = this.value;
                b.label = "";
                helpers.saveField("entity", b.entity);
                helpers.saveField("label", "");
            });
            var timezoneField: any = helpers.fieldWithControl("City / Timezone", helpers.idPrefix + "timezone", tzSelect);
            panel.appendChild(timezoneField);
            helpers.markCardPrimaryField(timezoneField, "entity");
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var tz: any = b.entity || defaultTimezoneCardEntity();
            var time: any = timezoneCardTimeParts(tz);
            var hideLabel: any = cardLargeNumbersHidePreviewLabel(b, helpers, metadata);
            return {
                buttonClass: hideLabel ? "sp-date-time-wide-large" : undefined,
                iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
                labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, timezoneCardCityLabel(tz), metadata.preview.timezoneBadge),
            };
        },
    });
}
