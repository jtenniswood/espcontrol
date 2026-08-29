export type ScreensaverBrightnessField = "clockBrightnessDay" | "clockBrightnessNight";
export type ScreensaverDimBrightnessField = "dimBrightnessDay" | "dimBrightnessNight";

export interface ScreensaverState {
  readonly action: string;
  readonly clockBrightnessDay: number;
  readonly clockBrightnessNight: number;
  readonly dimBrightness: number;
  readonly dimBrightnessDay: number;
  readonly dimBrightnessNight: number;
}

export interface ScreensaverUiState {
  readonly mode: string;
  readonly clockVisible: boolean;
  readonly dimVisible: boolean;
  readonly dayBrightnessLabel: string;
  readonly nightBrightnessLabel: string;
  readonly dimBrightnessLabel: string;
  readonly dimBrightnessDayLabel: string;
  readonly dimBrightnessNightLabel: string;
}

export interface ScreensaverNormalizers {
  readonly action: (value: unknown) => string;
  readonly dimBrightness: (value: unknown) => number;
  readonly clockBrightness: (value: unknown, fallback: number) => number;
}

export interface ScreensaverController {
  uiState(state: ScreensaverState): ScreensaverUiState;
  setAction(state: ScreensaverState, action: unknown): ScreensaverState;
  setDimBrightness(state: ScreensaverState, value: unknown): ScreensaverState;
  setDimBrightnessByPeriod(
    state: ScreensaverState,
    field: ScreensaverDimBrightnessField,
    value: unknown,
  ): ScreensaverState;
  setClockBrightness(
    state: ScreensaverState,
    field: ScreensaverBrightnessField,
    value: unknown,
  ): ScreensaverState;
}

/** Owns screensaver choices while the existing page remains its DOM and HTTP adapter. */
export function createScreensaverController(normalizers: ScreensaverNormalizers): ScreensaverController {
  return {
    uiState(state) {
      const mode = normalizers.action(state.action);
      return {
        mode,
        clockVisible: mode === "clock",
        dimVisible: mode === "dim",
        dayBrightnessLabel: `${Math.round(state.clockBrightnessDay)}%`,
        nightBrightnessLabel: `${Math.round(state.clockBrightnessNight)}%`,
        dimBrightnessLabel: `${Math.round(state.dimBrightness)}%`,
        dimBrightnessDayLabel: `${Math.round(state.dimBrightnessDay)}%`,
        dimBrightnessNightLabel: `${Math.round(state.dimBrightnessNight)}%`,
      };
    },
    setAction(state, action) {
      return { ...state, action: normalizers.action(action) };
    },
    setDimBrightness(state, value) {
      return { ...state, dimBrightness: normalizers.dimBrightness(value) };
    },
    setDimBrightnessByPeriod(state, field, value) {
      const fallback = field === "dimBrightnessNight"
        ? state.dimBrightnessDay
        : 10;
      return { ...state, [field]: normalizers.dimBrightness(value || fallback) };
    },
    setClockBrightness(state, field, value) {
      const fallback = field === "clockBrightnessNight"
        ? state.clockBrightnessDay
        : 35;
      return { ...state, [field]: normalizers.clockBrightness(value, fallback) };
    },
  };
}
