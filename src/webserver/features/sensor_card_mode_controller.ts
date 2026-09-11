import type { CardConfig, SavedConfigField } from "../contracts/types";

export const LOCAL_SENSOR_SOURCE = "local";

export type SensorDisplayMode = "numeric" | "time" | "text" | "icon";

export interface SensorCardModeControllerOptions {
  readonly normalizeOptions: (options: string, precision: string) => string;
  readonly localSensorSource?: string;
}

export interface SensorCardTransition {
  readonly mode: SensorDisplayMode;
  readonly fields: readonly SavedConfigField[];
}

const sourceFields: readonly SavedConfigField[] = [
  "type", "entity", "label", "sensor", "unit", "icon", "icon_on", "precision", "options",
];

const modeFields: Readonly<Record<SensorDisplayMode, readonly SavedConfigField[]>> = {
  numeric: ["precision", "icon", "icon_on", "options"],
  time: ["precision", "unit", "icon", "icon_on", "options"],
  text: ["precision", "label", "unit", "icon_on", "options"],
  icon: ["precision", "unit", "options"],
};

/** Owns the sensor editor's source and display-mode field transitions. */
export class SensorCardModeController {
  private readonly localSensorSource: string;

  constructor(private readonly options: SensorCardModeControllerOptions) {
    this.localSensorSource = options.localSensorSource || LOCAL_SENSOR_SOURCE;
  }

  isLocal(button: CardConfig | null | undefined): boolean {
    return !!button && (button.type === "local_sensor" ||
      (button.type === "sensor" && button.sensor === this.localSensorSource));
  }

  displayMode(button: CardConfig): SensorDisplayMode {
    if (button.precision === "icon" || button.precision === "time" || button.precision === "text") {
      return button.precision;
    }
    return "numeric";
  }

  selectSource(button: CardConfig, source: string): readonly SavedConfigField[] {
    const local = source === this.localSensorSource;
    if (local === this.isLocal(button)) return [];
    button.type = "sensor";
    button.entity = "";
    button.label = "";
    button.sensor = local ? this.localSensorSource : "";
    button.unit = "";
    button.icon = "Auto";
    button.icon_on = "Auto";
    button.precision = "";
    button.options = "";
    return sourceFields;
  }

  selectDisplayMode(button: CardConfig, requested: string): SensorCardTransition {
    const mode: SensorDisplayMode = requested === "time" || requested === "text" || requested === "icon"
      ? requested
      : "numeric";
    if (mode === "time") {
      button.precision = "time";
      button.unit = "";
      button.icon = "Auto";
      button.icon_on = "Auto";
    } else if (mode === "text") {
      button.precision = "text";
      button.label = "";
      button.unit = "";
      button.icon_on = "Auto";
    } else if (mode === "icon") {
      button.precision = "icon";
      button.unit = "";
    } else {
      button.precision = "";
      button.icon = "Auto";
      button.icon_on = "Auto";
    }
    button.options = this.options.normalizeOptions(button.options, button.precision);
    return { mode, fields: modeFields[mode] };
  }
}

export function createSensorCardModeController(options: SensorCardModeControllerOptions): SensorCardModeController {
  return new SensorCardModeController(options);
}
