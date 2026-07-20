import type { CardConfig } from "../contracts/types";
import type { ClipboardEntry } from "../features/clipboard";
import type { SlotSizeMap } from "../model/grid";

export interface DeviceConfigFeatures {
  internalRelays?: readonly { readonly key: string; readonly label: string }[];
  screenRotation?: boolean;
  screenRotationDefault?: string;
  screenRotationDisplayLabels?: Readonly<Record<string, string>>;
  screenRotationDisplayOffset?: number;
  screenRotationOptions?: readonly string[];
  subpageConfigChunks?: number;
  voiceServices?: boolean;
  battery?: boolean;
  alarmDelayAudio?: boolean;
}

export interface DeviceConfig {
  readonly slots: number;
  readonly cols: number;
  readonly rows: number;
  readonly screenSize: string;
  readonly dragMode: "swap" | "displace";
  readonly dragAnimation: boolean;
  readonly imageSlotCapacity: number;
  readonly largeSensorUnitOffsetPercent?: number;
  readonly coverArtSquareOverlay?: boolean;
  readonly disabledCardTypes?: readonly string[];
  readonly infoOnly?: boolean;
  readonly features?: DeviceConfigFeatures;
  readonly screen: {
    readonly width: string;
    readonly aspect: string;
  };
  readonly portrait?: {
    readonly cols: number;
    readonly rows: number;
    readonly screen: {
      readonly width: string;
      readonly aspect: string;
    };
  };
  readonly topbar?: Readonly<Record<string, string | number>>;
  readonly grid?: Readonly<Record<string, string | number>>;
  readonly btn?: Readonly<Record<string, number>>;
  readonly emptyCell?: Readonly<Record<string, number>>;
  readonly sensorBadge?: Readonly<Record<string, number>>;
  readonly subpageBadge?: Readonly<Record<string, number>>;
  readonly timezoneOptions?: readonly string[];
}

export interface EntityEvent {
  readonly id?: string;
  readonly state?: string | number | boolean | null;
  readonly value?: string | number | boolean | null;
  readonly name_id?: string;
  readonly domain?: string;
  readonly name?: string;
  readonly object_id?: string;
  readonly option?: readonly string[];
  readonly min_value?: number | string;
  readonly max_value?: number | string;
  readonly [key: string]: unknown;
}

export interface FirmwareEvent extends EntityEvent {
  readonly latest_version?: string;
  readonly release_url?: string;
  readonly ota_url?: string;
  readonly ota_filename?: string;
  readonly ota_md5?: string;
}

export interface FirmwareVersionInfo {
  latest_version: string;
  release_url?: string;
  ota_url?: string;
  ota_filename?: string;
  ota_md5?: string;
}

export interface RuntimeSubpage {
  order: string[];
  buttons: CardConfig[];
  grid: number[];
  sizes: SlotSizeMap;
  backLabel?: string;
}

export interface AppClipboard {
  buttons: ClipboardEntry[];
}

export interface SettingsDraft {
  key: string;
  slot: number;
  homeSlot: number | null;
  isSub: boolean;
  dirty: boolean;
  button: CardConfig;
  isNew?: boolean;
  pos?: number;
  typeSelected?: boolean;
  autoSelectedButton?: CardConfig | null;
}

export interface AppState {
  grid: number[];
  sizes: Record<string, number>;
  buttons: CardConfig[];
  onColor: string;
  selectedSlots: number[];
  lastClickedSlot: number;
  clockBarSelectedItem: string;
  activeTab: string;
  _indoorOn: boolean;
  _outdoorOn: boolean;
  _indoorVal: number | null;
  _outdoorVal: number | null;
  indoorEntity: string;
  outdoorEntity: string;
  clockBarTemperatureEntities: string[];
  _clockBarTemperatureEntitiesReceived: boolean;
  _clockBarTemperatureVisibilityReceived: boolean;
  temperatureUnit: string;
  clockBarOn: boolean;
  _clockBarStateValues: Record<string, boolean>;
  clockBarTimeOn: boolean;
  networkStatusOn: boolean;
  batteryStatusOn: boolean;
  voiceServicesOn: boolean;
  agendaViewDays: number;
  agendaViewHideEmpty: boolean;
  alarmDelayAudioOn: boolean;
  alarmDelayTtsOn: boolean;
  alarmDelayEntryAnnouncement: string;
  alarmDelayExitAnnouncement: string;
  alarmDelayBeepVolume: number;
  alarmDelayFinalCountdown: number;
  networkTransport: string;
  wifiStrengthPercent: number;
  temperatureDegreeSymbolOn: boolean;
  subpageChevronsOn: boolean;
  presenceEntity: string;
  mediaPlayerSleepPreventionOn: boolean;
  mediaPlayerSleepPreventionEntity: string;
  coverArtScreensaverOn: boolean;
  coverArtMediaPlayerEntity: string;
  coverArtSecondaryMediaPlayerEntity: string;
  coverArtAttributeConditions: string;
  coverArtFilteringEnabled: boolean;
  coverArtDelay: number;
  coverArtTrackOverlayDuration: number;
  coverArtHideExternalInputOn: boolean;
  homeAssistantArtworkProtocol: string;
  coverArtHomeAssistantPort: number;
  screensaverMode: string;
  _screensaverModeReceived: boolean;
  screensaverAction: string;
  _screensaverActionReceived: boolean;
  clockScreensaverOn: boolean;
  clockBrightnessDay: number;
  clockBrightnessNight: number;
  clockBrightnessSplitReceived: boolean;
  screensaverDimmedBrightness: number;
  photosFolder: string;
  photosInterval: number;
  photosShuffle: boolean;
  photosShowDatetime: boolean;
  photosShowDate: boolean;
  photosShowWeather: boolean;
  photosWeatherEntity: string;
  photosShowAgenda: boolean;
  photosAgendaEntities: string;
  photosAgendaStyle: string;
  photosAgendaOpacity: number;
  photosAgendaLimit: number;
  photosAgendaDays: number;
  photosAgendaHideEmpty: boolean;
  screensaverTimeout: number;
  screensaverTimeoutMin: number;
  screensaverTimeoutMax: number;
  screensaverTimeoutLimitsLoaded: boolean;
  homeScreenTimeout: number;
  brightnessDayVal: number;
  brightnessNightVal: number;
  automaticBrightnessEnabled: boolean;
  brightnessDawnTime: string;
  brightnessDuskTime: string;
  scheduleTrigger: string;
  _scheduleTriggerReceived: boolean;
  scheduleEnabled: boolean;
  scheduleSensorActivation: string;
  scheduleOnHour: number;
  scheduleOffHour: number;
  scheduleMode: string;
  scheduleWakeTimeout: number;
  scheduleWakeBrightness: number;
  scheduleDimmedBrightness: number;
  scheduleClockBrightness: number;
  scheduleClockTextColor: string;
  timezone: string;
  activeTimezone: string;
  timezoneOptions: string[];
  language: string;
  languageOptions: string[];
  clockFormat: string;
  clockFormatOptions: string[];
  customNtpServers: boolean;
  ntpServer1: string;
  ntpServer2: string;
  ntpServer3: string;
  screenRotation: string;
  screenRotationOptions: string[];
  screenRotationDeviceOptions: readonly string[] | null;
  screenRotationInitialReady: boolean;
  screenRotationInitialTimer: number | null;
  pendingButtonOrderRaw: string | null;
  sunrise: string;
  sunset: string;
  firmwareVersion: string;
  firmwareLatestVersion: string;
  firmwareUpdateState: string;
  firmwareReleaseUrl: string;
  firmwareChecking: boolean;
  firmwareVersionRefreshPending: boolean;
  firmwareInstallTargetVersion: string;
  firmwareInstallPostPending: boolean;
  firmwareInstallStatus: string;
  firmwareInstallError: string;
  firmwareUpdateControlsSupported: boolean;
  firmwareInstallControlsSupported: boolean;
  firmwareOtaUrl: string;
  firmwareOtaFilename: string;
  firmwareOtaMd5: string;
  firmwareVersionOptions: FirmwareVersionInfo[];
  firmwareSelectedVersion: string;
  firmwareVersionIndexLoaded: boolean;
  c6FirmwareCurrentVersion: string;
  c6FirmwareLatestVersion: string;
  c6FirmwareUpdateAvailable: string;
  c6FirmwareUpdateControlsSupported: boolean;
  c6FirmwareInstallControlsSupported: boolean;
  c6FirmwareAutoUpdateSupported: boolean;
  c6FirmwareAutoUpdate: boolean;
  c6FirmwareChecking: boolean;
  c6FirmwareInstalling: boolean;
  autoUpdate: boolean;
  updateFrequency: string;
  updateFreqOptions: string[];
  configLocked: boolean;
  configLockReason: string;
  clockBarDragItem: string;
  clockBarTempRestoreIndoor: boolean;
  clockBarTempRestoreOutdoor: boolean;
  clockBarTempRestoreEntities: string[];
  subpages: Record<string, RuntimeSubpage>;
  subpageRaw: Record<string, Record<string, string>>;
  subpageSavePending: Record<string, string>;
  editingSubpage: number | null;
  subpageSelectedSlots: number[];
  subpageLastClicked: number;
  clipboard: AppClipboard | null;
  settingsDraft: SettingsDraft | null;
  entityPostPaths: Record<string, string>;
  entityNames: Record<string, string[]>;
}
