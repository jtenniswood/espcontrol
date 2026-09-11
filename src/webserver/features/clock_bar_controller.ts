export interface ClockBarState {
  readonly enabled: boolean;
  readonly timeEnabled: boolean;
  readonly nightModeEnabled: boolean;
  readonly selectedItem: string;
}

export interface ClockBarUiState {
  readonly previewVisible: boolean;
  readonly badgeVisible: boolean;
  readonly selectedItem: string;
}

export interface ClockBarController {
  reconcile(state: ClockBarState): ClockBarState;
  uiState(state: ClockBarState): ClockBarUiState;
  setEnabled(state: ClockBarState, enabled: unknown): ClockBarState;
  setTimeEnabled(state: ClockBarState, enabled: unknown): ClockBarState;
  setNightModeEnabled(state: ClockBarState, enabled: unknown): ClockBarState;
}

/** Owns Clock Bar controls and preview visibility while existing modules remain DOM and HTTP adapters. */
export function createClockBarController(): ClockBarController {
  function reconcile(state: ClockBarState): ClockBarState {
    return state.enabled || !state.selectedItem ? state : { ...state, selectedItem: "" };
  }

  return {
    reconcile,
    uiState(state) {
      const reconciled = reconcile(state);
      return {
        previewVisible: reconciled.enabled,
        badgeVisible: reconciled.enabled,
        selectedItem: reconciled.selectedItem,
      };
    },
    setEnabled(state, enabled) {
      return reconcile({ ...state, enabled: !!enabled });
    },
    setTimeEnabled(state, enabled) {
      return { ...state, timeEnabled: !!enabled };
    },
    setNightModeEnabled(state, enabled) {
      return { ...state, nightModeEnabled: !!enabled };
    },
  };
}
