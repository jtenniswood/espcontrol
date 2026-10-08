"""Guard the runtime theme ownership boundary in authored firmware sources."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
DEVICE = ROOT / "common" / "device"

for name, page in (
    ("screen_loading.yaml", "loading_page"),
    ("screen_loading_ethernet.yaml", "loading_page"),
    ("screen_wifi_setup.yaml", "wifi_setup_page"),
    ("screen_ha_setup.yaml", "ha_setup_page"),
    ("screen_ha_actions.yaml", "ha_actions_page"),
    ("screen_ethernet_setup.yaml", "ethernet_setup_page"),
    ("screen_button_setup.yaml", "button_setup_page"),
):
    source = (DEVICE / name).read_text(encoding="utf-8")
    assert f"register_theme_static_page(id({page})->obj)" in source, name

clock = (DEVICE / "screen_clock.yaml").read_text(encoding="utf-8")
assert "register_theme_static_page" not in clock
assert "bg_color: 0x000000" in clock
assert not re.search(r"\$theme_\w+", clock)

cover_art = (DEVICE / "screen_cover_art.yaml").read_text(encoding="utf-8")
assert not re.search(r"register_theme_|apply_current_theme|current_theme\(\)", cover_art)
assert "${cover_art_text_color}" in cover_art
assert "espcontrol::cover_art::playback_icon_color(" in cover_art
assert "lv_obj_set_style_text_color(id(cover_art_playback_icon), lv_color_hex(icon_color)" in cover_art
assert not re.search(r"\$theme_\w+", cover_art)
assert "bg_color: 0x313131" in cover_art
assert "text_color: 0xFFFFFF" in cover_art

qr = (ROOT / "components" / "espcontrol" / "button_grid_wifi_qr.h").read_text(encoding="utf-8")
assert "lv_qrcode_set_dark_color(ui.qr, lv_color_black())" in qr
assert "lv_qrcode_set_light_color(ui.qr, lv_color_white())" in qr

button_theme = (ROOT / "common" / "theme" / "button.yaml").read_text(encoding="utf-8")
assert re.search(r"checked:\s+bg_color: \$button_on_color\s+(?:#.*\n\s+)?text_color: 0xFFFFFF", button_theme)

media_driver = (ROOT / "components" / "espcontrol" / "button_grid_media_driver.h").read_text(encoding="utf-8")
grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
assert 'media_card_mode(config.sensor) != "cover_art"' in media_driver
assert "media_driver_theme_owned_surface(context, p)" in grid
assert "media_driver_theme_owned_surface(context, sb_cfg)" in grid

# Both successful phase-2 exits refresh only after the main runtimes and any
# replacement subpages exist; do not move this back into phase-1 registration.
phase2 = grid[grid.index("inline void grid_phase2("):grid.index("// Secondary-page definitions")]
assert phase2.count("refresh_theme_grid_after_rebuild();") == 2
info_exit = phase2[phase2.index("if (cfg.info_only)"):phase2.index("// --- Subpage creation ---")]
assert info_exit.index("refresh_theme_grid_after_rebuild();") < info_exit.index("grid_phase2_complete_state() = true;")
assert phase2.rindex("refresh_weather_forecast_cards();") < phase2.rindex("refresh_theme_grid_after_rebuild();")
assert phase2.rindex("refresh_theme_grid_after_rebuild();") < phase2.rindex("grid_phase2_complete_state() = true;")

firmware = ROOT / "components" / "espcontrol"
runtime = (firmware / "theme_runtime_ui.h").read_text(encoding="utf-8")
registration = runtime[runtime.index("inline void register_theme_grid("):runtime.index("inline void refresh_theme_grid_after_rebuild()")]
assert "apply_current_theme();" not in registration
modal = (firmware / "button_grid_modal.h").read_text(encoding="utf-8")
network = (firmware / "network_status.h").read_text(encoding="utf-8")
climate = (firmware / "button_grid_climate.h").read_text(encoding="utf-8")
alarm = (firmware / "button_grid_alarm.h").read_text(encoding="utf-8")
for marker in ("register_theme_grid", "register_theme_hud", "navigation_subpages()",
               "theme_restyle_tree(entry.back_button", "theme_restyle_tree(card.button",
               "CARD_ACCENT_TEXT_COLOR"):
    assert marker in runtime, marker
for marker in ("control_modal_track_theme_tab", "control_modal_track_theme_pressed",
               "control_modal_track_theme_disabled", "control_modal_theme_child_deleted",
               "theme_restyle_tree(targets.panel"):
    assert marker in modal, marker
for marker in ("register_theme_refresh(ui.overlay, network_status_apply_theme",
               "unregister_theme_refresh(lv_event_get_target(event))",
               "theme_display_color(theme.surface_card)", "theme.text_primary"):
    assert marker in network, marker
assert "control_modal_track_theme_disabled(btn)" in climate
assert "alarm_theme_off_color(ctx)" in alarm
assert "current_theme().surface_primary" in alarm

print("Theme refresh ownership checks passed.")
