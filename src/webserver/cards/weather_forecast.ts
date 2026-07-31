import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerWeatherForecastCardTypes(): GlobalDescriptors {
    // Legacy read-only forecast card: displays tomorrow's high / low temperature.
    var WEATHER_FORECAST_CARD_METADATA: any = {
        entity: WEATHER_CARD_METADATA.entity,
        preview: WEATHER_CARD_METADATA.preview,
    };
    registerButtonType("weather_forecast", {
        label: "Weather Forecast",
        allowInSubpage: true,
        hideLabel: true,
        cardMetadata: WEATHER_FORECAST_CARD_METADATA,
        isAvailable: function (this: any) {
            return false;
        },
        onSelect: function (this: any, b?: any) {
            b.label = "";
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.precision = "tomorrow";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            helpers.renderCardEntityField(panel, b, helpers, WEATHER_FORECAST_CARD_METADATA);
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            return {
                iconHtml: cardSensorPreviewHtml(b, helpers, "18/10", temperatureUnitSymbol(), "sp-forecast-preview", "sp-forecast-value"),
                labelHtml: cardBadgeLabelHtml(helpers, "Temperatures Tomorrow", WEATHER_FORECAST_CARD_METADATA.preview.forecastBadge),
            };
        },
    });
    return {
        "WEATHER_FORECAST_CARD_METADATA": liveGlobal(() => WEATHER_FORECAST_CARD_METADATA, (value?: any) => { WEATHER_FORECAST_CARD_METADATA = value; }),
    };
}
