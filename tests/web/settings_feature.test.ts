import { powerModeControlState, screensaverControlState, timedSettingLabel } from "../../src/webserver/features/settings";
import { normalizeBackupScreenSettings, normalizePowerMode, normalizeScheduleWakeBrightness } from "../../src/webserver/model";

function equal<T>(actual: T, expected: T, message: string): void {
  if (actual !== expected) throw new Error(`${message}: expected ${String(expected)}, received ${String(actual)}`);
}

export function runSettingsFeatureTests(): void {
  equal(normalizePowerMode("battery-saver"), "Battery Saver", "battery power mode is normalized");
  equal(normalizePowerMode("unexpected"), "Normal", "unknown power modes safely default to Normal");
  equal(normalizeScheduleWakeBrightness(1), 1, "scheduled wake brightness accepts one percent");

  const unavailablePower = powerModeControlState("Battery Saver", false);
  equal(unavailablePower.disabled, true, "power mode is disabled on older firmware");
  equal(unavailablePower.mode, "Battery Saver", "disabled power mode retains the supplied backup value");
  const batteryPower = powerModeControlState("Battery Saver", true);
  equal(batteryPower.disabled, false, "power mode is enabled on current firmware");
  equal(batteryPower.description.includes("Display Off"), true, "battery mode recommends Display Off");

  const oldBackup = normalizeBackupScreenSettings({}, {
    scheduleWakeBrightness: 10,
    scheduleDimmedBrightness: 10,
    scheduleClockBrightness: 10,
    scheduleClockTextColor: "FFFFFF",
    scheduleSensorActivation: "off",
  });
  equal(oldBackup.powerMode, "Normal", "older backups default to Normal power mode");
  const batteryBackup = normalizeBackupScreenSettings({ power_mode: "Battery Saver" }, {
    scheduleWakeBrightness: 10,
    scheduleDimmedBrightness: 10,
    scheduleClockBrightness: 10,
    scheduleClockTextColor: "FFFFFF",
    scheduleSensorActivation: "off",
  });
  equal(batteryBackup.powerMode, "Battery Saver", "battery power mode survives backup normalization");

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
}
