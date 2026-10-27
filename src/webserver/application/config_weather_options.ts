import { configOptionValue } from "../model/config_primitives";
import type { DeviceConfig } from "../state/types";
import {
    cardContractOptionSpec,
} from "./config_option_core";

export function createConfigWeatherOptionsFeature(
    deviceProfile: Pick<DeviceConfig, "disabledCardTypes">,
) {
    function weatherForecastCardsSupported(this: any) {
        const disabled: readonly string[] = deviceProfile.disabledCardTypes || [];
        return disabled.indexOf("weather_forecast") === -1;
    }

    function weatherModeOptions(this: any) {
        const options: any = [
            ["", "Current Conditions"],
            ["wind", "Wind"],
            ["today", "Today's Forecast"],
            ["tomorrow", "Tomorrow's Forecast"],
            ["daily", "Daily Forecast"],
        ];
        return weatherForecastCardsSupported() ? options : options.slice(0, 2);
    }

    function weatherModeOptionValues(this: any) {
        const spec: any = cardContractOptionSpec("weather", "weather_mode");
        const values: any = spec && spec.values ? spec.values.slice() : ["", "today", "tomorrow", "daily", "wind"];
        return weatherForecastCardsSupported() ? values : values.filter(function (value: any) {
            return value === "" || value === "wind";
        });
    }

    function normalizeWeatherCardMode(this: any, mode?: any) {
        mode = String(mode || "");
        return weatherModeOptionValues().indexOf(mode) >= 0 ? mode : "";
    }

    function weatherCardIsForecastMode(this: any, button?: any) {
        return weatherForecastCardsSupported() &&
            !!button && ["today", "tomorrow", "daily"].indexOf(button.precision) >= 0;
    }

    function weatherCardDefaultForecastLabel(this: any, button?: any) {
        return button.precision === "today" ? "Today" : "Tomorrow";
    }

    function weatherWindUsesSensors(button?: any): boolean {
        return !!button && button.precision === "wind" &&
            configOptionValue(button.options, "wind_source") === "sensors";
    }

    function normalizeWeatherOptions(options?: string, button?: any): string {
        return weatherWindUsesSensors({ ...button, options }) ? "wind_source=sensors" : "";
    }

    return {
        weatherWindUsesSensors,
        normalizeWeatherOptions,
        weatherCardDefaultForecastLabel,
        weatherCardIsForecastMode,
        weatherForecastCardsSupported,
        weatherModeOptions,
        weatherModeOptionValues,
        normalizeWeatherCardMode,
    };
}

export type ConfigWeatherOptionsFeature = ReturnType<typeof createConfigWeatherOptionsFeature>;
