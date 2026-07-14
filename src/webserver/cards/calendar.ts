import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerCalendarCardTypes(): GlobalDescriptors {
    // Read-only date card: displays either the day/month or local time/date.
    var DATE_TIME_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "calendar-mode",
            options: [
                { value: "clock", label: "Clock" },
                { value: "datetime", label: "Time & Date" },
                { value: "", label: "Date" },
                { value: "timezone", label: "World Clock" }
            ],
            value: function (this: any, b?: any) {
                return dateTimeCardMode(b);
            },
            onChange: function (this: any, b?: any, helpers?: any) {
                setDateTimeCardMode(b, this.value, helpers);
            },
        },
        largeNumbers: {
            label: function (this: any, b?: any) {
                return dateTimeLargeNumbersLabel(b);
            },
            idSuffix: "large-date-time-numbers",
            supportedCardSize: function (this: any, b?: any, helpers?: any) {
                var cardSize: any = (helpers && helpers.cardSize) || CARD_SIZE_SINGLE;
                return dateTimeCardMode(b) === "clock"
                    ? cardSize === CARD_SIZE_WIDE || cardSize === CARD_SIZE_LARGE
                    : cardSize === CARD_SIZE_LARGE;
            },
            hideLabel: function (this: any, _b?: any, helpers?: any) {
                return ((helpers && helpers.cardSize) || CARD_SIZE_SINGLE) === CARD_SIZE_WIDE;
            },
        },
        preview: {
            dateBadge: "calendar-month",
            timezoneBadge: "map-clock",
        },
    };
    function dateTimeCardMode(this: any, b?: any) {
        if (b && b.type === "clock")
            return "clock";
        if (b && b.type === "timezone")
            return "timezone";
        return b && b.precision === "datetime" ? "datetime" : "";
    }
    function dateTimeLargeNumbersLabel(this: any, b?: any) {
        var mode: any = dateTimeCardMode(b);
        if (mode === "clock")
            return "Large Clock";
        if (mode === "datetime")
            return "Large Time";
        if (mode === "timezone")
            return "Large World Clock";
        return "Large Date";
    }
    function defaultTimezoneCardEntity(this: any) {
        return (typeof state !== "undefined" && state.timezone) || "UTC (GMT+0)";
    }
    function dateTimeModeOptionValues(this: any) {
        var spec: any = cardContractOptionSpec("calendar", "date_time_mode");
        return spec && spec.values ? spec.values.slice() : [];
    }
    function normalizeDateTimeCardMode(this: any, mode?: any) {
        mode = String(mode || "");
        return dateTimeModeOptionValues().indexOf(mode) >= 0 ? mode : "";
    }
    function setDateTimeCardMode(this: any, b?: any, mode?: any, helpers?: any) {
        mode = normalizeDateTimeCardMode(mode);
        if (b.type !== "timezone" && b.type !== "clock" && mode !== "timezone" && mode !== "clock") {
            b.precision = mode === "datetime" ? "datetime" : "";
            helpers.saveField("precision", b.precision);
            return;
        }
        if (mode === "clock") {
            b.type = "clock";
            helpers.applyCardMetadataFields(b, helpers, {
                type: "clock",
                entity: "",
                label: "",
                icon: "Auto",
                icon_on: "Auto",
                sensor: "",
                unit: "",
                precision: "",
                options: b.options,
            });
            renderButtonSettings();
            return;
        }
        if (mode === "timezone") {
            b.type = "timezone";
            helpers.applyCardMetadataFields(b, helpers, {
                type: "timezone",
                entity: defaultTimezoneCardEntity,
                label: "",
                icon: "Auto",
                icon_on: "Auto",
                sensor: "",
                unit: "",
                precision: "",
                options: b.options,
            });
            renderButtonSettings();
            return;
        }
        b.type = "calendar";
        helpers.applyCardMetadataFields(b, helpers, {
            type: "calendar",
            entity: "sensor.date",
            label: "",
            icon: "Auto",
            icon_on: "Auto",
            sensor: "",
            unit: "",
            precision: mode === "datetime" ? "datetime" : "",
            options: b.options,
        });
        if (mode !== "datetime")
            b.precision = "";
        renderButtonSettings();
    }
    function dateTimeCardTimeParts(this: any) {
        var now: any = webserverNow();
        var use12h: any = typeof state !== "undefined" && state.clockFormat === "12h";
        var hour: any = now.getUTCHours();
        var minute: any = String(now.getUTCMinutes()).padStart(2, "0");
        var timeValue: any = "";
        if (use12h) {
            var hour12: any = hour % 12;
            if (hour12 === 0)
                hour12 = 12;
            timeValue = String(hour12) + ":" + minute;
        }
        else {
            timeValue = String(hour).padStart(2, "0") + ":" + minute;
        }
        return {
            value: timeValue,
            unit: "",
        };
    }
    registerButtonType("calendar", {
        label: function (this: any) { return cardContractCardLabel("calendar"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("calendar"); },
        pickerKey: function (this: any) { return cardContractPickerKey("calendar"); },
        hidden: function (this: any) { return cardContractHidden("calendar"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("calendar"); },
        cardMetadata: DATE_TIME_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            var defaults: any = cardContractDefaultConfig("calendar");
            Object.keys(defaults).forEach(function (this: any, key?: any) { b[key] = defaults[key]; });
            b.precision = b.precision === "datetime" ? "datetime" : "";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            if (!b.entity)
                b.entity = "sensor.date";
            if (b.precision !== "datetime")
                b.precision = "";
            helpers.renderCardModeSelector(panel, b, helpers, DATE_TIME_CARD_METADATA);
            helpers.renderCardLargeNumbersToggle(panel, b, helpers, DATE_TIME_CARD_METADATA);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var now: any = webserverNow();
            var isDateTime: any = b.precision === "datetime";
            var hideLabel: any = cardLargeNumbersHidePreviewLabel(b, helpers, DATE_TIME_CARD_METADATA);
            var buttonClass: any = hideLabel
                ? (isDateTime ? "sp-clock-wide-large" : "sp-date-time-wide-large")
                : undefined;
            var day: any = String(now.getUTCDate());
            var month: any = typeof monthNameForIndex === "function"
                ? monthNameForIndex(now.getUTCMonth())
                : now.toLocaleString("en", { month: "long", timeZone: "UTC" });
            if (isDateTime) {
                var time: any = dateTimeCardTimeParts();
                return {
                    buttonClass: buttonClass,
                    iconHtml: cardSensorPreviewHtml(b, helpers, time.value, time.unit),
                    labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, day + " " + month, DATE_TIME_CARD_METADATA.preview.dateBadge),
                };
            }
            return {
                buttonClass: buttonClass,
                iconHtml: cardSensorPreviewHtml(b, helpers, day, null),
                labelHtml: hideLabel ? "" : cardBadgeLabelHtml(helpers, month, DATE_TIME_CARD_METADATA.preview.dateBadge),
            };
        },
    });
    return {
        "DATE_TIME_CARD_METADATA": liveGlobal(() => DATE_TIME_CARD_METADATA, (value?: any) => { DATE_TIME_CARD_METADATA = value; }),
        "dateTimeCardMode": staticGlobal(dateTimeCardMode),
        "dateTimeLargeNumbersLabel": staticGlobal(dateTimeLargeNumbersLabel),
        "defaultTimezoneCardEntity": staticGlobal(defaultTimezoneCardEntity),
        "dateTimeModeOptionValues": staticGlobal(dateTimeModeOptionValues),
        "normalizeDateTimeCardMode": staticGlobal(normalizeDateTimeCardMode),
        "setDateTimeCardMode": staticGlobal(setDateTimeCardMode),
        "dateTimeCardTimeParts": staticGlobal(dateTimeCardTimeParts),
    };
}
