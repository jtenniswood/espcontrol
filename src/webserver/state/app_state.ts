import type { CardConfig } from "../contracts/types";
import { WEB_UI_COLORS, defaultTheme } from "./ui_tokens";
import type { AppState, DeviceConfig } from "./types";

export const AUTO_TIMEZONE_OPTION = "Auto (Home Assistant)";
export const FALLBACK_TIMEZONE_OPTION = "UTC (GMT+0)";
export const NTP_SERVER_DEFAULTS = ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"] as const;
export const LANGUAGE_LABELS: Readonly<Record<string, string>> = {
  cs: "Čeština (Czech)", da: "Dansk (Danish)", de: "Deutsch (German)", en: "English",
  es: "Español (Spanish)", fi: "Suomi (Finnish)", fr: "Français (French)", hu: "Magyar (Hungarian)",
  it: "Italiano (Italian)", nb: "Norsk bokmål (Norwegian Bokmål)", nl: "Nederlands (Dutch)",
  pl: "Polski (Polish)", pt: "Português (Portuguese)", "pt-br": "Português (Brasil) (Brazilian Portuguese)",
  ro: "Română (Romanian)", sk: "Slovenčina (Slovak)", sl: "Slovenščina (Slovenian)",
  sv: "Svenska (Swedish)", tr: "Türkçe (Turkish)", uk: "Українська (Ukrainian)",
};
export const THEME_PRESETS = {
  Light: { on: "0073FF" },
  Dark: { on: WEB_UI_COLORS.primary },
} as const;
export const DEFAULT_COLOR_PRESET = THEME_PRESETS[defaultTheme()];

const LANGUAGE_OPTIONS = ["en", "cs", "da", "de", "es", "fi", "fr", "hu", "it", "nb", "nl", "pl", "pt", "pt-br", "ro", "sk", "sl", "sv", "tr", "uk"];

function emptyCardConfig(): CardConfig {
  return { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" };
}

export function defaultTimezoneOptionsForDevice(deviceConfig: DeviceConfig): string[] {
  return Array.isArray(deviceConfig.timezoneOptions) ? deviceConfig.timezoneOptions.slice() : [];
}

export function createInitialState(deviceConfig: DeviceConfig): AppState {
  const grid = Array.from({ length: deviceConfig.slots }, () => 0);
  const buttons = Array.from({ length: deviceConfig.slots }, emptyCardConfig);
  return {
    grid, sizes: {}, buttons, theme: defaultTheme(), onColor: DEFAULT_COLOR_PRESET.on,
    selectedSlots: [], lastClickedSlot: -1, clockBarSelectedItem: "", activeTab: "screen",
    _indoorOn: false, _outdoorOn: false, _indoorVal: null, _outdoorVal: null,
    indoorEntity: "", outdoorEntity: "", clockBarTemperatureEntities: [],
    _clockBarTemperatureEntitiesReceived: false, _clockBarTemperatureVisibilityReceived: false,
    temperatureUnit: "Auto", clockBarOn: false, _clockBarStateValues: {}, clockBarTimeOn: true,
    networkStatusOn: true, voiceServicesOn: false, networkTransport: "wifi", wifiStrengthPercent: 100,
    temperatureDegreeSymbolOn: true, subpageChevronsOn: true, presenceEntity: "",
    mediaPlayerSleepPreventionOn: true, mediaPlayerSleepPreventionEntity: "",
    coverArtScreensaverOn: false, coverArtMediaPlayerEntity: "", coverArtAttributeConditions: "",
    coverArtFilteringEnabled: false, coverArtDelay: 10, coverArtTrackOverlayDuration: 5,
    coverArtHideExternalInputOn: true, homeAssistantArtworkProtocol: "http", coverArtHomeAssistantPort: 8123,
    screensaverMode: "disabled", _screensaverModeReceived: false, screensaverAction: "off",
    _screensaverActionReceived: false, screensaverPinRequired: false, screensaverPinSet: false,
    clockScreensaverOn: false, clockBrightnessDay: 35,
    clockBrightnessNight: 35, clockBrightnessSplitReceived: false, screensaverDimmedBrightness: 10,
    screensaverTimeout: 300, screensaverTimeoutMin: 60, screensaverTimeoutMax: 3600,
    screensaverTimeoutLimitsLoaded: false, homeScreenTimeout: 60, brightnessDayVal: 100,
    brightnessNightVal: 75, automaticBrightnessEnabled: true, brightnessDawnTime: "06:00",
    brightnessDuskTime: "18:00", scheduleTrigger: "disabled", _scheduleTriggerReceived: false,
    scheduleEnabled: false, scheduleOnHour: 6, scheduleOffHour: 23, scheduleMode: "screen_off",
    scheduleWakeTimeout: 60, scheduleWakeBrightness: 10, scheduleDimmedBrightness: 10,
    scheduleClockBrightness: 10, scheduleClockTextColor: "FFFFFF", timezone: AUTO_TIMEZONE_OPTION,
    activeTimezone: FALLBACK_TIMEZONE_OPTION, timezoneOptions: defaultTimezoneOptionsForDevice(deviceConfig),
    language: "en", languageOptions: LANGUAGE_OPTIONS.slice(), clockFormat: "24h",
    clockFormatOptions: ["12h", "24h"], customNtpServers: false, ntpServer1: NTP_SERVER_DEFAULTS[0],
    ntpServer2: NTP_SERVER_DEFAULTS[1], ntpServer3: NTP_SERVER_DEFAULTS[2],
    screenRotation: deviceConfig.features?.screenRotationDefault || "0",
    screenRotationOptions: deviceConfig.features?.screenRotationOptions?.slice() || ["0", "90", "180", "270"],
    screenRotationDeviceOptions: null, screenRotationInitialReady: !deviceConfig.features?.screenRotation,
    screenRotationInitialTimer: null,
    pendingButtonOrderRaw: null, sunrise: "", sunset: "", firmwareVersion: "", firmwareLatestVersion: "",
    firmwareUpdateState: "", firmwareReleaseUrl: "", firmwareChecking: false,
    firmwareVersionRefreshPending: false, firmwareInstallTargetVersion: "", firmwareInstallPostPending: false,
    firmwareInstallStatus: "", firmwareInstallError: "", firmwareUpdateControlsSupported: false,
    firmwareInstallControlsSupported: false, firmwareOtaUrl: "", firmwareOtaFilename: "", firmwareOtaMd5: "",
    firmwareVersionOptions: [], firmwareSelectedVersion: "", firmwareVersionIndexLoaded: false,
    c6FirmwareCurrentVersion: "", c6FirmwareLatestVersion: "", c6FirmwareUpdateAvailable: "",
    c6FirmwareUpdateControlsSupported: false, c6FirmwareInstallControlsSupported: false,
    c6FirmwareChecking: false, c6FirmwareInstalling: false, autoUpdate: true, updateFrequency: "Daily",
    updateFreqOptions: ["Hourly", "Daily", "Weekly", "Monthly"], configLocked: false, configLockReason: "",
    clockBarDragItem: "", clockBarTempRestoreIndoor: false, clockBarTempRestoreOutdoor: true,
    clockBarTempRestoreEntities: [], subpages: {}, subpageRaw: {}, subpageSavePending: {}, editingSubpage: null,
    subpageSelectedSlots: [], subpageLastClicked: -1, clipboard: null, settingsDraft: null,
    entityPostPaths: {}, entityNames: {},
  };
}
