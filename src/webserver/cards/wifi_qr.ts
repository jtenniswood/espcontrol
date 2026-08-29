import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
} from "../generated/card_contract";
import {
    configOptionEnabled,
    configOptionValue,
    setConfigOption,
    setConfigOptionValue,
} from "../model/config_primitives";
import type { CardRegistry } from "../application/card_registry";
import type { ControlsFieldsFeature } from "../application/controls_fields";

// Credentials are encoded only to keep the compact card format unambiguous.
// This is deliberately not encryption: exported backups contain the password.
export function registerWifiQrCardTypes(registry: CardRegistry, fields: ControlsFieldsFeature): void {
    const SSID_OPTION = "ssid64";
    const SECURITY_OPTION = "security";
    const PASSWORD_OPTION = "pass64";
    const HIDDEN_OPTION = "hidden";
    const { cardBadgePreview } = fields;
    const WIFI_QR_CARD_METADATA: any = {
        labelField: { label: "Card title", idSuffix: "wifi-label", placeholder: "Guest Wi-Fi", bindName: "label", rerender: true },
        icon: { pickerIdSuffix: "wifi-icon-picker", idSuffix: "wifi-icon", field: "icon", fallback: "Wifi" },
    };
    function utf8Bytes(this: any, value?: any): any[] {
        return Array.prototype.slice.call(new TextEncoder().encode(String(value || "")));
    }
    function base64urlEncode(this: any, value?: any) {
        var bytes: any = utf8Bytes(value);
        var binary: any = "";
        bytes.forEach(function (this: any, byte?: any) { binary += String.fromCharCode(byte); });
        return btoa(binary).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
    }
    function base64urlDecode(this: any, value?: any) {
        try {
            var encoded: any = String(value || "").replace(/-/g, "+").replace(/_/g, "/");
            while (encoded.length % 4) encoded += "=";
            var binary: any = atob(encoded);
            return new TextDecoder().decode(Uint8Array.from(binary, function (this: any, c?: any) { return c.charCodeAt(0); }));
        } catch (_error) { return ""; }
    }
    function wifiQrSecurity(this: any, b?: any) {
        return configOptionValue(b && b.options, SECURITY_OPTION) === "open" ? "open" : "wpa";
    }
    function wifiQrSsid(this: any, b?: any) { return base64urlDecode(configOptionValue(b && b.options, SSID_OPTION)); }
    function wifiQrPassword(this: any, b?: any) { return base64urlDecode(configOptionValue(b && b.options, PASSWORD_OPTION)); }
    function wifiQrHidden(this: any, b?: any) { return configOptionEnabled(b && b.options, HIDDEN_OPTION); }
    function validWifiQrPassword(this: any, password?: any) {
        var bytes: any = utf8Bytes(password).length;
        return (bytes >= 8 && bytes <= 63) || (bytes === 64 && /^[0-9a-fA-F]{64}$/.test(String(password || "")));
    }
    function normalizeWifiQrConfig(this: any, b?: any) {
        if (!b) return;
        b.type = "wifi_qr";
        b.entity = ""; b.sensor = ""; b.unit = ""; b.precision = ""; b.icon_on = "Auto";
        if (!b.label) b.label = "Guest Wi-Fi";
        if (!b.icon || b.icon === "Auto") b.icon = "Wifi";
        var ssid: any = wifiQrSsid(b);
        var password: any = wifiQrPassword(b);
        var security: any = wifiQrSecurity(b);
        var options: any = ssid ? setConfigOptionValue("", SSID_OPTION, base64urlEncode(ssid)) : "";
        if (security === "open") options = setConfigOptionValue(options, SECURITY_OPTION, "open");
        else if (password) options = setConfigOptionValue(options, PASSWORD_OPTION, base64urlEncode(password));
        if (wifiQrHidden(b)) options = setConfigOption(options, HIDDEN_OPTION, true);
        b.options = options;
    }
    function updateOptions(this: any, b?: any, ssid?: any, security?: any, password?: any, hidden?: any) {
        if (!b) return;
        b.options = "";
        if (ssid) b.options = setConfigOptionValue(b.options, SSID_OPTION, base64urlEncode(ssid));
        if (security === "open") b.options = setConfigOptionValue(b.options, SECURITY_OPTION, "open");
        else if (password) b.options = setConfigOptionValue(b.options, PASSWORD_OPTION, base64urlEncode(password));
        if (hidden) b.options = setConfigOption(b.options, HIDDEN_OPTION, true);
        normalizeWifiQrConfig(b);
    }
    registry.register("wifi_qr", {
        label: function (this: any) { return cardContractCardLabel("wifi_qr"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("wifi_qr"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("wifi_qr"); },
        cardMetadata: WIFI_QR_CARD_METADATA,
        normalizeConfig: normalizeWifiQrConfig,
        onSelect: normalizeWifiQrConfig,
        renderSettings: function (this: any, panel?: any, b?: any, _slot?: any, helpers?: any) {
            normalizeWifiQrConfig(b);
            var ssidField: any = helpers.textField("Network name (SSID)", helpers.idPrefix + "wifi-ssid", wifiQrSsid(b), "Guest Wi-Fi");
            var securityField: any = helpers.selectField("Security", helpers.idPrefix + "wifi-security", [["wpa", "WPA/WPA2 Personal"], ["open", "Open"]], wifiQrSecurity(b));
            var passwordField: any = helpers.textField("Password", helpers.idPrefix + "wifi-password", wifiQrPassword(b), "8–63 characters, or 64 hexadecimal characters");
            passwordField.input.type = "password";
            var reveal: any = helpers.toggleRow("Show password", helpers.idPrefix + "wifi-reveal", false);
            var hidden: any = helpers.toggleRow("Hidden network", helpers.idPrefix + "wifi-hidden", wifiQrHidden(b));
            panel.appendChild(ssidField.field); panel.appendChild(securityField.field); panel.appendChild(passwordField.field);
            panel.appendChild(reveal.row); panel.appendChild(hidden.row);
            helpers.requireField(ssidField.input, "Add a network name before saving.");
            helpers.requireField(passwordField.input, "Add a Wi-Fi password before saving.", function () { return securityField.select.value === "wpa"; });
            helpers.renderBasicCardFields(panel, b, helpers, WIFI_QR_CARD_METADATA, { entity: false });
            function save(this: any) {
                var ssid: any = ssidField.input.value;
                var security: any = securityField.select.value;
                var password: any = passwordField.input.value;
                updateOptions(b, ssid, security, password, hidden.input.checked);
                helpers.saveField("options", b.options);
                passwordField.field.hidden = security === "open";
                if (ssid && utf8Bytes(ssid).length > 32) ssidField.input.setCustomValidity("The network name must be 32 bytes or fewer.");
                else ssidField.input.setCustomValidity("");
                if (security === "wpa" && password && !validWifiQrPassword(password)) passwordField.input.setCustomValidity("Use 8–63 bytes, or exactly 64 hexadecimal characters.");
                else passwordField.input.setCustomValidity("");
                if (b.options.length > 255) ssidField.input.setCustomValidity("These credentials are too long to save on the panel.");
            }
            [ssidField.input, securityField.select, passwordField.input, hidden.input].forEach(function (this: any, input?: any) {
                input.addEventListener("input", save); input.addEventListener("change", save); input.addEventListener("blur", save);
            });
            reveal.input.addEventListener("change", function (this: any) { passwordField.input.type = this.checked ? "text" : "password"; });
            save();
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return cardBadgePreview(b, helpers, { label: b.label || "Guest Wi-Fi", iconFallback: "Wifi", badge: "Wi-Fi Share" });
        },
    });
}
