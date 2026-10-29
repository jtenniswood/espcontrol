import { setConfigOptionValue } from "../model/config_primitives";
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
        weatherWindUsesSensors,
        rainfallUsesForecast,
        weatherForecastCardsSupported,
    } = weatherOptions;
    // Current conditions or a shared forecast layout for one or several days.
    const WEATHER_CARD_METADATA: any = {
        mode: {
            label: "Type",
            idSuffix: "weather-display",
            options: weatherModeOptions,
            value: function (this: any, b?: any) {
                return normalizeWeatherCardMode(b.precision);
            },
            onChange: function (this: any, b?: any, helpers?: any) {
                if (weatherWindUsesSensors(b) && this.value !== "wind") {
                    b.entity = "";
                    b.sensor = "";
                    b.options = setConfigOptionValue(b.options, "wind_source", "");
                    helpers.saveField("entity", b.entity);
                    helpers.saveField("sensor", b.sensor);
                    helpers.saveField("options", b.options);
                }
                if (b.precision === "rainfall" && !rainfallUsesForecast(b) && this.value !== "rainfall") {
                    b.entity = "";
                    helpers.saveField("entity", b.entity);
                }
                b.options = setConfigOptionValue(b.options, "rainfall_source", "");
                helpers.saveField("options", b.options);
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
            if (b.precision === "wind") {
                const sourceGroup = helpers.disclosureSection("Source", helpers.idPrefix + "wind-source-settings", true);
                helpers.markCardPrimaryField(sourceGroup.panel, "entity");
                panel.appendChild(sourceGroup.panel);
                const sourceFields = sourceGroup.section;
                sourceFields.className += " sp-weather-source-fields";
                const source = helpers.segmentControl(
                    [["weather", "Weather Entity"], ["sensors", "Separate Sensors"]],
                    weatherWindUsesSensors(b) ? "sensors" : "weather", function (value: string) {
                        b.options = setConfigOptionValue(b.options, "wind_source", value === "sensors" ? "sensors" : "");
                        b.entity = "";
                        b.sensor = "";
                        helpers.saveField("entity", b.entity);
                        helpers.saveField("sensor", b.sensor);
                        helpers.saveField("options", b.options);
                        renderButtonSettings();
                    });
                source.segment.id = helpers.idPrefix + "wind-source";
                sourceFields.appendChild(helpers.fieldWithControl("Wind Source", source.segment.id, source.segment));
                if (weatherWindUsesSensors(b)) {
                    helpers.renderCardEntityField(sourceFields, b, helpers, {
                        entity: {
                            label: "Wind Direction Sensor", idSuffix: "wind-direction",
                            placeholder: "e.g. sensor.wind_direction", domains: ["sensor", "input_number"],
                            bindName: "entity", rerender: false,
                            requiredMessage: "Add a wind direction sensor before saving.",
                        },
                    });
                    helpers.renderCardEntityField(sourceFields, b, helpers, {
                        entity: {
                            label: "Wind Speed Sensor", idSuffix: "wind-speed",
                            placeholder: "e.g. sensor.wind_speed", domains: ["sensor", "input_number"],
                            bindName: "sensor", rerender: false,
                            requiredMessage: "Add a wind speed sensor before saving.",
                        },
                    });
                    return;
                }
                helpers.renderCardEntityField(sourceFields, b, helpers, {
                    entity: { ...WEATHER_CARD_METADATA.entity, domains: ["weather"] },
                });
                return;
            }
            if (b.precision === "rainfall") {
                const sourceGroup = helpers.disclosureSection("Source", helpers.idPrefix + "rainfall-source-settings", true);
                helpers.markCardPrimaryField(sourceGroup.panel, "entity");
                panel.appendChild(sourceGroup.panel);
                const group = sourceGroup.section;
                group.className += " sp-weather-source-fields";
                const sources = [["sensor", "Rainfall Sensor"]];
                if (weatherForecastCardsSupported()) sources.push(["forecast", "Today's Forecast"]);
                const source = helpers.segmentControl(
                    sources, rainfallUsesForecast(b) ? "forecast" : "sensor", function (value: string) {
                        b.options = setConfigOptionValue(b.options, "rainfall_source", value === "forecast" ? "forecast" : "");
                        b.entity = "";
                        helpers.saveField("entity", b.entity);
                        helpers.saveField("options", b.options);
                        renderButtonSettings();
                    });
                source.segment.id = helpers.idPrefix + "rainfall-source";
                group.appendChild(helpers.fieldWithControl("Rainfall Source", source.segment.id, source.segment));
                const forecast = rainfallUsesForecast(b);
                helpers.renderCardEntityField(group, b, helpers, {
                    entity: {
                        ...WEATHER_CARD_METADATA.entity,
                        label: forecast ? "Weather Entity" : "Rainfall Sensor",
                        placeholder: forecast ? "e.g. weather.forecast_home" : "e.g. sensor.rainfall_today",
                        domains: forecast ? ["weather"] : ["sensor", "input_number"],
                    },
                });
                return;
            }
            helpers.renderCardEntityField(panel, b, helpers, {
                entity: { ...WEATHER_CARD_METADATA.entity, domains: ["weather"] },
            });
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            if (b.precision === "wind") {
                return {
                    buttonClass: "sp-weather-wind-card",
                    iconHtml: '<span class="sp-btn-icon mdi mdi-arrow-top-right"></span>',
                    labelHtml: '<span class="sp-btn-label">North East<span class="sp-weather-wind-speed">12.5 km/h</span></span>',
                    labelEditable: false,
                };
            }
            if (b.precision === "rainfall") {
                return {
                    buttonClass: "sp-rainfall-card",
                    iconHtml: '<span class="sp-btn-icon mdi mdi-weather-rainy"></span>',
                    labelHtml: '<span class="sp-btn-label">Rainfall<span class="sp-rainfall-value">2.4 mm</span></span>',
                    labelEditable: false,
                };
            }
            if (weatherCardIsForecastMode(b)) {
                let samples: [string, string, string][] = [["Today", "weather-rainy", "18°/2°"], ["Tues", "weather-sunny", "20°/9°"], ["Weds", "weather-partly-cloudy", "18°/14°"], ["Thurs", "weather-rainy", "20°/13°"], ["Fri", "weather-sunny", "14°/6°"], ["Sat", "weather-cloudy", "18°/10°"], ["Sun", "weather-sunny", "19°/11°"], ["Mon", "weather-cloudy", "17°/8°"]];
                if (b.precision === "today") samples = samples.slice(0, 1);
                else if (b.precision === "tomorrow") samples = samples.slice(1, 2).map(([, condition, temperature]) => ["Tomorrow", condition, temperature]);
                return {
                    buttonClass: "sp-weather-daily-card",
                    iconHtml: '<span class="sp-weather-days">' + samples.map((day) => '<span class="sp-weather-day" data-weather-weekday="' + (day[0] === "Today" ? "Mon" : day[0] === "Tomorrow" ? "Tues" : day[0]) + '"><span class="mdi mdi-' + day[1] + '"></span><span>' + day[0] + '</span><span class="sp-weather-temperature-high">' + day[2] + '</span></span>').join("") + '</span>',
                    labelHtml: "",
                };
            }
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-weather-cloudy"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, "Cloudy", WEATHER_CARD_METADATA.preview.currentBadge),
                labelEditable: false,
            };
        },
    });
    return {
        entityMetadata: { ...WEATHER_CARD_METADATA.entity, domains: ["weather"] },
        previewMetadata: WEATHER_CARD_METADATA.preview,
    };
}
