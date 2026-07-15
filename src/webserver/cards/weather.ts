import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerWeatherCardTypes(): GlobalDescriptors {
    // Read-only weather card: displays either current conditions or high / low temperatures.
    var WEATHER_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "weather-display",
            options: weatherModeOptions,
            value: function (this: any, b?: any) {
                return weatherCardIsForecastMode(b) ? b.precision : "";
            },
            onChange: function (this: any, b?: any, helpers?: any) {
                b.precision = this.value;
                helpers.saveField("precision", b.precision);
            },
        },
        entity: {
            label: "Weather Entity",
            idSuffix: "entity",
            placeholder: "e.g. weather.forecast_home",
            domains: function (this: any) { return cardContractDomains("weather"); },
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add an entity before saving.",
        },
        labelField: {
            label: "Label",
            idSuffix: "label",
            placeholder: function (this: any, b?: any) {
                return "e.g. " + weatherCardDefaultForecastLabel(b);
            },
        },
        largeNumbers: {
            label: "Large Temperature Numbers",
            idSuffix: "large-weather-numbers",
            supported: weatherCardIsForecastMode,
        },
        preview: {
            forecastBadge: "weather-partly-cloudy",
            currentBadge: "weather-cloudy",
        },
    };
    function weatherCardDefaultForecastLabel(this: any, b?: any) {
        return b.precision === "today" ? "Today" : "Tomorrow";
    }
    function weatherForecastCardsSupported(this: any) {
        var disabled: any = CFG.disabledCardTypes || [];
        return disabled.indexOf("weather_forecast") === -1;
    }
    function weatherModeOptions(this: any) {
        var options: any = [
            ["", "Current Conditions"],
            ["today", "Temperatures Today"],
            ["tomorrow", "Temperatures Tomorrow"],
        ];
        return weatherForecastCardsSupported() ? options : [options[0]];
    }
    function weatherModeOptionValues(this: any) {
        var spec: any = cardContractOptionSpec("weather", "weather_mode");
        var values: any = spec && spec.values ? spec.values.slice() : ["", "today", "tomorrow"];
        return weatherForecastCardsSupported() ? values : values.filter(function (this: any, value?: any) {
            return value === "";
        });
    }
    function normalizeWeatherCardMode(this: any, mode?: any) {
        mode = String(mode || "");
        return weatherModeOptionValues().indexOf(mode) >= 0 ? mode : "";
    }
    function weatherCardIsForecastMode(this: any, b?: any) {
        return weatherForecastCardsSupported() &&
            !!b &&
            cardContractOptionSupportedFor("weather", "large_numbers", { precision: b.precision });
    }
    registerButtonType("weather", {
        label: function (this: any) { return cardContractCardLabel("weather"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("weather"); },
        pickerKey: function (this: any) { return cardContractPickerKey("weather"); },
        hidden: function (this: any) { return cardContractHidden("weather"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("weather"); },
        cardMetadata: WEATHER_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.label = "";
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.options = "";
            b.precision = normalizeWeatherCardMode(b.precision);
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var modeField: any = helpers.renderCardModeSelector(panel, b, helpers, WEATHER_CARD_METADATA);
            var modeSelect: any = modeField.select;
            helpers.renderCardEntityField(panel, b, helpers, WEATHER_CARD_METADATA);
            var labelControl: any = helpers.renderCardTextField(panel, b, helpers, WEATHER_CARD_METADATA.labelField);
            var labelField: any = labelControl.field;
            var labelInp: any = labelControl.input;
            var largeNumbersToggle: any = helpers.renderCardLargeNumbersToggle(panel, b, helpers, WEATHER_CARD_METADATA);
            function syncForecastFields(this: any) {
                var forecast: any = weatherCardIsForecastMode(b);
                labelField.style.display = forecast ? "" : "none";
                labelInp.placeholder = "e.g. " + weatherCardDefaultForecastLabel(b);
                helpers.syncCardLargeNumbersToggle(largeNumbersToggle, b, helpers, forecast);
            }
            modeSelect.addEventListener("change", function (this: any) {
                syncForecastFields();
            });
            syncForecastFields();
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            if (weatherCardIsForecastMode(b)) {
                var defaultLabel: any = weatherCardDefaultForecastLabel(b);
                var label: any = b.label || defaultLabel;
                return {
                    iconHtml: cardSensorPreviewHtml(b, helpers, "18/10", temperatureUnitSymbol(), "sp-forecast-preview", "sp-forecast-value"),
                    labelHtml: cardBadgeLabelHtml(helpers, label, WEATHER_CARD_METADATA.preview.forecastBadge),
                };
            }
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-weather-cloudy"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, "Cloudy", WEATHER_CARD_METADATA.preview.currentBadge),
            };
        },
    });
    return {
        "WEATHER_CARD_METADATA": liveGlobal(() => WEATHER_CARD_METADATA, (value?: any) => { WEATHER_CARD_METADATA = value; }),
        "weatherCardDefaultForecastLabel": staticGlobal(weatherCardDefaultForecastLabel),
        "weatherForecastCardsSupported": staticGlobal(weatherForecastCardsSupported),
        "weatherModeOptions": staticGlobal(weatherModeOptions),
        "weatherModeOptionValues": staticGlobal(weatherModeOptionValues),
        "normalizeWeatherCardMode": staticGlobal(normalizeWeatherCardMode),
        "weatherCardIsForecastMode": staticGlobal(weatherCardIsForecastMode),
    };
}
