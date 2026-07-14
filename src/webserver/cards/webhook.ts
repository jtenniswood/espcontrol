import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerWebhookCardTypes(): GlobalDescriptors {
    // Webhook card: sends a direct HTTP request from the panel.
    var WEBHOOK_HEADERS_OPTION: any = "webhook_headers";
    var WEBHOOK_METHODS: any = [
        ["GET", "GET"],
        ["POST", "POST"],
        ["PUT", "PUT"],
        ["PATCH", "PATCH"],
        ["DELETE", "DELETE"],
    ];
    function webhookMethod(this: any, value?: any) {
        value = String(value || "").trim().toUpperCase();
        for (var i: any = 0; i < WEBHOOK_METHODS.length; i++) {
            if (WEBHOOK_METHODS[i][0] === value)
                return value;
        }
        return "GET";
    }
    function webhookHeaders(this: any, b?: any) {
        return configOptionValue(b && b.options, WEBHOOK_HEADERS_OPTION);
    }
    function setWebhookHeaders(this: any, b?: any, value?: any) {
        if (!b)
            return "";
        b.options = setConfigOptionValue(b.options, WEBHOOK_HEADERS_OPTION, value || "");
        return b.options;
    }
    function normalizeWebhookConfig(this: any, b?: any) {
        if (!b)
            return;
        b.sensor = webhookMethod(b.sensor);
        b.icon_on = "Auto";
        b.precision = "";
        if (b.sensor === "GET" || b.sensor === "DELETE")
            b.unit = "";
        if (!b.icon)
            b.icon = "Auto";
        var headers: any = webhookHeaders(b);
        b.options = headers ? setConfigOptionValue("", WEBHOOK_HEADERS_OPTION, headers) : "";
    }
    var WEBHOOK_CARD_METADATA: any = {
        url: {
            label: "URL",
            idSuffix: "webhook-url",
            placeholder: "e.g. http://jeedom.local/core/api/jeeApi.php?...",
        },
        method: {
            label: "Method",
            idSuffix: "webhook-method",
            options: WEBHOOK_METHODS,
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
    registerButtonType("webhook", {
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
            var methodField: any = helpers.selectField(WEBHOOK_CARD_METADATA.method.label, helpers.idPrefix + WEBHOOK_CARD_METADATA.method.idSuffix, WEBHOOK_CARD_METADATA.method.options, webhookMethod(b.sensor), function (this: any) {
                b.sensor = webhookMethod(this.value);
                helpers.saveField("sensor", b.sensor);
                if (b.sensor === "GET" || b.sensor === "DELETE") {
                    b.unit = "";
                    helpers.saveField("unit", "");
                }
                renderButtonSettings();
            });
            panel.appendChild(methodField.field);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            normalizeWebhookConfig(b);
            var urlField: any = helpers.textField(WEBHOOK_CARD_METADATA.url.label, helpers.idPrefix + WEBHOOK_CARD_METADATA.url.idSuffix, b.entity, WEBHOOK_CARD_METADATA.url.placeholder, "entity", true);
            panel.appendChild(urlField.field);
            helpers.requireField(urlField.input, "Add a webhook URL before saving.");
            if (b.sensor !== "GET" && b.sensor !== "DELETE") {
                var bodyField: any = helpers.textField("Body", helpers.idPrefix + "webhook-body", b.unit, "e.g. {\"value1\":\"Gate\"}", "unit", false);
                panel.appendChild(bodyField.field);
            }
            var headersField: any = helpers.textField("Headers", helpers.idPrefix + "webhook-headers", webhookHeaders(b), "e.g. Content-Type: application/json; Authorization: Bearer token", null, false);
            panel.appendChild(headersField.field);
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
    return {
        "WEBHOOK_HEADERS_OPTION": liveGlobal(() => WEBHOOK_HEADERS_OPTION, (value?: any) => { WEBHOOK_HEADERS_OPTION = value; }),
        "WEBHOOK_METHODS": liveGlobal(() => WEBHOOK_METHODS, (value?: any) => { WEBHOOK_METHODS = value; }),
        "webhookMethod": staticGlobal(webhookMethod),
        "webhookHeaders": staticGlobal(webhookHeaders),
        "setWebhookHeaders": staticGlobal(setWebhookHeaders),
        "normalizeWebhookConfig": staticGlobal(normalizeWebhookConfig),
        "WEBHOOK_CARD_METADATA": liveGlobal(() => WEBHOOK_CARD_METADATA, (value?: any) => { WEBHOOK_CARD_METADATA = value; }),
    };
}
