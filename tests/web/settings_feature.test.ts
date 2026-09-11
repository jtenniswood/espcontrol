import { screensaverControlState, timedSettingLabel } from "../../src/webserver/features/settings";
import { createAlarmDelayAudioController } from "../../src/webserver/features/alarm_delay_audio_controller";
import { createScreensaverController } from "../../src/webserver/features/screensaver_controller";
import { createCoverArtScreensaverController } from "../../src/webserver/features/cover_art_screensaver_controller";
import { createMediaPlaybackController } from "../../src/webserver/features/media_playback_controller";
import { createVoiceServicesController } from "../../src/webserver/features/voice_services_controller";
import { createClockBarController } from "../../src/webserver/features/clock_bar_controller";
import { createScreenScheduleController } from "../../src/webserver/features/screen_schedule_controller";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runSettingsFeatureTests(): void {
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
