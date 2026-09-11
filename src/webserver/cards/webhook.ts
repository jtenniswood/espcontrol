import {
    cardContractAllowInSubpage,
    cardContractCard,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ConfigWebhookOptionsFeature } from "../application/config_webhook_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerWebhookCardTypes(
    registry: CardRegistry,
    webhookOptions: ConfigWebhookOptionsFeature,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { renderButtonSettings } = cardUi;
    const { cardBadgePreview } = fields;
    const {
        methods,
        normalizeWebhookConfig,
        setWebhookHeaders,
        webhookHeaders,
        webhookMethod,
    } = webhookOptions;
    // Webhook card: sends a direct HTTP request from the panel.
    const WEBHOOK_CARD_METADATA: any = {
        url: {
            label: "URL",
            idSuffix: "webhook-url",
            placeholder: "e.g. http://jeedom.local/core/api/jeeApi.php?...",
        },
        method: {
            label: "Type",
            idSuffix: "webhook-method",
            options: methods,
        },
        icon: {
            pickerIdSuffix: "webhook-icon-picker",
            idSuffix: "webhook-icon",
            field: "icon",
            fallback: "Auto",
        },
        preview: {
            badge: "webhook",
        },
    };
    registry.register("webhook", {
        label: function (this: any) { return cardContractCardLabel("webhook"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("webhook"); },
        pickerKey: function (this: any) { return cardContractPickerKey("webhook"); },
        hidden: function (this: any) { return cardContractHidden("webhook"); },
        labelPlaceholder: "e.g. Gate Open",
        defaultConfig: function (this: any) { return cardContractDefaultConfig("webhook"); },
        cardMetadata: WEBHOOK_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("webhook");
            Object.keys(defaults).forEach(function (this: any, key?: any) {
                if (key !== "label")
                    b[key] = defaults[key];
            });
        },
        renderSettingsBeforeLabel: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            normalizeWebhookConfig(b);
            var webhookSettingsDisclosure: any = helpers.disclosureSection("Webhook Settings", helpers.idPrefix + "webhook-settings", false);
            var webhookSettings: any = webhookSettingsDisclosure.section;
            var methodField: any = helpers.selectField(WEBHOOK_CARD_METADATA.method.label, helpers.idPrefix + WEBHOOK_CARD_METADATA.method.idSuffix, WEBHOOK_CARD_METADATA.method.options, webhookMethod(b.sensor), function (this: any) {
                b.sensor = webhookMethod(this.value);
                helpers.saveField("sensor", b.sensor);
                if (b.sensor === "GET" || b.sensor === "DELETE") {
                    b.unit = "";
                    helpers.saveField("unit", "");
                }
                renderButtonSettings();
            });
            webhookSettings.appendChild(methodField.field);
            panel.appendChild(webhookSettingsDisclosure.panel);
            var cardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "webhook-card-settings", false);
            panel.appendChild(cardSettingsDisclosure.panel);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            normalizeWebhookConfig(b);
            var webhookSettingsButton: any = panel.querySelector("#" + helpers.idPrefix + "webhook-settings");
            var webhookSettings: any = webhookSettingsButton && webhookSettingsButton.nextElementSibling || panel;
            var urlField: any = helpers.textField(WEBHOOK_CARD_METADATA.url.label, helpers.idPrefix + WEBHOOK_CARD_METADATA.url.idSuffix, b.entity, WEBHOOK_CARD_METADATA.url.placeholder, "entity", true);
            webhookSettings.appendChild(urlField.field);
            helpers.requireField(urlField.input, "Add a webhook URL before saving.");
            if (b.sensor !== "GET" && b.sensor !== "DELETE") {
                var bodyField: any = helpers.textField("Body", helpers.idPrefix + "webhook-body", b.unit, "e.g. {\"value1\":\"Gate\"}", "unit", false);
                webhookSettings.appendChild(bodyField.field);
            }
            var headersField: any = helpers.textField("Headers", helpers.idPrefix + "webhook-headers", webhookHeaders(b), "e.g. Content-Type: application/json; Authorization: Bearer token", null, false);
            webhookSettings.appendChild(headersField.field);
            headersField.input.addEventListener("input", saveHeaders);
            headersField.input.addEventListener("change", saveHeaders);
            headersField.input.addEventListener("blur", saveHeaders);
            headersField.input.addEventListener("keydown", function (this: any, e?: any) {
                if (e.key === "Enter") {
                    saveHeaders();
                    this.blur();
                }
            });
            helpers.renderBasicCardFields(panel, b, helpers, WEBHOOK_CARD_METADATA, {
                entity: false,
                label: false,
            });
            function saveHeaders(this: any) {
                helpers.saveField("options", setWebhookHeaders(b, headersField.input.value));
            }
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || b.entity || "Webhook";
            return cardBadgePreview(b, helpers, {
                label: label,
                iconFallback: "Flash",
                badge: WEBHOOK_CARD_METADATA.preview.badge,
            });
        },
    });
}
