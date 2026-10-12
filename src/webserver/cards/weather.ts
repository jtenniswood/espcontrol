import {
    cardContractAllowInSubpage,
    cardContractCardLabel,
    cardContractDefaultConfig,
    cardContractDomains,
    cardContractHidden,
    cardContractPickerKey,
} from "../generated/card_contract";
import type { CardRegistry, CardUiServices } from "../application/card_registry";
import type { ConfigWeatherOptionsFeature } from "../application/config_weather_options";
import type { ClockBarFeature } from "../application/clock_bar_state";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export interface WeatherCardRegistration {
    readonly entityMetadata: any;
    readonly previewMetadata: any;
}

export function registerWeatherCardTypes(
    registry: CardRegistry,
    weatherOptions: ConfigWeatherOptionsFeature,
    _clockBar: Pick<ClockBarFeature, "temperatureUnitSymbol">,
    fields: ControlsFieldsFeature,
    cardUi: CardUiServices,
): WeatherCardRegistration {
    const { renderButtonSettings } = cardUi;
    const { cardBadgeLabelHtml } = fields;
    const {
        weatherCardIsForecastMode,
        weatherModeOptions,
        normalizeWeatherCardMode,
    } = weatherOptions;
    // Current conditions or a shared forecast layout for one or several days.
    const WEATHER_CARD_METADATA: any = {
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
                renderButtonSettings();
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
        preview: {
            forecastBadge: "weather-partly-cloudy",
            currentBadge: "weather-cloudy",
        },
    };
    registry.register("weather", {
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
            helpers.renderCardModeSelector(panel, b, helpers, WEATHER_CARD_METADATA);
            helpers.renderCardEntityField(panel, b, helpers, WEATHER_CARD_METADATA);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            if (weatherCardIsForecastMode(b)) {
                let samples: [string, string, string][] = [["Today", "weather-rainy", "18"], ["Tues", "weather-sunny", "20"], ["Weds", "weather-partly-cloudy", "18"], ["Thurs", "weather-rainy", "20"], ["Fri", "weather-sunny", "14"], ["Sat", "weather-cloudy", "18"], ["Sun", "weather-sunny", "19"]];
                if (b.precision === "today") samples = samples.slice(0, 1);
                else if (b.precision === "tomorrow") samples = samples.slice(1, 2).map(([, condition, temperature]) => ["Tomorrow", condition, temperature]);
                return {
                    buttonClass: "sp-weather-daily-card",
                    iconHtml: '<span class="sp-weather-days">' + samples.map((day) => '<span class="sp-weather-day"><span class="mdi mdi-' + day[1] + '"></span><span>' + day[0] + '</span><span class="sp-weather-temperature-high">' + day[2] + '\u00B0</span></span>').join("") + '</span>',
                    labelHtml: "",
                };
            }
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-weather-cloudy"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, "Cloudy", WEATHER_CARD_METADATA.preview.currentBadge),
            };
        },
    });
    return {
        entityMetadata: WEATHER_CARD_METADATA.entity,
        previewMetadata: WEATHER_CARD_METADATA.preview,
    };
}
