import { screensaverControlState, timedSettingLabel } from "../../src/webserver/features/settings";
import { createAlarmDelayAudioController } from "../../src/webserver/features/alarm_delay_audio_controller";
import { createScreensaverController } from "../../src/webserver/features/screensaver_controller";
import { createCoverArtScreensaverController } from "../../src/webserver/features/cover_art_screensaver_controller";
import { createMediaPlaybackController } from "../../src/webserver/features/media_playback_controller";
import { createVoiceServicesController } from "../../src/webserver/features/voice_services_controller";
import { createClockBarController } from "../../src/webserver/features/clock_bar_controller";
import { createScreenScheduleController } from "../../src/webserver/features/screen_schedule_controller";
import { normalizeThemeAutoMethod, normalizeThemeMode, normalizeTimeOfDay, parseThemeSunOffset } from "../../src/webserver/model/settings";
import { PREVIEW_THEME_COLORS, previewEffectiveTheme, previewThemeCss } from "../../src/webserver/state/preview_theme";
import { createBackupEnvelope, normalizeBackupEnvelope } from "../../src/webserver/model/backup";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

function luminance(hex: string): number {
  const channels = [0, 2, 4].map((start) => {
    const value = parseInt(hex.slice(start, start + 2), 16) / 255;
    return value <= 0.04045 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4;
  });
  return channels[0]! * 0.2126 + channels[1]! * 0.7152 + channels[2]! * 0.0722;
}

function contrast(first: string, second: string): number {
  const values = [luminance(first), luminance(second)].sort((a, b) => b - a);
  return (values[0]! + 0.05) / (values[1]! + 0.05);
}

function placeholderOutline(textMuted: string, background: string): string {
  // Matches the 70% muted-text mix in the preview's empty-cell CSS.
  return [0, 2, 4].map((start) => {
    const muted = parseInt(textMuted.slice(start, start + 2), 16);
    const base = parseInt(background.slice(start, start + 2), 16);
    return Math.round(muted * 0.7 + base * 0.3).toString(16).padStart(2, "0");
  }).join("");
}

export function runSettingsFeatureTests(): void {
  for (const mode of ["Dark", "Light", "Auto"])
    equal(normalizeThemeMode(mode.toLowerCase()), mode, "theme mode backup round trip");
  equal(normalizeThemeMode("Sun"), "Auto", "old Sun backups migrate to Auto");
  equal(normalizeThemeMode("Schedule"), "Auto", "old Schedule backups migrate to Auto");
  equal(normalizeThemeAutoMethod("Sun"), "Sunrise / Sunset", "old Sun backups retain their method");
  equal(normalizeThemeAutoMethod("Schedule"), "Time", "old Schedule backups retain their method");
  equal(parseThemeSunOffset("-180"), -180, "negative offset boundary");
  equal(parseThemeSunOffset("+180"), 180, "positive offset boundary");
  equal(parseThemeSunOffset("181"), null, "out-of-range offset rejected");
  equal(parseThemeSunOffset("1.5"), null, "fractional offset rejected");
  equal(previewEffectiveTheme({ themeMode: "Auto", themeActive: "Light" }), "Light", "Auto preview follows active firmware theme");
  equal(previewEffectiveTheme({ themeMode: "Dark", themeActive: "Light" }), "Dark", "manual preview follows chosen mode");
  equal(PREVIEW_THEME_COLORS.Light.surfacePrimary, "FFFFFF", "preview Light surface matches device palette");
  equal(previewThemeCss("Light").includes("--preview-text-primary:#181818"), true, "preview Light foreground is semantic");
  for (const [mode, palette] of Object.entries(PREVIEW_THEME_COLORS)) {
    equal(contrast(palette.textPrimary, palette.surfacePrimary) >= 4.5, true, `${mode} primary text contrast`);
    equal(contrast(palette.textMuted, palette.surfacePrimary) >= 4.5, true, `${mode} muted text contrast`);
    equal(contrast(placeholderOutline(palette.textMuted, palette.background), palette.background) >= 3,
          true, `${mode} dashed placeholder contrast`);
  }
  equal(normalizeThemeMode(undefined), "Dark", "old settings default to Dark");
  equal(normalizeTimeOfDay("20:00", "07:00"), "20:00", "overnight Light time round trips");
  equal(normalizeTimeOfDay("07:00", "20:00"), "07:00", "overnight Dark time round trips");
  equal(normalizeTimeOfDay("24:00", "07:00"), "07:00", "invalid backup time uses safe default");
  const themeScreen = { theme_mode: "Auto", theme_auto_method: "Sunrise / Sunset", theme_light_start: "20:00", theme_dark_start: "07:00", theme_sunrise_offset: -20, theme_sunset_offset: 45 };
  const backupOutputs = { buttons: [], subpages: {} };
  const savedTheme = createBackupEnvelope({ device: "panel", slots: 0, screen: themeScreen }, backupOutputs);
  const restoredTheme = normalizeBackupEnvelope(savedTheme as unknown as Record<string, unknown>, backupOutputs);
  equal(restoredTheme.screen?.theme_mode, "Auto", "backup retains configured Auto mode rather than effective palette");
  equal(restoredTheme.screen?.theme_auto_method, "Sunrise / Sunset", "backup retains automatic method");
  equal(restoredTheme.screen?.theme_sunrise_offset, -20, "backup retains sunrise offset");
  equal(restoredTheme.screen?.theme_sunset_offset, 45, "backup retains sunset offset");
  equal(restoredTheme.screen?.theme_light_start, "20:00", "backup retains Light boundary");
  equal(restoredTheme.screen?.theme_dark_start, "07:00", "backup retains Dark boundary");
  equal(normalizeBackupEnvelope({ version: 2, format: "espcontrol.backup", buttons: [], screen: {} }, backupOutputs).screen?.theme_mode,
        undefined, "old backups remain valid without a theme field");
  const legacyScreen = normalizeBackupEnvelope({ version: 2, format: "espcontrol.backup", device: "esp32-p4-86", buttons: [],
    screen: { schedule_clock_text_color: "FFFFFF" } }, backupOutputs).screen;
  for (const key of Object.keys(themeScreen))
    equal(legacyScreen?.[key], undefined, `legacy restore must not invent ${key}`);
  const explicitLight = { ...themeScreen, theme_mode: "Light" };
  const lightBackup = createBackupEnvelope({ device: "esp32-p4-86", slots: 0, screen: explicitLight }, backupOutputs);
  const lightRestore = normalizeBackupEnvelope(lightBackup as unknown as Record<string, unknown>, backupOutputs).screen;
  for (const [key, value] of Object.entries(explicitLight))
    equal(lightRestore?.[key], value, `modern Light restore retains ${key}`);
  const clock = screensaverControlState("Clock", 35.4, 12.6, 8.2);
  equal(clock.mode, "clock", "clock action is normalized");
  equal(clock.clockVisible, true, "clock controls are shown for clock mode");
  equal(clock.dimVisible, false, "dim controls are hidden for clock mode");
  equal(clock.dayBrightnessLabel, "35%", "day brightness label retains rounding");
  equal(clock.nightBrightnessLabel, "13%", "night brightness label retains rounding");
  equal(clock.dimBrightnessLabel, "8%", "dim brightness label retains rounding");

  const format = (seconds: number): string => `${seconds} seconds`;
  equal(timedSettingLabel(-1, format), "Always", "negative duration means always");
  equal(timedSettingLabel(0, format), "Never", "zero duration means never");
  equal(timedSettingLabel(15, format), "15 seconds", "positive duration uses the injected formatter");

  const alarm = createAlarmDelayAudioController({
    announcement: (value, fallback) => String(value).trim() || fallback,
    beepVolume: (value) => Math.max(0.05, Math.min(1, Number(value))),
    finalCountdown: (value) => Math.max(0, Math.min(60, Math.round(Number(value)))),
  });
  const initial = {
    audioEnabled: false,
    ttsEnabled: true,
    entryAnnouncement: "Entry",
    exitAnnouncement: "Exit",
    beepVolume: 0.45,
    finalCountdown: 10,
  };
  equal(alarm.uiState(initial).audioOptionsVisible, false, "audio controls hide when audio is disabled");
  equal(alarm.uiState(initial).ttsOptionsVisible, false, "tts controls hide when audio is disabled");
  const enabled = alarm.setAudioEnabled(initial, true);
  equal(alarm.uiState(enabled).ttsOptionsVisible, true, "tts controls show when both toggles are enabled");
  equal(alarm.setAnnouncement(enabled, "entryAnnouncement", "  ", "Default").entryAnnouncement,
        "Default", "announcement changes use their fallback");
  equal(alarm.setBeepVolume(enabled, 2).beepVolume, 1, "volume changes are normalized");
  equal(alarm.setFinalCountdown(enabled, 80).finalCountdown, 60, "countdown changes are normalized");

  const screensaver = createScreensaverController({
    action: (value) => ["off", "dim", "clock", "camera"].includes(String(value)) ? String(value) : "off",
    dimBrightness: (value) => Math.max(1, Math.min(100, Number(value))),
    clockBrightness: (value, fallback) => Math.max(1, Math.min(100, Number(value) || fallback)),
  });
  const dim = {
    action: "dim",
    clockBrightnessDay: 35,
    clockBrightnessNight: 12,
    dimBrightness: 10,
    dimBrightnessDay: 20,
    dimBrightnessNight: 5,
  };
  equal(screensaver.uiState(dim).dimVisible, true, "dim controls show in dim mode");
  equal(screensaver.uiState(dim).clockVisible, false, "clock controls hide in dim mode");
  const clockMode = screensaver.setAction(dim, "clock");
  equal(screensaver.uiState(clockMode).clockVisible, true, "clock controls show in clock mode");
  const cameraMode = screensaver.setAction(clockMode, "camera");
  equal(screensaver.uiState(cameraMode).cameraVisible, true, "camera entity control shows in camera mode");
  equal(screensaver.uiState(cameraMode).clockVisible, false, "clock controls hide in camera mode");
  equal(screensaver.setDimBrightness(clockMode, 200).dimBrightness, 100, "dim brightness is normalized");
  equal(screensaver.setDimBrightnessByPeriod(clockMode, "dimBrightnessDay", 25).dimBrightnessDay,
        25, "daytime dim brightness is updated independently");
  equal(screensaver.setDimBrightnessByPeriod(clockMode, "dimBrightnessNight", 0).dimBrightnessNight,
        20, "nighttime dim brightness falls back to the daytime value");
  equal(screensaver.setClockBrightness(clockMode, "clockBrightnessNight", 0).clockBrightnessNight,
        35, "night brightness uses daytime brightness as its fallback");

  const coverArt = createCoverArtScreensaverController({
    delay: (value) => Math.max(0, Math.min(120, Number(value) || 0)),
    trackOverlayDuration: (value) => Math.max(0, Number(value) || 0),
  });
  const coverArtInitial = {
    enabled: false,
    delay: 10,
    trackOverlayDuration: 5,
    hideExternalInput: true,
    filteringEnabled: false,
    attributeConditions: "",
  };
  equal(coverArt.uiState(coverArtInitial).contentVisible, false, "cover art settings hide when disabled");
  const coverArtEnabled = coverArt.setEnabled(coverArtInitial, true);
  equal(coverArt.uiState(coverArtEnabled).badgeVisible, true, "cover art badge shows when enabled");
  equal(coverArt.setDelay(coverArtEnabled, 300).delay, 120, "cover art delay is normalized");
  equal(coverArt.setShowExternalSources(coverArtEnabled, true).hideExternalInput, false,
        "showing external sources clears the hide setting");
  equal(coverArt.setFilteringEnabled({ ...coverArtEnabled, attributeConditions: "app_id=music" }, false).attributeConditions,
        "", "turning filtering off clears its conditions");
  equal(coverArt.initialState({ ...coverArtEnabled, filteringEnabled: true }).filteringEnabled, false,
        "a freshly built empty filter starts disabled");
  equal(coverArt.normalize({ ...coverArtEnabled, filteringEnabled: true }).filteringEnabled, true,
        "an enabled empty filter remains visible while editing");
  equal(coverArt.uiState(coverArt.setAttributeConditions(coverArtInitial, "media_content_type=music")).filterOptionsVisible,
        true, "saved conditions keep filtering controls visible");

  const mediaPlayback = createMediaPlaybackController();
  const playbackInitial = {
    sleepPreventionEnabled: true,
    sleepPreventionEntity: "media_player.living_room",
    coverArtEntity: "media_player.living_room",
  };
  equal(mediaPlayback.uiState(playbackInitial).sleepPreventionEnabled, true,
        "sleep prevention state is shared by every settings surface");
  equal(mediaPlayback.setSleepPreventionEnabled(playbackInitial, false).sleepPreventionEnabled, false,
        "sleep prevention toggle updates its shared state");
  const changedEntity = mediaPlayback.setCoverArtEntity(playbackInitial, "media_player.kitchen");
  equal(changedEntity.coverArtEntity, "media_player.kitchen", "cover art entity is updated");
  equal(changedEntity.sleepPreventionEntity, "media_player.kitchen",
        "cover art entity remains mirrored to sleep prevention");

  const voiceServices = createVoiceServicesController();
  const voiceInitial = { supported: true, enabled: false };
  equal(voiceServices.uiState(voiceInitial).settingsVisible, true,
        "supported Voice Services appear in settings");
  equal(voiceServices.uiState(voiceInitial).iconVisible, false,
        "disabled Voice Services hide their preview icon");
  equal(voiceServices.uiState(voiceServices.setEnabled(voiceInitial, true)).iconVisible, true,
        "enabling Voice Services updates the preview icon");
  equal(voiceServices.uiState({ supported: false, enabled: true }).clockBarItemVisible, false,
        "unsupported Voice Services stay out of the clock bar");

  const clockBar = createClockBarController();
  const clockBarInitial = { enabled: true, timeEnabled: true, nightModeEnabled: false, selectedItem: "voice" };
  equal(clockBar.uiState(clockBarInitial).previewVisible, true, "enabled clock bar remains visible in the preview");
  equal(clockBar.setNightModeEnabled(clockBarInitial, true).nightModeEnabled, true,
        "night-mode setting updates independently");
  const clockBarDisabled = clockBar.setEnabled(clockBarInitial, false);
  equal(clockBarDisabled.selectedItem, "", "disabling the clock bar closes its selected preview item");
  equal(clockBar.uiState(clockBarDisabled).badgeVisible, false, "disabled clock bar hides its status badge");

  const schedule = createScreenScheduleController({
    trigger: (value) => ["disabled", "time", "sensor"].includes(String(value)) ? String(value) : "disabled",
    sensorActivation: (value) => value === "on" ? "on" : "off",
    hour: (value, fallback) => Math.max(0, Math.min(23, Number.isFinite(Number(value)) ? Math.round(Number(value)) : fallback)),
    mode: (value) => ["screen_off", "screen_dimmed", "clock"].includes(String(value)) ? String(value) : "screen_off",
    wakeTimeout: (value) => Math.max(10, Number(value) || 10),
    wakeBrightness: (value) => Math.max(1, Math.min(100, Number(value) || 1)),
    dimmedBrightness: (value) => Math.max(1, Math.min(100, Number(value) || 1)),
    clockBrightness: (value) => Math.max(1, Math.min(100, Number(value) || 1)),
  });
  const scheduleInitial = {
    trigger: "time", sensorActivation: "off", onHour: 6, offHour: 23, mode: "screen_off",
    wakeTimeout: 30, wakeBrightness: 100, dimmedBrightness: 25, clockBrightness: 20,
  };
  equal(schedule.uiState(scheduleInitial).timeControlsVisible, true, "time schedules show their hour controls");
  equal(schedule.uiState(schedule.setTrigger(scheduleInitial, "sensor")).sensorControlsVisible, true,
        "sensor schedules show their sensor controls");
  equal(schedule.uiState(schedule.setMode(scheduleInitial, "clock")).clockOptionsVisible, true,
        "clock schedules show their clock options");
  equal(schedule.setOnHour(scheduleInitial, 28).onHour, 23, "schedule hours stay within a day");
}
