export interface CoverArtScreensaverState {
  readonly enabled: boolean;
  readonly delay: number;
  readonly trackOverlayDuration: number;
  readonly hideExternalInput: boolean;
  readonly filteringEnabled: boolean;
  readonly attributeConditions: string;
}

export interface CoverArtScreensaverUiState {
  readonly contentVisible: boolean;
  readonly badgeVisible: boolean;
  readonly externalSourcesVisible: boolean;
  readonly filterOptionsVisible: boolean;
}

export interface CoverArtScreensaverNormalizers {
  readonly delay: (value: unknown) => number;
  readonly trackOverlayDuration: (value: unknown) => number;
}

export interface CoverArtScreensaverController {
  normalize(state: CoverArtScreensaverState): CoverArtScreensaverState;
  initialState(state: CoverArtScreensaverState): CoverArtScreensaverState;
  uiState(state: CoverArtScreensaverState): CoverArtScreensaverUiState;
  setEnabled(state: CoverArtScreensaverState, enabled: unknown): CoverArtScreensaverState;
  setDelay(state: CoverArtScreensaverState, delay: unknown): CoverArtScreensaverState;
  setTrackOverlayDuration(state: CoverArtScreensaverState, duration: unknown): CoverArtScreensaverState;
  setShowExternalSources(state: CoverArtScreensaverState, show: unknown): CoverArtScreensaverState;
  setFilteringEnabled(state: CoverArtScreensaverState, enabled: unknown): CoverArtScreensaverState;
  setAttributeConditions(state: CoverArtScreensaverState, conditions: unknown): CoverArtScreensaverState;
}

/** Owns Cover Art screensaver choices while the existing page remains its DOM and HTTP adapter. */
export function createCoverArtScreensaverController(
  normalizers: CoverArtScreensaverNormalizers,
): CoverArtScreensaverController {
  function normalize(state: CoverArtScreensaverState): CoverArtScreensaverState {
    const attributeConditions = String(state.attributeConditions || "");
    return {
      ...state,
      delay: normalizers.delay(state.delay),
      attributeConditions,
      filteringEnabled: !!state.filteringEnabled || !!attributeConditions,
    };
  }

  return {
    normalize,
    initialState(state) {
      const normalized = normalize(state);
      return { ...normalized, filteringEnabled: !!normalized.attributeConditions };
    },
    uiState(state) {
      const normalized = normalize(state);
      return {
        contentVisible: normalized.enabled,
        badgeVisible: normalized.enabled,
        externalSourcesVisible: !normalized.hideExternalInput,
        filterOptionsVisible: normalized.filteringEnabled,
      };
    },
    setEnabled(state, enabled) {
      return { ...state, enabled: !!enabled };
    },
    setDelay(state, delay) {
      return { ...state, delay: normalizers.delay(delay) };
    },
    setTrackOverlayDuration(state, duration) {
      return { ...state, trackOverlayDuration: normalizers.trackOverlayDuration(duration) };
    },
    setShowExternalSources(state, show) {
      return { ...state, hideExternalInput: !show };
    },
    setFilteringEnabled(state, enabled) {
      const filteringEnabled = !!enabled;
      return {
        ...state,
        filteringEnabled,
        attributeConditions: filteringEnabled ? state.attributeConditions : "",
      };
    },
    setAttributeConditions(state, conditions) {
      const attributeConditions = String(conditions || "");
      return {
        ...state,
        attributeConditions,
        filteringEnabled: !!attributeConditions || state.filteringEnabled,
      };
    },
  };
}
