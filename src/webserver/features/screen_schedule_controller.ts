export interface ScreenScheduleState {
  readonly trigger: string;
  readonly sensorActivation: string;
  readonly onHour: number;
  readonly offHour: number;
  readonly mode: string;
  readonly wakeTimeout: number;
  readonly wakeBrightness: number;
  readonly dimmedBrightness: number;
  readonly clockBrightness: number;
}

export interface ScreenScheduleUiState {
  readonly enabled: boolean;
  readonly timeControlsVisible: boolean;
  readonly sensorControlsVisible: boolean;
  readonly actionsVisible: boolean;
  readonly screenOffOptionsVisible: boolean;
  readonly dimmedOptionsVisible: boolean;
  readonly clockOptionsVisible: boolean;
}

export interface ScreenScheduleNormalizers {
  trigger(value: unknown, enabled: boolean): string;
  sensorActivation(value: unknown): string;
  hour(value: unknown, fallback: number): number;
  mode(value: unknown): string;
  wakeTimeout(value: unknown): number;
  wakeBrightness(value: unknown): number;
  dimmedBrightness(value: unknown): number;
  clockBrightness(value: unknown): number;
}

export interface ScreenScheduleController {
  normalize(state: ScreenScheduleState): ScreenScheduleState;
  uiState(state: ScreenScheduleState): ScreenScheduleUiState;
  setTrigger(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setSensorActivation(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setOnHour(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setOffHour(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setMode(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setWakeTimeout(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setWakeBrightness(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setDimmedBrightness(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
  setClockBrightness(state: ScreenScheduleState, value: unknown): ScreenScheduleState;
}

/** Owns Night Schedule state and visibility while application modules remain UI and firmware adapters. */
export function createScreenScheduleController(normalize: ScreenScheduleNormalizers): ScreenScheduleController {
  function normalized(state: ScreenScheduleState): ScreenScheduleState {
    const trigger = normalize.trigger(state.trigger, state.trigger !== "disabled");
    return {
      trigger,
      sensorActivation: normalize.sensorActivation(state.sensorActivation),
      onHour: normalize.hour(state.onHour, 6),
      offHour: normalize.hour(state.offHour, 23),
      mode: normalize.mode(state.mode),
      wakeTimeout: normalize.wakeTimeout(state.wakeTimeout),
      wakeBrightness: normalize.wakeBrightness(state.wakeBrightness),
      dimmedBrightness: normalize.dimmedBrightness(state.dimmedBrightness),
      clockBrightness: normalize.clockBrightness(state.clockBrightness),
    };
  }

  function withValue<K extends keyof ScreenScheduleState>(
    state: ScreenScheduleState, key: K, value: ScreenScheduleState[K],
  ): ScreenScheduleState {
    return normalized({ ...state, [key]: value });
  }

  return {
    normalize: normalized,
    uiState(state) {
      const schedule = normalized(state);
      const enabled = schedule.trigger !== "disabled";
      return {
        enabled,
        timeControlsVisible: schedule.trigger === "time",
        sensorControlsVisible: schedule.trigger === "sensor",
        actionsVisible: enabled,
        screenOffOptionsVisible: schedule.mode === "screen_off",
        dimmedOptionsVisible: schedule.mode === "screen_dimmed",
        clockOptionsVisible: schedule.mode === "clock",
      };
    },
    setTrigger(state, value) {
      return withValue(state, "trigger", normalize.trigger(value, state.trigger !== "disabled"));
    },
    setSensorActivation(state, value) {
      return withValue(state, "sensorActivation", normalize.sensorActivation(value));
    },
    setOnHour(state, value) {
      return withValue(state, "onHour", normalize.hour(value, 6));
    },
    setOffHour(state, value) {
      return withValue(state, "offHour", normalize.hour(value, 23));
    },
    setMode(state, value) {
      return withValue(state, "mode", normalize.mode(value));
    },
    setWakeTimeout(state, value) {
      return withValue(state, "wakeTimeout", normalize.wakeTimeout(value));
    },
    setWakeBrightness(state, value) {
      return withValue(state, "wakeBrightness", normalize.wakeBrightness(value));
    },
    setDimmedBrightness(state, value) {
      return withValue(state, "dimmedBrightness", normalize.dimmedBrightness(value));
    },
    setClockBrightness(state, value) {
      return withValue(state, "clockBrightness", normalize.clockBrightness(value));
    },
  };
}
