import * as DeviceConfig from "./device_config";
import * as Model from "./model";
import * as DeviceApi from "./api/device_api";
import * as RequestFailure from "./api/request_failure";
import * as PreviewGridFeature from "./features/preview_grid";
import * as PreviewFeature from "./features/preview";
import * as BackupFeature from "./features/backup";
import * as SettingsFeature from "./features/settings";
import * as ClipboardFeature from "./features/clipboard";
import * as UiTokens from "./state/ui_tokens";
import * as AppState from "./state/app_state";
import * as AppInstance from "./state/app_instance";
import * as EventAliases from "./state/event_aliases";
import * as EventState from "./state/event_state";
import * as FirmwareEvents from "./state/firmware_events";
import * as ConfigPrimitives from "./model/config_primitives";
import * as CardContract from "./generated/card_contract";
import * as Icons from "./generated/icons";
import { ENTITY_CATALOG } from "./generated/entity_catalog";
import { installGlobals, installStaticGlobals } from "./runtime/globals";
import { installCore } from "./application/core";
import { installFirmwareMetadataModule } from "./application/firmware_metadata";
import { installStylesModule } from "./application/styles";
import { installStateModule } from "./application/state";
import { installLanguageStateModule } from "./application/language_state";
import { installEnvironmentStateModule } from "./application/environment_state";
import { installScreenRotationStateModule } from "./application/screen_rotation_state";
import { installScreenScheduleStateModule } from "./application/screen_schedule_state";
import { installNtpStateModule } from "./application/ntp_state";
import { installAppearanceStateModule } from "./application/appearance_state";
import { installIdleStateModule } from "./application/idle_state";
import { installArtworkStateModule } from "./application/artwork_state";
import { installScreensaverStateModule } from "./application/screensaver_state";
import { installFirmwareVersionStateModule } from "./application/firmware_version_state";
import { installEntityStateModule } from "./application/entity_state";
import { installClockBarStateModule } from "./application/clock_bar_state";
import { installFirmwareUpdateStateModule } from "./application/firmware_update_state";
import { installScreensaverTimeoutModule } from "./application/screensaver_timeout";
import { installC6FirmwareUiModule } from "./application/c6_firmware_ui";
import { installGridModule } from "./application/grid";
import { installApiModule } from "./application/api";
import { installFirmwareUpdatePostApiModule } from "./application/firmware_update_post_api";
import { installPublicFirmwareInstallModule } from "./application/public_firmware_install";
import { installConfigOptionCoreModule } from "./application/config_option_core";
import { installConfigMediaOptionsModule } from "./application/config_media_options";
import { installConfigImageOptionsModule } from "./application/config_image_options";
import { installConfigModalTabOptionsModule } from "./application/config_modal_tab_options";
import { installConfigSubpageOptionsModule } from "./application/config_subpage_options";
import { installConfigSensorOptionsModule } from "./application/config_sensor_options";
import { installConfigConfirmationOptionsModule } from "./application/config_confirmation_options";
import { installConfigAccessClimateAlarmOptionsModule } from "./application/config_access_climate_alarm_options";
import { installConfigCodecModule } from "./application/config_codec";
import { installConfigPostApiModule } from "./application/config_post_api";
import { installStateLoaderApiModule } from "./application/state_loader_api";
import { installArtworkPostApiModule } from "./application/artwork_post_api";
import { installScreenSchedulePostApiModule } from "./application/screen_schedule_post_api";
import { installClockBarPostApiModule } from "./application/clock_bar_post_api";
import { installControlsModule } from "./application/controls";
import { installControlsShellModule } from "./application/controls_shell";
import { installSettingsPageHelpersModule } from "./application/settings_page_helpers";
import { installSettingsScheduleSectionModule } from "./application/settings_schedule_section";
import { installSettingsCoverArtSectionModule } from "./application/settings_cover_art_section";
import { installSettingsSystemSectionModule } from "./application/settings_system_section";
import { installSettingsPageModule } from "./application/settings_page";
import { installControlsFieldsModule } from "./application/controls_fields";
import { installPreviewRenderModule } from "./application/preview_render";
import { installButtonSettingsSelectionModule } from "./application/button_settings_selection";
import { installButtonSettingsRenderQueueModule } from "./application/button_settings_render_queue";
import { installButtonSettingsIconPickerModule } from "./application/button_settings_icon_picker";
import { installButtonSettingsModule } from "./application/button_settings";
import { installPreviewGridPlacementModule } from "./application/preview_grid_placement";
import { installPreviewContextMenuModule } from "./application/preview_context_menu";
import { installPreviewClipboardModule } from "./application/preview_clipboard";
import { installPreviewInteractionsModule } from "./application/preview_interactions";
import { installBackupContractModule } from "./application/backup_contract";
import { installAppBackupModule } from "./application/app_backup";
import { installAppStatusPreviewModule } from "./application/app_status_preview";
import { installAppTitleModule } from "./application/app_title";
import { installAppConfigEventsModule } from "./application/app_config_events";
import { installAppStateEventHandlersModule } from "./application/app_state_event_handlers";
import { installAppEventsModule } from "./application/app_events";
import { installAppModule } from "./application/app";
import { installAppStartModule } from "./application/app_start";
import { registerActionCardTypes } from "./cards/action";
import { registerAlarmCardTypes } from "./cards/alarm";
import { registerCalendarCardTypes } from "./cards/calendar";
import { registerClimateCardTypes } from "./cards/climate";
import { registerClockCardTypes } from "./cards/clock";
import { registerCoverLikeCardHelpers } from "./cards/cover_like_card";
import { registerDoorWindowCardTypes } from "./cards/door_window";
import { registerEntityModeCardHelpers } from "./cards/entity_mode_card";
import { registerFanCardTypes } from "./cards/fan";
import { registerGarageCardTypes } from "./cards/garage";
import { registerGateCardTypes } from "./cards/gate";
import { registerImageCardTypes } from "./cards/image";
import { registerInternalCardTypes } from "./cards/internal";
import { registerLawnMowerCardTypes } from "./cards/lawn_mower";
import { registerLightTemperatureCardTypes } from "./cards/light_temperature";
import { registerLockCardTypes } from "./cards/lock";
import { registerMediaCardTypes } from "./cards/media";
import { registerPresenceCardTypes } from "./cards/presence";
import { registerPushCardTypes } from "./cards/push";
import { registerScreenLockCardTypes } from "./cards/screen_lock";
import { registerSensorCardTypes } from "./cards/sensor";
import { registerSliderCardTypes } from "./cards/slider";
import { registerSubpageCardTypes } from "./cards/subpage";
import { registerSwitchCardTypes } from "./cards/switch";
import { registerTimezoneCardTypes } from "./cards/timezone";
import { registerVacuumCardTypes } from "./cards/vacuum";
import { registerWeatherCardTypes } from "./cards/weather";
import { registerWeatherForecastCardTypes } from "./cards/weather_forecast";
import { registerWebhookCardTypes } from "./cards/webhook";
import { installAppTestHooks } from "./testing/app_test_hooks";
import { installAppTestHooksConfig } from "./testing/app_test_hooks_config";
import { installAppTestHooksPreview } from "./testing/app_test_hooks_preview";
import { installAppTestHooksBackup } from "./testing/app_test_hooks_backup";
import { installAppTestHooksSettings } from "./testing/app_test_hooks_settings";

declare const __ESPCONTROL_TEST_HOOKS_ENABLED__: boolean;

function startEspControl(): void {
  AppInstance.initializeAppState();
  installStaticGlobals({
    ...DeviceConfig,
    EspControlModel: Model,
    ...Model,
    ...DeviceApi,
    ...RequestFailure,
    PreviewGridFeature,
    PreviewFeature,
    ClipboardFeature,
    createBackupFeature: BackupFeature.createBackupFeature,
    createSettingsUiFeature: SettingsFeature.createSettingsUiFeature,
    screensaverControlState: SettingsFeature.screensaverControlState,
    timedSettingLabel: SettingsFeature.timedSettingLabel,
    ...UiTokens,
    ...AppState,
    ...AppInstance,
    ...EventAliases,
    ...EventState,
    ...FirmwareEvents,
    ...ConfigPrimitives,
    ...CardContract,
    ...Icons,
    ENTITY_CATALOG,
    defaultTimezoneOptions: () =>
      AppState.defaultTimezoneOptionsForDevice(DeviceConfig.deviceConfig),
  });

  installGlobals(installCore());
  installGlobals(installFirmwareMetadataModule());
  installGlobals(installStylesModule());
  installGlobals(installStateModule());
  installGlobals(installLanguageStateModule());
  installGlobals(installEnvironmentStateModule());
  installGlobals(installScreenRotationStateModule());
  installGlobals(installScreenScheduleStateModule());
  installGlobals(installNtpStateModule());
  installGlobals(installAppearanceStateModule());
  installGlobals(installIdleStateModule());
  installGlobals(installArtworkStateModule());
  installGlobals(installScreensaverStateModule());
  installGlobals(installFirmwareVersionStateModule());
  installGlobals(installEntityStateModule());
  installGlobals(installClockBarStateModule());
  installGlobals(installFirmwareUpdateStateModule());
  installGlobals(installScreensaverTimeoutModule());
  installGlobals(installC6FirmwareUiModule());
  installGlobals(installGridModule());
  installGlobals(installApiModule());
  installGlobals(installFirmwareUpdatePostApiModule());
  installGlobals(installPublicFirmwareInstallModule());
  installGlobals(installConfigOptionCoreModule());
  installGlobals(installConfigMediaOptionsModule());
  installGlobals(installConfigImageOptionsModule());
  installGlobals(installConfigModalTabOptionsModule());
  installGlobals(installConfigSubpageOptionsModule());
  installGlobals(installConfigSensorOptionsModule());
  installGlobals(installConfigConfirmationOptionsModule());
  installGlobals(installConfigAccessClimateAlarmOptionsModule());
  installGlobals(installConfigCodecModule());
  installGlobals(installConfigPostApiModule());
  installGlobals(installStateLoaderApiModule());
  installGlobals(installArtworkPostApiModule());
  installGlobals(installScreenSchedulePostApiModule());
  installGlobals(installClockBarPostApiModule());
  installGlobals(installControlsModule());
  installGlobals(installControlsShellModule());
  installGlobals(installSettingsPageHelpersModule());
  installGlobals(installSettingsScheduleSectionModule());
  installGlobals(installSettingsCoverArtSectionModule());
  installGlobals(installSettingsSystemSectionModule());
  installGlobals(installSettingsPageModule());
  installGlobals(installControlsFieldsModule());
  installGlobals(installPreviewRenderModule());
  installGlobals(installButtonSettingsSelectionModule());
  installGlobals(installButtonSettingsRenderQueueModule());
  installGlobals(installButtonSettingsIconPickerModule());
  installGlobals(installButtonSettingsModule());
  installGlobals(installPreviewGridPlacementModule());
  installGlobals(installPreviewContextMenuModule());
  installGlobals(installPreviewClipboardModule());
  installGlobals(installPreviewInteractionsModule());
  installGlobals(installBackupContractModule());
  installGlobals(installAppBackupModule());
  installGlobals(installAppStatusPreviewModule());
  installGlobals(installAppTitleModule());
  installGlobals(installAppConfigEventsModule());
  installGlobals(installAppStateEventHandlersModule());
  installGlobals(installAppEventsModule());
  installGlobals(installAppModule());

  // Card registration order is explicit and intentionally matches the established picker/runtime order.
  installGlobals(registerActionCardTypes());
  installGlobals(registerAlarmCardTypes());
  installGlobals(registerCalendarCardTypes());
  installGlobals(registerClimateCardTypes());
  installGlobals(registerClockCardTypes());
  installGlobals(registerCoverLikeCardHelpers());
  installGlobals(registerDoorWindowCardTypes());
  installGlobals(registerEntityModeCardHelpers());
  installGlobals(registerFanCardTypes());
  installGlobals(registerGarageCardTypes());
  installGlobals(registerGateCardTypes());
  installGlobals(registerImageCardTypes());
  installGlobals(registerInternalCardTypes());
  installGlobals(registerLawnMowerCardTypes());
  installGlobals(registerLightTemperatureCardTypes());
  installGlobals(registerLockCardTypes());
  installGlobals(registerMediaCardTypes());
  installGlobals(registerPresenceCardTypes());
  installGlobals(registerPushCardTypes());
  installGlobals(registerScreenLockCardTypes());
  installGlobals(registerSensorCardTypes());
  installGlobals(registerSliderCardTypes());
  installGlobals(registerSubpageCardTypes());
  installGlobals(registerSwitchCardTypes());
  installGlobals(registerTimezoneCardTypes());
  installGlobals(registerVacuumCardTypes());
  installGlobals(registerWeatherCardTypes());
  installGlobals(registerWeatherForecastCardTypes());
  installGlobals(registerWebhookCardTypes());

  if (__ESPCONTROL_TEST_HOOKS_ENABLED__) {
    installGlobals(installAppTestHooks());
    installGlobals(installAppTestHooksConfig());
    installGlobals(installAppTestHooksPreview());
    installGlobals(installAppTestHooksBackup());
    installGlobals(installAppTestHooksSettings());
  }

  installGlobals(installAppStartModule());
}

const deviceConfigReady = DeviceConfig.initializeDeviceConfig();
if (deviceConfigReady) {
  void deviceConfigReady.then(startEspControl).catch((error) => {
    console.error("Unable to start EspControl", error);
  });
} else {
  startEspControl();
}
