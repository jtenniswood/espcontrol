export interface VoiceServicesState {
  readonly supported: boolean;
  readonly enabled: boolean;
}

export interface VoiceServicesUiState {
  readonly settingsVisible: boolean;
  readonly clockBarItemVisible: boolean;
  readonly iconVisible: boolean;
}

export interface VoiceServicesController {
  uiState(state: VoiceServicesState): VoiceServicesUiState;
  setEnabled(state: VoiceServicesState, enabled: unknown): VoiceServicesState;
}

/** Owns Voice Services state shared by settings, the clock bar, and its preview icon. */
export function createVoiceServicesController(): VoiceServicesController {
  return {
    uiState(state) {
      return {
        settingsVisible: state.supported,
        clockBarItemVisible: state.supported,
        iconVisible: state.supported && state.enabled,
      };
    },
    setEnabled(state, enabled) {
      return { ...state, enabled: !!enabled };
    },
  };
}
