import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractPickerKey,
} from "../generated/card_contract";
import {
    configOptionEnabled,
    configOptionValue,
    setConfigOption,
    setConfigOptionValue,
} from "../model/config_primitives";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ConfigModalTabOptionsFeature } from "../application/config_modal_tab_options";
import type { ControlsFieldsFeature } from "../application/controls_fields";

// Credentials are encoded only to keep the compact card format unambiguous.
// This is deliberately not encryption: exported backups contain the password.
export function registerWifiQrCardTypes(
    registry: CardRegistry,
    modalTabs: ConfigModalTabOptionsFeature,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): void {
    const { renderButtonSettings } = cardUi;
    const SSID_OPTION = "ssid64";
    const SECURITY_OPTION = "security";
    const PASSWORD_OPTION = "pass64";
    const HIDDEN_OPTION = "hidden";
    const { cardBadgePreview } = fields;
    const {
        wifiQrTabDefinitions,
        wifiQrTabs,
        normalizeWifiQrTabOptions,
        setWifiQrTabs,
        renderModalTabSettings,
    } = modalTabs;
    const WIFI_QR_CARD_TYPE_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "wifi-card-type",
            options: [["wifi_qr", "Connect Card"], ["wifi_qr_card", "QR Card"]],
            value: function (this: any, b?: any) { return isQrCard(b) ? "wifi_qr_card" : "wifi_qr"; },
            onChange: function (this: any, b?: any, helpers?: any) {
                setWifiQrCardType(b, this.value, helpers);
            },
        },
    };
    const WIFI_QR_CARD_METADATA: any = {
        mode: WIFI_QR_CARD_TYPE_METADATA.mode,
        labelField: { label: "Card title", idSuffix: "wifi-label", placeholder: "Connect", bindName: "label", rerender: true },
        icon: { pickerIdSuffix: "wifi-icon-picker", idSuffix: "wifi-icon", field: "icon", fallback: "Wifi" },
    };
    // A valid QR for representative (non-user) Wifi credentials. Keeping this
    // static avoids adding a QR generator to the already large web preview.
    const WIFI_QR_PREVIEW_SVG = '<svg class="sp-wifi-qr-preview" viewBox="3 3 31 31" shape-rendering="crispEdges" aria-hidden="true">' +
        '<path fill="#fff" d="M0 0h37v37H0z"/><path stroke="#000" d="M4 4.5h7m1 0h1m1 0h1m3 0h2m3 0h1m2 0h7M4 5.5h1m5 0h1m1 0h1m1 0h2m1 0h2m1 0h4m2 0h1m5 0h1M4 6.5h1m1 0h3m1 0h1m2 0h1m2 0h5m1 0h1m3 0h1m1 0h3m1 0h1M4 7.5h1m1 0h3m1 0h1m1 0h2m1 0h3m1 0h2m1 0h1m1 0h1m1 0h1m1 0h3m1 0h1M4 8.5h1m1 0h3m1 0h1m2 0h1m1 0h2m1 0h2m1 0h1m2 0h1m1 0h1m1 0h3m1 0h1M4 9.5h1m5 0h1m2 0h1m1 0h2m4 0h1m4 0h1m5 0h1M4 10.5h7m1 0h1m1 0h1m1 0h1m1 0h1m1 0h1m1 0h1m1 0h1m1 0h7M12 11.5h1m1 0h2m3 0h2m1 0h2M4 12.5h1m1 0h2m1 0h3m1 0h1m2 0h1m4 0h1m1 0h1m2 0h1m2 0h1m1 0h2M4 13.5h1m2 0h1m6 0h1m1 0h1m1 0h1m3 0h3m1 0h4m2 0h1M4 14.5h1m1 0h1m1 0h3m1 0h1m1 0h2m1 0h2m1 0h3m1 0h2m1 0h1m3 0h2M5 15.5h3m1 0h1m1 0h1m3 0h2m1 0h2m4 0h2m1 0h1m3 0h1M7 16.5h1m1 0h7m1 0h1m1 0h2m3 0h1m4 0h4M8 17.5h2m1 0h1m1 0h2m1 0h1m4 0h2m1 0h2m1 0h3m2 0h1M4 18.5h2m1 0h1m1 0h3m1 0h1m2 0h1m3 0h3m2 0h8M4 19.5h1m2 0h3m1 0h2m2 0h4m1 0h2m1 0h4m1 0h3M5 20.5h3m2 0h4m1 0h1m1 0h2m2 0h1m3 0h1m2 0h1m2 0h1M5 21.5h1m2 0h2m4 0h1m2 0h2m1 0h1m3 0h3M4 22.5h1m5 0h3m1 0h1m4 0h3m2 0h1m2 0h2m2 0h1M6 23.5h4m2 0h1m2 0h1m2 0h1m1 0h2m3 0h2m2 0h4M5 24.5h1m4 0h2m1 0h3m1 0h1m5 0h8M12 25.5h1m1 0h4m1 0h2m3 0h1m3 0h3M4 26.5h7m1 0h1m3 0h2m4 0h1m1 0h1m1 0h1m1 0h2m1 0h1M4 27.5h1m5 0h1m1 0h4m3 0h1m1 0h1m1 0h2m3 0h1m2 0h2M4 28.5h1m1 0h3m1 0h1m2 0h1m2 0h1m1 0h2m4 0h5M4 29.5h1m1 0h3m1 0h1m1 0h2m1 0h4m1 0h3m5 0h1m1 0h1M4 30.5h1m1 0h3m1 0h1m1 0h1m1 0h1m1 0h8m1 0h1m2 0h3m1 0h1M4 31.5h1m5 0h1m3 0h1m3 0h1m1 0h1m6 0h1m3 0h2M4 32.5h7m1 0h5m4 0h1m1 0h5m2 0h1"/></svg>';
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
    function isLegacyWifiQrTitle(this: any, label?: any) {
        var normalized: any = String(label || "").trim().toLowerCase().replace(/[\s\-\u2011]+/g, "");
        return normalized === "guestwifi" || normalized === "guestswifi";
    }
    function isQrCard(this: any, b?: any) { return !!b && b.type === "wifi_qr_card"; }
    function setWifiQrCardType(this: any, b?: any, type?: any, helpers?: any) {
        var nextType: any = type === "wifi_qr_card" ? "wifi_qr_card" : "wifi_qr";
        if (!b || b.type === nextType) return;
        b.type = nextType;
        normalizeWifiQrConfig(b);
        helpers.saveField("type", b.type);
        helpers.saveField("label", b.label || "");
        helpers.saveField("icon", b.icon || "Auto");
        helpers.saveField("icon_on", b.icon_on || "Auto");
        helpers.saveField("options", b.options || "");
        renderButtonSettings();
    }
    function normalizeWifiQrConfig(this: any, b?: any) {
        if (!b) return;
        var tabs: any = wifiQrTabs(b);
        var qrCard: any = isQrCard(b);
        b.type = qrCard ? "wifi_qr_card" : "wifi_qr";
        b.entity = ""; b.sensor = ""; b.unit = ""; b.precision = ""; b.icon_on = "Auto";
        if (qrCard) { b.label = ""; b.icon = "Auto"; }
        else {
            if (!b.label || isLegacyWifiQrTitle(b.label)) b.label = "Connect";
            if (!b.icon || b.icon === "Auto") b.icon = "Wifi";
        }
        var ssid: any = wifiQrSsid(b);
        var password: any = wifiQrPassword(b);
        var security: any = wifiQrSecurity(b);
        var options: any = ssid ? setConfigOptionValue("", SSID_OPTION, base64urlEncode(ssid)) : "";
        if (security === "open") options = setConfigOptionValue(options, SECURITY_OPTION, "open");
        else if (password) options = setConfigOptionValue(options, PASSWORD_OPTION, base64urlEncode(password));
        if (wifiQrHidden(b)) options = setConfigOption(options, HIDDEN_OPTION, true);
        b.options = options;
        setWifiQrTabs(b, tabs);
    }
    function updateOptions(this: any, b?: any, ssid?: any, security?: any, password?: any, hidden?: any) {
        if (!b) return;
        var tabs: any = wifiQrTabs(b);
        b.options = "";
        if (ssid) b.options = setConfigOptionValue(b.options, SSID_OPTION, base64urlEncode(ssid));
        if (security === "open") b.options = setConfigOptionValue(b.options, SECURITY_OPTION, "open");
        else if (password) b.options = setConfigOptionValue(b.options, PASSWORD_OPTION, base64urlEncode(password));
        if (hidden) b.options = setConfigOption(b.options, HIDDEN_OPTION, true);
        setWifiQrTabs(b, tabs);
        normalizeWifiQrConfig(b);
    }
    function wifiQrDefinition(this: any, type?: any): any {
        return {
            label: function (this: any) { return cardContractCardLabel(type); },
            allowInSubpage: function (this: any) { return cardContractAllowInSubpage(type); },
            pickerKey: function (this: any) { return cardContractPickerKey(type); },
            hideLabel: true,
            defaultConfig: function (this: any) { return cardContractDefaultConfig(type); },
            cardMetadata: WIFI_QR_CARD_METADATA,
            normalizeConfig: normalizeWifiQrConfig,
            onSelect: normalizeWifiQrConfig,
            renderSettingsBeforeLabel: function (this: any, panel?: any, b?: any, _slot?: any, helpers?: any) {
                helpers.renderCardModeSelector(panel, b, helpers, WIFI_QR_CARD_TYPE_METADATA);
                var networkDisclosure: any = helpers.disclosureSection("Wifi Network", helpers.idPrefix + "wifi-network", false);
                panel.appendChild(networkDisclosure.panel);
                var modalTabsDisclosure: any = helpers.disclosureSection("Modal Settings", helpers.idPrefix + "wifi-modal-tabs", b && b._modalSettingsOpen === true);
                renderModalTabSettings(modalTabsDisclosure.section, b, helpers, {
                    definitions: wifiQrTabDefinitions,
                    tabs: wifiQrTabs,
                    normalizeOptions: normalizeWifiQrTabOptions,
                    setTabs: setWifiQrTabs,
                    idPrefix: "wifi-tab-",
                    hideHeading: true,
                });
                panel.appendChild(modalTabsDisclosure.panel);
                if (!isQrCard(b)) {
                    var cardSettingsDisclosure: any = helpers.disclosureSection("Card Settings", helpers.idPrefix + "wifi-card-settings", false);
                    panel.appendChild(cardSettingsDisclosure.panel);
                }
            },
            renderSettings: function (this: any, panel?: any, b?: any, _slot?: any, helpers?: any) {
                normalizeWifiQrConfig(b);
                var networkButton: any = panel.querySelector("#" + helpers.idPrefix + "wifi-network");
                var networkSettings: any = networkButton && networkButton.nextElementSibling || panel;
                var ssidField: any = helpers.textField("Network name (SSID)", helpers.idPrefix + "wifi-ssid", wifiQrSsid(b), "Guest Wifi");
                var securityField: any = helpers.selectField("Security", helpers.idPrefix + "wifi-security", [["wpa", "WPA/WPA2 Personal"], ["open", "Open"]], wifiQrSecurity(b));
                var passwordField: any = helpers.textField("Password", helpers.idPrefix + "wifi-password", wifiQrPassword(b), "8–63 characters, or 64 hexadecimal characters");
                var hidden: any = helpers.toggleRow("Hidden network", helpers.idPrefix + "wifi-hidden", wifiQrHidden(b));
                networkSettings.appendChild(ssidField.field); networkSettings.appendChild(securityField.field); networkSettings.appendChild(passwordField.field);
                networkSettings.appendChild(hidden.row);
                function hasCredentialBytes(this: any, value?: any) { return utf8Bytes(value).length > 0; }
                helpers.requireField(ssidField.input, "Add a network name before saving.", undefined, hasCredentialBytes);
                helpers.requireField(passwordField.input, "Add a Wifi password before saving.", function () { return securityField.select.value === "wpa"; }, hasCredentialBytes);
                if (!isQrCard(b)) helpers.renderBasicCardFields(panel, b, helpers, WIFI_QR_CARD_METADATA, { entity: false });
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
                save();
            },
            renderPreview: function (this: any, b?: any, helpers?: any) {
                if (isQrCard(b)) {
                    return {
                        buttonClass: "sp-wifi-qr-card",
                        iconHtml: WIFI_QR_PREVIEW_SVG,
                        labelHtml: "",
                    };
                }
                return cardBadgePreview(b, helpers, { label: b.label || "Connect", iconFallback: "Wifi", badge: "Wifi Sharing" });
            },
        };
    }
    registry.register("wifi_qr", wifiQrDefinition("wifi_qr"));
    registry.register("wifi_qr_card", wifiQrDefinition("wifi_qr_card"));
}
