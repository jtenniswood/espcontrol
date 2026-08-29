export interface MediaPlaybackState {
  readonly sleepPreventionEnabled: boolean;
  readonly sleepPreventionEntity: string;
  readonly coverArtEntity: string;
}

export interface MediaPlaybackUiState {
  readonly sleepPreventionEnabled: boolean;
}

export interface MediaPlaybackController {
  uiState(state: MediaPlaybackState): MediaPlaybackUiState;
  setSleepPreventionEnabled(state: MediaPlaybackState, enabled: unknown): MediaPlaybackState;
  setCoverArtEntity(state: MediaPlaybackState, entity: unknown): MediaPlaybackState;
}

/** Owns shared media playback settings while the existing page remains its DOM and HTTP adapter. */
export function createMediaPlaybackController(): MediaPlaybackController {
  return {
    uiState(state) {
      return { sleepPreventionEnabled: state.sleepPreventionEnabled };
    },
    setSleepPreventionEnabled(state, enabled) {
      return { ...state, sleepPreventionEnabled: !!enabled };
    },
    setCoverArtEntity(state, entity) {
      const coverArtEntity = String(entity == null ? "" : entity);
      return {
        ...state,
        coverArtEntity,
        sleepPreventionEntity: coverArtEntity,
      };
    },
  };
}
