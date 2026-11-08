"""Allowlisted bindings for the browser-compatible HTTP backup format.

Only configured settings are bound; Wi-Fi credentials, API keys and PINs are
never read. Names follow the same entity substitutions as the web editor.
"""

# (section, backup field, entity key, domain, default name)
BACKUP_FIELDS = (
    ('settings', 'indoor_temp_enable', 'indoor_temp_enable', 'switch', 'Indoor Temp Enable'),
    ('settings', 'outdoor_temp_enable', 'outdoor_temp_enable', 'switch', 'Outdoor Temp Enable'),
    ('settings', 'clock_bar', 'screen_clock_bar', 'switch', 'Screen: Clock Bar'),
    ('settings', 'clock_bar_time', 'screen_clock_bar_time', 'switch', 'Screen: Clock Bar Time'),
    ('settings', 'clock_bar_night_mode', 'screen_clock_bar_night_mode', 'switch', 'Screen: Clock Bar Night Mode Icon'),
    ('settings', 'clock_bar_temperature_entities', 'clock_bar_temperature_entities', 'text', 'Clock Bar: Temperature Entities'),
    ('settings', 'network_status_icon', 'screen_network_status_icon', 'switch', 'Screen: Network Status Icon'),
    ('settings', 'clock_bar_temperature_precision', 'clock_bar_temperature_precision', 'number', 'Clock Bar: Temperature Precision'),
    ('settings', 'temperature_degree_symbol', 'screen_temperature_degree_symbol', 'switch', 'Screen: Temperature Degree Symbol'),
    ('settings', 'subpage_chevron', 'screen_subpage_chevron', 'switch', 'Screen: Subpage Chevron'),
    ('settings', 'temperature_unit', 'screen_temperature_unit', 'select', 'Screen: Temperature Unit'),
    ('settings', 'indoor_temp_entity', 'indoor_temp_entity', 'text', 'Indoor Temp Entity'),
    ('settings', 'outdoor_temp_entity', 'outdoor_temp_entity', 'text', 'Outdoor Temp Entity'),
    ('settings', 'screensaver_mode', 'screensaver_mode', 'text', 'Screensaver Mode'),
    ('settings', 'screensaver_action', 'screen_saver_action', 'select', 'Screen Saver: Action'),
    ('settings', 'screensaver_camera_entity', 'screen_saver_camera_entity', 'text', 'Screen Saver: Camera Entity'),
    ('settings', 'screensaver_metadata_entity', 'screen_saver_metadata_entity', 'text', 'Screen Saver: Photo Metadata Entity'),
    ('settings', 'metadata_overlay', 'screen_saver_metadata_overlay', 'switch', 'Screen Saver: Metadata Overlay'),
    ('settings', 'screensaver_camera_image_mode', 'screen_saver_camera_image_mode', 'select', 'Screen Saver: Camera Image Mode'),
    ('settings', 'screensaver_camera_dimming', 'screen_saver_camera_dimming', 'number', 'Screen Saver: Camera Dimming'),
    ('settings', 'presence_sensor_entity', 'presence_sensor_entity', 'text', 'Presence Sensor Entity'),
    ('settings', 'cover_art_playback_control', 'screen_saver_cover_art_playback_control', 'switch', 'Screen Saver: Cover Art Playback Control'),
    ('settings', 'media_player_sleep_prevention', 'screen_saver_media_player_sleep_prevention', 'switch', 'Screen Saver: Media Player Sleep Prevention'),
    ('settings', 'media_player_sleep_prevention_entity', 'media_player_sleep_prevention_entity', 'text', 'Media Player Sleep Prevention Entity'),
    ('settings', 'cover_art_screensaver', 'screen_saver_cover_art', 'switch', 'Screen Saver: Cover Art'),
    ('settings', 'cover_art_media_player_entity', 'screen_saver_cover_art_entity', 'text', 'Screen Saver: Cover Art Entity'),
    ('settings', 'cover_art_secondary_media_player_entity', 'screen_saver_cover_art_secondary_entity', 'text', 'Screen Saver: External Source Media Entity'),
    ('settings', 'cover_art_attribute_conditions', 'screen_saver_cover_art_conditions', 'text', 'Screen Saver: Cover Art Conditions'),
    ('settings', 'cover_art_delay', 'screen_saver_cover_art_delay', 'number', 'Screen Saver: Cover Art Delay'),
    ('settings', 'cover_art_track_overlay_duration', 'screen_saver_track_overlay_duration', 'number', 'Screen Saver: Show Track Overlay'),
    ('settings', 'cover_art_hide_external_input', 'screen_saver_hide_cover_art_external_input', 'switch', 'Screen Saver: Hide for external sources'),
    ('settings', 'home_assistant_artwork_endpoint_mode', 'home_assistant_artwork_endpoint_mode', 'select', 'Home Assistant Artwork Connection'),
    ('settings', 'home_assistant_artwork_protocol', 'home_assistant_artwork_protocol', 'select', 'Home Assistant Artwork Protocol'),
    ('settings', 'home_assistant_artwork_port', 'home_assistant_artwork_port', 'number', 'Home Assistant Artwork Port'),
    ('settings', 'home_assistant_artwork_host', 'home_assistant_artwork_host', 'text', 'Home Assistant Artwork Host'),
    ('settings', 'clock_brightness', 'screen_saver_daytime_clock_brightness', 'number', 'Screen Saver: Daytime Clock Brightness'),
    ('settings', 'clock_brightness_day', 'screen_saver_daytime_clock_brightness', 'number', 'Screen Saver: Daytime Clock Brightness'),
    ('settings', 'clock_brightness_night', 'screen_saver_nighttime_clock_brightness', 'number', 'Screen Saver: Nighttime Clock Brightness'),
    ('settings', 'screensaver_dimmed_brightness', 'screen_saver_dimmed_brightness', 'number', 'Screen Saver: Dimmed Brightness'),
    ('settings', 'screensaver_dimmed_brightness_day', 'screen_saver_daytime_dimmed_brightness', 'number', 'Screen Saver: Daytime Dimmed Brightness'),
    ('settings', 'screensaver_dimmed_brightness_night', 'screen_saver_nighttime_dimmed_brightness', 'number', 'Screen Saver: Nighttime Dimmed Brightness'),
    ('settings', 'screensaver_timeout', 'screensaver_timeout', 'number', 'Screensaver Timeout'),
    ('settings', 'home_screen_timeout', 'home_screen_timeout', 'number', 'Home Screen Timeout'),
    ('settings', 'clock_screensaver', 'screen_saver_clock', 'switch', 'Screen Saver: Clock'),
    ('settings', 'clock_date', 'screen_saver_show_date', 'switch', 'Screen Saver: Show Date'),
    ('settings', 'clock_overlay', 'screen_saver_clock_overlay', 'switch', 'Screen Saver: Clock Overlay'),
    ('settings', 'timezone', 'screen_timezone', 'select', 'Screen: Timezone'),
    ('settings', 'language', 'screen_language', 'select', 'Screen: Language'),
    ('settings', 'clock_format', 'screen_clock_format', 'select', 'Screen: Clock Format'),
    ('settings', 'ntp_server_1', 'screen_ntp_server_1', 'text', 'Screen: NTP Server 1'),
    ('settings', 'ntp_server_2', 'screen_ntp_server_2', 'text', 'Screen: NTP Server 2'),
    ('settings', 'ntp_server_3', 'screen_ntp_server_3', 'text', 'Screen: NTP Server 3'),
    ('settings', 'firmware_auto_update', 'firmware_auto_update', 'switch', 'Firmware: Auto Update'),
    ('settings', 'firmware_update_frequency', 'firmware_update_frequency', 'select', 'Firmware: Update Frequency'),
    ('settings', 'screen_rotation', 'screen_rotation', 'select', 'Screen: Rotation'),
    ('screen', 'brightness_day', 'screen_daytime_brightness', 'number', 'Screen: Daytime Brightness'),
    ('screen', 'brightness_night', 'screen_nighttime_brightness', 'number', 'Screen: Nighttime Brightness'),
    ('screen', 'brightness_mode', 'screen_brightness_mode', 'select', 'Screen: Brightness Mode'),
    ('screen', 'theme_mode', 'screen_theme_mode', 'select', 'Screen: Theme Mode'),
    ('screen', 'card_curve', 'screen_card_curve', 'select', 'Screen: Card Curve'),
    ('screen', 'theme_auto_method', 'screen_theme_auto_method', 'select', 'Screen: Theme Auto Method'),
    ('screen', 'theme_light_start', 'screen_theme_light_start', 'text', 'Screen: Theme Light Start'),
    ('screen', 'theme_dark_start', 'screen_theme_dark_start', 'text', 'Screen: Theme Dark Start'),
    ('screen', 'theme_sunrise_offset', 'screen_theme_sunrise_offset', 'number', 'Screen: Theme Sunrise Offset'),
    ('screen', 'theme_sunset_offset', 'screen_theme_sunset_offset', 'number', 'Screen: Theme Sunset Offset'),
    ('screen', 'brightness_dawn_time', 'screen_brightness_dawn_time', 'text', 'Screen: Brightness Dawn Time'),
    ('screen', 'brightness_dusk_time', 'screen_brightness_dusk_time', 'text', 'Screen: Brightness Dusk Time'),
    ('screen', 'schedule_enabled', 'screen_schedule_enabled', 'switch', 'Screen: Schedule Enabled'),
    ('screen', 'schedule_trigger', 'screen_schedule_trigger', 'text', 'Screen: Schedule Trigger'),
    ('screen', 'schedule_sensor_activation', 'screen_schedule_sensor_activation', 'select', 'Screen: Schedule Sensor Activation'),
    ('screen', 'schedule_sensor_entity', 'screen_schedule_sensor_entity', 'text', 'Screen Schedule Sensor Entity'),
    ('screen', 'schedule_mode', 'screen_schedule_mode', 'select', 'Screen: Schedule Mode'),
    ('screen', 'schedule_on_hour', 'screen_schedule_on_hour', 'number', 'Screen: Schedule On Hour'),
    ('screen', 'schedule_off_hour', 'screen_schedule_off_hour', 'number', 'Screen: Schedule Off Hour'),
    ('screen', 'schedule_wake_timeout', 'screen_schedule_wake_timeout', 'number', 'Screen: Schedule Wake Timeout'),
    ('screen', 'schedule_wake_brightness', 'screen_schedule_wake_brightness', 'number', 'Screen: Schedule Wake Brightness'),
    ('screen', 'schedule_dimmed_brightness', 'screen_schedule_dimmed_brightness', 'number', 'Screen: Schedule Dimmed Brightness'),
    ('screen', 'schedule_clock_brightness', 'screen_schedule_clock_brightness', 'number', 'Screen: Schedule Clock Brightness'),
    ('screen', 'schedule_clock_text_color', 'screen_schedule_clock_text_color', 'text', 'Screen: Schedule Clock Text Color'),
    ('screen', 'manual_brightness', 'display_backlight', 'light', 'Display Backlight'),
)


def configured_backup_fields(config):
    substitutions = config.get("substitutions", {})
    for section, field, key, domain, default_name in BACKUP_FIELDS:
        name = substitutions.get("entity_" + key, default_name)
        for entity in config.get(domain, []):
            if entity.get("name") == name:
                yield section, field, domain, entity["id"]
                break


async def register_backup(panel_config, cg, core_config):
    cg.add_global(cg.RawStatement('#include "esphome/components/espcontrol/panel_backup_endpoint.h"'), prepend=True)
    statements = []
    for section, field, domain, entity_id in configured_backup_fields(core_config):
        entity = await cg.get_variable(entity_id)
        value = f"{entity}->state"
        if domain == "select":
            value = f"{entity}->current_option().c_str()"
        elif domain == "light":
            value = f"std::max(1L, std::lround({entity}->remote_values.get_brightness() * 100.0f))"
        assignment = f'{section}["{field}"] = {value};'
        # Configured text entities may have an empty state without has_state().
        # Export the empty string so restoring clears an old destination value.
        if domain not in ("light", "text"):
            assignment = f"if ({entity}->has_state()) {{ {assignment} }}"
        statements.append(assignment)
    writer = cg.RawExpression("[=](JsonObject settings, JsonObject screen) { " + " ".join(statements) + " }")
    cg.add(cg.global_ns.namespace("espcontrol").namespace("backup").configure(
        panel_config["device_profile"], len(panel_config["buttons"]), writer))
