import type { AppState } from "../state/types";
import { CARD_SIZE_LARGE, CARD_SIZE_SINGLE, CARD_SIZE_WIDE } from "../model/grid";
import { cardContractOptionSpec } from "./config_option_core";

export interface ConfigDateTimeOptionsDependencies {
    readonly state: AppState;
    readonly now: () => Date;
    readonly renderButtonSettings: () => void;
    readonly effectiveTimezoneOption: (value: string) => string;
    readonly timezoneId: (value: string) => string;
    readonly timezoneOptionsWithFallback: (options: readonly string[], selected: string) => readonly string[];
    readonly appendTimezoneOption: (select: HTMLSelectElement, option: string) => void;
    readonly monthNameForIndex: (index: number) => string;
}

export function createConfigDateTimeOptionsFeature(dependencies: ConfigDateTimeOptionsDependencies) {
    function dateTimeCardMode(this: any, button?: any) {
        if (button && button.type === "clock") return "clock";
        if (button && button.type === "timezone") return "timezone";
        return button && button.precision === "datetime" ? "datetime" : "";
    }

    function dateTimeLargeNumbersLabel(this: any, button?: any) {
        const mode = dateTimeCardMode(button);
        if (mode === "clock") return "Large Clock";
        if (mode === "datetime") return "Large Time";
        if (mode === "timezone") return "Large World Clock";
        return "Large Date";
    }

    function defaultTimezoneCardEntity(this: any) {
        return dependencies.state.timezone || "UTC (GMT+0)";
    }

    function dateTimeModeOptionValues(this: any) {
        const spec: any = cardContractOptionSpec("calendar", "date_time_mode");
        return spec && spec.values ? spec.values.slice() : [];
    }

    function normalizeDateTimeCardMode(this: any, mode?: any) {
        mode = String(mode || "");
        return dateTimeModeOptionValues().indexOf(mode) >= 0 ? mode : "";
    }

    function setDateTimeCardMode(this: any, button?: any, mode?: any, helpers?: any) {
        mode = normalizeDateTimeCardMode(mode);
        if (button.type !== "timezone" && button.type !== "clock" && mode !== "timezone" && mode !== "clock") {
            button.precision = mode === "datetime" ? "datetime" : "";
            helpers.saveField("precision", button.precision);
            return;
        }
        if (mode === "clock") {
            button.type = "clock";
            helpers.applyCardMetadataFields(button, helpers, {
                type: "clock", entity: "", label: "", icon: "Auto", icon_on: "Auto",
                sensor: "", unit: "", precision: "", options: button.options,
            });
            dependencies.renderButtonSettings();
            return;
        }
        if (mode === "timezone") {
            button.type = "timezone";
            helpers.applyCardMetadataFields(button, helpers, {
                type: "timezone", entity: defaultTimezoneCardEntity, label: "", icon: "Auto", icon_on: "Auto",
                sensor: "", unit: "", precision: "", options: button.options,
            });
            dependencies.renderButtonSettings();
            return;
        }
        button.type = "calendar";
        helpers.applyCardMetadataFields(button, helpers, {
            type: "calendar", entity: "sensor.date", label: "", icon: "Auto", icon_on: "Auto",
            sensor: "", unit: "", precision: mode === "datetime" ? "datetime" : "", options: button.options,
        });
        if (mode !== "datetime") button.precision = "";
        dependencies.renderButtonSettings();
    }

    function dateTimeCardTimeParts(this: any) {
        const now = dependencies.now();
        const use12h = dependencies.state.clockFormat === "12h";
        const hour = now.getUTCHours();
        const minute = String(now.getUTCMinutes()).padStart(2, "0");
        if (use12h) {
            const hour12 = hour % 12 || 12;
            return { value: String(hour12) + ":" + minute, unit: "" };
        }
        return { value: String(hour).padStart(2, "0") + ":" + minute, unit: "" };
    }

    function timezoneCardCityLabel(this: any, timezoneOption?: any) {
        const timezoneId = dependencies.timezoneId(dependencies.effectiveTimezoneOption(timezoneOption || ""));
        if (!timezoneId) return "World Clock";
        if (timezoneId === "UTC") return "UTC";
        return timezoneId.substring(timezoneId.lastIndexOf("/") + 1).replace(/_/g, " ");
    }

    function timezoneCardTimeParts(this: any, timezoneOption?: any) {
        const use12h = dependencies.state.clockFormat === "12h";
        const timezoneId = dependencies.timezoneId(dependencies.effectiveTimezoneOption(timezoneOption || "UTC"));
        try {
            const options: any = { timeZone: timezoneId, hour: "numeric", minute: "2-digit" };
            if (use12h) options.hour12 = true;
            else options.hourCycle = "h23";
            const parts = new Intl.DateTimeFormat("en-US", options).formatToParts(dependencies.now());
            let hour = "";
            let minute = "";
            for (const part of parts) {
                if (part.type === "hour") hour = part.value;
                else if (part.type === "minute") minute = part.value;
            }
            if (!hour || !minute) return { value: "--:--", unit: "" };
            return { value: (use12h ? hour : hour.padStart(2, "0")) + ":" + minute, unit: "" };
        }
        catch (_error) {
            return { value: "--:--", unit: "" };
        }
    }

    const metadata: any = {
        mode: {
            label: "Type",
            idSuffix: "calendar-mode",
            options: [
                { value: "clock", label: "Clock" },
                { value: "datetime", label: "Time & Date" },
                { value: "", label: "Date" },
                { value: "timezone", label: "World Clock" },
            ],
            value: function (button?: any) { return dateTimeCardMode(button); },
            onChange: function (this: any, button?: any, helpers?: any) {
                setDateTimeCardMode(button, this.value, helpers);
            },
        },
        largeNumbers: {
            label: function (button?: any) { return dateTimeLargeNumbersLabel(button); },
            idSuffix: "large-date-time-numbers",
            supportedCardSize: function (button?: any, helpers?: any) {
                const cardSize = (helpers && helpers.cardSize) || CARD_SIZE_SINGLE;
                return dateTimeCardMode(button) === "clock"
                    ? cardSize === CARD_SIZE_WIDE || cardSize === CARD_SIZE_LARGE
                    : cardSize === CARD_SIZE_LARGE;
            },
            hideLabel: function (_button?: any, helpers?: any) {
                return ((helpers && helpers.cardSize) || CARD_SIZE_SINGLE) === CARD_SIZE_WIDE;
            },
        },
        preview: { dateBadge: "calendar-month", timezoneBadge: "map-clock" },
    };

    return {
        appendTimezoneOption: dependencies.appendTimezoneOption,
        dateTimeCardMode,
        dateTimeCardTimeParts,
        dateTimeLargeNumbersLabel,
        dateTimeModeOptionValues,
        defaultTimezoneCardEntity,
        metadata,
        monthNameForIndex: dependencies.monthNameForIndex,
        now: dependencies.now,
        normalizeDateTimeCardMode,
        setDateTimeCardMode,
        timezoneCardCityLabel,
        timezoneCardTimeParts,
        timezoneOptionsFor(selected: string) {
            return dependencies.timezoneOptionsWithFallback(dependencies.state.timezoneOptions || [], selected);
        },
    };
}

export type ConfigDateTimeOptionsFeature = ReturnType<typeof createConfigDateTimeOptionsFeature>;
