import type { CardRegistry } from "../application/card_registry";
import type { WeatherCardRegistration } from "./weather";
import type { ClockBarFeature } from "../application/clock_bar_state";
import type { ControlsFieldsFeature } from "../application/controls_fields";

export function registerWeatherForecastCardTypes(
    registry: CardRegistry,
    weather: WeatherCardRegistration,
    clockBar: Pick<ClockBarFeature, "temperatureUnitSymbol">,
    fields: ControlsFieldsFeature,
): void {
    const { cardBadgeLabelHtml, cardSensorPreviewHtml } = fields;
    const { temperatureUnitSymbol } = clockBar;
    // Legacy read-only forecast card: displays tomorrow's high / low temperature.
    const WEATHER_FORECAST_CARD_METADATA: any = {
        entity: weather.entityMetadata,
        preview: weather.previewMetadata,
    };
    registry.register("weather_forecast", {
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
}
