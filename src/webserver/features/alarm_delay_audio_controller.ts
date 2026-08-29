export interface AlarmDelayAudioState {
  readonly audioEnabled: boolean;
  readonly ttsEnabled: boolean;
  readonly entryAnnouncement: string;
  readonly exitAnnouncement: string;
  readonly beepVolume: number;
  readonly finalCountdown: number;
}

export interface AlarmDelayAudioUiState {
  readonly audioOptionsVisible: boolean;
  readonly ttsOptionsVisible: boolean;
  readonly beepVolumePercent: number;
  readonly finalCountdown: number;
}

export interface AlarmDelayAudioNormalizers {
  readonly announcement: (value: unknown, fallback: string) => string;
  readonly beepVolume: (value: unknown) => number;
  readonly finalCountdown: (value: unknown) => number;
}

export interface AlarmDelayAudioController {
  uiState(state: AlarmDelayAudioState): AlarmDelayAudioUiState;
  setAudioEnabled(state: AlarmDelayAudioState, enabled: unknown): AlarmDelayAudioState;
  setTtsEnabled(state: AlarmDelayAudioState, enabled: unknown): AlarmDelayAudioState;
  setAnnouncement(state: AlarmDelayAudioState, field: "entryAnnouncement" | "exitAnnouncement", value: unknown, fallback: string): AlarmDelayAudioState;
  setBeepVolume(state: AlarmDelayAudioState, value: unknown): AlarmDelayAudioState;
  setFinalCountdown(state: AlarmDelayAudioState, value: unknown): AlarmDelayAudioState;
}

/** Owns the Alarm Audio journey while the existing page remains its DOM adapter. */
export function createAlarmDelayAudioController(normalizers: AlarmDelayAudioNormalizers): AlarmDelayAudioController {
  return {
    uiState(state) {
      return {
        audioOptionsVisible: state.audioEnabled,
        ttsOptionsVisible: state.audioEnabled && state.ttsEnabled,
        beepVolumePercent: Math.round(state.beepVolume * 100),
        finalCountdown: state.finalCountdown,
      };
    },
    setAudioEnabled(state, enabled) {
      return { ...state, audioEnabled: !!enabled };
    },
    setTtsEnabled(state, enabled) {
      return { ...state, ttsEnabled: !!enabled };
    },
    setAnnouncement(state, field, value, fallback) {
      return { ...state, [field]: normalizers.announcement(value, fallback) };
    },
    setBeepVolume(state, value) {
      return { ...state, beepVolume: normalizers.beepVolume(value) };
    },
    setFinalCountdown(state, value) {
      return { ...state, finalCountdown: normalizers.finalCountdown(value) };
    },
  };
}
