import { state } from "../state/app_instance";
import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerTimezoneCardTypes(): GlobalDescriptors {
    // Read-only world clock card: displays local time for a selected city.
    function timezoneCardCityLabel(this: any, tzOption?: any) {
        var tzId: any = getTzId(effectiveTimezoneOptionForWeb(tzOption || ""));
        if (!tzId)
            return "World Clock";
        if (tzId === "UTC")
            return "UTC";
        var city: any = tzId.substring(tzId.lastIndexOf("/") + 1);
        return city.replace(/_/g, " ");
    }
    function timezoneCardTimeParts(this: any, tzOption?: any) {
        var use12h: any = typeof state !== "undefined" && state.clockFormat === "12h";
        var tzId: any = getTzId(effectiveTimezoneOptionForWeb(tzOption || "UTC"));
        try {
            var opts: any = { timeZone: tzId, hour: "numeric", minute: "2-digit" };
            if (use12h)
                opts.hour12 = true;
            else
                opts.hourCycle = "h23";
            var parts: any = new Intl.DateTimeFormat("en-US", opts).formatToParts(webserverNow());
            var hour: any = "";
            var minute: any = "";
            for (var i: any = 0; i < parts.length; i++) {
                if (parts[i].type === "hour")
                    hour = parts[i].value;
                else if (parts[i].type === "minute")
                    minute = parts[i].value;
            }
            if (!hour || !minute)
                return { value: "--:--", unit: "" };
            return {
                value: (use12h ? hour : hour.padStart(2, "0")) + ":" + minute,
                unit: "",
            };
        }
        catch (e) {
            return { value: "--:--", unit: "" };
        }
    }
    registerButtonType("timezone", {
        label: function (this: any) { return cardContractCardLabel("timezone"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("timezone"); },
        pickerKey: function (this: any) { return cardContractPickerKey("timezone"); },
        hidden: function (this: any) { return cardContractHidden("timezone"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("timezone"); },
        isAvailable: function (this: any) {
            return false;
        },
        cardMetadata: DATE_TIME_CARD_METADATA,
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
            helpers.renderCardModeSelector(panel, b, helpers, DATE_TIME_CARD_METADATA);
            helpers.renderCardLargeNumbersToggle(panel, b, helpers, DATE_TIME_CARD_METADATA);
            var tzSelect: any = document.createElement("select");
            tzSelect.className = "sp-select";
            tzSelect.id = helpers.idPrefix + "timezone";
            var options: any = typeof state !== "undefined"
                ? timezoneOptionsWithFallback(state.timezoneOptions, b.entity)
                : [b.entity];
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
            panel.appendChild(helpers.fieldWithControl("City / Timezone", helpers.idPrefix + "timezone", tzSelect));
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var tz: any = b.entity || (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
            var time: any = timezoneCardTimeParts(tz);
            var hideLabel: any = cardLargeNumbersHidePreviewLabel(b, helpers, DATE_TIME_CARD_METADATA);
            return {
                buttonClass: hideLabel ? "sp-date-time-wide-large" : undefined,
                iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
                labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, timezoneCardCityLabel(tz), DATE_TIME_CARD_METADATA.preview.timezoneBadge),
            };
        },
    });
    return {
        "timezoneCardCityLabel": staticGlobal(timezoneCardCityLabel),
        "timezoneCardTimeParts": staticGlobal(timezoneCardTimeParts),
    };
}
