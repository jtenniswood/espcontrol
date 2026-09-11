import { configOptionValue, setConfigOptionValue } from "../model/config_primitives";

export const WEBHOOK_HEADERS_OPTION = "webhook_headers";

export const WEBHOOK_METHODS = [
    ["GET", "GET"],
    ["POST", "POST"],
    ["PUT", "PUT"],
    ["PATCH", "PATCH"],
    ["DELETE", "DELETE"],
] as const;

export function createConfigWebhookOptionsFeature() {
    function webhookMethod(this: any, value?: any) {
        value = String(value || "").trim().toUpperCase();
        for (let index = 0; index < WEBHOOK_METHODS.length; index++) {
            const method = WEBHOOK_METHODS[index];
            if (method && method[0] === value)
                return value;
        }
        return "GET";
    }

    function webhookHeaders(this: any, button?: any) {
        return configOptionValue(button && button.options, WEBHOOK_HEADERS_OPTION);
    }

    function setWebhookHeaders(this: any, button?: any, value?: any) {
        if (!button)
            return "";
        button.options = setConfigOptionValue(button.options, WEBHOOK_HEADERS_OPTION, value || "");
        return button.options;
    }

    function normalizeWebhookConfig(this: any, button?: any) {
        if (!button)
            return;
        button.sensor = webhookMethod(button.sensor);
        button.icon_on = "Auto";
        button.precision = "";
        if (button.sensor === "GET" || button.sensor === "DELETE")
            button.unit = "";
        if (!button.icon)
            button.icon = "Auto";
        const headers = webhookHeaders(button);
        button.options = headers ? setConfigOptionValue("", WEBHOOK_HEADERS_OPTION, headers) : "";
    }

    return {
        methods: WEBHOOK_METHODS,
        normalizeWebhookConfig,
        setWebhookHeaders,
        webhookHeaders,
        webhookMethod,
    };
}

export type ConfigWebhookOptionsFeature = ReturnType<typeof createConfigWebhookOptionsFeature>;
