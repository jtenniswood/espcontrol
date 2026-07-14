export function normalizeTemperatureUnit(value: unknown): string {
  const unit = String(value == null ? "" : value).trim().toLowerCase();
  if (unit === "f" || unit === "\u00B0f" || unit === "fahrenheit") return "\u00B0F";
  if (unit === "c" || unit === "\u00B0c" || unit === "celsius" || unit === "centigrade") return "\u00B0C";
  return "Auto";
}

export function normalizeClockBarTemperatureEntities(value: unknown): string[] {
  const input = Array.isArray(value) ? value : String(value || "").split(/[|,\n]/);
  const out: string[] = [];
  for (const entry of input) {
    const entity = String(entry || "").trim();
    if (entity && out.indexOf(entity) === -1) out.push(entity);
  }
  return out.slice(0, 1);
}

export function normalizeLanguage(value: unknown): string {
  const language = String(value == null ? "" : value).trim().toLowerCase();
  return language || "en";
}

export function normalizeHour(value: unknown, fallback: number): number {
  const n = parseInt(String(value), 10);
  if (!Number.isFinite(n)) return fallback;
  if (n < 0) return 0;
  if (n > 23) return 23;
  return n;
}

export function normalizeTimeOfDay(value: unknown, fallback: string): string {
  const text = String(value == null ? "" : value).trim();
  const match = /^(\d{1,2}):(\d{2})$/.exec(text);
  if (!match) return fallback;
  const hour = parseInt(match[1] || "", 10);
  const minute = parseInt(match[2] || "", 10);
  if (!Number.isFinite(hour) || !Number.isFinite(minute)) return fallback;
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return fallback;
  return String(hour).padStart(2, "0") + ":" + String(minute).padStart(2, "0");
}

export function normalizeScheduleWakeTimeout(value: unknown): number {
  const n = parseFloat(String(value));
  if (!Number.isFinite(n) || n <= 0) return 60;
  if (n < 10) return 10;
  if (n > 3600) return 3600;
  return Math.round(n);
}

export function normalizeScheduleWakeBrightness(value: unknown): number {
  const n = parseFloat(String(value));
  if (!Number.isFinite(n) || n <= 0) return 10;
  if (n < 10) return 10;
  if (n > 100) return 100;
  return Math.round(n);
}

export function normalizeScheduleClockBrightness(value: unknown): number {
  const n = parseFloat(String(value));
  if (!Number.isFinite(n) || n <= 0) return 10;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

export function normalizeHexColor(value: unknown, fallback: string): string {
  const color = String(value == null ? "" : value).replace(/^#/, "").trim().toUpperCase();
  return /^[0-9A-F]{6}$/.test(color) ? color : fallback;
}

export function normalizeScheduleDimmedBrightness(value: unknown): number {
  const n = parseFloat(String(value));
  if (!Number.isFinite(n) || n <= 0) return 10;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

export function normalizeScheduleMode(value: unknown): string {
  const mode = String(value || "").toLowerCase().replace(/[\s-]+/g, "_");
  if (mode === "screen_dimmed" || mode === "dimmed" || mode === "always_on" || mode === "always") {
    return "screen_dimmed";
  }
  if (mode === "clock") return "clock";
  return "screen_off";
}

export function normalizeScheduleTrigger(value: unknown, scheduleEnabled = false): string {
  const trigger = String(value || "").toLowerCase().replace(/[\s-]+/g, "_");
  if (trigger === "sensor") return "sensor";
  if (trigger === "time" || trigger === "timer") return "time";
  if (trigger === "disabled" || trigger === "off") return "disabled";
  return scheduleEnabled ? "time" : "disabled";
}

export function normalizeScreensaverAction(value: unknown): string {
  const action = String(value || "").toLowerCase().replace(/[\s-]+/g, "_");
  if (action === "screen_dimmed" || action === "dimmed" || action === "dim") return "dim";
  if (action === "clock") return "clock";
  return "off";
}

export function screensaverActionOption(value: unknown): string {
  const action = normalizeScreensaverAction(value);
  if (action === "dim") return "Screen Dimmed";
  if (action === "clock") return "Clock";
  return "Display Off";
}

export function scheduleModeOption(value: unknown): string {
  const mode = normalizeScheduleMode(value);
  if (mode === "screen_dimmed") return "Screen Dimmed";
  if (mode === "clock") return "Clock";
  return "Screen off";
}

export function normalizeClockBrightness(value: unknown, fallback: number): number {
  const n = parseFloat(String(value));
  if (!Number.isFinite(n) || n <= 0) return fallback;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

export function normalizeScreensaverDimmedBrightness(value: unknown): number {
  const n = parseFloat(String(value));
  if (!Number.isFinite(n) || n <= 0) return 10;
  if (n < 1) return 1;
  if (n > 100) return 100;
  return Math.round(n);
}

export function normalizeHomeAssistantArtworkPort(value: unknown): number {
  const port = parseInt(String(value), 10);
  if (!Number.isFinite(port)) return 8123;
  if (port < 1) return 1;
  if (port > 65535) return 65535;
  return port;
}

export function normalizeHomeAssistantArtworkProtocol(value: unknown): string {
  return String(value || "").trim().toLowerCase() === "https" ? "https" : "http";
}

export function normalizeNtpServer(value: unknown, fallback: string): string {
  const server = String(value == null ? "" : value).trim();
  return server || fallback;
}

export interface BackupScreenSettingsState {
  brightnessDayVal: number;
  brightnessNightVal: number;
  automaticBrightnessEnabled: boolean;
  brightnessDawnTime: string;
  brightnessDuskTime: string;
  scheduleTrigger: string;
  scheduleEnabled: boolean;
  scheduleOnHour: number;
  scheduleOffHour: number;
  scheduleMode: string;
  scheduleWakeTimeout: number;
  scheduleWakeBrightness: number;
  scheduleDimmedBrightness: number;
  scheduleClockBrightness: number;
  scheduleClockTextColor: string;
}

function numberOrFallback(value: unknown, fallback: number): number {
  const n = parseFloat(String(value));
  return Number.isFinite(n) ? n : fallback;
}

function objectValue(source: Record<string, unknown>, key: string): unknown {
  return Object.prototype.hasOwnProperty.call(source, key) ? source[key] : undefined;
}

export function normalizeBackupScreenSettings(
  screenSettings: Record<string, unknown>,
  current: Partial<BackupScreenSettingsState>,
): BackupScreenSettingsState {
  const legacyScheduleEnabled = !!screenSettings.schedule_enabled;
  const scheduleTrigger = normalizeScheduleTrigger(screenSettings.schedule_trigger, legacyScheduleEnabled);
  return {
    brightnessDayVal: numberOrFallback(screenSettings.brightness_day, 100),
    brightnessNightVal: numberOrFallback(screenSettings.brightness_night, 75),
    automaticBrightnessEnabled: objectValue(screenSettings, "automatic_brightness") != null
      ? !!screenSettings.automatic_brightness
      : true,
    brightnessDawnTime: normalizeTimeOfDay(screenSettings.brightness_dawn_time, "06:00"),
    brightnessDuskTime: normalizeTimeOfDay(screenSettings.brightness_dusk_time, "18:00"),
    scheduleTrigger,
    scheduleEnabled: scheduleTrigger !== "disabled",
    scheduleOnHour: normalizeHour(screenSettings.schedule_on_hour, 6),
    scheduleOffHour: normalizeHour(screenSettings.schedule_off_hour, 23),
    scheduleMode: normalizeScheduleMode(screenSettings.schedule_mode),
    scheduleWakeTimeout: normalizeScheduleWakeTimeout(screenSettings.schedule_wake_timeout),
    scheduleWakeBrightness: normalizeScheduleWakeBrightness(
      objectValue(screenSettings, "schedule_wake_brightness") != null
        ? screenSettings.schedule_wake_brightness
        : current.scheduleWakeBrightness,
    ),
    scheduleDimmedBrightness: normalizeScheduleDimmedBrightness(
      objectValue(screenSettings, "schedule_dimmed_brightness") != null
        ? screenSettings.schedule_dimmed_brightness
        : current.scheduleDimmedBrightness,
    ),
    scheduleClockBrightness: normalizeScheduleClockBrightness(
      objectValue(screenSettings, "schedule_clock_brightness") != null
        ? screenSettings.schedule_clock_brightness
        : current.scheduleClockBrightness,
    ),
    scheduleClockTextColor: normalizeHexColor(
      objectValue(screenSettings, "schedule_clock_text_color") != null
        ? screenSettings.schedule_clock_text_color
        : current.scheduleClockTextColor,
      "FFFFFF",
    ),
  };
}

export interface BackupPanelSettingsCurrent {
  timezone: string;
  language: string;
  clockFormat: string;
  clockFormatOptions: readonly string[];
  ntpDefaults: readonly string[];
  ntpServer1: string;
  ntpServer2: string;
  ntpServer3: string;
  coverArtHomeAssistantProtocol: string;
  coverArtHomeAssistantPort: number;
  autoUpdate: boolean;
  updateFrequency: string;
  updateFrequencyOptions: readonly string[];
  screenRotationOptions: readonly string[];
}

export interface BackupPanelSettingsState {
  indoorTempEnable: boolean;
  outdoorTempEnable: boolean;
  indoorTempEntity: string;
  outdoorTempEntity: string;
  clockBarTemperatureEntities: string[];
  clockBar: boolean;
  clockBarTime: boolean;
  networkStatusIcon: boolean;
  voiceServices: boolean;
  temperatureDegreeSymbol: boolean;
  subpageChevron: boolean;
  timezone: string;
  temperatureUnit: string;
  language: string;
  clockFormat: string;
  hasNtpServer1: boolean;
  hasNtpServer2: boolean;
  hasNtpServer3: boolean;
  ntpServer1: string;
  ntpServer2: string;
  ntpServer3: string;
  screensaverMode: string;
  presenceSensorEntity: string;
  mediaPlayerSleepPrevention: boolean;
  mediaPlayerSleepPreventionEntity: string;
  coverArtScreensaver: boolean;
  coverArtMediaPlayerEntity: string;
  coverArtAttributeConditions: string;
  coverArtDelay: unknown;
  coverArtTrackOverlayDuration: unknown;
  coverArtHideExternalInput: boolean;
  coverArtHomeAssistantProtocol: string;
  coverArtHomeAssistantPort: number;
  autoUpdate: boolean;
  updateFrequency: string;
  screensaverAction: string;
  clockScreensaver: boolean;
  clockBrightnessDay: number;
  clockBrightnessNight: number;
  screensaverDimmedBrightness: number;
  screensaverTimeout: unknown;
  homeScreenTimeout: unknown;
  screenRotation: string;
}

function normalizeScreensaverMode(value: unknown): string {
  const mode = String(value || "disabled");
  return mode === "sensor" || mode === "timer" || mode === "disabled" ? mode : "disabled";
}

function normalizeScreenRotationValue(value: unknown, options: readonly string[]): string {
  const rotation = String(value == null ? "" : value);
  return options.indexOf(rotation) !== -1 ? rotation : "0";
}

function normalizeUpdateFrequency(value: unknown, options: readonly string[], fallback: string): string {
  const frequency = String(value == null ? "" : value);
  return options.indexOf(frequency) !== -1 ? frequency : fallback;
}

export function normalizeBackupPanelSettings(
  settings: Record<string, unknown>,
  current: BackupPanelSettingsCurrent,
): BackupPanelSettingsState {
  const hasNtpServer1 = objectValue(settings, "ntp_server_1") !== undefined;
  const hasNtpServer2 = objectValue(settings, "ntp_server_2") !== undefined;
  const hasNtpServer3 = objectValue(settings, "ntp_server_3") !== undefined;
  const hasOutdoorTempEnable = objectValue(settings, "outdoor_temp_enable") !== undefined;
  const clockFormat = current.clockFormatOptions.indexOf(String(settings.clock_format || "")) !== -1
    ? String(settings.clock_format)
    : current.clockFormat;
  const screensaverAction = normalizeScreensaverAction(
    objectValue(settings, "screensaver_action") != null
      ? settings.screensaver_action
      : (settings.clock_screensaver ? "clock" : "off"),
  );
  const clockBrightnessDay = normalizeClockBrightness(
    objectValue(settings, "clock_brightness_day") != null ? settings.clock_brightness_day : settings.clock_brightness,
    35,
  );
  const clockBrightnessNight = normalizeClockBrightness(
    objectValue(settings, "clock_brightness_night") != null ? settings.clock_brightness_night : settings.clock_brightness,
    clockBrightnessDay,
  );
  const legacyTemperatureEntities: string[] = [];
  if (settings.outdoor_temp_enable && settings.outdoor_temp_entity) {
    legacyTemperatureEntities.push(String(settings.outdoor_temp_entity));
  }
  if (settings.indoor_temp_enable && settings.indoor_temp_entity) {
    legacyTemperatureEntities.push(String(settings.indoor_temp_entity));
  }
  const clockBarTemperatureEntities = normalizeClockBarTemperatureEntities(
    objectValue(settings, "clock_bar_temperature_entities") != null
      ? settings.clock_bar_temperature_entities
      : legacyTemperatureEntities,
  );
  return {
    indoorTempEnable: false,
    outdoorTempEnable: hasOutdoorTempEnable ? !!settings.outdoor_temp_enable : clockBarTemperatureEntities.length > 0,
    indoorTempEntity: "",
    outdoorTempEntity: clockBarTemperatureEntities[0] || "",
    clockBarTemperatureEntities,
    clockBar: objectValue(settings, "clock_bar") != null ? !!settings.clock_bar : false,
    clockBarTime: objectValue(settings, "clock_bar_time") != null ? !!settings.clock_bar_time : true,
    networkStatusIcon: objectValue(settings, "network_status_icon") != null ? !!settings.network_status_icon : true,
    voiceServices: objectValue(settings, "voice_services") != null ? !!settings.voice_services : false,
    temperatureDegreeSymbol: objectValue(settings, "temperature_degree_symbol") != null
      ? !!settings.temperature_degree_symbol
      : true,
    subpageChevron: objectValue(settings, "subpage_chevron") != null
      ? !!settings.subpage_chevron
      : true,
    timezone: String(settings.timezone || current.timezone),
    temperatureUnit: normalizeTemperatureUnit(settings.temperature_unit),
    language: normalizeLanguage(settings.language || current.language),
    clockFormat,
    hasNtpServer1,
    hasNtpServer2,
    hasNtpServer3,
    ntpServer1: hasNtpServer1
      ? normalizeNtpServer(settings.ntp_server_1, current.ntpDefaults[0] || "")
      : current.ntpServer1,
    ntpServer2: hasNtpServer2
      ? normalizeNtpServer(settings.ntp_server_2, current.ntpDefaults[1] || "")
      : current.ntpServer2,
    ntpServer3: hasNtpServer3
      ? normalizeNtpServer(settings.ntp_server_3, current.ntpDefaults[2] || "")
      : current.ntpServer3,
    screensaverMode: normalizeScreensaverMode(settings.screensaver_mode),
    presenceSensorEntity: String(settings.presence_sensor_entity || ""),
    mediaPlayerSleepPrevention: !!settings.media_player_sleep_prevention,
    mediaPlayerSleepPreventionEntity: String(settings.media_player_sleep_prevention_entity || settings.cover_art_media_player_entity || ""),
    coverArtScreensaver: !!settings.cover_art_screensaver,
    coverArtMediaPlayerEntity: String(settings.cover_art_media_player_entity || settings.media_player_sleep_prevention_entity || ""),
    coverArtAttributeConditions: String(settings.cover_art_attribute_conditions || settings.cover_art_conditions || ""),
    coverArtDelay: objectValue(settings, "cover_art_delay") != null ? settings.cover_art_delay : 10,
    coverArtTrackOverlayDuration: objectValue(settings, "cover_art_track_overlay_duration") != null ? settings.cover_art_track_overlay_duration : 5,
    coverArtHideExternalInput: objectValue(settings, "cover_art_hide_external_input") != null
      ? !!settings.cover_art_hide_external_input
      : true,
    coverArtHomeAssistantProtocol: objectValue(settings, "home_assistant_artwork_protocol") != null
      ? normalizeHomeAssistantArtworkProtocol(settings.home_assistant_artwork_protocol)
      : normalizeHomeAssistantArtworkProtocol(current.coverArtHomeAssistantProtocol),
    coverArtHomeAssistantPort: objectValue(settings, "home_assistant_artwork_port") != null
      ? normalizeHomeAssistantArtworkPort(settings.home_assistant_artwork_port)
      : normalizeHomeAssistantArtworkPort(current.coverArtHomeAssistantPort),
    autoUpdate: objectValue(settings, "firmware_auto_update") != null
      ? !!settings.firmware_auto_update
      : current.autoUpdate,
    updateFrequency: objectValue(settings, "firmware_update_frequency") != null
      ? normalizeUpdateFrequency(
        settings.firmware_update_frequency,
        current.updateFrequencyOptions,
        current.updateFrequency,
      )
      : current.updateFrequency,
    screensaverAction,
    clockScreensaver: screensaverAction === "clock",
    clockBrightnessDay,
    clockBrightnessNight,
    screensaverDimmedBrightness: normalizeScreensaverDimmedBrightness(settings.screensaver_dimmed_brightness),
    screensaverTimeout: settings.screensaver_timeout || 300,
    homeScreenTimeout: objectValue(settings, "home_screen_timeout") != null ? settings.home_screen_timeout : 60,
    screenRotation: normalizeScreenRotationValue(settings.screen_rotation, current.screenRotationOptions),
  };
}
