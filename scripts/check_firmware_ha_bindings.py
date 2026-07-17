#!/usr/bin/env python3
"""Guard firmware Home Assistant access behind button_grid_ha.h helpers."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_DIR = ROOT / "components" / "espcontrol"
CORE_INFRA_PATH = ROOT / "common" / "device" / "core_infra.yaml"
API_NAVIGATE_PATH = ROOT / "common" / "device" / "api_navigate.yaml"
C6_FIRMWARE_UPDATE_PATH = ROOT / "common" / "device" / "esp32_c6_firmware_update.yaml"
COVER_ART_PATH = ROOT / "common" / "device" / "screen_cover_art.yaml"
SCREEN_CLOCK_PATH = ROOT / "common" / "device" / "screen_clock.yaml"
ARTWORK_IMAGE_PATH = ROOT / "components" / "artwork_image" / "artwork_image.cpp"
BACKLIGHT_PATH = ROOT / "common" / "addon" / "backlight.yaml"
DISPLAY_CONFIG_PATH = ROOT / "common" / "config" / "display.yaml"
TIME_ADDON_PATH = ROOT / "common" / "addon" / "time.yaml"
SUN_CALC_PATH = ROOT / "components" / "espcontrol" / "sun_calc.h"
S3_DEVICE_PATH = ROOT / "devices" / "guition-esp32-s3-4848s040" / "device" / "device.yaml"
S3_PACKAGES_PATH = ROOT / "devices" / "guition-esp32-s3-4848s040" / "packages.yaml"
DEVICE_DEVICE_PATHS = tuple(sorted((ROOT / "devices").glob("*/device/device.yaml")))
DEVICE_SENSOR_PATHS = tuple(sorted((ROOT / "devices").glob("*/device/sensors.yaml")))
DEVICE_PACKAGE_PATHS = tuple(sorted((ROOT / "devices").glob("*/packages.yaml")))
DEVICE_TOUCH_PATHS = tuple(sorted((ROOT / "devices").glob("*/device/*.yaml")))
CONNECTIVITY_PATHS = (
    ROOT / "common" / "addon" / "connectivity.yaml",
    ROOT / "common" / "addon" / "connectivity_deployed.yaml",
    ROOT / "common" / "addon" / "connectivity_ethernet.yaml",
)
DISPLAY_LIFECYCLE_ROOTS = (
    ROOT / "common",
    ROOT / "devices",
    ROOT / "components" / "espcontrol",
)
REMOVED_DISPLAY_LIFECYCLE_SYMBOLS = (
    "display_asleep",
    "screen_schedule_asleep",
    "backlight_manual_off",
    "screensaver_display_off_active",
    "screensaver_dimmed_active",
    "is_clock_showing",
    "cover_art_screensaver_active",
    "setup_screen_dimmed_shadow_active",
    "display_mode_shadow_observer",
    "LegacyDisplayState",
    "LegacyModeResult",
    "DisplayShadowObservation",
    "DisplayModeShadowObserver",
)


def firmware_weather_header_path(firmware_dir: Path) -> Path:
    weather_path = firmware_dir / "button_grid_weather_forecast.h"
    return weather_path if weather_path.exists() else firmware_dir / "button_grid_config.h"


def firmware_display_controller_ownership_errors(
    roots: tuple[Path, ...], root: Path
) -> list[str]:
    errors: list[str] = []
    for scan_root in roots:
        if not scan_root.exists():
            continue
        for path in scan_root.rglob("*"):
            if path.suffix not in {".yaml", ".h", ".cpp"}:
                continue
            text = path.read_text(encoding="utf-8")
            removed = [symbol for symbol in REMOVED_DISPLAY_LIFECYCLE_SYMBOLS if symbol in text]
            if removed:
                errors.append(
                    f"{path.relative_to(root)}: use display_mode_controller ownership; "
                    f"removed compatibility state remains ({', '.join(removed)})"
                )
    return errors


def package_api_navigate_enabled(package_path: Path, root: Path) -> bool:
    manifest_path = root / "devices" / "manifest.json"
    if not manifest_path.exists():
        return True
    try:
        slug = package_path.relative_to(root / "devices").parts[0]
    except (ValueError, IndexError):
        return True
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return True
    device = manifest.get("devices", {}).get(slug, {})
    package = device.get("firmware", {}).get("package", {})
    return bool(package.get("apiNavigateAction", True))


def package_local_voice_services_enabled(package_path: Path, root: Path) -> bool:
    manifest_path = root / "devices" / "manifest.json"
    if not manifest_path.exists():
        return False
    try:
        slug = package_path.relative_to(root / "devices").parts[0]
    except (ValueError, IndexError):
        return False
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return False
    device = manifest.get("devices", {}).get(slug, {})
    package = device.get("firmware", {}).get("package", {})
    return bool(package.get("localVoiceServices"))


HA_BOUNDARY_ALLOWLIST = {
    "button_grid_ha.h",
}
DIRECT_HA_PATTERNS = (
    (re.compile(r"\bglobal_api_server\b"), "access Home Assistant API through button_grid_ha.h helpers"),
    (re.compile(r"(?:->|\.)send_homeassistant_action\s*\("), "send Home Assistant actions through button_grid_ha.h helpers"),
    (re.compile(r"(?:->|\.)subscribe_home_assistant_state\s*\("), "subscribe to Home Assistant state through button_grid_ha.h helpers"),
    (re.compile(r"(?:->|\.)get_home_assistant_state\s*\("), "get Home Assistant state through button_grid_ha.h helpers"),
    (re.compile(r"(?:->|\.)register_action_response_callback\s*\("), "register action callbacks through button_grid_ha.h helpers"),
    (re.compile(r"(?:->|\.)handle_action_response\s*\("), "cancel action callbacks through button_grid_ha.h helpers"),
)
STATE_HELPER_PATTERN = re.compile(
    r"inline\s+bool\s+ha_subscribe_state\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
ATTRIBUTE_HELPER_PATTERN = re.compile(
    r"inline\s+bool\s+ha_subscribe_attribute\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
TODO_GET_ITEMS_HELPER_PATTERN = re.compile(
    r"inline\s+bool\s+todo_begin_get_items_request\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
WEATHER_FORECAST_REQUEST_PATTERN = re.compile(
    r"inline\s+void\s+request_weather_forecast_entity\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
MEDIA_CONTROL_STATE_PATTERN = re.compile(
    r"inline\s+void\s+subscribe_media_control_state\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}\n\ninline\s+bool\s+media_seek_pending_active",
    re.DOTALL,
)
COVER_COMMAND_REQUEST_PATTERN = re.compile(
    r"inline\s+void\s+send_cover_command_action\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    re.DOTALL,
)
YAML_SCRIPT_PATTERN_TEMPLATE = r"(?ms)^  - id: {script_id}\n(?P<body>.*?)(?=^  - id: |\Z)"


def yaml_script_body(text: str, script_id: str) -> str | None:
    match = re.search(YAML_SCRIPT_PATTERN_TEMPLATE.format(script_id=re.escape(script_id)), text)
    return match.group("body") if match else None


def firmware_ha_binding_errors(firmware_dir: Path, root: Path) -> list[str]:
    errors: list[str] = []
    for path in sorted(firmware_dir.glob("button_grid*.h")):
        if path.name in HA_BOUNDARY_ALLOWLIST:
            continue
        for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            for pattern, message in DIRECT_HA_PATTERNS:
                if pattern.search(line):
                    rel = path.relative_to(root)
                    errors.append(f"{rel}:{line_no}: {message}")
                    break
    return errors


def firmware_ha_boundary_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_ha.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    coordinator_path = firmware_dir / "ha_read_coordinator.h"
    coordinator_text = coordinator_path.read_text(encoding="utf-8") if coordinator_path.exists() else ""
    read_boundary_text = text + "\n" + coordinator_text
    errors: list[str] = []

    state_helper = STATE_HELPER_PATTERN.search(text)
    if not state_helper:
        errors.append(f"{rel}: missing ha_subscribe_state helper")
    elif "heap_available" in state_helper.group("body"):
        errors.append(f"{rel}: keep core HA state subscriptions off the low-heap guard")

    retry_symbols = (
        "HaUnavailableStateRetryRef",
        "ha_unavailable_state_retry_refs",
        "ha_note_state_retry_result",
        "ha_retry_unavailable_states",
        "ha_reset_unavailable_state_retries",
        "HA_UNAVAILABLE_STATE_RETRY",
    )
    if any(symbol in text for symbol in retry_symbols):
        errors.append(f"{rel}: do not reintroduce unavailable HA state retry polling")

    attribute_helper = ATTRIBUTE_HELPER_PATTERN.search(text)
    if not attribute_helper:
        errors.append(f"{rel}: missing ha_subscribe_attribute helper")
    elif "heap_available" in attribute_helper.group("body"):
        errors.append(f"{rel}: keep HA metadata attribute subscriptions off the low-heap guard")

    if "ha_cancel_action_response_callback" not in text or "handle_action_response" not in text:
        errors.append(f"{rel}: expose a helper to cancel stale HA action response callbacks")
    action_send_match = re.search(
        r"inline\s+bool\s+ha_action_send\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        re.DOTALL,
    )
    if not action_send_match:
        errors.append(f"{rel}: missing ha_action_send helper")
    elif "ha_api_state_connected()" not in action_send_match.group("body"):
        errors.append(f"{rel}: send Home Assistant actions only after state subscription is ready")
    elif "HA_ACTION_INTERNAL_FREE_MIN_BYTES" not in action_send_match.group("body"):
        errors.append(f"{rel}: defer Home Assistant actions when S3 internal heap is critically low")
    if (
        "ha_read_coordinator().get(" not in text
        or "HA_READ_INTERNAL_FREE_MIN_BYTES" not in text
        or 'heap_probe_.available("Home Assistant state request"' not in coordinator_text
    ):
        errors.append(f"{rel}: defer one-off Home Assistant attribute reads when S3 internal heap is critically low")
    if "callback_depth_ != 0 || !state_connected()" not in coordinator_text:
        errors.append(f"{rel}: queue one-off Home Assistant reads until state subscription is ready")
    if (
        "request.callbacks.push_back(std::move(callback))" not in read_boundary_text
        or "request.entity_id == entity_id" not in read_boundary_text
        or "for (const auto &callback : *callback_refs)" not in read_boundary_text
    ):
        errors.append(f"{rel}: fan out duplicate deferred Home Assistant reads")
    if "subscriptions_.push_back({callback_ref, scope})" not in coordinator_text:
        errors.append(f"{rel}: track Home Assistant subscription callbacks for generation cleanup")
    if "release_subscriptions" not in coordinator_text or "*ref.callback = nullptr" not in coordinator_text:
        errors.append(f"{rel}: release retired Home Assistant subscription callback bodies")

    return errors


def firmware_unavailable_retry_errors(
    firmware_dir: Path,
    core_infra_path: Path,
    root: Path,
) -> list[str]:
    config_path = firmware_dir / "button_grid_config.h"
    errors: list[str] = []
    if config_path.exists():
        config_rel = config_path.relative_to(root)
        config_text = config_path.read_text(encoding="utf-8")
        bump_match = re.search(
            r"inline\s+void\s+bump_ha_subscription_generation\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            config_text,
            re.DOTALL,
        )
        if bump_match and "ha_reset_unavailable_state_retries" in bump_match.group("body"):
            errors.append(f"{config_rel}: do not reset removed unavailable HA state retries")
        if bump_match and "ha_reset_subscription_callbacks(HA_SUBSCRIPTION_SCOPE_DEFAULT)" not in bump_match.group("body"):
            errors.append(f"{config_rel}: release retired default Home Assistant subscription callbacks on generation bumps")
        if "ha_reset_unavailable_state_retries" in config_text:
            errors.append(f"{config_rel}: do not keep removed unavailable HA state retry helpers")

    if core_infra_path.exists():
        core_rel = core_infra_path.relative_to(root)
        core_text = core_infra_path.read_text(encoding="utf-8")
        if "ha_retry_unavailable_states" in core_text:
            errors.append(f"{core_rel}: do not retry unavailable HA states after reconnects or during maintenance")
    return errors


def firmware_todo_request_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_todo.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    helper = TODO_GET_ITEMS_HELPER_PATTERN.search(text)
    if not helper:
        errors.append(f"{rel}: missing todo_begin_get_items_request helper")
        return errors

    body = helper.group("body")
    if '"todo.get_items"' not in body:
        errors.append(f"{rel}: todo_begin_get_items_request must call todo.get_items")
    if "wants_response" not in body or "response_template" not in body:
        errors.append(f"{rel}: todo.get_items requests must capture a compact response template")
    if "std::string response_template" in body:
        errors.append(f"{rel}: keep the todo response template alive until after the request is sent")
    if "TODO_RESPONSE_KEY_MAX_LEN" not in text or "TODO_RESPONSE_SUMMARY_MAX_LEN" not in text:
        errors.append(f"{rel}: bound todo response text before Home Assistant sends it")
    if "std::to_string(TODO_RESPONSE_TEXT_MAX_LEN)" not in text or "|length" not in text:
        errors.append(f"{rel}: cap rendered todo responses before Home Assistant sends them")
    if 'ha_action_add_data(req, "status"' in body:
        errors.append(f"{rel}: filter todo items in the response template, not in action data")
    if "TODO_REQUEST_TIMEOUT_MS" not in text or text.count("todo_cancel_stale_request()") < 2:
        errors.append(f"{rel}: bound pending todo item requests with a timeout")
    if "stale_request_cancelled = todo_cancel_stale_request()" not in text:
        errors.append(f"{rel}: periodically expire stale todo requests while the modal is open")
    if 'todo_cancel_pending_request("modal closed"' not in text:
        errors.append(f"{rel}: cancel pending todo item requests when the modal closes")
    if 'todo_cancel_pending_request("modal closed", false)' not in text:
        errors.append(f"{rel}: close todo modals without retrying their cancelled request")
    if '"send failed"' in text and 'ui.waiting_for_ha = true;' not in text:
        errors.append(f"{rel}: retry todo loads when Home Assistant disconnects during send")
    pending_match = re.search(
        r"if\s*\(\s*todo_request_state\(\)\.call_id\s*!=\s*0\s*\)\s*\{(?P<body>.*?)\n\s*\}",
        text,
        re.DOTALL,
    )
    if not pending_match or "ui.waiting_for_ha = true;" not in pending_match.group("body"):
        errors.append(f"{rel}: retry todo loads when another todo request is already pending")
    if text.count("todo_clear_request_state(call_id)") < 2:
        errors.append(f"{rel}: clear pending todo request state when responses arrive")
    if "ha_api_state_connected()" not in text:
        errors.append(f"{rel}: wait for Home Assistant state subscription before todo actions")
    callback_sections = [text]
    lite_marker = "#elif defined(ESPCONTROL_TODO_LITE) && ESPCONTROL_TODO_LITE"
    full_marker = "#else\n\nconstexpr int TODO_MAX_ITEMS"
    if lite_marker in text and full_marker in text:
        before_lite, lite_and_full = text.split(lite_marker, 1)
        lite, full = lite_and_full.split(full_marker, 1)
        callback_sections = [before_lite, lite, full]
    if any(section.count("ha_register_action_response_callback(") > 1 for section in callback_sections):
        errors.append(f"{rel}: only todo list loading should register a response callback")
    return errors


def firmware_todo_disconnect_errors(firmware_dir: Path, core_infra_path: Path, root: Path) -> list[str]:
    todo_path = firmware_dir / "button_grid_todo.h"
    if not todo_path.exists() or not core_infra_path.exists():
        return []
    todo_rel = todo_path.relative_to(root)
    core_rel = core_infra_path.relative_to(root)
    todo_text = todo_path.read_text(encoding="utf-8")
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "todo_cancel_pending_request" not in todo_text:
        errors.append(f"{todo_rel}: expose a helper to cancel pending todo requests")
    if "todo_reload_active_modal" not in todo_text:
        errors.append(f"{todo_rel}: expose a helper to reload an open todo modal after HA reconnects")
    if "waiting_for_ha" not in todo_text or "todo_retry_waiting_modal" not in todo_text:
        errors.append(f"{todo_rel}: retry open todo modals that are waiting for Home Assistant")
    if "ctx->available) return" in todo_text:
        errors.append(f"{todo_rel}: allow todo modals to open while waiting for Home Assistant availability")
    if "apply_control_availability(ctx->btn, ctx->btn, ctx->available, false)" in todo_text:
        errors.append(f"{todo_rel}: do not dim or disable todo cards for unavailable entity states")
    if "on_client_disconnected:" not in core_text or "todo_cancel_pending_request" not in core_text:
        errors.append(f"{core_rel}: cancel pending todo requests when the HA API disconnects")
    if "on_client_connected:" not in core_text or "todo_reload_active_modal" not in core_text:
        errors.append(f"{core_rel}: retry open todo modals when the HA API reconnects")
    if "todo_retry_waiting_modal" not in core_text:
        errors.append(f"{core_rel}: periodically retry todo modals waiting for Home Assistant")
    return errors


def firmware_action_card_availability_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_grid.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    driver_path = firmware_dir / "button_grid_basic_action_driver.h"
    if driver_path.exists():
        driver_rel = driver_path.relative_to(root)
        driver_text = driver_path.read_text(encoding="utf-8")
        required = (
            "basic_action_driver_bind_action_state",
            "subscribe_action_card_display_state",
            "case Driver::PUSH:",
            '"Push %d"',
            'ha_action_add_data(request, "slot", slot_buffer)',
            '"esphome.push_button_pressed"',
        )
        for needle in required:
            if needle not in driver_text:
                errors.append(
                    f"{driver_rel}: preserve shared Action state and Trigger event behavior ({needle})"
                )
        if "register_ha_control_availability" in driver_text:
            errors.append(
                f"{driver_rel}: keep stateless Action and Trigger cards tappable while Home Assistant availability is pending"
            )
        return errors

    stateless_main_pattern = re.compile(
        r"std::string\s+state_entity\s*=\s*action_card_state_entity\(p\);"
        r"(?P<body>.*?)continue;",
        re.DOTALL,
    )
    stateless_subpage_pattern = re.compile(
        r"std::string\s+state_entity\s*=\s*action_card_state_entity\(sb_cfg\);"
        r"(?P<body>.*?)ParsedCfg\s+\*ctx\s*=",
        re.DOTALL,
    )
    push_main_pattern = re.compile(
        r"if\s*\(\s*p\.type\s*==\s*\"push\"\s*\)\s*\{"
        r"(?P<body>.*?)continue;",
        re.DOTALL,
    )
    push_subpage_pattern = re.compile(
        r"if\s*\(\s*sb_cfg\.type\s*==\s*\"push\"\s*\)\s*\{"
        r"(?P<body>.*?)continue;",
        re.DOTALL,
    )

    for match in stateless_main_pattern.finditer(text):
        body = match.group("body")
        if "else" in body and "register_ha_control_availability(s.btn, s.btn)" in body:
            errors.append(f"{rel}: keep stateless action cards tappable while Home Assistant availability is pending")
    for match in stateless_subpage_pattern.finditer(text):
        body = match.group("body")
        if "else" in body and "register_ha_control_availability(sub_slot.btn, sub_slot.btn)" in body:
            errors.append(f"{rel}: keep stateless subpage action cards tappable while Home Assistant availability is pending")
    for match in push_main_pattern.finditer(text):
        body = match.group("body")
        if "register_ha_control_availability(s.btn, s.btn)" in body:
            errors.append(f"{rel}: keep trigger cards tappable while Home Assistant availability is pending")
    for match in push_subpage_pattern.finditer(text):
        body = match.group("body")
        if "register_ha_control_availability(sb_btn, sb_btn)" in body:
            errors.append(f"{rel}: keep subpage trigger cards tappable while Home Assistant availability is pending")
    return errors


def firmware_card_disabled_state_errors(firmware_dir: Path, root: Path) -> list[str]:
    errors: list[str] = []
    config_path = firmware_dir / "button_grid_config.h"
    actions_path = firmware_dir / "button_grid_actions.h"
    grid_path = firmware_dir / "button_grid_grid.h"

    if config_path.exists():
        rel = config_path.relative_to(root)
        text = config_path.read_text(encoding="utf-8")
        if "apply_control_availability" in text:
            errors.append(f"{rel}: do not keep a generic helper that dims or disables card controls")

    for path in (actions_path, grid_path):
        if not path.exists():
            continue
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        if re.search(r"lv_obj_has_state\s*\([^;\n]*LV_STATE_DISABLED", text):
            errors.append(f"{rel}: card tap handlers must not ignore taps because a card is disabled")

    return errors


def firmware_media_card_availability_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_media.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "media_control_apply_availability(ctx->btn, ctx->btn" in text:
        errors.append(f"{rel}: do not dim or disable media cards for unavailable entity states")
    if "media_control_apply_availability(ui.panel, ui.panel" in text:
        errors.append(f"{rel}: do not dim the media control panel for unavailable entity states")

    return errors


def firmware_action_card_script_fields_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_actions.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "script_fields" not in text:
        return errors
    if "std::vector<ActionCardScriptField> script_fields" not in text:
        errors.append(f"{rel}: keep parsed script field strings alive until the action is sent")
    if "req.variables.init(script_fields.size())" not in text:
        errors.append(f"{rel}: initialize script field variables separately from service data")
    if "ha_action_add_variable(req, field.key.c_str(), field.value.c_str())" not in text:
        errors.append(f"{rel}: send script fields through Home Assistant action variables")
    if "req.data_template.init(1)" not in text or 'ha_action_add_data_template(req, "variables"' not in text:
        errors.append(f"{rel}: send script fields in the script.turn_on variables service payload")
    add_fields_match = re.search(
        r"inline\s+void\s+action_card_add_script_field_variables\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        re.DOTALL,
    )
    if add_fields_match and "ha_action_add_data(req, key.c_str(), value.c_str())" in add_fields_match.group("body"):
        errors.append(f"{rel}: do not send script fields as top-level script.turn_on service data")
    return errors


def firmware_local_sensor_binding_order_errors(firmware_dir: Path, root: Path) -> list[str]:
    grid_path = firmware_dir / "button_grid_grid.h"
    driver_path = firmware_dir / "button_grid_sensor_driver.h"
    if not grid_path.exists():
        return []
    grid_rel = grid_path.relative_to(root)
    grid_text = grid_path.read_text(encoding="utf-8")
    errors: list[str] = []

    if not driver_path.exists():
        errors.append(f"{grid_rel}: bind local sensor subtypes through the shared sensor driver")
        return errors

    driver_rel = driver_path.relative_to(root)
    driver_text = driver_path.read_text(encoding="utf-8")
    bind_match = re.search(
        r"inline\s+bool\s+sensor_driver_bind_data\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        driver_text,
        re.DOTALL,
    )
    if bind_match is None:
        errors.append(f"{driver_rel}: keep local sensor binding in sensor_driver_bind_data")
    else:
        bind_body = bind_match.group("body")
        local_start = bind_body.find("if (sensor_driver_is_local(config, context))")
        ha_text_start = bind_body.find("if (is_text_sensor_card(config))")
        ha_sensor_start = bind_body.find("if (!config.sensor.empty())")
        if local_start < 0 or "sensor_driver_register_local_value(slot, config)" not in bind_body:
            errors.append(f"{driver_rel}: bind local sensor values through the local registry")
        elif any(start >= 0 and local_start > start for start in (ha_text_start, ha_sensor_start)):
            errors.append(f"{driver_rel}: bind local sensor values before Home Assistant sensor subscriptions")

    if "sensor_driver_bind_data(" not in grid_text:
        errors.append(f"{grid_rel}: bind sensor cards through the shared sensor driver")

    for match in re.finditer(r"if\s*\(\s*bind_basic_sensor_card\s*\(", grid_text):
        bind_start = match.start()
        image_start = grid_text.rfind("image_driver_bind_main", 0, bind_start)
        line_no = grid_text.count("\n", 0, bind_start) + 1
        if image_start < 0:
            errors.append(f"{grid_rel}:{line_no}: bind image cards before basic sensor cards")

    return errors


def firmware_time_reconnect_errors(time_path: Path, root: Path) -> list[str]:
    if not time_path.exists():
        return []
    rel = time_path.relative_to(root)
    text = time_path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "id(homeassistant_time).update();" in text.replace(
        "if (ha_api_state_connected()) id(homeassistant_time).update();", ""
    ):
        errors.append(f"{rel}: guard Home Assistant time updates until state subscription is ready")
    if "on_client_connected:" in text and "ha_api_state_connected()" not in text:
        errors.append(f"{rel}: wait for Home Assistant state readiness before reconnect time sync")
    if "on_client_connected:" in text and "delay: 2s" not in text:
        errors.append(f"{rel}: defer Home Assistant time sync after API reconnect")
    api_connect_match = re.search(
        r"(?ms)^api:\n\s+on_client_connected:\n(?P<body>.*?)(?:^#|^select:|^text:|^text_sensor:|^time:|^script:|\Z)",
        text,
    )
    if api_connect_match:
        api_connect_body = api_connect_match.group("body")
        if "script.execute: backlight_recalc_sunrise_sunset" not in api_connect_body:
            errors.append(f"{rel}: recalculate sunrise and sunset after reconnect time sync")
        if "script.execute: screen_schedule_check" not in api_connect_body:
            errors.append(f"{rel}: recheck the screen schedule after reconnect time sync")
    return errors


def firmware_ntp_startup_errors(
    time_path: Path,
    sun_calc_path: Path,
    connectivity_paths: tuple[Path, ...],
    root: Path,
) -> list[str]:
    errors: list[str] = []
    if time_path.exists():
        rel = time_path.relative_to(root)
        text = time_path.read_text(encoding="utf-8")
        boot_section = text.split("# Ask Home Assistant", 1)[0]
        if "- script.execute: ntp_servers_apply" in boot_section:
            errors.append(f"{rel}: defer custom NTP server apply until networking has an IP address")
        if "# NTP server settings" in text:
            ntp_text_section = text.split("# NTP server settings", 1)[1].split("text_sensor:", 1)[0]
            if "- script.execute: ntp_servers_apply\n" in ntp_text_section:
                errors.append(f"{rel}: debounce restored NTP text values before applying SNTP settings")
            if "id: ntp_servers_apply_after_network" not in text:
                errors.append(f"{rel}: route NTP server changes through delayed network-ready apply script")
    if sun_calc_path.exists():
        rel = sun_calc_path.relative_to(root)
        text = sun_calc_path.read_text(encoding="utf-8")
        if "apply_ntp_servers" in text and "App.is_setup_complete()" not in text:
            errors.append(f"{rel}: skip SNTP reconfiguration until application setup has completed")
        if (
            "apply_ntp_servers" in text
            and "get_ip_addresses().empty()" not in text
        ):
            errors.append(f"{rel}: skip SNTP reconfiguration until networking has an IP address")
    for path in connectivity_paths:
        if not path.exists():
            continue
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        if "on_connect:" in text and "- script.execute: ntp_servers_apply_after_network" not in text:
            errors.append(f"{rel}: apply configured NTP servers after network connect")
    return errors


def firmware_weather_request_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_weather_header_path(firmware_dir)
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    request = WEATHER_FORECAST_REQUEST_PATTERN.search(text)
    if not request:
        errors.append(f"{rel}: missing request_weather_forecast_entity helper")
        return errors
    body = request.group("body")
    if "ha_api_state_connected()" not in body:
        errors.append(f"{rel}: wait for Home Assistant state subscription before automatic forecast requests")
    if "low internal heap" not in body:
        errors.append(f"{rel}: retry automatic forecast requests instead of sending during critically low internal heap")
    if "ha_cancel_action_response_callback(req.call_id" not in text:
        errors.append(f"{rel}: cancel forecast response callbacks when sends fail")
    if (
        'ha_cancel_action_response_callback(req.call_id, "send failed");' in body
        and 'weather_forecast_schedule_retry(entity_id, day, "send failed");' not in body
    ):
        errors.append(f"{rel}: retry weather forecast sends that fail after callback registration")
    if "WEATHER_FORECAST_PENDING_MAX" not in text or "weather_forecast_track_pending" not in text:
        errors.append(f"{rel}: bound pending forecast response callbacks")
    if "weather_forecast_cancel_pending_requests" not in text:
        errors.append(f"{rel}: expose a helper to cancel pending forecast callbacks")
    if (
        "uint32_t generation = ha_subscription_generation();" not in body
        or "generation != ha_subscription_generation()" not in body
    ):
        errors.append(f"{rel}: ignore stale forecast action responses after dashboard reconfiguration")
    if "WEATHER_FORECAST_RETRY_DELAY_MS" not in text or "weather_forecast_schedule_retry" not in text:
        errors.append(f"{rel}: retry failed weather forecast requests later")
    if (
        "response if response is defined and response is not none else {}" not in text
        or "'forecast' in response_data" not in text
        or "response_data[entity] if entity in response_data else {}" not in text
    ):
        errors.append(f"{rel}: accept both direct and entity-keyed Home Assistant forecast response shapes")
    if (
        "today_date = now().date()" not in text
        or "tomorrow_date = (now() + timedelta(days=1)).date()" not in text
        or "as_datetime(item['datetime'])" not in text
        or "as_local(item_dt).date()" not in text
        or "as_datetime(item['date']).date()" not in text
        or "ns.today if ns.today is not none else (forecasts[0]" not in text
        or "ns.tomorrow if ns.tomorrow is not none else (forecasts[1]" not in text
    ):
        errors.append(f"{rel}: select today/tomorrow weather forecasts by date/datetime before falling back to list order")
    if (
        "unit_keys = ['temperature_unit','native_temperature_unit','unit_of_measurement','native_unit_of_measurement','unit']" not in text
        or "key in entity_response" not in text
        or "state_attr(entity, 'temperature_unit')" not in text
        or "state_attr(entity, 'unit_of_measurement')" not in text
    ):
        errors.append(f"{rel}: preserve forecast temperature units from response data or weather entity attributes")
    if (
        "native_temperature" not in text
        or "native_templow" not in text
        or "native_temperature_unit" not in text
    ):
        errors.append(f"{rel}: accept native Home Assistant forecast temperature fields and units")
    if (
        "max_temperature" not in text
        or "temperature_max" not in text
        or "max_temp" not in text
        or "min_temperature" not in text
        or "temperature_min" not in text
        or "min_temp" not in text
    ):
        errors.append(f"{rel}: accept max/min weather forecast temperature field aliases")
    if (
        "unit_keys" not in text
        or "today is not none and key in today" not in text
        or "tomorrow is not none and key in tomorrow" not in text
    ):
        errors.append(f"{rel}: preserve forecast temperature units from individual forecast items")
    if "parse_weather_forecast_temp" in text and "std::isfinite(parsed)" not in text:
        errors.append(f"{rel}: reject non-finite weather forecast temperatures before rendering")
    if (
        "No usable forecast temperatures" in body
        and 'weather_forecast_schedule_retry(entity_id, day, "no usable forecast temperatures");' not in body
    ):
        errors.append(f"{rel}: retry weather forecasts when Home Assistant returns no usable temperatures")
    if (
        "weather_forecast_error_is_timeout" not in text
        or 'find("timed out")' not in text
        or "apply_weather_forecast_actions_required_for_entity" not in body
    ):
        errors.append(f"{rel}: detect Home Assistant forecast timeout errors robustly")
    return errors


def firmware_weather_disconnect_errors(firmware_dir: Path, core_infra_path: Path, root: Path) -> list[str]:
    config_path = firmware_weather_header_path(firmware_dir)
    if not config_path.exists() or not core_infra_path.exists():
        return []
    core_rel = core_infra_path.relative_to(root)
    config_text = config_path.read_text(encoding="utf-8")
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    if "weather_forecast_cancel_pending_requests" in config_text and (
        "on_client_disconnected:" not in core_text or "weather_forecast_cancel_pending_requests" not in core_text
    ):
        errors.append(f"{core_rel}: cancel pending forecast callbacks when the HA API disconnects")
    return errors


def firmware_weather_reconnect_errors(core_infra_path: Path, root: Path) -> list[str]:
    if not core_infra_path.exists():
        return []
    core_rel = core_infra_path.relative_to(root)
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    connected_match = re.search(
        r"(?ms)^  on_client_connected:\n(?P<body>.*?)(?:^  on_client_disconnected:|^interval:|\Z)",
        core_text,
    )
    if not connected_match:
        errors.append(f"{core_rel}: refresh weather forecasts after Home Assistant reconnects")
        return errors

    body = connected_match.group("body")
    for match in re.finditer(r"refresh_weather_forecast_cards\(\);", body):
        guard_window = body[max(0, match.start() - 160) : match.end()]
        if "ha_api_state_connected()" not in guard_window:
            errors.append(f"{core_rel}: wait for Home Assistant state readiness before forecast reconnect refreshes")
            break
    if "refresh_weather_forecast_cards();" in body and ("delay: 20s" not in body or "delay: 25s" not in body):
        errors.append(f"{core_rel}: retry weather forecast refresh after slow S3 reconnects")
    return errors


def firmware_cover_request_errors(firmware_dir: Path, core_infra_path: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_actions.h"
    if not path.exists() or not core_infra_path.exists():
        return []
    rel = path.relative_to(root)
    core_rel = core_infra_path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    core_text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []

    request = COVER_COMMAND_REQUEST_PATTERN.search(text)
    if not request:
        errors.append(f"{rel}: missing send_cover_command_action helper")
        return errors
    body = request.group("body")
    if "cover_stop_track_pending" not in text or "cover_stop_clear_pending" not in text:
        errors.append(f"{rel}: track pending cover stop callbacks")
    if "cover_stop_cancel_pending_request" not in text:
        errors.append(f"{rel}: expose a helper to cancel pending cover stop callbacks")
    if "cover_stop_tracked" not in body or "ha_cancel_action_response_callback(req.call_id" not in body:
        errors.append(f"{rel}: cancel cover stop callbacks when sends fail")
    if "on_client_disconnected:" not in core_text or "cover_stop_cancel_pending_request" not in core_text:
        errors.append(f"{core_rel}: cancel pending cover stop callbacks when the HA API disconnects")
    return errors


def firmware_cover_art_external_input_errors(path: Path, root: Path) -> list[str]:
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "cover_art_hide_external_input_enabled" not in text:
        errors.append(f"{rel}: expose a cover art external-input hide setting")
    if ('std::string("source")' not in text or
            "handle_media_source" not in text or
            "HA_SUBSCRIPTION_SCOPE_COVER_ART" not in text):
        errors.append(f"{rel}: subscribe to the media player source attribute")
    if "ha_reset_subscription_callbacks(HA_SUBSCRIPTION_SCOPE_COVER_ART)" not in text:
        errors.append(f"{rel}: release retired cover art Home Assistant subscriptions")
    resubscribe_body = yaml_script_body(text, "cover_art_resubscribe") or ""
    if "ha_get_" in resubscribe_body:
        errors.append(f"{rel}: avoid retained one-shot reads in the cover art subscription lifecycle")
    if ('normalized_source == "tv"' not in text or
            'normalized_source == "line-in"' not in text or
            'normalized_source == "line in"' not in text or
            "std::tolower" not in text):
        errors.append(f"{rel}: treat TV and Line-in sources as external inputs")
    if "cover_art_apply_external_input_policy" not in text:
        errors.append(f"{rel}: centralize cover art external-input behavior")
    if "script.stop: cover_art_request_artwork" not in text:
        errors.append(f"{rel}: stop pending artwork requests while an external input is active")
    if "id(cover_art_hide_external_input_enabled).state &&" not in text:
        errors.append(f"{rel}: guard cover art start/download paths when external input hiding is enabled")
    return errors


def firmware_cover_art_stale_image_errors(path: Path, root: Path) -> list[str]:
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    script_match = re.search(
        r"(?ms)^  - id: cover_art_show_black_screen\n(?P<body>.*?)(?=^  - id: |\Z)",
        text,
    )
    if not script_match:
        errors.append(f"{rel}: keep a black fallback for cover art with no current image")
        return errors

    body = script_match.group("body")
    if "lvgl.widget.hide: cover_art_image_widget" not in body:
        errors.append(f"{rel}: hide stale cover art image when the black fallback is shown")
    if "id(cover_art_runtime).loaded_url.empty()" in body:
        errors.append(f"{rel}: hide stale cover art image even after previous artwork loaded")
    if "cover_art_error_label" in text or 'text: "Artwork unavailable"' in text:
        errors.append(f"{rel}: do not show an unavailable cover art message")
    return errors


def firmware_cover_art_refresh_errors(path: Path, root: Path) -> list[str]:
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    required_state = (
        ("cover_art_runtime).refresh_needed", "track/source metadata changes as stale artwork"),
        ("cover_art_runtime).effective_download_url", "keep source artwork URLs separate from downloader URLs"),
        ("espcontrol::cover_art::RuntimeState", "own cover art lifecycle state in one controller"),
        ("cover_art_album", "track album names for artwork refresh decisions"),
    )
    for token, message in required_state:
        if token not in text:
            errors.append(f"{rel}: {message}")

    download_body = yaml_script_body(text, "cover_art_download")
    if not download_body:
        errors.append(f"{rel}: missing cover_art_download script")
    else:
        if "/api/media_player_proxy/" not in download_body:
            errors.append(f"{rel}: only cache-bust Home Assistant media proxy artwork URLs")
        if "?time=" not in download_body or "&time=" not in download_body:
            errors.append(f"{rel}: add a refresh marker that preserves existing artwork query strings")
        if "request_update_url(id(cover_art_runtime).effective_download_url)" not in download_body:
            errors.append(f"{rel}: download through the refresh-aware artwork URL")
        if (
            "const bool replacing_active_download = id(cover_art_runtime).download_active();" not in download_body
            or 'replacing_active_download ? "Re-queuing" : "Downloading"' not in download_body
        ):
            errors.append(f"{rel}: coalesce changed cover art URLs into the queued image request")
        if (
            "needs_artwork_refresh" not in download_body
            or "id(cover_art_runtime).refresh_needed || !id(cover_art_runtime).image_available" not in download_body
        ):
            errors.append(f"{rel}: refresh Home Assistant media proxy artwork when no image is currently available")
        replacement_match = re.search(
            r"(?ms)target_mode_is\(espcontrol::DisplayMode::COVER_ART\).*?"
            r"id\(cover_art_runtime\)\.image_available.*?"
            r"id\(cover_art_runtime\)\.refresh_needed.*?"
            r"!\$\{cover_art_live_image_updates\}.*?"
            r"then:\n(?P<body>.*?)(?=^\s+- lambda: |\Z)",
            download_body,
        )
        if not replacement_match:
            errors.append(f"{rel}: keep a dedicated replacement artwork transition path")
        else:
            replacement_body = replacement_match.group("body")
            if "script.execute: cover_art_show_track_overlay" not in replacement_body:
                errors.append(f"{rel}: keep the current artwork visible with updated track text during replacement downloads")
            if "script.execute: cover_art_show_black_screen" in replacement_body:
                errors.append(f"{rel}: do not use the full black fallback for replacement artwork downloads")
            if "script.execute: cover_art_clear_image_source" in replacement_body:
                errors.append(f"{rel}: do not detach visible artwork before replacement artwork is ready")
            if "artwork_image.release: cover_art_downloaded_image" in replacement_body:
                errors.append(f"{rel}: do not release visible artwork before replacement artwork is ready")
            if "id(cover_art_runtime).loaded_url.clear()" in replacement_body:
                errors.append(f"{rel}: keep the previous loaded artwork marker until replacement artwork applies")

    for script_id in ("cover_art_deferred_download", "cover_art_prepare_download"):
        body = yaml_script_body(text, script_id)
        if not body:
            errors.append(f"{rel}: missing {script_id} script")
        elif "id(cover_art_runtime).refresh_needed" not in body:
            errors.append(f"{rel}: let {script_id} refresh unchanged artwork URLs after metadata changes")

    for script_id in ("cover_art_use_cached_artwork", "cover_art_request_artwork"):
        body = yaml_script_body(text, script_id)
        if not body:
            errors.append(f"{rel}: missing {script_id} script")
        elif (
            "chosen == id(cover_art_runtime).source_url" not in body
            or "!id(cover_art_runtime).image_available || id(cover_art_runtime).refresh_needed" not in body
        ):
            errors.append(f"{rel}: do not exit early from {script_id} when stale artwork needs refresh")

    cached_body = yaml_script_body(text, "cover_art_use_cached_artwork")
    if cached_body and "id(cover_art_runtime).select_source(chosen);" not in cached_body:
        errors.append(f"{rel}: mark changed cached artwork URLs as stale before downloading")
    resubscribe_body = yaml_script_body(text, "cover_art_resubscribe") or ""
    if resubscribe_body and "if (!url.empty() && url != id(cover_art_runtime).source_url)" not in resubscribe_body:
        errors.append(f"{rel}: mark changed Home Assistant artwork attributes as stale")

    apply_body = yaml_script_body(text, "cover_art_apply_downloaded_image")
    if not apply_body:
        errors.append(f"{rel}: missing cover_art_apply_downloaded_image script")
    else:
        if "expected_url" not in apply_body or "id(cover_art_runtime).effective_download_url" not in apply_body:
            errors.append(f"{rel}: accept the refresh-aware downloader URL when artwork finishes")
        if "id(cover_art_runtime).apply_download(completed_url)" not in apply_body:
            errors.append(f"{rel}: remember the clean source artwork URL after a download")
        if "id(cover_art_runtime).apply_download(completed_url)" not in apply_body:
            errors.append(f"{rel}: clear stale artwork state only after a replacement image applies")
        if (
            "script.execute: cover_art_clear_image_source" not in apply_body
            or "script.wait: cover_art_clear_image_source" not in apply_body
        ):
            errors.append(f"{rel}: detach the previous LVGL artwork source before showing a replacement image")

    if text.count("mark_artwork_refresh_needed();") < 4:
        errors.append(f"{rel}: mark title, artist, album, and source changes as artwork refresh triggers")
    if ('std::string("media_album_name")' not in resubscribe_body or
            "handle_media_album" not in resubscribe_body):
        errors.append(f"{rel}: subscribe to the media_album_name attribute")
    if "id(cover_art_runtime).refresh_needed = true" not in text:
        errors.append(f"{rel}: set stale artwork state when track/source metadata changes")
    playback_started_body = yaml_script_body(text, "cover_art_playback_started")
    if not playback_started_body:
        errors.append(f"{rel}: missing cover_art_playback_started script")
    elif (
        "!id(cover_art_runtime).image_available" not in playback_started_body
        or "id(cover_art_runtime).retry_count = 0" not in playback_started_body
        or "id(cover_art_runtime).retry_url.clear()" not in playback_started_body
    ):
        errors.append(f"{rel}: reset artwork retry state when playback resumes without a visible image")
    if playback_started_body and "espcontrol::cover_art::display_allowed(" in playback_started_body:
        errors.append(f"{rel}: let the playback-start event activate cover art before mirrored playback state settles")
    if (
        "cover_art_artist_label" in text
        and "if (!id(cover_art_artist).empty()) return id(cover_art_artist);" not in text
    ):
        errors.append(f"{rel}: prefer a real artist name over the external-source fallback label")
    pause_body = yaml_script_body(text, "cover_art_pause_after_touch")
    if pause_body is not None and (
        "target_mode_is(espcontrol::DisplayMode::COVER_ART)" not in pause_body
        or "id(cover_art_manual_pause_until_ms) != 0" not in pause_body
        or "id(cover_art_manual_pause_until_ms) = 1;" not in pause_body
    ):
        errors.append(
            f"{rel}: arm cover art return after dismissal and restart its countdown after every touch"
        )
    return errors


def firmware_cover_art_playback_grace_errors(path: Path, root: Path) -> list[str]:
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    delayed_body = yaml_script_body(text, "cover_art_delayed_playback_stopped")
    if not delayed_body:
        errors.append(f"{rel}: buffer brief non-playing states between tracks")
    else:
        if "delay: 2s" not in delayed_body:
            errors.append(f"{rel}: keep the cover art stop grace period at two seconds")
        if "script.execute: cover_art_playback_stopped" not in delayed_body:
            errors.append(f"{rel}: apply a sustained playback stop after the grace period")
        if (
            'state != "playing"' not in delayed_body
            or 'state != "buffering"' not in delayed_body
            or 'state != "paused"' not in delayed_body
        ):
            errors.append(f"{rel}: recheck playback before applying a delayed stop")

    if "id(cover_art_delayed_playback_stopped).execute(" not in text:
        errors.append(f"{rel}: delay non-playing playback transitions")
    if not re.search(
        r"id\(cover_art_delay_interrupted_by_transition\)\s*=\s*"
        r"id\(cover_art_delay_interrupted_by_transition\)\s*\|\|\s*"
        r"id\(cover_art_delay_timer\)\.is_running\(\);\s+"
        r"id\(cover_art_delay_timer\)\.stop\(\);\s+"
        r"id\(cover_art_delayed_playback_stopped\)\.execute\([^;]*\);",
        text,
    ):
        errors.append(f"{rel}: remember and cancel a pending cover art opening when playback stops")
    if (
        "bool restart_cover_art_delay = id(cover_art_delay_interrupted_by_transition);" not in text
        or "if (!was_playing || restart_cover_art_delay) id(cover_art_playback_started).execute();" not in text
    ):
        errors.append(f"{rel}: restart an interrupted cover art opening when playback resumes")
    if text.count("id(cover_art_delayed_playback_stopped).stop();") < 2:
        errors.append(f"{rel}: cancel a pending stop when playback resumes or pauses")
    if "url.empty() && id(cover_art_delayed_playback_stopped).is_running()" not in text:
        errors.append(f"{rel}: keep cached artwork when Home Assistant clears it during a brief playback transition")

    stopped_body = yaml_script_body(text, "cover_art_playback_stopped")
    if not stopped_body or "script.stop: cover_art_delayed_playback_stopped" not in stopped_body:
        errors.append(f"{rel}: cancel pending playback grace during an immediate stop")
    return errors


def firmware_cover_art_disable_errors(path: Path, root: Path) -> list[str]:
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    body = yaml_script_body(text, "cover_art_disable")
    if not body:
        errors.append(f"{rel}: missing cover_art_disable script")
    elif "switch.turn_off: media_player_sleep_prevention_enabled" in body:
        errors.append(f"{rel}: keep media sleep prevention independent when cover art is disabled")
    return errors


def firmware_cover_art_lifecycle_controller_errors(
    backlight_path: Path, cover_art_path: Path, root: Path
) -> list[str]:
    if not backlight_path.exists() or not cover_art_path.exists():
        return []
    backlight_rel = backlight_path.relative_to(root)
    cover_art_rel = cover_art_path.relative_to(root)
    backlight_text = backlight_path.read_text(encoding="utf-8")
    cover_art_text = cover_art_path.read_text(encoding="utf-8")
    errors: list[str] = []

    adapter = yaml_script_body(backlight_text, "display_mode_apply_transition") or ""
    request = yaml_script_body(backlight_text, "display_mode_request_cover_art") or ""
    clear = yaml_script_body(backlight_text, "display_mode_clear_cover_art") or ""
    reconcile = yaml_script_body(backlight_text, "display_mode_reconcile") or ""
    effect = yaml_script_body(cover_art_text, "display_mode_effect_cover_art") or ""
    hide_effect = yaml_script_body(cover_art_text, "cover_art_hide_effect") or ""
    hide_adapter = yaml_script_body(cover_art_text, "hide_cover_art_view") or ""
    disable = yaml_script_body(cover_art_text, "cover_art_disable") or ""
    playback_restore = yaml_script_body(
        cover_art_text, "cover_art_return_home_after_playback"
    ) or ""

    adapter_markers = (
        "target_mode == static_cast<int>(espcontrol::DisplayMode::COVER_ART)",
        "id: display_mode_effect_cover_art",
        "id: cover_art_hide_effect",
    )
    if (
        any(marker not in adapter for marker in adapter_markers)
        or "complete_transition(" not in adapter
        or "espcontrol::DisplayMode::COVER_ART);" not in adapter
    ):
        errors.append(f"{backlight_rel}: route cover art presentation exclusively through the display adapter")
    if (
        "DisplayRequestSource::MEDIA_PLAYBACK" not in request
        or "DisplayMode::COVER_ART" not in request
        or "script.execute: display_mode_reconcile" not in request
        or "script.wait: display_mode_reconcile" not in request
        or "script.wait: display_mode_apply_transition" not in request
    ):
        errors.append(f"{backlight_rel}: create cover art through a controller media request")
    if (
        "DisplayRequestSource::MEDIA_PLAYBACK" not in clear
        or ".clear(" not in clear
        or "script.execute: display_mode_reconcile" not in clear
        or "script.wait: display_mode_reconcile" not in clear
        or "script.wait: display_mode_apply_transition" not in clear
    ):
        errors.append(f"{backlight_rel}: dismiss cover art by clearing its controller media request")
    if (
        "script.execute: display_mode_clear_cover_art" not in hide_adapter
        or "script.wait: display_mode_clear_cover_art" not in hide_adapter
    ):
        errors.append(
            f"{cover_art_rel}: keep hide_cover_art_view synchronous for compatibility callers"
        )
    if "script.wait: display_mode_clear_cover_art" not in disable:
        errors.append(f"{cover_art_rel}: wait for controller dismissal before releasing cover art resources")
    if "script.wait: display_mode_clear_cover_art" not in playback_restore:
        errors.append(f"{cover_art_rel}: wait for controller dismissal before restoring playback UI")
    if "DisplayRequestSource::MEDIA_PLAYBACK" in reconcile:
        errors.append(f"{backlight_rel}: do not rebuild media requests from the compatibility cover art flag")
    if (
        "previous_cover_generation" not in reconcile
        or "id(cover_art_transition_generation) = transition.generation" not in reconcile
        or "id(cover_art_download_generation) = transition.generation" not in reconcile
        or "!transition_required" not in reconcile
    ):
        errors.append(
            f"{backlight_rel}: preserve active cover art across lower-priority generation changes"
        )

    if "cover_art_screensaver_active" in cover_art_text or "cover_art_screensaver_active" in backlight_text:
        errors.append(f"{cover_art_rel}: use controller mode ownership instead of a compatibility cover art flag")

    if (
        "transition_is_current(" not in effect
        or "espcontrol::DisplayMode::COVER_ART" not in effect
        or "lv_obj_move_foreground(id(cover_art_screensaver))" not in effect
    ):
        errors.append(f"{cover_art_rel}: guard the controller-owned cover art effect by transition generation")
    if (
        "transition_is_current(" not in hide_effect
        or "artwork_image.release: cover_art_downloaded_image" not in hide_effect
        or 'std::string("${device_slug}") == "guition-esp32-s3-4848s040"' not in hide_effect
    ):
        errors.append(f"{cover_art_rel}: preserve guarded S3 image release when cover art is hidden")

    guarded_scripts = {
        "cover_art_delay_timer": "generation_is_current(",
        "cover_art_apply_downloaded_image": "cover_art_download_generation",
        "cover_art_deferred_download": "transition_is_current(",
        "cover_art_retry_download": "cover_art_download_generation",
        "cover_art_refresh_progress": "transition_is_current(",
        "cover_art_delayed_playback_stopped": "generation_is_current(",
    }
    for script_id, marker in guarded_scripts.items():
        body = yaml_script_body(cover_art_text, script_id) or ""
        if marker not in body:
            errors.append(f"{cover_art_rel}: guard {script_id} against obsolete display generations")
    stopped_body = yaml_script_body(cover_art_text, "cover_art_delayed_playback_stopped") or ""
    if (
        "id: cover_art_media_playing" not in stopped_body
        or "id: cover_art_delay_interrupted_by_transition" not in stopped_body
        or "script.execute: display_mode_clear_cover_art" not in stopped_body
        or "script.wait: display_mode_clear_cover_art" not in stopped_body
    ):
        errors.append(
            f"{cover_art_rel}: retire stopped playback state before clearing an obsolete cover art request"
        )
    return errors


def firmware_media_sleep_prevention_errors(
    backlight_path: Path, display_path: Path, cover_art_path: Path, root: Path
) -> list[str]:
    errors: list[str] = []

    if backlight_path.exists():
        rel = backlight_path.relative_to(root)
        text = backlight_path.read_text(encoding="utf-8")
        idle_body = yaml_script_body(text, "screensaver_idle_check")
        if idle_body is None:
            errors.append(f"{rel}: missing screensaver_idle_check script")
        else:
            if (
                "id(cover_art_screensaver_enabled).state &&" not in idle_body
                or "id(media_player_sleep_prevention_enabled).state &&" not in idle_body
                or "id(media_player_playing)" not in idle_body
            ):
                errors.append(
                    f"{rel}: keep media playback awake only while cover art and media sleep prevention are enabled"
                )
            if "id(cover_art_media_playing)" in idle_body:
                errors.append(f"{rel}: use the dedicated media sleep prevention playback state")
            if re.search(
                r"\(id\(media_player_sleep_prevention_enabled\)\.state\s*\|\|\s*"
                r"id\(cover_art_screensaver_enabled\)\.state\)\s*&&\s*"
                r"id\(media_player_playing\)",
                idle_body,
            ):
                errors.append(f"{rel}: require both cover art and media sleep prevention to keep the idle timer awake")
        sleep_body = yaml_script_body(text, "screensaver_sleep_timer")
        if sleep_body is not None:
            if "id(cover_art_media_playing)" in sleep_body and not re.search(
                r"id\(cover_art_last_playback_state\)[\s\S]{0,240}"
                r'state != "playing"[\s\S]{0,120}'
                r'state != "buffering"[\s\S]{0,120}'
                r'state != "paused"[\s\S]{0,240}'
                r"script\.execute:\s*screensaver_idle_check",
                sleep_body,
            ):
                errors.append(f"{rel}: keep the normal screensaver idle during cover art stop grace")
            cover_art_sleep_match = re.search(
                r"id\(cover_art_screensaver_enabled\)\.state[\s\S]{0,360}"
                r"id\(cover_art_media_playing\)[\s\S]{0,360}"
                r"then:\s*\n\s*-\s*script\.execute:\s*([a-zA-Z0-9_]+)",
                sleep_body,
            )
            if cover_art_sleep_match and cover_art_sleep_match.group(1) != "show_cover_art_view":
                errors.append(f"{rel}: start cover art directly after the normal screensaver timeout")
        if "const bool cover_art_immediate_return" in text:
            errors.append(f"{rel}: remove the retired immediate cover art return path")
        if "const bool cover_art_disabled_mode_delay" in text and (
            'id(screensaver_mode).state != "timer"' not in text
            or 'id(screensaver_mode).state != "sensor"' not in text
            or "show_after_seconds > 0.0f" not in text
            or "if (show_after_seconds < 3.0f) show_after_seconds = 3.0f;" not in text
        ):
            errors.append(f"{rel}: restart positive Show After delay after touches in disabled screensaver mode")

        takeover_restore_body = yaml_script_body(text, "display_takeover_resume_restore")
        if takeover_restore_body is not None and "script.execute: show_cover_art_view" in takeover_restore_body:
            if (
                "id(cover_art_last_playback_state)" not in takeover_restore_body
                or 'state == "playing"' not in takeover_restore_body
                or 'state == "buffering"' not in takeover_restore_body
            ):
                errors.append(f"{rel}: keep takeover restore from showing cover art during stop grace")

    if display_path.exists():
        rel = display_path.relative_to(root)
        text = display_path.read_text(encoding="utf-8")
        if "switch.turn_on: media_player_sleep_prevention_enabled" in text:
            errors.append(f"{rel}: do not turn on media sleep prevention when cover art is enabled")
        if re.search(r"(?m)^\s+id: media_player_sleep_prevention_enabled\s*$", text):
            sleep_prevention_index = text.find("id: media_player_sleep_prevention_enabled")
            sleep_prevention_end = text.find("\n  - platform:", sleep_prevention_index + 1)
            sleep_prevention_body = text[sleep_prevention_index:sleep_prevention_end]
            if "restore_mode: RESTORE_DEFAULT_ON" not in sleep_prevention_body:
                errors.append(f"{rel}: default media sleep prevention on")
            migration_tokens = (
                "id: media_player_sleep_prevention_default_on_migrated",
                "if (!id(media_player_sleep_prevention_default_on_migrated))",
                "id(media_player_sleep_prevention_enabled).turn_on();",
                "id(media_player_sleep_prevention_default_on_migrated) = true;",
            )
            if any(token not in text for token in migration_tokens):
                errors.append(f"{rel}: migrate media sleep prevention on exactly once")
        if re.search(r"(?m)^\s+id: cover_art_delay\s*$", text):
            cover_art_delay_index = text.find("id: cover_art_delay")
            cover_art_delay_end = text.find("\n  - platform:", cover_art_delay_index + 1)
            cover_art_delay_body = text[cover_art_delay_index:cover_art_delay_end]
            if "min_value: 3" not in cover_art_delay_body:
                errors.append(f"{rel}: keep the cover art Show After minimum at three seconds")

    if cover_art_path.exists():
        rel = cover_art_path.relative_to(root)
        text = cover_art_path.read_text(encoding="utf-8")
        playback_started_body = yaml_script_body(text, "cover_art_playback_started")
        if playback_started_body is None:
            errors.append(f"{rel}: missing cover_art_playback_started script")
        elif (
            "script.execute: cover_art_start_delay" in playback_started_body
            and (
                "id(media_player_sleep_prevention_enabled).state" not in playback_started_body
                or 'id(screensaver_mode).state != "timer"' not in playback_started_body
                or 'id(screensaver_mode).state != "sensor"' not in playback_started_body
                or "script.execute: screensaver_idle_check" not in playback_started_body
            )
        ):
            errors.append(f"{rel}: let cover art use its own delay when the normal screensaver is disabled")

    return errors


def firmware_touch_cover_art_delay_errors(paths: tuple[Path, ...], root: Path) -> list[str]:
    errors: list[str] = []
    required_sequence = (
        "on_touch:\n"
        "      - script.execute: cover_art_pause_after_touch\n"
        "      - script.wait: cover_art_pause_after_touch\n"
        "      - script.execute: screensaver_wake"
    )
    for path in paths:
        text = path.read_text(encoding="utf-8")
        if "on_touch:" not in text or "script.execute: screensaver_wake" not in text:
            continue
        if required_sequence not in text:
            errors.append(
                f"{path.relative_to(root)}: restart the cover art Show After delay before every touchscreen wake"
            )
    return errors


def firmware_media_sleep_prevention_subscription_errors(paths: tuple[Path, ...], root: Path) -> list[str]:
    errors: list[str] = []
    for path in paths:
        if not path.exists():
            continue
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        if "id(media_player_sleep_prevention_entity).state" not in text:
            errors.append(f"{rel}: subscribe media sleep prevention to its configured media player entity")
        if re.search(
            r"id\(cover_art_media_player_entity\)\.state,\s*\n\s*&id\(media_player_playing\)",
            text,
        ):
            errors.append(f"{rel}: do not drive media sleep prevention from the cover art media player")
    return errors


def firmware_media_control_low_heap_metadata_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_media.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    match = MEDIA_CONTROL_STATE_PATTERN.search(text)
    if not match:
        errors.append(f"{rel}: missing subscribe_media_control_state helper")
        return errors

    body = match.group("body")
    marker = "#ifndef ESPCONTROL_LOW_HEAP_MEDIA_CONTROL"
    if marker not in body:
        errors.append(f"{rel}: keep S3 media modal progress metadata behind the low-heap guard")
        return errors

    always_on, low_heap_excluded = body.split(marker, 1)
    metadata_helper = ""
    progress_helper = ""
    if "inline void media_playback_subscribe_metadata" in text:
        metadata_helper = text.split("inline void media_playback_subscribe_metadata", 1)[1]
        metadata_helper = metadata_helper.split(
            "\n\ninline void media_playback_subscribe_progress", 1
        )[0]
    if "inline void media_playback_subscribe_progress" in text:
        progress_helper = text.split("inline void media_playback_subscribe_progress", 1)[1]
        progress_helper = progress_helper.split(
            "\n\ninline void media_playback_subscribe_volume", 1
        )[0]

    for attr in ("media_title", "media_artist"):
        if (
            "media_playback_subscribe_metadata(state)" not in always_on
            or f'std::string("{attr}")' not in metadata_helper
        ):
            errors.append(f"{rel}: keep {attr} subscribed for the S3 media modal")
    if 'state->entity_id, std::string("media_artist")' in metadata_helper:
        errors.append(f"{rel}: avoid duplicate one-shot metadata reads on every track change")
    progress_in_always_on = "media_playback_subscribe_progress(state)" in always_on
    progress_in_low_heap_excluded = "media_playback_subscribe_progress(state)" in low_heap_excluded
    for attr in ("media_duration", "media_position", "media_position_updated_at"):
        if (
            f'std::string("{attr}")' in always_on
            or progress_in_always_on
        ):
            errors.append(f"{rel}: keep {attr} out of the S3 low-heap media modal path")
        if (
            not progress_in_always_on
            and not progress_in_low_heap_excluded
            or f'std::string("{attr}")' not in progress_helper
        ):
            errors.append(f"{rel}: full media modal builds should still subscribe {attr}")
    return errors


def firmware_cover_art_low_heap_progress_errors(
    firmware_dir: Path, cover_art_path: Path, root: Path
) -> list[str]:
    media_path = firmware_dir / "button_grid_media.h"
    ha_path = firmware_dir / "button_grid_ha.h"
    errors: list[str] = []

    if media_path.exists():
        rel = media_path.relative_to(root)
        media_text = media_path.read_text(encoding="utf-8")
        for token in (
            "struct MediaPlaybackState",
            "media_playback_ensure_state(entity_id)",
            "media_playback_attach_slider(state, ctx)",
            "media_playback_subscribe_state(state)",
            "media_playback_prepare_cover_art_progress",
            "media_playback_set_playing_hint(state, playing)",
            "media_playback_subscribe_progress(state)",
            "media_playback_state_snapshot",
            "media_playback_state_has_progress",
        ):
            if token not in media_text:
                errors.append(f"{rel}: let S3 cover art initialise shared media playback progress")
                break

        prepare_match = re.search(
            r"inline\s+MediaPlaybackState\s*\*media_playback_prepare_cover_art_progress\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            media_text,
            re.DOTALL,
        )
        prepare_body = prepare_match.group("body") if prepare_match else ""
        if any(
            token not in prepare_body
            for token in (
                "media_playback_ensure_state(entity_id)",
                "media_playback_set_playing_hint(state, playing)",
                "media_playback_subscribe_progress(",
                "HA_SUBSCRIPTION_SCOPE_COVER_ART",
                "HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS",
            )
        ) or "media_playback_subscribe_playback_state" in prepare_body:
            errors.append(f"{rel}: prepare S3 progress idempotently without another playback-state subscription")

        progress_match = re.search(
            r"inline\s+void\s+media_playback_subscribe_progress\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            media_text,
            re.DOTALL,
        )
        progress_body = progress_match.group("body") if progress_match else ""
        if (
            "state->progress_subscribed" not in progress_body
            or "state->progress_subscription_scope = scope" not in progress_body
            or progress_body.count("scope") < 4
        ):
            errors.append(f"{rel}: subscribe to shared progress only once per dashboard generation")
        for attr in ("media_duration", "media_position", "media_position_updated_at"):
            if f'std::string("{attr}")' not in progress_body:
                errors.append(f"{rel}: subscribe shared progress to {attr}")

        for token in (
            "state->generation != ha_subscription_generation()",
            "media_playback_reset_state(state, entity_id)",
        ):
            if token not in media_text:
                errors.append(f"{rel}: rebuild shared progress after dashboard subscription changes")
                break

        snapshot_match = re.search(
            r"inline\s+bool\s+media_playback_state_snapshot\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            media_text,
            re.DOTALL,
        )
        snapshot_body = snapshot_match.group("body") if snapshot_match else ""
        if (
            "!state->has_duration" not in snapshot_body
            or "!state->has_position" in snapshot_body
            or "state->has_position ? media_playback_current_position_seconds(state) : 0.0f" not in snapshot_body
        ):
            errors.append(f"{rel}: expose valid duration immediately and start delayed position at zero")

        progress_support_match = re.search(
            r"inline\s+bool\s+media_control_progress_supported\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            media_text,
            re.DOTALL,
        )
        progress_support_body = progress_support_match.group("body") if progress_support_match else ""
        if (
            "#ifdef ESPCONTROL_LOW_HEAP_MEDIA_CONTROL" not in progress_support_body
            or "ctx && media_playback_state_has_progress(ctx->entity_id)" not in progress_support_body
        ):
            errors.append(f"{rel}: give only matching S3 media controls a shared Progress tab")

        timer_match = re.search(
            r"inline\s+void\s+media_playback_refresh_progress_timer\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            media_text,
            re.DOTALL,
        )
        timer_body = timer_match.group("body") if timer_match else ""
        consumer_guard = timer_body.find("has_timer_consumer")
        timer_create = timer_body.find("lv_timer_create")
        if consumer_guard < 0 or timer_create < 0 or consumer_guard > timer_create:
            errors.append(f"{rel}: create the one-second progress timer only for card or modal consumers")

        invalidation_match = re.search(
            r"inline\s+void\s+media_playback_invalidate_stale_progress\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            media_text,
            re.DOTALL,
        )
        invalidation_body = invalidation_match.group("body") if invalidation_match else ""
        if any(
            token not in invalidation_body
            for token in (
                "last_duration_callback_ms",
                "state->duration = 0.0f",
                "state->has_duration = false",
                "state->position_seconds = 0.0f",
                "state->position_updated_ms = 0",
                "state->position_updated_at_known = false",
                "state->position_updated_at_ms = 0",
                "state->has_position = false",
                "media_playback_apply_progress_consumers(state)",
            )
        ):
            errors.append(f"{rel}: hide stale shared progress while preserving fresh track callbacks")

        cover_cleanup_match = re.search(
            r"inline\s+void\s+media_playback_reset_cover_art_progress_subscriptions\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            media_text,
            re.DOTALL,
        )
        cover_cleanup_body = cover_cleanup_match.group("body") if cover_cleanup_match else ""
        if any(
            token not in cover_cleanup_body
            for token in (
                "ha_reset_subscription_callbacks(HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS)",
                "HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS",
                "state->progress_subscribed = false",
                "state->progress_subscription_scope = 0",
                "state->has_position = false",
                "media_playback_subscribe_progress(state)",
            )
        ):
            errors.append(f"{rel}: release cover-art-owned progress and preserve active card consumers")
        elif cover_cleanup_body.find(
            "ha_reset_subscription_callbacks(HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS)"
        ) > cover_cleanup_body.find("state->progress_subscribed = false"):
            errors.append(f"{rel}: release cover-art progress callbacks before clearing local state")
    else:
        errors.append(f"{media_path.relative_to(root)}: missing media card helpers")

    if ha_path.exists():
        ha_rel = ha_path.relative_to(root)
        ha_text = ha_path.read_text(encoding="utf-8")
        bump_match = re.search(
            r"inline\s+void\s+bump_ha_subscription_generation\s*\([^)]*\)\s*\{"
            r"(?P<body>.*?)\n\}",
            ha_text,
            re.DOTALL,
        )
        bump_body = bump_match.group("body") if bump_match else ""
        if (
            "HA_SUBSCRIPTION_SCOPE_DEFAULT" not in bump_body
            or "HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS" not in bump_body
        ):
            errors.append(f"{ha_rel}: release cover-art progress subscriptions on generation bumps")

    if not cover_art_path.exists():
        return errors

    rel = cover_art_path.relative_to(root)
    text = cover_art_path.read_text(encoding="utf-8")
    stripped_low_heap = re.sub(
        r"#ifndef ESPCONTROL_LOW_HEAP_COVER_ART.*?#endif",
        "",
        text,
        flags=re.DOTALL,
    )
    for attr in ("media_duration", "media_position", "media_position_updated_at"):
        if f'std::string("{attr}")' in stripped_low_heap:
            errors.append(f"{rel}: keep {attr} out of the S3 low-heap cover art path")
        if f'std::string("{attr}")' not in text:
            errors.append(f"{rel}: full cover art builds should still subscribe {attr}")

    refresh_body = yaml_script_body(text, "cover_art_refresh_progress")
    if not refresh_body:
        errors.append(f"{rel}: missing cover_art_refresh_progress script")
    elif (
        "#ifdef ESPCONTROL_LOW_HEAP_COVER_ART" not in refresh_body
        or "media_playback_prepare_cover_art_progress" not in refresh_body
        or "media_playback_state_snapshot" not in refresh_body
        or "lv_obj_clear_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN)" not in refresh_body
    ):
        errors.append(f"{rel}: let S3 cover art initialise and consume shared progress")

    resubscribe_body = yaml_script_body(text, "cover_art_resubscribe")
    if not resubscribe_body or any(
        token not in resubscribe_body
        for token in (
            "#ifdef ESPCONTROL_LOW_HEAP_COVER_ART",
            "media_playback_prepare_cover_art_progress",
            "media_playback_invalidate_stale_progress(cover_entity)",
            "media_playback_reset_cover_art_progress_subscriptions()",
        )
    ):
        errors.append(f"{rel}: prepare and invalidate S3 shared cover art progress")
    elif not re.search(
        r"if \(id\(cover_art_screensaver_enabled\)\.state\) \{\s*"
        r"media_playback_prepare_cover_art_progress\(.*?\}\s*else \{\s*"
        r"media_playback_reset_cover_art_progress_subscriptions\(\);",
        resubscribe_body,
        re.DOTALL,
    ):
        errors.append(f"{rel}: keep S3 progress unsubscribed when cover art is disabled")

    disable_body = yaml_script_body(text, "cover_art_disable")
    if not disable_body or "script.execute: cover_art_resubscribe" not in disable_body:
        errors.append(f"{rel}: release cover art subscriptions when the feature is disabled")

    prepared_visibility = re.search(
        r"media_playback_prepare_cover_art_progress\(.*?"
        r"return media_playback_state_has_progress\(id\(cover_art_media_player_entity\)\.state\);",
        text,
        re.DOTALL,
    )
    if not prepared_visibility:
        errors.append(f"{rel}: prepare S3 progress before checking cover art visibility")

    low_heap_refresh = re.search(
        r"- interval: 1s.*?#ifdef ESPCONTROL_LOW_HEAP_COVER_ART(.*?)#else",
        text,
        re.DOTALL,
    )
    if not low_heap_refresh:
        errors.append(f"{rel}: missing S3 cover art refresh condition")
    else:
        condition_body = low_heap_refresh.group(1)
        active_guard = condition_body.find(
            "if (!id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART))"
        )
        prepare_progress = condition_body.find("media_playback_prepare_cover_art_progress")
        if active_guard < 0 or prepare_progress < 0 or active_guard > prepare_progress:
            errors.append(f"{rel}: prepare S3 progress only while cover art is active")

    return errors


def firmware_cover_art_progress_visibility_errors(path: Path, root: Path) -> list[str]:
    errors: list[str] = []
    if not path.exists():
        return errors

    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    time_widget = re.search(
        r"(?ms)^\s+- label:\n\s+id: cover_art_time_label\n(?P<body>.*?)(?=^\s+- bar:)",
        text,
    )
    if time_widget is None or "hidden: true" not in time_widget.group("body"):
        errors.append(f"{rel}: initialize cover art playback time hidden")

    full_build_guard = (
        "return espcontrol::cover_art::progress_available("
        "id(cover_art_media_duration));"
    )
    for script_id in ("cover_art_show_black_screen", "cover_art_show_track_overlay"):
        body = yaml_script_body(text, script_id)
        if body is None or full_build_guard not in body:
            errors.append(f"{rel}: guard every cover art progress reveal with usable duration")

    sync_body = yaml_script_body(text, "cover_art_sync_track_text")
    if sync_body is None or any(
        token not in sync_body
        for token in (
            "espcontrol::cover_art::progress_available(id(cover_art_media_duration))",
            "lv_obj_add_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN)",
            "lv_obj_add_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN)",
        )
    ):
        errors.append(f"{rel}: hide cover art playback time when duration is unavailable")

    refresh_body = yaml_script_body(text, "cover_art_refresh_progress")
    if refresh_body is None or any(
        token not in refresh_body
        for token in (
            "espcontrol::cover_art::progress_available(duration)",
            "lv_bar_set_value(id(cover_art_progress_bar), 0, LV_ANIM_OFF)",
            'lv_label_set_text(id(cover_art_time_label), "0:00  /  0:00")',
            "lv_obj_add_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN)",
            "lv_obj_add_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN)",
            "lv_obj_clear_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN)",
            "lv_obj_clear_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN)",
        )
    ):
        errors.append(f"{rel}: hide and reset unavailable cover art progress")

    duration_handler_match = re.search(
        r"handle_media_duration\s*=\s*.*?\{(?P<body>.*?)\n\s*\};\n\s*if\s*\(!already_subscribed\)",
        text,
        re.DOTALL,
    )
    duration_handler = duration_handler_match.group("body") if duration_handler_match else ""
    if (
        "const bool next_progress_available =" not in duration_handler
        or "if (!next_progress_available)" not in duration_handler
        or "next_duration = 0.0f" not in duration_handler
    ):
        errors.append(f"{rel}: normalize invalid cover art durations")
    if any(
        token not in duration_handler
        for token in (
            "id(cover_art_media_position) = 0.0f",
            "id(cover_art_position_anchor) = 0.0f",
            "id(cover_art_position_anchor_epoch) = 0",
            "id(cover_art_last_position_timestamp) = 0",
        )
    ):
        errors.append(f"{rel}: reset cover art position when duration becomes unavailable")
    if "current_progress_available" in duration_handler:
        errors.append(f"{rel}: preserve fresh cover art position when duration arrives late")
    if "id(cover_art_last_duration_callback_ms) = millis()" not in duration_handler:
        errors.append(f"{rel}: track cover art duration callback freshness")

    duration_invalidator_match = re.search(
        r"invalidate_stale_media_duration\s*=\s*\[[^\]]*\]\(\)\s*\{(?P<body>.*?)\n\s*\};",
        text,
        re.DOTALL,
    )
    duration_invalidator = (
        duration_invalidator_match.group("body") if duration_invalidator_match else ""
    )
    if any(
        token not in duration_invalidator
        for token in (
            "id(cover_art_last_duration_callback_ms)",
            "millis() - last_duration_ms",
            "duration_callback_is_fresh",
            "if (!duration_callback_is_fresh)",
            "id(cover_art_media_duration) = 0.0f",
        )
    ):
        errors.append(f"{rel}: preserve fresh cover art duration when metadata arrives late")
    if any(
        token in duration_invalidator
        for token in (
            "id(cover_art_media_position) = 0.0f",
            "id(cover_art_position_anchor) = 0.0f",
            "id(cover_art_position_anchor_epoch) = 0",
            "id(cover_art_last_position_timestamp) = 0",
        )
    ):
        errors.append(f"{rel}: preserve fresh cover art position while invalidating stale metadata")

    for metadata_name, assignment in (
        ("title", "id(cover_art_title) = next"),
        ("artist", "id(cover_art_artist) = next"),
        ("album", "id(cover_art_album) = next"),
        ("source", "id(cover_art_media_source) = next"),
    ):
        handler_match = re.search(
            rf"handle_media_{metadata_name}\s*=\s*.*?\{{(?P<body>.*?)\n\s*\}};\n\s*if\s*\(!already_subscribed\)",
            text,
            re.DOTALL,
        )
        handler = handler_match.group("body") if handler_match else ""
        metadata_assignment = handler.find(assignment)
        duration_invalidation = handler.find("invalidate_stale_media_duration()")
        if (
            metadata_assignment < 0
            or duration_invalidation < 0
            or duration_invalidation > metadata_assignment
        ):
            errors.append(
                f"{rel}: mark stale cover art duration unavailable when media {metadata_name} changes"
            )
        if any(
            token in handler
            for token in (
                "id(cover_art_media_position) = 0.0f",
                "id(cover_art_position_anchor) = 0.0f",
                "id(cover_art_position_anchor_epoch) = 0",
                "id(cover_art_last_position_timestamp) = 0",
            )
        ):
            errors.append(
                f"{rel}: preserve fresh cover art position when {metadata_name} metadata arrives late"
            )
        if metadata_name == "album":
            progress_refresh = handler.find("id(cover_art_sync_track_text).execute()")
            if progress_refresh < 0 or progress_refresh < duration_invalidation:
                errors.append(
                    f"{rel}: refresh cover art progress immediately when media album changes"
                )

    return errors


def firmware_image_card_entity_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_image.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "image_card_entity_supported" not in text:
        errors.append(f"{rel}: centralize image card entity-domain support")
    if 'entity_id.rfind("camera.", 0) == 0' not in text:
        errors.append(f"{rel}: keep camera entities supported by image cards")
    if 'entity_id.rfind("image.", 0) == 0' not in text:
        errors.append(f"{rel}: support Home Assistant image entities in image cards")
    if 'if (!image_card_entity_supported(p.entity))' not in text:
        errors.append(f"{rel}: use the shared image card entity-domain guard")
    if "only supports camera entities" in text:
        errors.append(f"{rel}: do not reject Home Assistant image entities as unavailable")
    return errors


def firmware_image_card_base_url_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_image.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "base_url_provider" not in text:
        errors.append(f"{rel}: keep image card Home Assistant base URL lookup live")
    if ("image_card_join_url(image_card_base_url(ctx), raw)" not in text and
        ("std::string base_url = image_card_base_url(ctx)" not in text or
         "image_card_join_url(base_url, raw)" not in text)):
        errors.append(f"{rel}: resolve image card base URL when entity_picture is handled")
    if 'ctx->base_url = cfg.home_assistant_base_url ? cfg.home_assistant_base_url() : "";' in text:
        if "ctx->base_url_provider = cfg.home_assistant_base_url" not in text:
            errors.append(f"{rel}: do not rely only on the startup-time image card base URL")
    return errors


def firmware_image_card_quality_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_image.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "IMAGE_CARD_MODAL_MAX_TARGET_SIDE_PX" not in text:
        errors.append(f"{rel}: cap high-resolution image card modal downloads")
    if (
        "IMAGE_CARD_CONSTRAINED_MODAL_MAX_TARGET_SIDE_PX" not in text
        or "image_pipeline_modal_max_target_side" not in text
    ):
        errors.append(f"{rel}: cap constrained-display image card modals at 320 pixels")
    if "IMAGE_CARD_MAX_CONTEXTS = 6" not in text:
        errors.append(f"{rel}: support six concurrent image cards on P4 displays")
    if "image_card_limit_target_size" not in text:
        errors.append(f"{rel}: scale image card modal downloads to a display-appropriate size")
    if "image_card_memory_available" not in text or "IMAGE_CARD_MEMORY_HEADROOM_BYTES" not in text:
        errors.append(f"{rel}: check free memory before image-card downloads")
    if "MALLOC_CAP_SPIRAM" not in text or "external_largest" not in text:
        errors.append(f"{rel}: include PSRAM in image-card memory checks")
    if (
        "image_pipeline_memory_failure" not in text
        or "IMAGE_CARD_CONSTRAINED_INTERNAL_FREE_BYTES" not in text
        or "IMAGE_CARD_CONSTRAINED_INTERNAL_LARGEST_BYTES" not in text
    ):
        errors.append(f"{rel}: guard constrained internal RAM separately from PSRAM")
    if (
        "image_card_release_modal_cache" not in text
        or "image_card_retain_modal_cache" not in text
        or "modal_image->release()" not in text
    ):
        errors.append(f"{rel}: release constrained modal image buffers after closing")
    if "ctx->image->cancel_update();" not in text:
        errors.append(f"{rel}: cancel in-flight image downloads before opening image card modals")
    if "Deferring image refresh while modal is open" not in text:
        errors.append(f"{rel}: defer image downloads while image card modals are open")
    if "image_card_clear_widget_source(ui.image_widget)" not in text:
        errors.append(f"{rel}: detach image sources before deleting image card modals")
    if (
        "ctx->image->set_target_size(width, height)" not in text
        and "image_card_tile_decode_size(width, height, &target_width, &target_height)" not in text
        and "ctx->image->set_target_size(decode_width, decode_height)" not in text
    ):
        errors.append(f"{rel}: set image card download target size before requesting images")
    if "modal_image" not in text or "image_card_request_modal_source_url" not in text:
        errors.append(f"{rel}: use a separate modal image downloader for expanded image-card quality")
    if "ctx->modal_image->request_update_url(ctx->modal_url" not in text:
        errors.append(f"{rel}: request expanded image-card downloads through the modal downloader")
    if "image_card_set_widget_source(ui.image_widget, ctx->modal_image)" not in text:
        errors.append(f"{rel}: swap expanded image cards to the modal-quality image after it downloads")
    if "ImageCardModalCache" not in text or "image_card_modal_cache" not in text:
        errors.append(f"{rel}: retain one shared modal image cache for instant reopen")
    if 'image_card_set_loading_state(loading, "Too many")' not in text:
        errors.append(f"{rel}: show a visible image-card limit message when downloaders run out")
    modal_refresh = re.search(
        r"inline\s+bool\s+image_card_modal_refresh_supported\s*\(\s*\)\s*\{\s*return\s+true\s*;",
        text,
        re.S,
    )
    if not modal_refresh:
        errors.append(f"{rel}: keep modal-quality image refresh enabled on the 4.3-inch P4 screen")
    tile_size = re.search(
        r"inline\s+void\s+image_card_tile_request_size[^\{]*\{[^\}]*image_card_limit_target_size",
        text,
        re.S,
    )
    if not tile_size or "image_card_high_quality_request_size" in text:
        errors.append(f"{rel}: size every image-card tile request to its on-screen bounds")
    if "Closing image modal" not in text:
        errors.append(f"{rel}: log image-card modal close events")
    if "image_card_abort_modal_open" not in text or "modal shell setup failed" not in text:
        errors.append(f"{rel}: clean up partially-created image card modals")
    if (
        "lv_obj_set_size(ui.loading_widget, width, height)" not in text
        or "lv_obj_align(icon, LV_ALIGN_CENTER" not in text
        or "LV_ALIGN_OUT_BOTTOM_MID" not in text
    ):
        errors.append(f"{rel}: keep image-card modal loading overlay centered")
    if (
        "image_card_show_modal_image(ctx, ctx->image)" not in text
        or "image_card_queue_modal_source_request(ctx)" not in text
    ):
        errors.append(f"{rel}: show the cached image-card tile while modal-quality image loads")
    modal_failure = re.search(
        r"inline\s+void\s+image_card_show_modal_download_failure[^\{]*\{(?P<body>.*?)\n\}",
        text,
        re.S,
    )
    modal_failure_body = modal_failure.group("body") if modal_failure else ""
    modal_error_handler = re.search(
        r"inline\s+void\s+image_card_handle_modal_download_error[^\{]*\{(?P<body>.*?)\n\}",
        text,
        re.S,
    )
    modal_error_body = modal_error_handler.group("body") if modal_error_handler else ""
    if (
        "image_card_modal_has_preview(ctx)" not in modal_failure_body
        or 'image_card_show_modal_loading(ctx, "Unavailable")' not in modal_failure_body
        or "image_card_show_modal_download_failure(ctx)" not in modal_error_body
    ):
        errors.append(f"{rel}: keep an error state when image-card modals have no preview")
    if "lv_obj_set_style_clip_corner(ui.panel, true, LV_PART_MAIN)" not in text:
        errors.append(f"{rel}: clip image card modal content to rounded panel corners")
    if "image_card_apply_corner_clip" not in text:
        errors.append(f"{rel}: preserve image card rounded corners while pressed")
    if "image_card_pressed_selector" not in text:
        errors.append(f"{rel}: apply image card corner clipping to the pressed state")
    if "image_card_refresh_tile_geometry" not in text or "resized tile" not in text:
        errors.append(f"{rel}: refresh image-card downloads when card size changes")
    if "image_card_reset_resized_tile" not in text or "ctx->image->release()" not in text:
        errors.append(f"{rel}: clear stale image-card tile buffers when card size changes")
    if "image_card_tile_request_size" not in text:
        errors.append(f"{rel}: keep small-display image card tile downloads sized to the tile")

    driver_path = firmware_dir / "button_grid_image_driver.h"
    if driver_path.exists():
        driver_rel = driver_path.relative_to(root)
        driver_text = driver_path.read_text(encoding="utf-8")
        if "image_card_refresh_tile_geometry(image_context)" not in driver_text:
            errors.append(f"{driver_rel}: update active image-card geometry during grid refresh")
    else:
        grid_path = firmware_dir / "button_grid_grid.h"
        if grid_path.exists():
            grid_rel = grid_path.relative_to(root)
            grid_text = grid_path.read_text(encoding="utf-8")
            if "image_card_refresh_tile_geometry(ctx)" not in grid_text:
                errors.append(f"{grid_rel}: update active image-card geometry during grid refresh")
    return errors


def firmware_image_card_startup_errors(
    firmware_dir: Path,
    core_infra_path: Path,
    root: Path,
) -> list[str]:
    path = firmware_dir / "button_grid_image.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "inline void refresh_image_cards()" not in text:
        errors.append(f"{rel}: refresh image cards when Home Assistant reconnects")
    if (
        "image_card_request_current_picture" not in text
        or "if (ctx->media_artwork)" not in text
        or "image_card_request_media_artwork(ctx);" not in text
        or "image_card_refresh_current_picture(ctx);" not in text
        or "ctx->media_artwork_retry_mask = 0;" not in text
        or "ctx->media_artwork_sources.clear();" not in text
        or "artwork_picture_response_clears_retry" not in text
        or "inline void image_card_refresh_due()" not in text
        or text.count("image_card_request_current_picture(ctx);") < 2
        or "media_artwork_retry_mask" not in text
        or "artwork_source_request_mask" not in text
        or "artwork_source_failed_mask" not in text
    ):
        errors.append(f"{rel}: retry media cover artwork when Home Assistant reconnects")
    if "IMAGE_CARD_API_RETRY_INTERVAL_MS" not in text:
        errors.append(f"{rel}: retry image-card startup quickly after Home Assistant API connects")
    if "if (!ha_api_connected()) return;" not in text:
        errors.append(f"{rel}: arm image-card refresh from the Home Assistant API connection")
    if "if (!ha_api_connected())" not in text or "ha_get_attribute(" not in text:
        errors.append(f"{rel}: request image-card attributes once the Home Assistant API is connected")
    if '"access_token"' not in text or "image_card_proxy_path_with_token" not in text:
        errors.append(f"{rel}: load Home Assistant image-card proxy URLs with the entity access token")
    if '"/api/image_proxy/" + entity_id' not in text or '"/api/camera_proxy/" + entity_id' not in text:
        errors.append(f"{rel}: fall back to Home Assistant proxy URLs when image-card entity_picture is unavailable")
    if "Waiting for Home Assistant base URL" not in text:
        errors.append(f"{rel}: keep image cards loading until the Home Assistant base URL is ready")
    if "subscribe_image_card_entity_state" not in text or "ha_subscribe_state(" not in text:
        errors.append(f"{rel}: refresh image cards when the camera/image entity state changes")
    if "image_card_context_current" not in text or "generation == ha_subscription_generation()" not in text:
        errors.append(f"{rel}: ignore stale image-card callbacks after grid rebuild")
    if (
        "image_card_generation" not in text
        or "image_card_context_current(ctx, image_card_entity_id, image_card_generation)" not in text
    ):
        errors.append(f"{rel}: ignore stale image-card entity_picture callbacks after grid rebuild")
    if ("image_card_tile_request_size(width, height" not in text and
        "image_card_tile_request_size(decode_width, decode_height" not in text):
        errors.append(f"{rel}: request display-sized Home Assistant image card downloads")
    if "image_card_sized_url(ctx->source_url, request_width, request_height)" not in text:
        errors.append(f"{rel}: request bounded Home Assistant image card proxy downloads")
    if '"/api/camera_proxy/"' not in text or '"/api/image_proxy/"' not in text:
        errors.append(f"{rel}: recognize Home Assistant camera and image proxy URLs")

    if core_infra_path.exists():
        core_rel = core_infra_path.relative_to(root)
        core_text = core_infra_path.read_text(encoding="utf-8")
        if "is_home_assistant && ha_api_connected()" not in core_text:
            errors.append(f"{core_rel}: start image-card refresh when Home Assistant API connects")
        if core_text.count("refresh_image_cards();") < 4:
            errors.append(f"{core_rel}: refresh image cards through Home Assistant connect retries")
    return errors


def firmware_artwork_image_auth_errors(path: Path, root: Path) -> list[str]:
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "HTTP_AUTH_TYPE_BASIC" in text:
        errors.append(
            f"{rel}: keep local Home Assistant image proxy requests off HTTP Basic auth"
        )
    if "config.auth_type = HTTP_AUTH_TYPE_NONE;" not in text:
        errors.append(f"{rel}: explicitly disable HTTP auth for local artwork requests")
    if (
        'container->status_code <= 0 && is_ha_media_proxy_url(url)' not in text
        or "trying artwork bytes anyway" not in text
        or "container->status_code = HTTP_CODE_OK;" not in text
    ):
        errors.append(f"{rel}: allow Home Assistant media proxy artwork to fall back to image-byte detection")
    if "download_buffer_.shrink_to(0)" not in text:
        errors.append(f"{rel}: fully release artwork download buffers after image requests")
    return errors


def firmware_screensaver_wake_guard_errors(
    backlight_path: Path,
    screen_clock_path: Path,
    cover_art_path: Path,
    root: Path,
) -> list[str]:
    errors: list[str] = []
    if backlight_path.exists():
        rel = backlight_path.relative_to(root)
        text = backlight_path.read_text(encoding="utf-8")
        body = yaml_script_body(text, "screensaver_wake")
        if body is None:
            errors.append(f"{rel}: missing screensaver_wake script")
        else:
            pending_restore_tokens = (
                "id: screensaver_wake_restore_pending",
                "id(screensaver_wake_restore_pending) =",
                "!id(display_mode_controller).target_mode_is(",
                "const bool restore_pending = id(screensaver_wake_restore_pending);",
                "id(screensaver_wake_restore_pending) = false;",
                "return restore_pending ||",
            )
            if any(token not in text for token in pending_restore_tokens):
                errors.append(f"{rel}: restore ACTIVE when touch cancels a pending sleep transition")
            marker = "const bool restore_pending = id(screensaver_wake_restore_pending);"
            if marker not in body:
                errors.append(f"{rel}: keep the normal screensaver wake branch explicit")
            else:
                normal_wake_body = body.split(marker, 1)[1]
                if re.search(
                    r"id:\s*screensaver_wake_touch_guard_active\s*\n\s*value:\s*'true'",
                    normal_wake_body,
                ):
                    errors.append(f"{rel}: do not block the first button tap after normal screensaver wake")
                if "script.execute: screensaver_wake_touch_guard_clear" in normal_wake_body:
                    errors.append(f"{rel}: do not arm a delayed wake guard clear for normal screensaver wake")
                if "id(screensaver_wake_touch_guard_skip_once) = false" not in normal_wake_body:
                    errors.append(f"{rel}: consume stale cover art wake guard state during screensaver wake")
                if not re.search(
                    r"id:\s*screensaver_wake_touch_guard_active\s*\n\s*value:\s*'false'",
                    normal_wake_body,
                ):
                    errors.append(f"{rel}: clear stale wake guard state during normal screensaver wake")
                if "bool keep_wake_guard = id(screensaver_wake_touch_guard_skip_once);" not in normal_wake_body:
                    errors.append(f"{rel}: preserve the shared wake guard while screensaver state is restored")

            clear_marker = (
                "id(display_mode_controller).clear("
                "espcontrol::DisplayRequestSource::IDLE_TIMER);"
            )
            clear_index = body.find(clear_marker)
            guard_execute_index = body.find("script.execute: screensaver_wake_touch_block")
            pre_clear_body = body[:clear_index] if clear_index >= 0 else ""
            if (
                "id(screensaver_wake_touch_guard_skip_once) =" not in pre_clear_body
                or "espcontrol::DisplayMode::COVER_ART" not in pre_clear_body
                or "espcontrol::DisplayMode::DISPLAY_OFF" not in pre_clear_body
            ):
                errors.append(
                    f"{rel}: arm the shared wake guard from the pre-wake Cover Art and Display Off modes"
                )
            if (
                clear_index < 0
                or guard_execute_index < 0
                or guard_execute_index > clear_index
            ):
                errors.append(f"{rel}: raise the shared wake guard before clearing screensaver state")

        guard_body = yaml_script_body(text, "screensaver_wake_touch_block")
        if guard_body is None:
            errors.append(f"{rel}: missing shared screensaver_wake_touch_block script")
        else:
            if not re.search(
                r"id:\s*screensaver_wake_touch_guard_active\s*\n\s*value:\s*'true'",
                guard_body,
            ):
                errors.append(f"{rel}: keep screensaver wake taps guarded")
            if "LV_INDEV_STATE_PRESSED" not in guard_body:
                errors.append(f"{rel}: keep the shared wake guard until the wake touch is released")
            if "timeout: 2s" not in guard_body:
                errors.append(f"{rel}: clear the shared wake guard after a stuck touch timeout")
            if "delay: 250ms" not in guard_body:
                errors.append(f"{rel}: retain the shared wake guard while touch input settles")
            if "screensaver_fill_screen(id(screensaver_wake_touch_guard))" not in guard_body:
                errors.append(f"{rel}: resize the shared wake guard for every display layout")
            if "lv_obj_move_foreground(id(screensaver_wake_touch_guard))" not in guard_body:
                errors.append(f"{rel}: raise the shared wake guard above active controls")
            if "lvgl.widget.show: screensaver_wake_touch_guard" not in guard_body:
                errors.append(f"{rel}: show the shared full-screen wake guard")
            if not re.search(
                r"id:\s*screensaver_wake_touch_guard_active\s*\n\s*value:\s*'false'",
                guard_body,
            ):
                errors.append(f"{rel}: accept the first deliberate tap after wake-touch release")
            if "lvgl.widget.hide: screensaver_wake_touch_guard" not in guard_body:
                errors.append(f"{rel}: hide the shared wake guard after touch release")

    if screen_clock_path.exists():
        rel = screen_clock_path.relative_to(root)
        text = screen_clock_path.read_text(encoding="utf-8")
        widget_marker = "id: screensaver_wake_touch_guard"
        widget_index = text.find(widget_marker)
        widget_body = text[widget_index:] if widget_index >= 0 else ""
        if widget_index < 0:
            errors.append(f"{rel}: define the shared screensaver wake touch guard")
        elif not all(
            token in widget_body
            for token in ("width: 100%", "height: 100%", "clickable: true", "hidden: true")
        ):
            errors.append(f"{rel}: keep the shared wake guard full-screen, clickable, and hidden by default")

    if cover_art_path.exists():
        rel = cover_art_path.relative_to(root)
        text = cover_art_path.read_text(encoding="utf-8")
        if "cover_art_wake_touch_guard" in text or "cover_art_wake_touch_block" in text:
            errors.append(f"{rel}: use the shared screensaver wake guard for Cover Art")
    return errors


def firmware_screen_wake_button_errors(backlight_path: Path, root: Path) -> list[str]:
    if not backlight_path.exists():
        return []

    rel = backlight_path.relative_to(root)
    text = backlight_path.read_text(encoding="utf-8")
    errors: list[str] = []
    match = re.search(
        r'(?ms)^button:\s*\n(?P<section>.*?)(?=^[a-zA-Z_][\w-]*:\s*(?:#.*)?$|\Z)',
        text,
    )
    button_block: str | None = None
    if match:
        for item in re.finditer(
            r'(?ms)^  - platform:\s*template\s*\n(?P<body>.*?)(?=^  - platform:|\Z)',
            match.group("section"),
        ):
            candidate = item.group(0)
            if re.search(r'^\s+name:\s*["\']?\$\{entity_screen_wake\}["\']?\s*$', candidate, re.MULTILINE):
                button_block = candidate
                break

    if button_block is None:
        return [f"{rel}: expose Screen: Wake as a template button"]

    if re.search(r'^\s+internal:\s*true\s*$', button_block, re.MULTILINE):
        errors.append(f"{rel}: keep Screen: Wake exposed to Home Assistant")
    if re.search(r'^\s+disabled_by_default:\s*true\s*$', button_block, re.MULTILINE):
        errors.append(f"{rel}: enable Screen: Wake by default")
    if re.search(r'^\s+entity_category:', button_block, re.MULTILINE):
        errors.append(f"{rel}: keep Screen: Wake operational rather than a configuration entity")
    if not re.search(r'^\s+icon:\s*["\']?mdi:monitor-arrow-up["\']?\s*$', button_block, re.MULTILINE):
        errors.append(f"{rel}: use mdi:monitor-arrow-up for Screen: Wake")

    required_sequence = (
        "      - script.execute: cover_art_pause_after_touch\n"
        "      - script.wait: cover_art_pause_after_touch\n"
        "      - script.execute: screensaver_wake"
    )
    if "on_press:" not in button_block or required_sequence not in button_block:
        errors.append(f"{rel}: run the complete touch-equivalent Screen: Wake sequence")
    return errors


def firmware_clock_bar_pending_wake_errors(display_path: Path, root: Path) -> list[str]:
    if not display_path.exists():
        return []
    rel = display_path.relative_to(root)
    text = display_path.read_text(encoding="utf-8")
    body = yaml_script_body(text, "clock_bar_apply")
    if body is None:
        return [f"{rel}: missing clock_bar_apply script"]
    if "id(display_mode_controller).target_mode()" not in body:
        return [f"{rel}: resolve clock bar visibility from the pending display target"]
    return []


def firmware_clock_screensaver_overlay_errors(backlight_path: Path, root: Path) -> list[str]:
    errors: list[str] = []
    if not backlight_path.exists():
        return errors

    rel = backlight_path.relative_to(root)
    text = backlight_path.read_text(encoding="utf-8")
    sleep_body = yaml_script_body(text, "screensaver_sleep_timer")
    adapter_body = yaml_script_body(text, "display_mode_apply_transition")
    show_body = yaml_script_body(text, "show_clock_view")
    keep_on_top_body = yaml_script_body(text, "clock_screensaver_keep_on_top")

    if sleep_body is None:
        errors.append(f"{rel}: missing screensaver_sleep_timer script")
    else:
        activation_markers = (
            "script.execute: show_clock_view",
            "espcontrol::DisplayMode::CLOCK",
        )
        activation_indexes = [
            sleep_body.find(marker) for marker in activation_markers
            if sleep_body.find(marker) != -1
        ]
        show_index = min(activation_indexes) if activation_indexes else -1
        if show_index == -1:
            errors.append(f"{rel}: keep clock screensaver activation explicit")
        else:
            pre_clock_show = sleep_body[:show_index]
            cleanup_tokens = (
                "media_volume_hide_modal();",
                "climate_control_hide_modal();",
                "option_select_hide_modal();",
                "switch_confirmation_hide_modal();",
                "alarm_pin_hide_modal();",
                "network_status_hide_modal();",
                "script.execute: hide_cover_art_view",
            )
            if any(token in pre_clock_show for token in cleanup_tokens):
                errors.append(f"{rel}: let the clock screensaver overlay the existing UI without closing it")

        if "espcontrol::DisplayMode::CLOCK" in sleep_body and (
            adapter_body is None or "id: show_clock_view" not in adapter_body
        ):
            errors.append(f"{rel}: route controller clock decisions through show_clock_view")

    if show_body is None:
        errors.append(f"{rel}: missing show_clock_view script")
    elif (
        "hide_clock_bar_top_layer_widgets(" not in show_body
        or "lv_obj_clear_flag(id(clock_screensaver), LV_OBJ_FLAG_HIDDEN);" not in show_body
        or "lv_obj_move_foreground(id(clock_screensaver));" not in show_body
    ):
        errors.append(f"{rel}: raise the clock screensaver above existing top-layer UI")

    if keep_on_top_body is None:
        errors.append(f"{rel}: missing clock screensaver keep-on-top script")
    else:
        required_keep_on_top_tokens = (
            "current_mode_is(",
            "hide_clock_bar_top_layer_widgets(",
            "refresh_screensaver_fullscreen(id(clock_screensaver), id(dim_screensaver_touch_guard));",
            "lv_obj_move_foreground(id(clock_screensaver));",
        )
        if any(token not in keep_on_top_body for token in required_keep_on_top_tokens):
            errors.append(f"{rel}: keep hiding clock-bar widgets and re-raising the active clock screensaver above overlays")
        if "lv_obj_clear_flag(id(clock_screensaver), LV_OBJ_FLAG_HIDDEN)" in keep_on_top_body:
            errors.append(
                f"{rel}: do not un-hide the clock screensaver in keep-on-top; "
                "show_clock_view owns widget visibility to avoid premature display during the fade"
            )

    if (
        "interval: 1s" not in text
        or "script.execute: clock_screensaver_keep_on_top" not in text.split("interval:", 1)[-1]
    ):
        errors.append(f"{rel}: keep the active clock screensaver above overlays after it starts")

    dimmed_body = yaml_script_body(text, "show_dimmed_view")
    if dimmed_body is None:
        errors.append(f"{rel}: missing show_dimmed_view script")
    elif "lv_obj_move_foreground(id(dim_screensaver_touch_guard))" not in dimmed_body:
        errors.append(f"{rel}: raise the dim screensaver touch guard above any existing top-layer elements")

    return errors


def firmware_screen_schedule_screensaver_overlay_errors(cover_art_path: Path, root: Path) -> list[str]:
    errors: list[str] = []
    if not cover_art_path.exists():
        return errors

    rel = cover_art_path.relative_to(root)
    text = cover_art_path.read_text(encoding="utf-8")
    show_body = yaml_script_body(text, "show_cover_art_view")
    effect_body = yaml_script_body(text, "display_mode_effect_cover_art")

    if show_body is None:
        errors.append(f"{rel}: missing show_cover_art_view script")
    else:
        if "screen_schedule_blocks_cover_art(" not in show_body:
            errors.append(f"{rel}: prevent cover art from overriding active screen schedule night mode")
    if effect_body is None:
        errors.append(f"{rel}: missing display_mode_effect_cover_art script")
    elif "lv_obj_move_foreground(id(cover_art_screensaver))" not in effect_body:
        errors.append(f"{rel}: raise the cover art screensaver above any existing top-layer elements")

    delay_body = yaml_script_body(text, "cover_art_delay_timer")
    if delay_body is None:
        errors.append(f"{rel}: missing cover_art_delay_timer script")
    elif "screen_schedule_blocks_cover_art(" not in delay_body:
        errors.append(f"{rel}: keep delayed cover art from starting during screen schedule night mode")

    playback_started_body = yaml_script_body(text, "cover_art_playback_started")
    if playback_started_body is None:
        errors.append(f"{rel}: missing cover_art_playback_started script")
    elif "screen_schedule_blocks_cover_art(" not in playback_started_body:
        errors.append(f"{rel}: keep playback-start cover art from overriding screen schedule night mode")

    return errors


def firmware_screen_schedule_screensaver_override_errors(backlight_path: Path, root: Path) -> list[str]:
    errors: list[str] = []
    if not backlight_path.exists():
        return errors

    rel = backlight_path.relative_to(root)
    text = backlight_path.read_text(encoding="utf-8")

    idle_body = yaml_script_body(text, "screensaver_idle_check")
    if idle_body is None:
        errors.append(f"{rel}: missing screensaver_idle_check script")
    else:
        night_index = idle_body.find("screen_schedule_night_active(")
        mode_index = idle_body.find("return id(screensaver_mode).state")
        schedule_index = idle_body.find("id(screen_schedule_check).execute();", night_index)
        if night_index == -1 or schedule_index == -1 or (
            mode_index != -1 and schedule_index > mode_index
        ):
            errors.append(f"{rel}: let the night screen schedule override timer screensaver actions")

    schedule_off_body = yaml_script_body(text, "backlight_schedule_display_off")
    controller_off_body = yaml_script_body(text, "display_mode_effect_off")
    adapter_body = yaml_script_body(text, "display_mode_apply_transition")
    reconcile_body = yaml_script_body(text, "display_mode_reconcile")
    if schedule_off_body is None:
        errors.append(f"{rel}: missing backlight_schedule_display_off script")
    elif not (
        adapter_body is not None
        and "target_mode != static_cast<int>(espcontrol::DisplayMode::COVER_ART)" in adapter_body
        and "id: cover_art_hide_effect" in adapter_body
        and controller_off_body is not None
        and "script.stop: cover_art_delay_timer" in controller_off_body
    ) and (
        "script.stop: cover_art_delay_timer" not in schedule_off_body
        or "script.execute: hide_cover_art_view" not in schedule_off_body
    ):
        errors.append(f"{rel}: screen schedule display-off should clear cover art before forcing the screen off")

    if reconcile_body is not None and "DisplayRequestSource::SCREEN_SCHEDULE" in reconcile_body:
        controller_owns_cover_art = (
            adapter_body is not None
            and "espcontrol::DisplayMode::COVER_ART" in adapter_body
            and "id: cover_art_hide_effect" in adapter_body
            and "DisplayRequestSource::MEDIA_PLAYBACK" not in reconcile_body
        )
        legacy_clears_cover_art = (
            "if (schedule_night && id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART))" in reconcile_body
            or (
                "if (id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART))" in reconcile_body
                and "id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) = false;" in reconcile_body
                and "id(hide_cover_art_view).execute();" in reconcile_body
            )
        )
        if not controller_owns_cover_art and not legacy_clears_cover_art:
            errors.append(f"{rel}: scheduled night modes should clear active cover art before resolving")

    schedule_path = backlight_path.with_name("backlight_schedule.yaml")
    if schedule_path.exists():
        schedule_rel = schedule_path.relative_to(root)
        schedule_text = schedule_path.read_text(encoding="utf-8")
        brightness_body = yaml_script_body(schedule_text, "backlight_apply_brightness")
        if brightness_body is not None:
            active_target = brightness_body.find(
                "target_mode_is(\n                espcontrol::DisplayMode::ACTIVE)"
            )
            cover_art_target = brightness_body.find(
                "target_mode_is(\n                espcontrol::DisplayMode::COVER_ART)"
            )
            if active_target == -1 or cover_art_target == -1:
                errors.append(
                    f"{schedule_rel}: apply brightness from the pending active display target during wake transitions"
                )

            schedule_source = brightness_body.find(
                "DisplayRequestSource::SCREEN_SCHEDULE"
            )
            schedule_off = brightness_body.find(
                "target_mode_is(\n                  espcontrol::DisplayMode::DISPLAY_OFF)",
                schedule_source,
            )
            force_off = brightness_body.find(
                "id(backlight_force_display_off).execute();", schedule_off
            )
            always_on = brightness_body.find("id(schedule_mode_always_on)")
            if not (
                schedule_source != -1
                and schedule_off != -1
                and force_off != -1
                and always_on != -1
            ):
                errors.append(
                    f"{schedule_rel}: only blank display-off schedules so Always On can apply its configured brightness"
                )
        migrated_live_schedule = (
            reconcile_body is not None
            and "schedule_was_active" in reconcile_body
            and "DisplayRequestSource::BOOT_GUARD" in reconcile_body
            and "DisplayRequestSource::SCREEN_SCHEDULE" in reconcile_body
        )
        if migrated_live_schedule:
            boot_guard_index = schedule_text.find("script.execute: screen_schedule_boot_guard")
            boot_guard_prefix = schedule_text[:boot_guard_index] if boot_guard_index != -1 else ""
            if (
                "priority: -190" not in boot_guard_prefix
                or "screen_schedule_waiting_for_time(" not in boot_guard_prefix
                or "screen_schedule_night_active(" not in boot_guard_prefix
                or "script.execute: screen_schedule_boot_guard" not in schedule_text
            ):
                errors.append(
                    f"{schedule_rel}: publish the live fail-dark schedule before the loading screen can light the panel"
                )

            loading_path = backlight_path.parent.parent / "device" / "screen_loading.yaml"
            if loading_path.exists():
                loading_rel = loading_path.relative_to(root)
                loading_text = loading_path.read_text(encoding="utf-8")
                setup_index = loading_text.find("id: connectivity_setup_display_active")
                setup_true_index = loading_text.find("value: 'true'", setup_index)
                reconcile_index = loading_text.find("script.execute: display_mode_reconcile", setup_true_index)
                if not (0 <= setup_index < setup_true_index < reconcile_index):
                    errors.append(
                        f"{loading_rel}: bypass the boot guard before showing WiFi setup"
                    )
                if reconcile_body is None or (
                    "id(connectivity_setup_display_active)" not in reconcile_body
                    or "!connectivity_setup" not in reconcile_body
                ):
                    errors.append(
                        f"{rel}: let connectivity setup override boot guard and scheduled night requests"
                    )
        sleep_body = yaml_script_body(schedule_text, "screen_schedule_sleep")
        if sleep_body is None:
            errors.append(f"{schedule_rel}: missing screen_schedule_sleep script")
        else:
            asleep_index = sleep_body.find("id: screen_schedule_asleep")
            asleep_true_index = sleep_body.find("value: 'true'", asleep_index)
            request_index = sleep_body.find("DisplayRequestSource::SCREEN_SCHEDULE")
            reconcile_index = sleep_body.find("script.execute: display_mode_reconcile")
            controller_reconciles_live_schedule = (
                "screen_schedule_night_active(" in text
                and "controller.request(espcontrol::DisplayRequestSource::SCREEN_SCHEDULE" in text
                and "schedule_was_active" in text
                and "controller.clear(espcontrol::DisplayRequestSource::IDLE_TIMER)" in text
                and "controller.clear(espcontrol::DisplayRequestSource::PRESENCE_SENSOR)" in text
                and "restore_value: true" not in text[text.find("id: screen_schedule_asleep"):text.find("id: backlight_manual_off")]
                and "id: screen_schedule_asleep" not in sleep_body
                and reconcile_index != -1
            )
            if not controller_reconciles_live_schedule and not (
                0 <= asleep_index <= asleep_true_index < request_index < reconcile_index
            ):
                errors.append(
                    f"{schedule_rel}: set the schedule-asleep marker before reconciling display-off"
                )

    wake_body = yaml_script_body(text, "screensaver_presence_wake")
    if wake_body is None:
        errors.append(f"{rel}: missing screensaver_presence_wake script")
    else:
        typed_presence_wake = (
            "presence_can_wake_display(" in wake_body
            and "script.execute: screensaver_wake" in wake_body
        )
        controller_presence_wake = (
            not typed_presence_wake
            and "script.execute: display_mode_clear_automatic" in wake_body
            and "screen_schedule_night_active(" in text
            and "DisplayRequestSource::SCREEN_SCHEDULE" in text
        )
        if typed_presence_wake and "script.execute: display_mode_clear_automatic" not in wake_body:
            errors.append(
                f"{rel}: clear stale automatic sleep when presence leaves the visible mode unchanged"
            )
        if typed_presence_wake and "id(cover_art_screensaver_active)" in wake_body:
            errors.append(f"{rel}: leave active cover art unchanged on presence detection")
        if typed_presence_wake:
            reconcile_index = wake_body.find("script.execute: screen_schedule_check")
            sensor_guard_index = wake_body.find("screen_schedule_sensor_trigger(")
            wake_action_index = wake_body.find("script.execute: screensaver_wake")
            if not (0 <= reconcile_index < sensor_guard_index < wake_action_index):
                errors.append(
                    f"{rel}: reconcile sensor-triggered night schedule before presence wake behavior"
                )
        if controller_presence_wake and (
            "target_mode_is(\n                          espcontrol::DisplayMode::COVER_ART)" not in wake_body
            or "script.execute: screensaver_wake" not in wake_body
        ):
            errors.append(f"{rel}: clear cover art when presence wakes the screensaver")
        wake_index = wake_body.find("script.execute: screensaver_wake")
        pre_wake_body = wake_body[:wake_index] if wake_index != -1 else wake_body
        required_tokens = (
            "screen_schedule_waiting_for_time(",
            "screen_schedule_night_active(",
            "id(screen_schedule_check).execute();",
        )
        if not typed_presence_wake and not controller_presence_wake and (
            wake_index == -1 or any(token not in pre_wake_body for token in required_tokens)
        ):
            errors.append(f"{rel}: let the night screen schedule override sensor screensaver wake")
        disabled_wake_index = pre_wake_body.rfind("if (!id(schedule_enabled).state) return true;")
        schedule_check_index = pre_wake_body.find("id(screen_schedule_check).execute();")
        if not typed_presence_wake and not controller_presence_wake and (disabled_wake_index == -1 or (
            schedule_check_index != -1 and disabled_wake_index < schedule_check_index
        )):
            errors.append(f"{rel}: let sensor screensaver wake when the screen schedule is disabled")

    presence_sleep_body = yaml_script_body(text, "screensaver_presence_sleep")
    if presence_sleep_body is None:
        errors.append(f"{rel}: missing screensaver_presence_sleep script")
    else:
        reconcile_index = presence_sleep_body.find("script.execute: screen_schedule_check")
        sensor_guard_index = presence_sleep_body.find("screen_schedule_sensor_trigger(")
        sleep_action_index = presence_sleep_body.find("script.execute: screensaver_sleep_sensor")
        if not (0 <= reconcile_index < sensor_guard_index < sleep_action_index):
            errors.append(
                f"{rel}: reconcile sensor-triggered night schedule before presence sleep behavior"
            )

    return errors


def firmware_climate_step_errors(firmware_dir: Path, root: Path) -> list[str]:
    path = firmware_dir / "button_grid_climate.h"
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    helper = re.search(
        r"inline\s+int\s+climate_effective_step_tenths\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        re.DOTALL,
    )
    if not helper:
        errors.append(f"{rel}: keep climate temperature changes at a display-appropriate minimum")
    else:
        body = helper.group("body")
        if (
            "CLIMATE_DEFAULT_STEP_TENTHS" not in body
            or "CLIMATE_WHOLE_NUMBER_STEP_TENTHS" not in body
            or "ctx->configured_step_tenths" not in body
            or "return ctx->configured_step_tenths" not in body
            or "ctx->step_tenths > minimum" in body
        ):
            errors.append(f"{rel}: use the configured climate temperature step")
    if "int step = climate_effective_step_tenths(ctx);" not in text:
        errors.append(f"{rel}: round climate targets using the display-appropriate minimum step")
    if "int base = ctx->precision <= 0 ? 0 : ctx->min_tenths;" not in text:
        errors.append(f"{rel}: round whole-number climate targets to whole-degree boundaries")
    if 'cfg_option_value(p.options, "temperature_step")' not in text:
        errors.append(f"{rel}: use the configured climate temperature step")
    if "climate_selected_target(ui.active) - ui.active->step_tenths" in text:
        errors.append(f"{rel}: use the display-appropriate minimum step for the climate minus button")
    if "climate_selected_target(ui.active) + ui.active->step_tenths" in text:
        errors.append(f"{rel}: use the display-appropriate minimum step for the climate plus button")
    return errors


def firmware_s3_api_errors(
    device_path: Path,
    s3_packages_path: Path,
    core_infra_path: Path,
    api_navigate_path: Path,
    package_paths: tuple[Path, ...],
    root: Path,
) -> list[str]:
    if not device_path.exists():
        return []
    rel = device_path.relative_to(root)
    text = device_path.read_text(encoding="utf-8")
    errors: list[str] = []

    queue_match = re.search(r"(?m)^\s*max_send_queue:\s*(\d+)\s*$", text)
    if not queue_match:
        errors.append(f"{rel}: set an explicit S3 native API send queue")
    elif int(queue_match.group(1)) < 12:
        errors.append(f"{rel}: keep the S3 native API send queue high enough for HA reconnect bursts")
    connections_match = re.search(r"(?m)^\s*max_connections:\s*(\d+)\s*$", text)
    if not connections_match:
        errors.append(f"{rel}: set an explicit S3 native API connection pool")
    elif int(connections_match.group(1)) < 3:
        errors.append(f"{rel}: keep enough S3 native API slots for HA reconnects after OTA")
    if "ESPCONTROL_DISABLE_TODO=1" not in text:
        errors.append(f"{rel}: keep the S3 todo list disabled until its HA action response path is stable")

    if api_navigate_path.exists():
        api_rel = api_navigate_path.relative_to(root)
        api_text = api_navigate_path.read_text(encoding="utf-8")
        if "action: navigate" not in api_text:
            errors.append(f"{api_rel}: keep the Home Assistant navigate action in the dedicated API package")
    else:
        errors.append(f"{api_navigate_path.relative_to(root)}: missing dedicated Home Assistant navigate API package")

    if core_infra_path.exists():
        core_rel = core_infra_path.relative_to(root)
        core_text = core_infra_path.read_text(encoding="utf-8")
        if "action: navigate" in core_text:
            errors.append(f"{core_rel}: keep the navigate action out of core_infra so S3 can omit it")

    if s3_packages_path.exists():
        s3_rel = s3_packages_path.relative_to(root)
        s3_packages = s3_packages_path.read_text(encoding="utf-8")
        if "api_navigate" in s3_packages or "api_navigate.yaml" in s3_packages:
            errors.append(f"{s3_rel}: omit the Home Assistant navigate API action on S3")

    for package_path in package_paths:
        if package_path == s3_packages_path or not package_path.exists():
            continue
        if not package_api_navigate_enabled(package_path, root):
            continue
        package_rel = package_path.relative_to(root)
        package_text = package_path.read_text(encoding="utf-8")
        if "api_navigate" not in package_text or "api_navigate.yaml" not in package_text:
            errors.append(f"{package_rel}: include the dedicated Home Assistant navigate API package")
    return errors


def firmware_navigation_target_errors(
    firmware_dir: Path,
    api_navigate_path: Path,
    package_paths: tuple[Path, ...],
    root: Path,
) -> list[str]:
    errors: list[str] = []
    navigation_path = firmware_dir / "button_grid_navigation.h"
    navigation_driver_path = firmware_dir / "button_grid_navigation_driver.h"
    grid_path = firmware_dir / "button_grid_grid.h"

    if not navigation_path.exists():
        errors.append("components/espcontrol/button_grid_navigation.h: keep Home Assistant navigation targets available")
        return errors
    navigation_rel = navigation_path.relative_to(root)
    navigation_text = navigation_path.read_text(encoding="utf-8")
    if "NavigationHomeTargetEntry" not in navigation_text or "navigation_home_targets()" not in navigation_text:
        errors.append(f"{navigation_rel}: register general home-screen navigation targets")
    if "navigation_find_label_target" not in navigation_text or "navigation_home_targets()" not in navigation_text:
        errors.append(f"{navigation_rel}: resolve navigate labels against home-screen cards")
    if "navigation_has_home_label_target" not in navigation_text:
        errors.append(f"{navigation_rel}: let configured card labels take priority over voice aliases")
    if "entry.display_order < best->display_order" not in navigation_text:
        errors.append(f"{navigation_rel}: choose the first displayed card when labels are duplicated")
    if "navigation_find_slot_target" not in navigation_text or "entry.slot == slot" not in navigation_text:
        errors.append(f"{navigation_rel}: resolve slot:n against home-screen card slots")
    if "navigation_return_home(main_page_obj)" not in navigation_text or "handle_button_click(target->config, target->slot, target->button)" not in navigation_text:
        errors.append(f"{navigation_rel}: activate navigated home-screen cards through the normal tap handler")
    if "navigation_is_voice_target" not in navigation_text or '"device_volume"' not in navigation_text:
        errors.append(f"{navigation_rel}: reserve voice volume navigation aliases")
    if "normalized == \"home\" || normalized == \"main\"" not in navigation_text:
        errors.append(f"{navigation_rel}: preserve home/main navigation targets")

    if not grid_path.exists():
        errors.append("components/espcontrol/button_grid_grid.h: register home-screen navigation targets during grid refresh")
    else:
        grid_rel = grid_path.relative_to(root)
        grid_text = grid_path.read_text(encoding="utf-8")
        if grid_text.count("navigation_clear_home_targets();") < 2:
            errors.append(f"{grid_rel}: refresh home-screen navigation targets when the displayed grid order changes")
        if "navigation_register_home_target(idx, pos, p.label, scfg, s.btn);" not in grid_text:
            errors.append(f"{grid_rel}: register every displayed home-screen card for Home Assistant navigation")
        if "navigation_register_home_target(idx, pos, p.label, s.config->state, s.btn);" not in grid_text:
            errors.append(f"{grid_rel}: refresh displayed home-screen card targets during layout-only updates")
    if not navigation_driver_path.exists():
        errors.append("components/espcontrol/button_grid_navigation_driver.h: preserve subpage navigation registration")
    else:
        navigation_driver_rel = navigation_driver_path.relative_to(root)
        navigation_driver_text = navigation_driver_path.read_text(encoding="utf-8")
        if (
            "navigation_driver_own_subpage(" not in navigation_driver_text
            or "navigation_register_subpage(" not in navigation_driver_text
        ):
            errors.append(f"{navigation_driver_rel}: preserve subpage navigation registration")

    if not api_navigate_path.exists():
        errors.append("common/device/api_navigate.yaml: route voice aliases through the navigate action")
    else:
        api_rel = api_navigate_path.relative_to(root)
        api_text = api_navigate_path.read_text(encoding="utf-8")
        if "navigation_is_voice_target(target)" not in api_text or "${navigate_voice_target_code}" not in api_text:
            errors.append(f"{api_rel}: route reserved voice targets through the device-specific voice hook")
        if "!navigation_has_home_label_target(target)" not in api_text:
            errors.append(f"{api_rel}: resolve configured card labels before reserved voice aliases")
        if "espcontrol_navigate(target, id(main_page)->obj);" not in api_text:
            errors.append(f"{api_rel}: keep normal navigate targets routed through espcontrol_navigate")

    voice_package_found = False
    for package_path in package_paths:
        if not package_path.exists():
            continue
        package_rel = package_path.relative_to(root)
        package_text = package_path.read_text(encoding="utf-8")
        if "navigate_voice_target_code" not in package_text:
            errors.append(f"{package_rel}: define a voice-target navigate hook")
        if package_local_voice_services_enabled(package_path, root):
            voice_package_found = True
            if "id(open_device_volume_control).execute();" not in package_text:
                errors.append(f"{package_rel}: open the local voice volume modal for voice navigation aliases")
            if "id(voice_services_enabled).state" not in package_text:
                errors.append(f"{package_rel}: only open the voice volume modal when Voice Services are enabled")
        elif "open_device_volume_control" in package_text:
            errors.append(f"{package_rel}: keep the voice volume modal hook limited to local voice service packages")
    if not voice_package_found:
        errors.append("devices/manifest.json: define a voice volume navigate hook for a local voice service package")
    return errors


def firmware_todo_disabled_errors(device_paths: tuple[Path, ...], root: Path) -> list[str]:
    errors: list[str] = []
    for path in device_paths:
        if not path.exists():
            continue
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        if "ESPCONTROL_DISABLE_TODO=1" not in text:
            errors.append(f"{rel}: keep the todo list disabled on every device")
    return errors


def firmware_connectivity_api_errors(paths: tuple[Path, ...], root: Path) -> list[str]:
    errors: list[str] = []
    for path in paths:
        if not path.exists():
            continue
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        if re.search(r"(?m)^\s*api\.connected:\s*$", text):
            errors.append(f"{rel}: wait for Home Assistant state subscription, not any API client")
        if "on_client_connected:" in text and "ha_api_state_connected()" not in text:
            errors.append(f"{rel}: only navigate after a Home Assistant state connection is ready")
        api_connected_match = re.search(
            r"(?ms)^api:\n(?P<body>.*?)(?:^\S|\Z)",
            text,
        )
        if api_connected_match and "on_client_connected:" in api_connected_match.group("body"):
            api_connected_body = api_connected_match.group("body")
            if "ha_reconnect_flow" in api_connected_body:
                errors.append(f"{rel}: do not manage a Home Assistant waiting screen on reconnect")
            if "script.execute: ha_restore_after_api" not in api_connected_body:
                errors.append(f"{rel}: continue initial setup when Home Assistant connects")
            if "wait_until:" in api_connected_body or "timeout: 2s" in api_connected_body:
                errors.append(f"{rel}: do not delay initial setup when Home Assistant connects")
        restore_body = yaml_script_body(text, "ha_restore_after_api")
        if restore_body is None:
            errors.append(f"{rel}: define the Home Assistant initial-setup continuation script")
        elif (
            "ha_api_connected()" not in restore_body
            or "lv_scr_act() == id(ha_setup_page)->obj" not in restore_body
            or restore_body.count("script.execute: navigate_after_api") != 1
        ):
            errors.append(f"{rel}: only navigate away when the initial Home Assistant setup page is active")
        body = yaml_script_body(text, "ha_reconnect_flow")
        if body is not None or "Connecting to\\nHome Assistant" in text:
            errors.append(f"{rel}: keep the current display visible when Home Assistant disconnects")
    return errors


def firmware_ha_connection_screen_errors(core_infra_path: Path, root: Path) -> list[str]:
    if not core_infra_path.exists():
        return []
    rel = core_infra_path.relative_to(root)
    text = core_infra_path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "ha_reconnect_flow" in text:
        errors.append(f"{rel}: do not start a display flow when Home Assistant disconnects")
    if "on_client_connected:" not in text or "id(ha_restore_after_api).execute();" not in text:
        errors.append(f"{rel}: continue initial setup when Home Assistant connects")
    if "apply_registered_ha_control_availability" in text:
        errors.append(f"{rel}: do not dim registered cards when HA disconnects")
    return errors


def firmware_c6_update_status_errors(path: Path, root: Path) -> list[str]:
    if not path.exists():
        return []
    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "UPDATE_STATE_UNKNOWN" not in text:
        errors.append(f"{rel}: keep the C6 update status unknown until ESPHome checks for updates")
    latest_match = re.search(
        r"(?ms)id:\s*c6_update_latest_firmware\b(?P<body>.*?)(?:^button:|\Z)",
        text,
    )
    if not latest_match:
        errors.append(f"{rel}: define the C6 latest firmware sensor")
    else:
        latest_body = latest_match.group("body")
        if (
            "UPDATE_STATE_NO_UPDATE" not in latest_body
            or "upd->update_info.current_version" not in latest_body
        ):
            errors.append(f"{rel}: report the current C6 firmware as available when no update exists")
    return errors


def run_scan() -> int:
    errors = firmware_ha_binding_errors(FIRMWARE_DIR, ROOT)
    errors.extend(firmware_display_controller_ownership_errors(DISPLAY_LIFECYCLE_ROOTS, ROOT))
    errors.extend(firmware_ha_boundary_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_todo_request_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_todo_disconnect_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_action_card_availability_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_card_disabled_state_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_media_card_availability_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_action_card_script_fields_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_local_sensor_binding_order_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_time_reconnect_errors(TIME_ADDON_PATH, ROOT))
    errors.extend(firmware_ntp_startup_errors(TIME_ADDON_PATH, SUN_CALC_PATH, CONNECTIVITY_PATHS, ROOT))
    errors.extend(firmware_weather_request_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_weather_disconnect_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_weather_reconnect_errors(CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_unavailable_retry_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_cover_request_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_cover_art_external_input_errors(COVER_ART_PATH, ROOT))
    errors.extend(firmware_cover_art_stale_image_errors(COVER_ART_PATH, ROOT))
    errors.extend(firmware_cover_art_refresh_errors(COVER_ART_PATH, ROOT))
    errors.extend(firmware_cover_art_playback_grace_errors(COVER_ART_PATH, ROOT))
    errors.extend(firmware_cover_art_disable_errors(COVER_ART_PATH, ROOT))
    errors.extend(firmware_cover_art_lifecycle_controller_errors(BACKLIGHT_PATH, COVER_ART_PATH, ROOT))
    errors.extend(firmware_media_sleep_prevention_errors(BACKLIGHT_PATH, DISPLAY_CONFIG_PATH, COVER_ART_PATH, ROOT))
    errors.extend(firmware_touch_cover_art_delay_errors(DEVICE_TOUCH_PATHS, ROOT))
    errors.extend(firmware_media_sleep_prevention_subscription_errors(DEVICE_SENSOR_PATHS, ROOT))
    errors.extend(firmware_media_control_low_heap_metadata_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_cover_art_low_heap_progress_errors(FIRMWARE_DIR, COVER_ART_PATH, ROOT))
    errors.extend(firmware_cover_art_progress_visibility_errors(COVER_ART_PATH, ROOT))
    errors.extend(firmware_image_card_entity_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_image_card_base_url_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_image_card_quality_errors(FIRMWARE_DIR, ROOT))
    errors.extend(firmware_image_card_startup_errors(FIRMWARE_DIR, CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_artwork_image_auth_errors(ARTWORK_IMAGE_PATH, ROOT))
    errors.extend(
        firmware_screensaver_wake_guard_errors(
            BACKLIGHT_PATH, SCREEN_CLOCK_PATH, COVER_ART_PATH, ROOT
        )
    )
    errors.extend(firmware_screen_wake_button_errors(BACKLIGHT_PATH, ROOT))
    errors.extend(firmware_clock_bar_pending_wake_errors(DISPLAY_CONFIG_PATH, ROOT))
    errors.extend(firmware_clock_screensaver_overlay_errors(BACKLIGHT_PATH, ROOT))
    errors.extend(firmware_screen_schedule_screensaver_overlay_errors(COVER_ART_PATH, ROOT))
    errors.extend(firmware_screen_schedule_screensaver_override_errors(BACKLIGHT_PATH, ROOT))
    errors.extend(firmware_climate_step_errors(FIRMWARE_DIR, ROOT))
    errors.extend(
        firmware_s3_api_errors(
            S3_DEVICE_PATH,
            S3_PACKAGES_PATH,
            CORE_INFRA_PATH,
            API_NAVIGATE_PATH,
            DEVICE_PACKAGE_PATHS,
            ROOT,
        )
    )
    errors.extend(firmware_navigation_target_errors(FIRMWARE_DIR, API_NAVIGATE_PATH, DEVICE_PACKAGE_PATHS, ROOT))
    errors.extend(firmware_todo_disabled_errors(DEVICE_DEVICE_PATHS, ROOT))
    errors.extend(firmware_connectivity_api_errors(CONNECTIVITY_PATHS, ROOT))
    errors.extend(firmware_ha_connection_screen_errors(CORE_INFRA_PATH, ROOT))
    errors.extend(firmware_c6_update_status_errors(C6_FIRMWARE_UPDATE_PATH, ROOT))
    if errors:
        print("Firmware Home Assistant binding check failed:")
        for error in errors:
            print(f"  {error}")
        return 1
    print("Firmware Home Assistant binding checks passed.")
    return 0


def expect_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        for filename, text in files.items():
            (firmware_dir / filename).write_text(text, encoding="utf-8")

        errors = firmware_ha_binding_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_ha_boundary_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        for filename, text in files.items():
            (firmware_dir / filename).write_text(text, encoding="utf-8")

        errors = firmware_ha_boundary_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_unavailable_retry_errors(
    name: str,
    config_text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_config.h").write_text(config_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_unavailable_retry_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_todo_request_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(text, encoding="utf-8")

        errors = firmware_todo_request_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_todo_disconnect_errors(
    name: str,
    todo_text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(todo_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_todo_disconnect_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_action_card_availability_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_grid.h").write_text(text, encoding="utf-8")

        errors = firmware_action_card_availability_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_media_card_availability_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_media.h").write_text(text, encoding="utf-8")

        errors = firmware_media_card_availability_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_action_card_script_fields_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_actions.h").write_text(text, encoding="utf-8")

        errors = firmware_action_card_script_fields_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_local_sensor_binding_order_errors(
    name: str, files: dict[str, str], expected: tuple[str, ...]
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        for filename, text in files.items():
            (firmware_dir / filename).write_text(text, encoding="utf-8")

        errors = firmware_local_sensor_binding_order_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_todo_disabled_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        paths = []
        for filename, text in files.items():
            path = root / filename
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
            paths.append(path)

        errors = firmware_todo_disabled_errors(tuple(paths), root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_time_reconnect_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        time_path = root / "common" / "addon" / "time.yaml"
        time_path.parent.mkdir(parents=True)
        time_path.write_text(text, encoding="utf-8")

        errors = firmware_time_reconnect_errors(time_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_ntp_startup_errors(
    name: str,
    time_text: str,
    sun_calc_text: str,
    connectivity_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        time_path = root / "common" / "addon" / "time.yaml"
        sun_calc_path = root / "components" / "espcontrol" / "sun_calc.h"
        connectivity_path = root / "common" / "addon" / "connectivity.yaml"
        time_path.parent.mkdir(parents=True, exist_ok=True)
        sun_calc_path.parent.mkdir(parents=True, exist_ok=True)
        connectivity_path.parent.mkdir(parents=True, exist_ok=True)
        time_path.write_text(time_text, encoding="utf-8")
        sun_calc_path.write_text(sun_calc_text, encoding="utf-8")
        connectivity_path.write_text(connectivity_text, encoding="utf-8")

        errors = firmware_ntp_startup_errors(
            time_path,
            sun_calc_path,
            (connectivity_path,),
            root,
        )
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_weather_request_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_config.h").write_text(text, encoding="utf-8")

        errors = firmware_weather_request_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_weather_disconnect_errors(
    name: str,
    config_text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_config.h").write_text(config_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_weather_disconnect_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_weather_reconnect_errors(name: str, core_text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        core_path = root / "common" / "device" / "core_infra.yaml"
        core_path.parent.mkdir(parents=True)
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_weather_reconnect_errors(core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_request_errors(
    name: str,
    action_text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_actions.h").write_text(action_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_cover_request_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_external_input_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "device" / "screen_cover_art.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_cover_art_external_input_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_stale_image_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "device" / "screen_cover_art.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_cover_art_stale_image_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_refresh_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "device" / "screen_cover_art.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_cover_art_refresh_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_playback_grace_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "device" / "screen_cover_art.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_cover_art_playback_grace_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_disable_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "device" / "screen_cover_art.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_cover_art_disable_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_lifecycle_controller_errors(
    name: str,
    backlight_text: str,
    cover_art_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        backlight_path = root / "common" / "addon" / "backlight.yaml"
        cover_art_path = root / "common" / "device" / "screen_cover_art.yaml"
        backlight_path.parent.mkdir(parents=True)
        cover_art_path.parent.mkdir(parents=True)
        backlight_path.write_text(backlight_text, encoding="utf-8")
        cover_art_path.write_text(cover_art_text, encoding="utf-8")

        errors = firmware_cover_art_lifecycle_controller_errors(
            backlight_path, cover_art_path, root
        )
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_media_sleep_prevention_errors(
    name: str,
    backlight_text: str,
    display_text: str,
    cover_art_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        backlight_path = root / "common" / "addon" / "backlight.yaml"
        display_path = root / "common" / "config" / "display.yaml"
        cover_art_path = root / "common" / "device" / "screen_cover_art.yaml"
        backlight_path.parent.mkdir(parents=True)
        display_path.parent.mkdir(parents=True)
        cover_art_path.parent.mkdir(parents=True)
        backlight_path.write_text(backlight_text, encoding="utf-8")
        display_path.write_text(display_text, encoding="utf-8")
        cover_art_path.write_text(cover_art_text, encoding="utf-8")

        errors = firmware_media_sleep_prevention_errors(backlight_path, display_path, cover_art_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_media_control_low_heap_metadata_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_media.h").write_text(text, encoding="utf-8")

        errors = firmware_media_control_low_heap_metadata_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_low_heap_progress_errors(
    name: str,
    media_text: str,
    cover_art_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        cover_art_path = root / "common" / "device" / "screen_cover_art.yaml"
        firmware_dir.mkdir(parents=True)
        cover_art_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_media.h").write_text(media_text, encoding="utf-8")
        cover_art_path.write_text(cover_art_text, encoding="utf-8")

        errors = firmware_cover_art_low_heap_progress_errors(firmware_dir, cover_art_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_cover_art_progress_visibility_errors(
    name: str, text: str, expected: tuple[str, ...]
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "device" / "screen_cover_art.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_cover_art_progress_visibility_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_image_card_entity_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_image.h").write_text(text, encoding="utf-8")

        errors = firmware_image_card_entity_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_image_card_base_url_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_image.h").write_text(text, encoding="utf-8")

        errors = firmware_image_card_base_url_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_image_card_quality_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_image.h").write_text(text, encoding="utf-8")

        errors = firmware_image_card_quality_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_image_card_startup_errors(
    name: str,
    text: str,
    core_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        core_path = root / "common" / "device" / "core_infra.yaml"
        firmware_dir.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_image.h").write_text(text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")

        errors = firmware_image_card_startup_errors(firmware_dir, core_path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_screensaver_wake_guard_errors(
    name: str,
    backlight_text: str,
    screen_clock_text: str,
    cover_art_text: str,
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        backlight_path = root / "common" / "addon" / "backlight.yaml"
        screen_clock_path = root / "common" / "device" / "screen_clock.yaml"
        cover_art_path = root / "common" / "device" / "screen_cover_art.yaml"
        backlight_path.parent.mkdir(parents=True)
        screen_clock_path.parent.mkdir(parents=True)
        cover_art_path.parent.mkdir(parents=True, exist_ok=True)
        backlight_path.write_text(backlight_text, encoding="utf-8")
        screen_clock_path.write_text(screen_clock_text, encoding="utf-8")
        cover_art_path.write_text(cover_art_text, encoding="utf-8")

        errors = firmware_screensaver_wake_guard_errors(
            backlight_path, screen_clock_path, cover_art_path, root
        )
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_screen_wake_button_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "addon" / "backlight.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")
        errors = firmware_screen_wake_button_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_clock_screensaver_overlay_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "addon" / "backlight.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")
        errors = firmware_clock_screensaver_overlay_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_screen_schedule_screensaver_override_errors(
    name: str,
    text: str,
    expected: tuple[str, ...],
    schedule_text: str | None = None,
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "addon" / "backlight.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")
        if schedule_text is not None:
            path.with_name("backlight_schedule.yaml").write_text(
                schedule_text, encoding="utf-8"
            )
        errors = firmware_screen_schedule_screensaver_override_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_artwork_image_auth_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "components" / "artwork_image" / "artwork_image.cpp"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_artwork_image_auth_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_climate_step_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_climate.h").write_text(text, encoding="utf-8")

        errors = firmware_climate_step_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_s3_api_errors(
    name: str,
    text: str,
    expected: tuple[str, ...],
    s3_packages_text: str = "packages:\n  device: !include device/device.yaml\n",
    core_text: str = "api:\n  reboot_timeout: 0s\n",
    api_navigate_text: str = "api:\n  actions:\n    - action: navigate\n",
    extra_packages: dict[str, str] | None = None,
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        device_path = root / "devices" / "guition-esp32-s3-4848s040" / "device" / "device.yaml"
        s3_packages_path = root / "devices" / "guition-esp32-s3-4848s040" / "packages.yaml"
        core_path = root / "common" / "device" / "core_infra.yaml"
        api_navigate_path = root / "common" / "device" / "api_navigate.yaml"
        device_path.parent.mkdir(parents=True)
        core_path.parent.mkdir(parents=True)
        device_path.write_text(text, encoding="utf-8")
        s3_packages_path.write_text(s3_packages_text, encoding="utf-8")
        core_path.write_text(core_text, encoding="utf-8")
        api_navigate_path.write_text(api_navigate_text, encoding="utf-8")
        package_paths = [s3_packages_path]
        for filename, package_text in (extra_packages or {}).items():
            package_path = root / "devices" / filename / "packages.yaml"
            package_path.parent.mkdir(parents=True)
            package_path.write_text(package_text, encoding="utf-8")
            package_paths.append(package_path)

        errors = firmware_s3_api_errors(
            device_path,
            s3_packages_path,
            core_path,
            api_navigate_path,
            tuple(package_paths),
            root,
        )
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_navigation_target_errors(
    name: str,
    navigation_text: str,
    grid_text: str,
    navigation_driver_text: str,
    api_text: str,
    package_texts: dict[str, str],
    local_voice_slugs: tuple[str, ...],
    expected: tuple[str, ...],
) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        api_path = root / "common" / "device" / "api_navigate.yaml"
        firmware_dir.mkdir(parents=True)
        api_path.parent.mkdir(parents=True)
        (firmware_dir / "button_grid_navigation.h").write_text(navigation_text, encoding="utf-8")
        (firmware_dir / "button_grid_grid.h").write_text(grid_text, encoding="utf-8")
        (firmware_dir / "button_grid_navigation_driver.h").write_text(
            navigation_driver_text, encoding="utf-8")
        api_path.write_text(api_text, encoding="utf-8")
        manifest_path = root / "devices" / "manifest.json"
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        manifest_path.write_text(
            json.dumps(
                {
                    "devices": {
                        slug: {"firmware": {"package": {"localVoiceServices": slug in local_voice_slugs}}}
                        for slug in package_texts
                    }
                }
            ),
            encoding="utf-8",
        )
        package_paths: list[Path] = []
        for slug, package_text in package_texts.items():
            package_path = root / "devices" / slug / "packages.yaml"
            package_path.parent.mkdir(parents=True)
            package_path.write_text(package_text, encoding="utf-8")
            package_paths.append(package_path)

        errors = firmware_navigation_target_errors(firmware_dir, api_path, tuple(package_paths), root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_connectivity_api_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "addon" / "connectivity.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_connectivity_api_errors((path,), root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_c6_update_status_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "common" / "device" / "esp32_c6_firmware_update.yaml"
        path.parent.mkdir(parents=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_c6_update_status_errors(path, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def run_self_test() -> int:
    expect_screen_wake_button_errors(
        "missing Screen: Wake button",
        "script:\n  - id: screensaver_wake\n",
        ("expose Screen: Wake as a template button",),
    )
    expect_screen_wake_button_errors(
        "hidden incomplete Screen: Wake button",
        "button:\n"
        "  - platform: template\n"
        "    name: \"${entity_screen_wake}\"\n"
        "    icon: mdi:power\n"
        "    internal: true\n"
        "    disabled_by_default: true\n"
        "    entity_category: config\n"
        "    on_press:\n"
        "      - script.execute: screensaver_wake\n",
        (
            "keep Screen: Wake exposed to Home Assistant",
            "enable Screen: Wake by default",
            "keep Screen: Wake operational",
            "use mdi:monitor-arrow-up",
            "complete touch-equivalent Screen: Wake sequence",
        ),
    )
    expect_screen_wake_button_errors(
        "complete Screen: Wake button",
        "button:\n"
        "  - platform: template\n"
        "    name: \"${entity_screen_wake}\"\n"
        "    icon: mdi:monitor-arrow-up\n"
        "    disabled_by_default: false\n"
        "    on_press:\n"
        "      - script.execute: cover_art_pause_after_touch\n"
        "      - script.wait: cover_art_pause_after_touch\n"
        "      - script.execute: screensaver_wake\n"
        "script:\n"
        "  - id: screensaver_wake\n",
        (),
    )
    expect_c6_update_status_errors(
        "missing c6 no-update fallback",
        'text_sensor:\n'
        '  - platform: template\n'
        '    id: c6_update_available\n'
        '    lambda: |-\n'
        '      if (upd->state == esphome::update::UpdateState::UPDATE_STATE_UNKNOWN) return "Unknown";\n'
        '  - platform: template\n'
        '    id: c6_update_latest_firmware\n'
        '    lambda: |-\n'
        '      auto &lat = upd->update_info.latest_version;\n'
        '      if (lat.empty()) return "Unknown";\n'
        'button:\n',
        ("report the current C6 firmware as available when no update exists",),
    )
    expect_c6_update_status_errors(
        "missing c6 unknown status",
        'text_sensor:\n'
        '  - platform: template\n'
        '    id: c6_update_available\n'
        '    lambda: |-\n'
        '      return "Up-to-date";\n'
        '  - platform: template\n'
        '    id: c6_update_latest_firmware\n'
        '    lambda: |-\n'
        '      if (upd->state == esphome::update::UpdateState::UPDATE_STATE_NO_UPDATE) {\n'
        '        auto &cur = upd->update_info.current_version;\n'
        '        if (!cur.empty()) return cur;\n'
        '      }\n'
        'button:\n',
        ("keep the C6 update status unknown until ESPHome checks for updates",),
    )
    expect_errors(
        "direct action send",
        {"button_grid_actions.h": "esphome::api::global_api_server->send_homeassistant_action(req);\n"},
        ("access Home Assistant API through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct action send through reference",
        {"button_grid_actions.h": "api.send_homeassistant_action(req);\n"},
        ("send Home Assistant actions through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct state subscription",
        {"button_grid_media.h": "api->subscribe_home_assistant_state(entity, {}, cb);\n"},
        ("subscribe to Home Assistant state through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct state subscription through reference",
        {"button_grid_media.h": "api.subscribe_home_assistant_state(entity, {}, cb);\n"},
        ("subscribe to Home Assistant state through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct state get",
        {"button_grid_media.h": "api->get_home_assistant_state(entity, {}, cb);\n"},
        ("get Home Assistant state through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct state get through reference",
        {"button_grid_media.h": "api.get_home_assistant_state(entity, {}, cb);\n"},
        ("get Home Assistant state through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct callback registration",
        {"button_grid_alarm.h": "api->register_action_response_callback(id, cb);\n"},
        ("register action callbacks through button_grid_ha.h helpers",),
    )
    expect_errors(
        "direct callback cancellation",
        {"button_grid_alarm.h": "api.handle_action_response(id, false, error);\n"},
        ("cancel action callbacks through button_grid_ha.h helpers",),
    )
    expect_errors(
        "helper boundary",
        {"button_grid_ha.h": "esphome::api::global_api_server->send_homeassistant_action(req);\n"},
        (),
    )
    expect_errors(
        "helper use",
        {"button_grid_media.h": "ha_subscribe_state(entity, cb);\n"},
        (),
    )
    expect_ha_boundary_errors(
        "missing callback cancel helper",
        {
            "button_grid_ha.h": (
                "inline bool ha_subscribe_state() {\n  return true;\n}\n"
                "inline bool ha_subscribe_attribute() {\n  return true;\n}\n"
                "inline bool ha_action_send() {\n  return ha_api_state_connected() && HA_ACTION_INTERNAL_FREE_MIN_BYTES;\n}\n"
            )
        },
        ("expose a helper to cancel stale HA action response callbacks",),
    )
    expect_ha_boundary_errors(
        "action send only checks socket",
        {
            "button_grid_ha.h": (
                "inline bool ha_subscribe_state() {\n  return true;\n}\n"
                "inline bool ha_subscribe_attribute() {\n  return true;\n}\n"
                "inline bool ha_cancel_action_response_callback() {\n  handle_action_response(); return true;\n}\n"
                "inline bool ha_action_send() {\n  return ha_api_connected();\n}\n"
            )
        },
        ("send Home Assistant actions only after state subscription is ready",),
    )
    expect_ha_boundary_errors(
        "one-off state read before state connection",
        {
            "button_grid_ha.h": (
                "inline bool ha_subscribe_state() {\n  return true;\n}\n"
                "inline bool ha_subscribe_attribute() {\n  return true;\n}\n"
                "inline bool ha_cancel_action_response_callback() {\n  handle_action_response(); return true;\n}\n"
                "inline bool ha_action_send() {\n"
                "  return ha_api_state_connected() && HA_ACTION_INTERNAL_FREE_MIN_BYTES;\n"
                "}\n"
                "inline bool ha_get_attribute() {\n"
                "  ha_internal_heap_available(\"Home Assistant attribute request\");\n"
                "  if (ha_state_callback_depth() != 0) return true;\n"
                "  return true;\n"
                "}\n"
            )
        },
        ("queue one-off Home Assistant reads until state subscription is ready",),
    )
    expect_ha_boundary_errors(
        "duplicate deferred state reads",
        {
            "button_grid_ha.h": (
                "inline bool ha_subscribe_state() {\n  return true;\n}\n"
                "inline bool ha_subscribe_attribute() {\n  return true;\n}\n"
                "inline bool ha_cancel_action_response_callback() {\n  handle_action_response(); return true;\n}\n"
                "inline bool ha_action_send() {\n"
                "  return ha_api_state_connected() && HA_ACTION_INTERNAL_FREE_MIN_BYTES;\n"
                "}\n"
                "inline bool ha_get_state() {\n"
                "  ha_internal_heap_available(\"Home Assistant attribute request\");\n"
                "  if (ha_state_callback_depth() != 0 || !ha_api_state_connected()) return true;\n"
                "  return true;\n"
                "}\n"
                "inline bool ha_get_attribute() {\n"
                "  if (ha_state_callback_depth() != 0 || !ha_api_state_connected()) return true;\n"
                "  return true;\n"
                "}\n"
            )
        },
        ("fan out duplicate deferred Home Assistant reads",),
    )
    expect_ha_boundary_errors(
        "subscription callback bodies retained",
        {
            "button_grid_ha.h": (
                "inline bool ha_subscribe_state() {\n"
                "  ha_track_subscription_callback(callback_ref);\n"
                "  return true;\n"
                "}\n"
                "inline bool ha_subscribe_attribute() {\n"
                "  ha_track_subscription_callback(callback_ref);\n"
                "  return true;\n"
                "}\n"
                "inline bool ha_cancel_action_response_callback() {\n  handle_action_response(); return true;\n}\n"
                "inline bool ha_action_send() {\n"
                "  return ha_api_state_connected() && HA_ACTION_INTERNAL_FREE_MIN_BYTES;\n"
                "}\n"
                "inline bool ha_get_state() {\n"
                "  ha_internal_heap_available(\"Home Assistant attribute request\");\n"
                "  if (ha_state_callback_depth() != 0 || !ha_api_state_connected()) return true;\n"
                "  request.callback = std::move(callback);\n"
                "  request.entity_id == entity_id;\n"
                "  return true;\n"
                "}\n"
                "inline bool ha_get_attribute() {\n"
                "  if (ha_state_callback_depth() != 0 || !ha_api_state_connected()) return true;\n"
                "  return true;\n"
                "}\n"
            )
        },
        ("release retired Home Assistant subscription callback bodies",),
    )
    expect_ha_boundary_errors(
        "unavailable retry helper symbols",
        {
            "button_grid_ha.h": (
                "struct HaUnavailableStateRetryRef {};\n"
                "inline bool ha_subscribe_state() {\n  return true;\n}\n"
                "inline bool ha_subscribe_attribute() {\n  return true;\n}\n"
                "inline bool ha_cancel_action_response_callback() {\n  handle_action_response(); return true;\n}\n"
                "inline bool ha_action_send() {\n"
                "  return ha_api_state_connected() && HA_ACTION_INTERNAL_FREE_MIN_BYTES;\n"
                "}\n"
            )
        },
        ("do not reintroduce unavailable HA state retry polling",),
    )
    expect_unavailable_retry_errors(
        "unavailable retry reset and interval",
        "inline void bump_ha_subscription_generation() {\n"
        "  ha_reset_unavailable_state_retries();\n"
        "  ha_reset_deferred_state_requests();\n"
        "}\n",
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          ha_retry_unavailable_states();\n",
        (
            "do not reset removed unavailable HA state retries",
            "do not keep removed unavailable HA state retry helpers",
            "do not retry unavailable HA states",
        ),
    )
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("must capture a compact response template" in error for error in errors), errors
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 2, call_id);\n'
            '  req.wants_response = true;\n'
            '  req.response_template = response_template;\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  ha_action_add_data(req, "status", "needs_action");\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("filter todo items in the response template" in error for error in errors), errors
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
            '  req.wants_response = true;\n'
            '  std::string response_template = todo_items_response_template(ctx->entity_id);\n'
            '  req.response_template = response_template;\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("keep the todo response template alive" in error for error in errors), errors
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        (firmware_dir / "button_grid_todo.h").write_text(
            'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
            'inline bool todo_begin_get_items_request() {\n'
            '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
            '  req.wants_response = true;\n'
            '  req.response_template = response_template;\n'
            '  ha_action_add_entity(req, ctx->entity_id);\n'
            '  return true;\n'
            '}\n',
            encoding="utf-8",
        )
        errors = firmware_todo_request_errors(firmware_dir, root)
        assert any("bound todo response text" in error for error in errors), errors
    expect_todo_request_errors(
        "unbounded rendered todo response",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_RESPONSE_TEXT_MAX_LEN = 1536;\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n',
        ("cap rendered todo responses",),
    )
    expect_todo_request_errors(
        "unbounded pending todo request",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  if (!ha_register_action_response_callback(req.call_id, cb)) return;\n'
        '}\n',
        ("bound pending todo item requests with a timeout",),
    )
    expect_todo_request_errors(
        "extra todo response callback",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline void todo_cancel_stale_request() {}\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '  ha_register_action_response_callback(other_call_id, cb);\n'
        '}\n',
        ("only todo list loading should register a response callback",),
    )
    expect_todo_request_errors(
        "timeout only checked while requesting",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline bool todo_cancel_stale_request() { return false; }\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '}\n',
        ("periodically expire stale todo requests",),
    )
    expect_todo_request_errors(
        "modal close leaves todo request pending",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline bool todo_cancel_stale_request() { return false; }\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void todo_modal_hide() {\n'
        '  ui = TodoModalUi();\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  bool stale_request_cancelled = todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '}\n',
        ("cancel pending todo item requests when the modal closes",),
    )
    expect_todo_request_errors(
        "modal close retries cancelled request",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline bool todo_cancel_stale_request() { return false; }\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void todo_modal_hide() {\n'
        '  todo_cancel_pending_request("modal closed");\n'
        '  ui = TodoModalUi();\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  bool stale_request_cancelled = todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '}\n',
        ("close todo modals without retrying their cancelled request",),
    )
    expect_todo_request_errors(
        "todo send failed has no retry",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline bool todo_cancel_stale_request() { return false; }\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void todo_modal_hide() {\n'
        '  todo_cancel_pending_request("modal closed");\n'
        '  ui = TodoModalUi();\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  bool stale_request_cancelled = todo_cancel_stale_request();\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '  if (!ha_action_send(req)) {\n'
        '    todo_cancel_request(req.call_id, "send failed");\n'
        '    todo_modal_set_status("Could not load");\n'
        '  }\n'
        '}\n',
        ("retry todo loads when Home Assistant disconnects during send",),
    )
    expect_todo_request_errors(
        "pending todo request leaves modal loading",
        'constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;\n'
        'constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;\n'
        'constexpr int TODO_REQUEST_TIMEOUT_MS = 15000;\n'
        'inline bool todo_cancel_stale_request() { return false; }\n'
        'inline bool todo_begin_get_items_request() {\n'
        '  ha_action_begin(req, "todo.get_items", false, 1, call_id);\n'
        '  req.wants_response = true;\n'
        '  req.response_template = response_template;\n'
        '  ha_action_add_entity(req, ctx->entity_id);\n'
        '  return true;\n'
        '}\n'
        'inline void todo_modal_hide() {\n'
        '  todo_cancel_pending_request("modal closed");\n'
        '  ui = TodoModalUi();\n'
        '}\n'
        'inline void request_todo_items() {\n'
        '  todo_cancel_stale_request();\n'
        '  bool stale_request_cancelled = todo_cancel_stale_request();\n'
        '  if (todo_request_state().call_id != 0) {\n'
        '    return;\n'
        '  }\n'
        '  if (!ha_api_state_connected()) return;\n'
        '  todo_clear_request_state(call_id);\n'
        '  todo_clear_request_state(call_id);\n'
        '  ha_register_action_response_callback(req.call_id, cb);\n'
        '  if (!ha_action_send(req)) {\n'
        '    todo_cancel_request(req.call_id, "send failed");\n'
        '    ui.waiting_for_ha = true;\n'
        '  }\n'
        '}\n',
        ("retry todo loads when another todo request is already pending",),
    )
    expect_todo_disconnect_errors(
        "missing disconnect cleanup",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("cancel pending todo requests when the HA API disconnects",),
    )
    expect_todo_disconnect_errors(
        "missing reconnect retry",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n",
        "api:\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("retry open todo modals when the HA API reconnects",),
    )
    expect_todo_disconnect_errors(
        "missing waiting modal retry",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n",
        ("retry open todo modals that are waiting for Home Assistant",),
    )
    expect_todo_disconnect_errors(
        "availability blocks todo modal",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n"
        "inline void todo_card_open_modal(TodoCardCtx *ctx) {\n"
        "  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty() || !ctx->available) return;\n"
        "}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("allow todo modals to open while waiting",),
    )
    expect_todo_disconnect_errors(
        "availability dims todo card",
        "inline void todo_cancel_pending_request(const char *reason) {}\n"
        "inline void todo_reload_active_modal() {}\n"
        "inline void todo_retry_waiting_modal() { waiting_for_ha = true; }\n"
        "inline void todo_card_open_modal(TodoCardCtx *ctx) {\n"
        "  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty()) return;\n"
        "}\n"
        "inline void subscribe_todo_state(TodoCardCtx *ctx) {\n"
        "  apply_control_availability(ctx->btn, ctx->btn, ctx->available, false);\n"
        "}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: todo_reload_active_modal();\n"
        "  on_client_disconnected:\n"
        "    - lambda: todo_cancel_pending_request(\"api disconnected\");\n"
        "interval:\n"
        "  - interval: 5s\n"
        "    then:\n"
        "      - lambda: todo_retry_waiting_modal();\n",
        ("do not dim or disable todo cards",),
    )
    expect_action_card_availability_errors(
        "stateless main action registered for availability",
        "if (p.type == \"action\") {\n"
        "  std::string state_entity = action_card_state_entity(p);\n"
        "  if (!state_entity.empty()) {\n"
        "    subscribe_action_card_display_state(ctx, state_entity);\n"
        "  } else {\n"
        "    register_ha_control_availability(s.btn, s.btn);\n"
        "  }\n"
        "  continue;\n"
        "}\n",
        ("keep stateless action cards tappable",),
    )
    expect_action_card_availability_errors(
        "stateless subpage action registered for availability",
        "if (sb_cfg.type == \"action\") {\n"
        "  std::string state_entity = action_card_state_entity(sb_cfg);\n"
        "  if (!state_entity.empty()) {\n"
        "    subscribe_action_card_display_state(action_ctx, state_entity);\n"
        "  } else {\n"
        "    register_ha_control_availability(sub_slot.btn, sub_slot.btn);\n"
        "  }\n"
        "  ParsedCfg *ctx = new ParsedCfg(sb_cfg);\n"
        "}\n",
        ("keep stateless subpage action cards tappable",),
    )
    expect_action_card_availability_errors(
        "stateful action only subscribes display state",
        "if (p.type == \"action\") {\n"
        "  std::string state_entity = action_card_state_entity(p);\n"
        "  if (!state_entity.empty()) {\n"
        "    subscribe_action_card_display_state(ctx, state_entity);\n"
        "  }\n"
        "  continue;\n"
        "}\n",
        (),
    )
    expect_action_card_availability_errors(
        "main trigger registered for availability",
        "if (p.type == \"push\") {\n"
        "  register_ha_control_availability(s.btn, s.btn);\n"
        "  continue;\n"
        "}\n",
        ("keep trigger cards tappable",),
    )
    expect_action_card_availability_errors(
        "subpage trigger registered for availability",
        "if (sb_cfg.type == \"push\") {\n"
        "  register_ha_control_availability(sb_btn, sb_btn);\n"
        "  std::string push_label = sb_cfg.label.empty() ? espcontrol_i18n(std::string(\"Push\")) : sb_cfg.label;\n"
        "  continue;\n"
        "}\n",
        ("keep subpage trigger cards tappable",),
    )
    expect_action_card_availability_errors(
        "trigger cards stay stateless",
        "if (p.type == \"push\") continue;\n"
        "if (sb_cfg.type == \"push\") {\n"
        "  std::string push_label = sb_cfg.label.empty() ? espcontrol_i18n(std::string(\"Push\")) : sb_cfg.label;\n"
        "  continue;\n"
        "}\n",
        (),
    )
    expect_media_card_availability_errors(
        "unavailable entity dims media card",
        "inline void media_playback_apply_state_to_control(MediaPlaybackState *state, MediaControlCtx *ctx) {\n"
        "  media_control_apply_availability(ctx->btn, ctx->btn, ctx->available);\n"
        "}\n",
        ("do not dim or disable media cards",),
    )
    expect_media_card_availability_errors(
        "unavailable entity dims media panel",
        "inline void media_control_refresh_modal(MediaControlCtx *ctx) {\n"
        "  media_control_apply_availability(ui.panel, ui.panel, ctx->available, false);\n"
        "}\n",
        ("do not dim the media control panel",),
    )
    expect_media_card_availability_errors(
        "media progress remains guarded",
        "inline void media_control_refresh_progress(MediaControlCtx *ctx) {\n"
        "  media_control_apply_availability(ui.progress_slider, ui.progress_slider, has_duration);\n"
        "}\n",
        (),
    )
    expect_action_card_script_fields_errors(
        "script fields sent as service data",
        "inline void action_card_add_script_field_variables(esphome::api::HomeassistantActionRequest &req) {\n"
        "  std::string fields = cfg_option_value(options, \"script_fields\");\n"
        "  ha_action_add_data(req, key.c_str(), value.c_str());\n"
        "}\n"
        "inline void send_action_card_action(const ParsedCfg &p) {\n"
        "  size_t script_field_count = action_card_script_field_count(p.options);\n"
        "  ha_action_begin(req, p.sensor.c_str(), false, 1 + script_field_count);\n"
        "}\n",
        (
            "keep parsed script field strings alive",
            "initialize script field variables separately",
            "send script fields through Home Assistant action variables",
            "do not send script fields as top-level",
        ),
    )
    expect_action_card_script_fields_errors(
        "script fields sent as template context only",
        "inline void action_card_add_script_field_variables(esphome::api::HomeassistantActionRequest &req) {\n"
        "  std::string fields = cfg_option_value(options, \"script_fields\");\n"
        "  ha_action_add_variable(req, key.c_str(), value.c_str());\n"
        "}\n"
        "inline void send_action_card_action(const ParsedCfg &p) {\n"
        "  std::vector<ActionCardScriptField> script_fields = action_card_script_fields(p.options);\n"
        "  ha_action_begin(req, p.sensor.c_str(), false, 1);\n"
        "  req.variables.init(script_fields.size());\n"
        "}\n",
        (
            "send script fields in the script.turn_on variables service payload",
            "send script fields through Home Assistant action variables",
        ),
    )
    expect_action_card_script_fields_errors(
        "script fields sent in variables payload",
        "inline void action_card_add_script_field_variables(esphome::api::HomeassistantActionRequest &req) {\n"
        "  std::string fields = cfg_option_value(options, \"script_fields\");\n"
        "  for (const auto &field : fields) {\n"
        "    ha_action_add_variable(req, field.key.c_str(), field.value.c_str());\n"
        "  }\n"
        "}\n"
        "inline void send_action_card_action(const ParsedCfg &p) {\n"
        "  std::vector<ActionCardScriptField> script_fields = action_card_script_fields(p.options);\n"
        "  ha_action_begin(req, p.sensor.c_str(), false, 1);\n"
        "  req.data_template.init(1);\n"
        "  req.variables.init(script_fields.size());\n"
        "  ha_action_add_data_template(req, \"variables\", script_fields_template.c_str());\n"
        "}\n",
        (),
    )
    expect_local_sensor_binding_order_errors(
        "local sensor subtype reaches HA subscription first",
        {
            "button_grid_grid.h":
                "if (image_driver_bind_main(s, p, context, cfg)) continue;\n"
                "if (sensor_driver_bind_data(s, p, context, palette)) return true;\n"
                "if (bind_basic_sensor_card(s, p, context, palette)) continue;\n",
            "button_grid_sensor_driver.h":
                "inline bool sensor_driver_bind_data(BtnSlot &slot, const ParsedCfg &config, const Context &context) {\n"
                "  if (!config.sensor.empty()) subscribe_sensor_value();\n"
                "  if (sensor_driver_is_local(config, context)) {\n"
                "    sensor_driver_register_local_value(slot, config);\n"
                "    return true;\n"
                "  }\n"
                "}\n",
        },
        ("bind local sensor values before Home Assistant sensor subscriptions",),
    )
    expect_local_sensor_binding_order_errors(
        "local sensor subtype binds through shared driver",
        {
            "button_grid_grid.h":
                "if (image_driver_bind_main(s, p, context, cfg)) continue;\n"
                "if (sensor_driver_bind_data(s, p, context, palette)) return true;\n"
                "if (bind_basic_sensor_card(s, p, context, palette)) continue;\n",
            "button_grid_sensor_driver.h":
                "inline bool sensor_driver_bind_data(BtnSlot &slot, const ParsedCfg &config, const Context &context) {\n"
                "  if (sensor_driver_is_local(config, context)) {\n"
                "    sensor_driver_register_local_value(slot, config);\n"
                "    return true;\n"
                "  }\n"
                "  if (!config.sensor.empty()) subscribe_sensor_value();\n"
                "}\n",
        },
        (),
    )
    expect_time_reconnect_errors(
        "home assistant time sync runs on raw api connect",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        id(homeassistant_time).update();\n",
        (
            "guard Home Assistant time updates",
            "wait for Home Assistant state readiness",
            "defer Home Assistant time sync",
            "recalculate sunrise and sunset",
            "recheck the screen schedule",
        ),
    )
    expect_time_reconnect_errors(
        "home assistant time sync waits for state readiness",
        "api:\n"
        "  on_client_connected:\n"
        "    - delay: 2s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) id(homeassistant_time).update();\n"
        "    - delay: 1s\n"
        "    - script.execute: time_update\n"
        "    - script.execute: backlight_recalc_sunrise_sunset\n"
        "    - script.execute: screen_schedule_check\n"
        "script:\n"
        "  - id: time_update\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          if (ha_api_state_connected()) id(homeassistant_time).update();\n",
        (),
    )
    expect_ntp_startup_errors(
        "custom ntp apply runs during boot",
        "esphome:\n"
        "  on_boot:\n"
        "    then:\n"
        "      - script.execute: ntp_servers_apply\n",
        "inline void apply_ntp_servers() {\n"
        "  esp_sntp_init();\n"
        "}\n",
        "wifi:\n"
        "  on_connect:\n"
        "    - script.execute: network_status_refresh\n",
        (
            "defer custom NTP server apply",
            "skip SNTP reconfiguration until application setup",
            "skip SNTP reconfiguration",
            "apply configured NTP servers after network connect",
        ),
    )
    expect_ntp_startup_errors(
        "custom ntp apply waits for network",
        "esphome:\n"
        "  on_boot:\n"
        "    then:\n"
        "      - lambda: apply_timezone();\n"
        "# Ask Home Assistant for time once connected\n"
        "# NTP server settings\n"
        "text:\n"
        "  - platform: template\n"
        "    on_value:\n"
        "      then:\n"
        "        - script.execute: ntp_servers_apply_after_network\n"
        "text_sensor:\n"
        "script:\n"
        "  - id: ntp_servers_apply_after_network\n"
        "    then:\n"
        "      - delay: 2s\n"
        "      - script.execute: ntp_servers_apply\n",
        "inline void apply_ntp_servers() {\n"
        "  if (!esphome::App.is_setup_complete()) return;\n"
        "  if (esphome::network::get_ip_addresses().empty()) return;\n"
        "  esp_sntp_init();\n"
        "}\n",
        "wifi:\n"
        "  on_connect:\n"
        "    - script.execute: ntp_servers_apply_after_network\n",
        (),
    )
    expect_weather_request_errors(
        "weather request during reconnect",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
        "  if (!ha_api_available()) return;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"low internal heap\");\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  ha_action_send(req);\n"
        "}\n",
        ("wait for Home Assistant state subscription",),
    )
    expect_weather_request_errors(
        "weather callback leak on send failure",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"low internal heap\");\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  if (!ha_action_send(req)) return;\n"
        "}\n",
        ("cancel forecast response callbacks",),
    )
    expect_weather_request_errors(
        "weather send failure missing retry",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"setup failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"low internal heap\");\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  if (!ha_action_send(req)) {\n"
        "    weather_forecast_clear_pending(req.call_id);\n"
        "    ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "    weather_forecast_send_next_queued();\n"
        "  }\n"
        "}\n",
        ("retry weather forecast sends that fail after callback registration",),
    )
    expect_weather_request_errors(
        "weather payload without temperatures missing retry",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"setup failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"low internal heap\");\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  bool valid = false;\n"
        "  if (!valid) {\n"
        "    ESP_LOGW(\"weather_forecast\", \"No usable forecast temperatures for %s\", entity_id.c_str());\n"
        "  }\n"
        "}\n",
        ("retry weather forecasts when Home Assistant returns no usable temperatures",),
    )
    expect_weather_request_errors(
        "unbounded weather callbacks",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr uint32_t WEATHER_FORECAST_RETRY_DELAY_MS = 300000;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"low internal heap\");\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "}\n",
        ("bound pending forecast response callbacks",),
    )
    expect_weather_request_errors(
        "missing delayed weather request retry",
        "inline void request_weather_forecast_entity() {\n"
        "  constexpr int WEATHER_FORECAST_PENDING_MAX = 8;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  if (!ha_api_state_connected()) return;\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"low internal heap\");\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "  ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "  ha_action_send(req);\n"
        "}\n",
        ("retry failed weather forecast requests later",),
    )
    expect_weather_request_errors(
        "missing robust weather timeout detection",
        "inline void request_weather_forecast_entity() {\n"
        "  if (!weather_forecast_actions_ready()) return;\n"
        "  weather_forecast_track_pending(req.call_id);\n"
        "  weather_forecast_cancel_pending_requests();\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"low internal heap\");\n"
        "  weather_forecast_schedule_retry(entity_id, day, \"failed\");\n"
        "}\n",
        ("detect Home Assistant forecast timeout errors robustly",),
    )
    expect_weather_disconnect_errors(
        "missing weather disconnect cleanup",
        "inline void weather_forecast_cancel_pending_requests() {}\n",
        "api:\n  on_client_connected:\n    - lambda: refresh_weather_forecast_cards();\n",
        ("cancel pending forecast callbacks when the HA API disconnects",),
    )
    expect_weather_reconnect_errors(
        "unguarded weather reconnect refresh",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        refresh_weather_forecast_cards();\n"
        "    - delay: 20s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n",
        ("wait for Home Assistant state readiness",),
    )
    expect_weather_reconnect_errors(
        "missing delayed weather reconnect retry",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n",
        ("retry weather forecast refresh",),
    )
    expect_weather_reconnect_errors(
        "guarded weather reconnect refresh missing late retry",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n"
        "    - delay: 20s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n",
        ("retry weather forecast refresh",),
    )
    expect_weather_reconnect_errors(
        "guarded weather reconnect refresh with delayed retries",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n"
        "    - delay: 20s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n"
        "    - delay: 25s\n"
        "    - lambda: |-\n"
        "        if (ha_api_state_connected()) refresh_weather_forecast_cards();\n",
        (),
    )
    expect_cover_request_errors(
        "missing cover callback tracking",
        "inline void cover_stop_cancel_pending_request() {}\n"
        "inline void send_cover_command_action() {\n"
        "  ha_register_action_response_callback(req.call_id, cb);\n"
        "}\n",
        "api:\n"
        "  on_client_disconnected:\n"
        "    - lambda: cover_stop_cancel_pending_request();\n",
        ("track pending cover stop callbacks",),
    )
    expect_cover_request_errors(
        "missing cover disconnect cleanup",
        "inline void cover_stop_track_pending() {}\n"
        "inline void cover_stop_clear_pending() {}\n"
        "inline void cover_stop_cancel_pending_request() {}\n"
        "inline void send_cover_command_action() {\n"
        "  bool cover_stop_tracked = true;\n"
        "  ha_cancel_action_response_callback(req.call_id, \"send failed\");\n"
        "}\n",
        "api:\n  on_client_connected:\n    - lambda: refresh_weather_forecast_cards();\n",
        ("cancel pending cover stop callbacks when the HA API disconnects",),
    )
    expect_cover_art_external_input_errors(
        "missing cover art source handling",
        "script:\n"
        "  - id: cover_art_request_artwork\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(cover_art_hide_external_input_enabled).state && id(cover_art_external_input_active);\n",
        (
            "subscribe to the media player source attribute",
            "treat TV and Line-in sources as external inputs",
            "stop pending artwork requests",
        ),
    )
    expect_cover_art_external_input_errors(
        "cover art source handling present",
        "switch:\n"
        "  - platform: template\n"
        "    id: cover_art_hide_external_input_enabled\n"
        "script:\n"
        "  - id: cover_art_apply_external_input_policy\n"
        "    then:\n"
        "      - script.stop: cover_art_request_artwork\n"
        "      - lambda: |-\n"
        "          id(cover_art_hide_external_input_enabled).state && id(cover_art_external_input_active);\n"
        "          std::string normalized_source = next;\n"
        "          for (char &ch : normalized_source) {\n"
        "            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));\n"
        "          }\n"
        "          bool external = normalized_source == \"tv\" ||\n"
        "                          normalized_source == \"line-in\" ||\n"
        "                          normalized_source == \"line in\";\n"
        "          ha_reset_subscription_callbacks(HA_SUBSCRIPTION_SCOPE_COVER_ART);\n"
        "          ha_subscribe_attribute(\n"
        "            cover_entity,\n"
        "            std::string(\"source\"),\n"
        "            handle_media_source,\n"
        "            HA_SUBSCRIPTION_SCOPE_COVER_ART\n"
        "          );\n"
        "          // Live subscriptions supply both initial values and updates.\n",
        (),
    )
    expect_cover_art_stale_image_errors(
        "missing black fallback image hide",
        "script:\n"
        "  - id: cover_art_show_black_screen\n"
        "    then:\n"
        "      - globals.set:\n"
        "          id: cover_art_image_available\n"
        "          value: 'false'\n",
        ("hide stale cover art image when the black fallback is shown",),
    )
    expect_cover_art_stale_image_errors(
        "conditional black fallback image hide",
        "script:\n"
        "  - id: cover_art_show_black_screen\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return id(cover_art_runtime).loaded_url.empty();'\n"
        "          then:\n"
        "            - lvgl.widget.hide: cover_art_image_widget\n",
        ("hide stale cover art image even after previous artwork loaded",),
    )
    expect_cover_art_stale_image_errors(
        "unconditional black fallback image hide",
        "script:\n"
        "  - id: cover_art_show_black_screen\n"
        "    then:\n"
        "      - lvgl.widget.hide: cover_art_image_widget\n",
        (),
    )
    expect_cover_art_stale_image_errors(
        "visible unavailable cover art message",
        "lvgl:\n"
        "  top_layer:\n"
        "    widgets:\n"
        "      - label:\n"
        "          id: cover_art_error_label\n"
        "          text: \"Artwork unavailable\"\n"
        "script:\n"
        "  - id: cover_art_show_black_screen\n"
        "    then:\n"
        "      - lvgl.widget.hide: cover_art_image_widget\n",
        ("do not show an unavailable cover art message",),
    )
    expect_cover_art_refresh_errors(
        "missing stale cover refresh guard",
        "script:\n"
        "  - id: cover_art_download\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(cover_art_runtime).source_url;\n",
        (
            "track/source metadata changes as stale artwork",
            "subscribe to the media_album_name attribute",
        ),
    )
    expect_cover_art_refresh_errors(
        "queued cover art changes are not coalesced",
        "script:\n"
        "  - id: cover_art_download\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(cover_art_runtime).effective_download_url = id(cover_art_downloaded_image)->request_update_url(id(cover_art_runtime).effective_download_url);\n",
        ("coalesce changed cover art URLs into the queued image request",),
    )
    expect_cover_art_refresh_errors(
        "cover art touch delay ignores later touches",
        "script:\n"
        "  - id: cover_art_pause_after_touch\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART);'\n"
        "          then:\n"
        "            - lambda: 'id(cover_art_manual_pause_until_ms) = 1;'\n",
        ("restart its countdown after every touch",),
    )
    expect_cover_art_refresh_errors(
        "stale cover refresh guard present",
        "globals:\n"
        "  - id: cover_art_runtime\n"
        "    type: espcontrol::cover_art::RuntimeState\n"
        "# cover_art_runtime).refresh_needed\n"
        "# cover_art_runtime).effective_download_url\n"
        "  - id: cover_art_album\n"
        "script:\n"
        "  - id: cover_art_download\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) &&\n"
        "                     id(cover_art_runtime).image_available &&\n"
        "                     id(cover_art_runtime).refresh_needed &&\n"
        "                     !${cover_art_live_image_updates};\n"
        "          then:\n"
        "            - script.execute: cover_art_show_track_overlay\n"
        "      - lambda: |-\n"
        "          const bool replacing_active_download = id(cover_art_runtime).download_active();\n"
        "          if (url.find(\"/api/media_player_proxy/\") != std::string::npos) {\n"
        "            url += url.find('?') == std::string::npos ? \"?time=\" : \"&time=\";\n"
        "          }\n"
        "          const bool needs_artwork_refresh = id(cover_art_runtime).refresh_needed || !id(cover_art_runtime).image_available;\n"
        "          ESP_LOGI(\"cover_art\", \"%s\", replacing_active_download ? \"Re-queuing\" : \"Downloading\");\n"
        "          id(cover_art_runtime).effective_download_url = id(cover_art_downloaded_image)->request_update_url(id(cover_art_runtime).effective_download_url);\n"
        "  - id: cover_art_deferred_download\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(cover_art_runtime).refresh_needed;\n"
        "  - id: cover_art_prepare_download\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(cover_art_runtime).refresh_needed;\n"
        "  - id: cover_art_use_cached_artwork\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(cover_art_runtime).select_source(chosen);\n"
        "          if (chosen == id(cover_art_runtime).source_url) {\n"
        "            if (!id(cover_art_runtime).image_available || id(cover_art_runtime).refresh_needed) {}\n"
        "          }\n"
        "          id(cover_art_runtime).source_url = chosen;\n"
        "          id(cover_art_runtime).refresh_needed = true;\n"
        "  - id: cover_art_request_artwork\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          if (chosen == id(cover_art_runtime).source_url) {\n"
        "            if (!id(cover_art_runtime).image_available || id(cover_art_runtime).refresh_needed) {}\n"
        "          }\n"
        "  - id: cover_art_apply_downloaded_image\n"
        "    then:\n"
        "      - script.execute: cover_art_clear_image_source\n"
        "      - script.wait: cover_art_clear_image_source\n"
        "      - lambda: |-\n"
        "          std::string expected_url = id(cover_art_runtime).effective_download_url;\n"
        "          std::string completed_url;\n"
        "          id(cover_art_runtime).apply_download(completed_url);\n"
        "  - id: cover_art_playback_started\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          if (!id(cover_art_runtime).image_available) {\n"
        "            id(cover_art_runtime).retry_url.clear();\n"
        "            id(cover_art_runtime).retry_count = 0;\n"
        "          }\n"
        "  - id: cover_art_resubscribe\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(cover_art_runtime).refresh_needed = true;\n"
        "          mark_artwork_refresh_needed();\n"
        "          mark_artwork_refresh_needed();\n"
        "          mark_artwork_refresh_needed();\n"
        "          mark_artwork_refresh_needed();\n"
        "          if (!url.empty() && url != id(cover_art_runtime).source_url) {\n"
        "            id(cover_art_runtime).refresh_needed = true;\n"
        "          }\n"
        "          ha_subscribe_attribute(cover_entity, std::string(\"media_album_name\"), handle_media_album);\n"
        "          // Live subscriptions supply both initial values and updates.\n",
        (),
    )
    expect_cover_art_playback_grace_errors(
        "missing cover art playback grace",
        "script:\n"
        "  - id: cover_art_playback_stopped\n"
        "    then:\n"
        "      - lambda: id(cover_art_media_playing) = false;\n",
        (
            "buffer brief non-playing states between tracks",
            "delay non-playing playback transitions",
            "remember and cancel a pending cover art opening when playback stops",
            "restart an interrupted cover art opening when playback resumes",
            "cancel a pending stop when playback resumes or pauses",
            "keep cached artwork when Home Assistant clears it during a brief playback transition",
            "cancel pending playback grace during an immediate stop",
        ),
    )
    expect_cover_art_playback_grace_errors(
        "cover art playback grace present",
        "script:\n"
        "  - id: cover_art_resubscribe\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          bool restart_cover_art_delay = id(cover_art_delay_interrupted_by_transition);\n"
        "          if (!was_playing || restart_cover_art_delay) id(cover_art_playback_started).execute();\n"
        "          id(cover_art_delayed_playback_stopped).stop();\n"
        "          id(cover_art_delayed_playback_stopped).stop();\n"
        "          id(cover_art_delay_interrupted_by_transition) =\n"
        "            id(cover_art_delay_interrupted_by_transition) || id(cover_art_delay_timer).is_running();\n"
        "          id(cover_art_delay_timer).stop();\n"
        "          id(cover_art_delayed_playback_stopped).execute();\n"
        "          if (url.empty() && id(cover_art_delayed_playback_stopped).is_running()) return;\n"
        "  - id: cover_art_delayed_playback_stopped\n"
        "    mode: restart\n"
        "    then:\n"
        "      - delay: 2s\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return state != \"playing\" && state != \"buffering\" && state != \"paused\";\n"
        "          then:\n"
        "            - script.execute: cover_art_playback_stopped\n"
        "  - id: cover_art_playback_stopped\n"
        "    then:\n"
        "      - script.stop: cover_art_delayed_playback_stopped\n",
        (),
    )
    expect_cover_art_disable_errors(
        "independent media sleep prevention when cover art is disabled",
        "script:\n"
        "  - id: cover_art_disable\n"
        "    then:\n"
        "      - script.stop: cover_art_delay_timer\n",
        (),
    )
    expect_cover_art_disable_errors(
        "coupled media sleep prevention when cover art is disabled",
        "script:\n"
        "  - id: cover_art_disable\n"
        "    then:\n"
        "      - switch.turn_off: media_player_sleep_prevention_enabled\n",
        ("keep media sleep prevention independent",),
    )
    valid_cover_art_adapter = (
        "script:\n"
        "  - id: display_mode_apply_transition\n"
        "    then:\n"
        "      - lambda: 'return target_mode == static_cast<int>(espcontrol::DisplayMode::COVER_ART);'\n"
        "      - script.execute:\n"
        "          id: display_mode_effect_cover_art\n"
        "      - script.execute:\n"
        "          id: cover_art_hide_effect\n"
        "      - lambda: 'id(display_mode_controller).complete_transition(generation, espcontrol::DisplayMode::COVER_ART);'\n"
        "  - id: display_mode_request_cover_art\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).request(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK, espcontrol::DisplayMode::COVER_ART);'\n"
        "      - script.execute: display_mode_reconcile\n"
        "      - script.wait: display_mode_reconcile\n"
        "      - script.wait: display_mode_apply_transition\n"
        "  - id: display_mode_clear_cover_art\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).clear(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK);'\n"
        "      - script.execute: display_mode_reconcile\n"
        "      - script.wait: display_mode_reconcile\n"
        "      - script.wait: display_mode_apply_transition\n"
        "  - id: display_mode_reconcile\n"
        "    then:\n"
        "      - lambda: 'auto transition = controller.resolve(); bool transition_required = controller.transition_required(transition); if (!transition_required) { auto previous_cover_generation = id(cover_art_transition_generation); id(cover_art_transition_generation) = transition.generation; if (id(cover_art_download_generation) == previous_cover_generation) id(cover_art_download_generation) = transition.generation; }'\n"
    )
    valid_cover_art_effects = (
        "globals:\n"
        "  - id: cover_art_transition_generation\n"
        "script:\n"
        "  - id: display_mode_effect_cover_art\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).transition_is_current(generation, espcontrol::DisplayMode::COVER_ART); lv_obj_move_foreground(id(cover_art_screensaver));'\n"
        "  - id: cover_art_hide_effect\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).transition_is_current(generation, target); return std::string(\"${device_slug}\") == \"guition-esp32-s3-4848s040\";'\n"
        "      - artwork_image.release: cover_art_downloaded_image\n"
        "  - id: hide_cover_art_view\n"
        "    then:\n"
        "      - script.execute: display_mode_clear_cover_art\n"
        "      - script.wait: display_mode_clear_cover_art\n"
        "  - id: cover_art_disable\n"
        "    then:\n"
        "      - script.wait: display_mode_clear_cover_art\n"
        "  - id: cover_art_return_home_after_playback\n"
        "    then:\n"
        "      - script.wait: display_mode_clear_cover_art\n"
        "  - id: cover_art_delay_timer\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).generation_is_current(generation);'\n"
        "  - id: cover_art_apply_downloaded_image\n"
        "    then:\n"
        "      - lambda: 'return id(cover_art_download_generation);'\n"
        "  - id: cover_art_deferred_download\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).transition_is_current(generation, mode);'\n"
        "  - id: cover_art_retry_download\n"
        "    then:\n"
        "      - lambda: 'return id(cover_art_download_generation);'\n"
        "  - id: cover_art_refresh_progress\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).transition_is_current(generation, mode);'\n"
        "  - id: cover_art_delayed_playback_stopped\n"
        "    then:\n"
        "      - lambda: 'id(display_mode_controller).generation_is_current(generation);'\n"
        "      - globals.set: { id: cover_art_delay_interrupted_by_transition, value: 'false' }\n"
        "      - globals.set: { id: cover_art_media_playing, value: 'false' }\n"
        "      - script.execute: display_mode_clear_cover_art\n"
        "      - script.wait: display_mode_clear_cover_art\n"
    )
    expect_cover_art_lifecycle_controller_errors(
        "controller-owned cover art lifecycle",
        valid_cover_art_adapter,
        valid_cover_art_effects,
        (),
    )
    expect_cover_art_lifecycle_controller_errors(
        "legacy cover art lifecycle",
        "script:\n  - id: display_mode_reconcile\n    then: []\n",
        "script:\n  - id: show_cover_art_view\n    then: []\n",
        (
            "route cover art presentation exclusively through the display adapter",
            "create cover art through a controller media request",
            "dismiss cover art by clearing its controller media request",
            "guard the controller-owned cover art effect",
            "preserve guarded S3 image release",
            "guard cover_art_delay_timer",
        ),
    )
    expect_media_sleep_prevention_errors(
        "cover art alone keeps media awake",
        "script:\n"
        "  - id: screensaver_idle_check\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "                     ((id(media_player_sleep_prevention_enabled).state ||\n"
        "                       id(cover_art_screensaver_enabled).state) &&\n"
        "                      id(media_player_playing));\n",
        "switch:\n"
        "  - platform: template\n"
        "    id: cover_art_screensaver_enabled\n"
        "    on_turn_on:\n"
        "      - switch.turn_on: media_player_sleep_prevention_enabled\n",
        "script:\n"
        "  - id: cover_art_playback_started\n"
        "    then:\n"
        "      - script.execute: cover_art_start_delay\n",
        (
            "require both cover art and media sleep prevention to keep the idle timer awake",
            "do not turn on media sleep prevention",
            "let cover art use its own delay",
        ),
    )
    expect_media_sleep_prevention_errors(
        "playback cannot block sleep while cover art is disabled",
        "script:\n"
        "  - id: screensaver_idle_check\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "                     (id(media_player_sleep_prevention_enabled).state &&\n"
        "                      id(media_player_playing));\n",
        "",
        "",
        ("keep media playback awake only while cover art and media sleep prevention are enabled",),
    )
    expect_media_sleep_prevention_errors(
        "media sleep prevention applies while cover art is enabled",
        "script:\n"
        "  - id: screensaver_idle_check\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "                     (id(cover_art_screensaver_enabled).state &&\n"
        "                      id(media_player_sleep_prevention_enabled).state &&\n"
        "                      id(media_player_playing));\n"
        "  - id: screensaver_wake\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              float show_after_seconds = id(cover_art_delay).state;\n"
        "              if (show_after_seconds < 3.0f) show_after_seconds = 3.0f;\n"
        "              const bool cover_art_disabled_mode_delay =\n"
        "                id(screensaver_mode).state != \"timer\" &&\n"
        "                id(screensaver_mode).state != \"sensor\" &&\n"
        "                show_after_seconds > 0.0f;\n",
        "switch:\n"
        "  - platform: template\n"
        "    id: cover_art_screensaver_enabled\n"
        "    on_turn_on:\n"
        "      - script.execute: cover_art_resubscribe\n",
        "script:\n"
        "  - id: cover_art_playback_started\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(media_player_sleep_prevention_enabled).state ||\n"
        "                     (id(screensaver_mode).state != \"timer\" &&\n"
        "                      id(screensaver_mode).state != \"sensor\") ||\n"
        "                     id(display_asleep);\n"
        "          then:\n"
        "            - script.execute: cover_art_start_delay\n"
        "          else:\n"
        "            - script.execute: screensaver_idle_check\n",
        (),
    )
    expect_media_sleep_prevention_errors(
        "normal screensaver timeout starts cover art directly",
        "script:\n"
        "  - id: screensaver_idle_check\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "                     (id(cover_art_screensaver_enabled).state &&\n"
        "                      id(media_player_sleep_prevention_enabled).state &&\n"
        "                      id(media_player_playing));\n"
        "  - id: screensaver_sleep_timer\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              const std::string &state = id(cover_art_last_playback_state);\n"
        "              return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "                     (id(cover_art_media_playing) &&\n"
        "                      state != \"playing\" && state != \"buffering\" && state != \"paused\");\n"
        "          then:\n"
        "            - script.execute: screensaver_idle_check\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(cover_art_screensaver_enabled).state &&\n"
        "                     id(cover_art_media_playing) &&\n"
        "                     !id(cover_art_media_player_entity).state.empty();\n"
        "          then:\n"
        "            - script.execute: show_cover_art_view\n",
        "switch:\n"
        "  - platform: template\n"
        "    id: cover_art_screensaver_enabled\n",
        "script:\n"
        "  - id: cover_art_playback_started\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(media_player_sleep_prevention_enabled).state ||\n"
        "                     (id(screensaver_mode).state != \"timer\" &&\n"
        "                      id(screensaver_mode).state != \"sensor\") ||\n"
        "                     id(display_asleep);\n"
        "          then:\n"
        "            - script.execute: cover_art_delay_timer\n"
        "          else:\n"
        "            - script.execute: screensaver_idle_check\n",
        (),
    )
    expect_media_sleep_prevention_errors(
        "normal screensaver timeout does not add cover art delay",
        "script:\n"
        "  - id: screensaver_idle_check\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "                     (id(cover_art_screensaver_enabled).state &&\n"
        "                      id(media_player_sleep_prevention_enabled).state &&\n"
        "                      id(media_player_playing));\n"
        "  - id: screensaver_sleep_timer\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(cover_art_screensaver_enabled).state &&\n"
        "                     id(cover_art_media_playing) &&\n"
        "                     !id(cover_art_media_player_entity).state.empty();\n"
        "          then:\n"
        "            - script.execute: cover_art_delay_timer\n",
        "switch:\n"
        "  - platform: template\n"
        "    id: cover_art_screensaver_enabled\n",
        "script:\n"
        "  - id: cover_art_playback_started\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(media_player_sleep_prevention_enabled).state ||\n"
        "                     (id(screensaver_mode).state != \"timer\" &&\n"
        "                      id(screensaver_mode).state != \"sensor\") ||\n"
        "                     id(display_asleep);\n"
        "          then:\n"
        "            - script.execute: cover_art_delay_timer\n"
        "          else:\n"
        "            - script.execute: screensaver_idle_check\n",
        (
            "start cover art directly after the normal screensaver timeout",
            "keep the normal screensaver idle during cover art stop grace",
        ),
    )
    expect_media_sleep_prevention_errors(
        "takeover restore trusts delayed playing flag",
        "script:\n"
        "  - id: display_takeover_resume_restore\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return id(cover_art_media_playing);'\n"
        "          then:\n"
        "            - script.execute: show_cover_art_view\n",
        "",
        "",
        ("keep takeover restore from showing cover art during stop grace",),
    )
    expect_media_control_low_heap_metadata_errors(
        "low heap media modal keeps title and artist",
        "inline void media_playback_subscribe_metadata(MediaPlaybackState *state) {\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_title\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_artist\"), cb);\n"
        "}\n\n"
        "inline void media_playback_subscribe_progress(MediaPlaybackState *state) {\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_duration\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position_updated_at\"), cb);\n"
        "}\n\n"
        "inline void media_playback_subscribe_volume(MediaPlaybackState *state) {}\n"
        "inline void subscribe_media_control_state(MediaControlCtx *ctx) {\n"
        "  MediaPlaybackState *state = media_playback_ensure_state(ctx->entity_id);\n"
        "  media_playback_subscribe_metadata(state);\n"
        "#ifndef ESPCONTROL_LOW_HEAP_MEDIA_CONTROL\n"
        "  media_playback_subscribe_progress(state);\n"
        "#endif\n"
        "}\n\n"
        "inline bool media_seek_pending_active() { return false; }\n",
        (),
    )
    expect_media_control_low_heap_metadata_errors(
        "low heap media modal lost title and artist",
        "inline void media_playback_subscribe_metadata(MediaPlaybackState *state) {\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_title\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_artist\"), cb);\n"
        "}\n\n"
        "inline void media_playback_subscribe_progress(MediaPlaybackState *state) {\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_duration\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position_updated_at\"), cb);\n"
        "}\n\n"
        "inline void media_playback_subscribe_volume(MediaPlaybackState *state) {}\n"
        "inline void subscribe_media_control_state(MediaControlCtx *ctx) {\n"
        "#ifndef ESPCONTROL_LOW_HEAP_MEDIA_CONTROL\n"
        "  media_playback_subscribe_metadata(state);\n"
        "  media_playback_subscribe_progress(state);\n"
        "#endif\n"
        "}\n\n"
        "inline bool media_seek_pending_active() { return false; }\n",
        (
            "keep media_title subscribed",
            "keep media_artist subscribed",
        ),
    )
    expect_media_control_low_heap_metadata_errors(
        "low heap media modal progress metadata is guarded",
        "inline void media_playback_subscribe_metadata(MediaPlaybackState *state) {\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_title\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_artist\"), cb);\n"
        "}\n\n"
        "inline void media_playback_subscribe_progress(MediaPlaybackState *state) {\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_duration\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position\"), cb);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position_updated_at\"), cb);\n"
        "}\n\n"
        "inline void media_playback_subscribe_volume(MediaPlaybackState *state) {}\n"
        "inline void subscribe_media_control_state(MediaControlCtx *ctx) {\n"
        "  MediaPlaybackState *state = media_playback_ensure_state(ctx->entity_id);\n"
        "  media_playback_subscribe_metadata(state);\n"
        "  media_playback_subscribe_progress(state);\n"
        "#ifndef ESPCONTROL_LOW_HEAP_MEDIA_CONTROL\n"
        "#endif\n"
        "}\n\n"
        "inline bool media_seek_pending_active() { return false; }\n",
        (
            "keep media_duration out of the S3 low-heap media modal path",
            "keep media_position out of the S3 low-heap media modal path",
            "keep media_position_updated_at out of the S3 low-heap media modal path",
        ),
    )
    cover_art_shared_media = (
        "struct MediaPlaybackState {};\n"
        "inline MediaPlaybackState *media_playback_find_state(const std::string &entity_id) {\n"
        "  if (state->generation != ha_subscription_generation()) media_playback_reset_state(state, entity_id);\n"
        "  return state;\n"
        "}\n"
        "inline void media_playback_subscribe_progress(MediaPlaybackState *state, uint32_t scope) {\n"
        "  if (!state || state->progress_subscribed) return;\n"
        "  state->progress_subscription_scope = scope;\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_duration\"), cb, scope);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position\"), cb, scope);\n"
        "  ha_subscribe_attribute(entity_id, std::string(\"media_position_updated_at\"), cb, scope);\n"
        "}\n"
        "inline bool media_control_progress_supported(MediaControlCtx *ctx) {\n"
        "  #ifdef ESPCONTROL_LOW_HEAP_MEDIA_CONTROL\n"
        "  return ctx && media_playback_state_has_progress(ctx->entity_id);\n"
        "  #endif\n"
        "}\n"
        "inline void media_playback_refresh_progress_timer(MediaPlaybackState *state) {\n"
        "  bool has_timer_consumer = !state->sliders.empty() || !state->controls.empty();\n"
        "  if (!has_timer_consumer) return;\n"
        "  state->progress_timer = lv_timer_create(cb, 1000, state);\n"
        "}\n"
        "inline void media_playback_invalidate_stale_progress(const std::string &entity_id) {\n"
        "  uint32_t last_duration_callback_ms = state->last_duration_callback_ms;\n"
        "  state->duration = 0.0f;\n"
        "  state->has_duration = false;\n"
        "  state->position_seconds = 0.0f;\n"
        "  state->position_updated_ms = 0;\n"
        "  state->position_updated_at_known = false;\n"
        "  state->position_updated_at_ms = 0;\n"
        "  state->has_position = false;\n"
        "  media_playback_apply_progress_consumers(state);\n"
        "}\n"
        "inline void media_playback_reset_cover_art_progress_subscriptions() {\n"
        "  ha_reset_subscription_callbacks(HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS);\n"
        "  if ((state->progress_subscription_scope & HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS) == 0) return;\n"
        "  state->progress_subscribed = false;\n"
        "  state->progress_subscription_scope = 0;\n"
        "  state->has_position = false;\n"
        "  media_playback_subscribe_progress(state);\n"
        "}\n"
        "inline MediaPlaybackState *media_playback_prepare_cover_art_progress(const std::string &entity_id, bool playing) {\n"
        "  MediaPlaybackState *state = media_playback_ensure_state(entity_id);\n"
        "  media_playback_set_playing_hint(state, playing);\n"
        "  media_playback_subscribe_progress(\n"
        "    state, HA_SUBSCRIPTION_SCOPE_COVER_ART | HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS);\n"
        "  return state;\n"
        "}\n"
        "inline void subscribe_media_slider_state(lv_obj_t *btn_ptr, lv_obj_t *slider, const std::string &entity_id) {\n"
        "  MediaPlaybackState *state = media_playback_ensure_state(entity_id);\n"
        "  media_playback_attach_slider(state, ctx);\n"
        "  media_playback_subscribe_state(state);\n"
        "}\n"
        "inline bool media_playback_state_snapshot(const std::string &entity_id) {\n"
        "  if (!state || !state->has_duration) return false;\n"
        "  position = state->has_position ? media_playback_current_position_seconds(state) : 0.0f;\n"
        "  return true;\n"
        "}\n"
        "inline bool media_playback_state_has_progress() { return true; }\n"
    )
    expect_cover_art_low_heap_progress_errors(
        "low heap cover art reuses shared progress",
        cover_art_shared_media,
        "script:\n"
        "  - id: cover_art_refresh_progress\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          #ifdef ESPCONTROL_LOW_HEAP_COVER_ART\n"
        "          media_playback_prepare_cover_art_progress(id(cover_art_media_player_entity).state, id(cover_art_media_playing));\n"
        "          media_playback_state_snapshot(id(cover_art_media_player_entity).state, playing, duration, position);\n"
        "          lv_obj_clear_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN);\n"
        "          #endif\n"
        "          media_playback_prepare_cover_art_progress(id(cover_art_media_player_entity).state, id(cover_art_media_playing));\n"
        "          return media_playback_state_has_progress(id(cover_art_media_player_entity).state);\n"
        "interval:\n"
        "  - interval: 1s\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              #ifdef ESPCONTROL_LOW_HEAP_COVER_ART\n"
        "              if (!id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART)) return false;\n"
        "              media_playback_prepare_cover_art_progress(id(cover_art_media_player_entity).state, id(cover_art_media_playing));\n"
        "              return true;\n"
        "              #else\n"
        "              return id(cover_art_media_playing);\n"
        "              #endif\n"
        "  - id: cover_art_resubscribe\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          #ifdef ESPCONTROL_LOW_HEAP_COVER_ART\n"
        "          if (id(cover_art_screensaver_enabled).state) {\n"
        "            media_playback_prepare_cover_art_progress(cover_entity, id(cover_art_media_playing));\n"
        "          } else {\n"
        "            media_playback_reset_cover_art_progress_subscriptions();\n"
        "          }\n"
        "          media_playback_invalidate_stale_progress(cover_entity);\n"
        "          #endif\n"
        "  - id: cover_art_disable\n"
        "    then:\n"
        "      - script.execute: cover_art_resubscribe\n"
        "#ifndef ESPCONTROL_LOW_HEAP_COVER_ART\n"
        "ha_subscribe_attribute(cover_entity, std::string(\"media_duration\"), cb);\n"
        "ha_subscribe_attribute(cover_entity, std::string(\"media_position\"), cb);\n"
        "ha_subscribe_attribute(cover_entity, std::string(\"media_position_updated_at\"), cb);\n"
        "#endif\n",
        (),
    )
    expect_cover_art_low_heap_progress_errors(
        "low heap cover art direct progress subscriptions",
        cover_art_shared_media,
        "script:\n"
        "  - id: cover_art_refresh_progress\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          return false;\n"
        "ha_subscribe_attribute(cover_entity, std::string(\"media_duration\"), cb);\n"
        "ha_subscribe_attribute(cover_entity, std::string(\"media_position\"), cb);\n"
        "ha_subscribe_attribute(cover_entity, std::string(\"media_position_updated_at\"), cb);\n",
        (
            "keep media_duration out of the S3 low-heap cover art path",
            "keep media_position out of the S3 low-heap cover art path",
            "keep media_position_updated_at out of the S3 low-heap cover art path",
            "let S3 cover art initialise and consume shared progress",
            "prepare and invalidate S3 shared cover art progress",
            "prepare S3 progress before checking cover art visibility",
        ),
    )
    cover_art_progress_visibility = (
        "lvgl:\n"
        "  widgets:\n"
        "    - label:\n"
        "        id: cover_art_time_label\n"
        "        hidden: true\n"
        "    - bar:\n"
        "        id: cover_art_progress_bar\n"
        "script:\n"
        "  - id: cover_art_sync_track_text\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          bool available = espcontrol::cover_art::progress_available(id(cover_art_media_duration));\n"
        "          lv_obj_add_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN);\n"
        "          lv_obj_add_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN);\n"
        "  - id: cover_art_refresh_progress\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          bool available = espcontrol::cover_art::progress_available(duration);\n"
        "          lv_bar_set_value(id(cover_art_progress_bar), 0, LV_ANIM_OFF);\n"
        "          lv_label_set_text(id(cover_art_time_label), \"0:00  /  0:00\");\n"
        "          lv_obj_add_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN);\n"
        "          lv_obj_add_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN);\n"
        "          lv_obj_clear_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN);\n"
        "          lv_obj_clear_flag(id(cover_art_progress_bar), LV_OBJ_FLAG_HIDDEN);\n"
        "  - id: cover_art_show_black_screen\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          return espcontrol::cover_art::progress_available(id(cover_art_media_duration));\n"
        "  - id: cover_art_show_track_overlay\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          return espcontrol::cover_art::progress_available(id(cover_art_media_duration));\n"
        "std::function<void()> invalidate_stale_media_duration = []() {\n"
        "  const uint32_t last_duration_ms = id(cover_art_last_duration_callback_ms);\n"
        "  const bool duration_callback_is_fresh = last_duration_ms != 0 &&\n"
        "    (uint32_t)(millis() - last_duration_ms) <= 250;\n"
        "  if (!duration_callback_is_fresh) {\n"
        "    id(cover_art_media_duration) = 0.0f;\n"
        "  }\n"
        "};\n"
        "# title callback\n"
        "std::function<void(esphome::StringRef)> handle_media_title = [](esphome::StringRef title) {\n"
        "  invalidate_stale_media_duration();\n"
        "  id(cover_art_title) = next;\n"
        "};\n"
        "if (!already_subscribed) {}\n"
        "# artist callback\n"
        "std::function<void(esphome::StringRef)> handle_media_artist = [](esphome::StringRef artist) {\n"
        "  invalidate_stale_media_duration();\n"
        "  id(cover_art_artist) = next;\n"
        "};\n"
        "if (!already_subscribed) {}\n"
        "# album callback\n"
        "std::function<void(esphome::StringRef)> handle_media_album = [](esphome::StringRef album) {\n"
        "  invalidate_stale_media_duration();\n"
        "  id(cover_art_album) = next;\n"
        "  id(cover_art_sync_track_text).execute();\n"
        "};\n"
        "if (!already_subscribed) {}\n"
        "# source callback\n"
        "std::function<void(esphome::StringRef)> handle_media_source = [](esphome::StringRef source) {\n"
        "  invalidate_stale_media_duration();\n"
        "  id(cover_art_media_source) = next;\n"
        "};\n"
        "if (!already_subscribed) {}\n"
        "# duration callback\n"
        "std::function<void(esphome::StringRef)> handle_media_duration = [](esphome::StringRef duration) {\n"
        "  id(cover_art_last_duration_callback_ms) = millis();\n"
        "  const bool next_progress_available = espcontrol::cover_art::progress_available(next_duration);\n"
        "  if (!next_progress_available) {\n"
        "    next_duration = 0.0f;\n"
        "    id(cover_art_media_position) = 0.0f;\n"
        "    id(cover_art_position_anchor) = 0.0f;\n"
        "    id(cover_art_position_anchor_epoch) = 0;\n"
        "    id(cover_art_last_position_timestamp) = 0;\n"
        "  }\n"
        "};\n"
        "if (!already_subscribed) {}\n"
    )
    expect_cover_art_progress_visibility_errors(
        "cover art progress hides without duration",
        cover_art_progress_visibility,
        (),
    )
    expect_cover_art_progress_visibility_errors(
        "unguarded cover art progress",
        cover_art_progress_visibility
        .replace("        hidden: true\n", "")
        .replace(
            "return espcontrol::cover_art::progress_available(id(cover_art_media_duration));",
            "return true;",
        )
        .replace(
            "bool available = espcontrol::cover_art::progress_available(id(cover_art_media_duration));\n",
            "",
        )
        .replace(
            "bool available = espcontrol::cover_art::progress_available(duration);\n",
            "",
        )
        .replace(
            "  if (!next_progress_available) {\n",
            "  if (true) {\n",
        ),
        (
            "initialize cover art playback time hidden",
            "guard every cover art progress reveal",
            "hide cover art playback time",
            "hide and reset unavailable cover art progress",
            "normalize invalid cover art durations",
        ),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art progress keeps stale position state",
        cover_art_progress_visibility.replace(
            "    id(cover_art_media_position) = 0.0f;\n",
            "",
        ),
        ("reset cover art position when duration becomes unavailable",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art progress discards fresh position before late duration",
        cover_art_progress_visibility.replace(
            "  const bool next_progress_available = espcontrol::cover_art::progress_available(next_duration);\n",
            "  const bool current_progress_available = espcontrol::cover_art::progress_available(id(cover_art_media_duration));\n"
            "  const bool next_progress_available = espcontrol::cover_art::progress_available(next_duration);\n",
        ),
        ("preserve fresh cover art position when duration arrives late",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art track change keeps stale duration",
        cover_art_progress_visibility.replace(
            "  invalidate_stale_media_duration();\n",
            "",
            1,
        ),
        ("mark stale cover art duration unavailable when media title changes",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art title change discards a fresh position",
        cover_art_progress_visibility.replace(
            "  invalidate_stale_media_duration();\n",
            "  invalidate_stale_media_duration();\n"
            "  id(cover_art_media_position) = 0.0f;\n",
            1,
        ),
        ("preserve fresh cover art position when title metadata arrives late",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art artist change keeps stale duration",
        cover_art_progress_visibility.replace(
            "handle_media_artist = [](esphome::StringRef artist) {\n"
            "  invalidate_stale_media_duration();\n",
            "handle_media_artist = [](esphome::StringRef artist) {\n",
            1,
        ),
        ("mark stale cover art duration unavailable when media artist changes",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art album change keeps stale duration",
        cover_art_progress_visibility.replace(
            "handle_media_album = [](esphome::StringRef album) {\n"
            "  invalidate_stale_media_duration();\n",
            "handle_media_album = [](esphome::StringRef album) {\n",
            1,
        ),
        ("mark stale cover art duration unavailable when media album changes",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art album change delays progress refresh",
        cover_art_progress_visibility.replace(
            "  id(cover_art_sync_track_text).execute();\n",
            "",
            1,
        ),
        ("refresh cover art progress immediately when media album changes",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art source change keeps stale duration",
        cover_art_progress_visibility.replace(
            "handle_media_source = [](esphome::StringRef source) {\n"
            "  invalidate_stale_media_duration();\n",
            "handle_media_source = [](esphome::StringRef source) {\n",
            1,
        ),
        ("mark stale cover art duration unavailable when media source changes",),
    )
    expect_cover_art_progress_visibility_errors(
        "cover art metadata discards a fresh duration",
        cover_art_progress_visibility.replace(
            "  if (!duration_callback_is_fresh) {\n",
            "  if (true) {\n",
            1,
        ),
        ("preserve fresh cover art duration when metadata arrives late",),
    )
    expect_image_card_entity_errors(
        "legacy camera-only image card guard",
        "inline bool image_card_entity_supported(const std::string &entity_id) {\n"
        "  return entity_id.rfind(\"camera.\", 0) == 0;\n"
        "}\n"
        "if (p.entity.rfind(\"camera.\", 0) != 0) {\n"
        "  ESP_LOGW(\"image_card\", \"Image card only supports camera entities: %s\", p.entity.c_str());\n"
        "}\n",
        (
            "support Home Assistant image entities in image cards",
            "use the shared image card entity-domain guard",
            "do not reject Home Assistant image entities as unavailable",
        ),
    )
    expect_image_card_entity_errors(
        "camera and image entities accepted",
        "inline bool image_card_entity_supported(const std::string &entity_id) {\n"
        "  return entity_id.rfind(\"camera.\", 0) == 0 || entity_id.rfind(\"image.\", 0) == 0;\n"
        "}\n"
        "if (!image_card_entity_supported(p.entity)) {\n"
        "  ESP_LOGW(\"image_card\", \"Image card only supports camera and image entities: %s\", p.entity.c_str());\n"
        "}\n",
        (),
    )
    expect_image_card_base_url_errors(
        "image card uses stale startup base URL",
        "std::string base_url;\n"
        "inline void image_card_handle_picture(ImageCardCtx *ctx, esphome::StringRef picture) {\n"
        "  std::string raw = string_ref_limited(picture, 4096);\n"
        "  std::string url = image_card_join_url(ctx->base_url, raw);\n"
        "}\n"
        "ctx->base_url = cfg.home_assistant_base_url ? cfg.home_assistant_base_url() : \"\";\n",
        (
            "keep image card Home Assistant base URL lookup live",
            "resolve image card base URL when entity_picture is handled",
            "do not rely only on the startup-time image card base URL",
        ),
    )
    expect_image_card_base_url_errors(
        "image card resolves live base URL",
        "std::function<std::string()> base_url_provider;\n"
        "std::string base_url;\n"
        "inline std::string image_card_base_url(ImageCardCtx *ctx) {\n"
        "  return ctx->base_url_provider ? ctx->base_url_provider() : ctx->base_url;\n"
        "}\n"
        "inline void image_card_handle_picture(ImageCardCtx *ctx, esphome::StringRef picture) {\n"
        "  std::string raw = string_ref_limited(picture, 4096);\n"
        "  std::string url = image_card_join_url(image_card_base_url(ctx), raw);\n"
        "}\n"
        "ctx->base_url = cfg.home_assistant_base_url ? cfg.home_assistant_base_url() : \"\";\n"
        "ctx->base_url_provider = cfg.home_assistant_base_url;\n",
        (),
    )
    expect_image_card_quality_errors(
        "image card modal only scales tile image",
        "inline void image_card_open_modal(ImageCardCtx *ctx) {\n"
        "  image_card_set_widget_source(ui.image_widget, ctx->image);\n"
        "  image_card_apply_modal_geometry(ctx);\n"
        "}\n",
        (
            "cap high-resolution image card modal downloads",
            "scale image card modal downloads to a display-appropriate size",
            "cancel in-flight image downloads before opening image card modals",
            "defer image downloads while image card modals are open",
            "detach image sources before deleting image card modals",
            "set image card download target size before requesting images",
            "use a separate modal image downloader for expanded image-card quality",
            "request expanded image-card downloads through the modal downloader",
            "swap expanded image cards to the modal-quality image after it downloads",
            "retain one shared modal image cache for instant reopen",
            "support six concurrent image cards on P4 displays",
            "check free memory before image-card downloads",
            "include PSRAM in image-card memory checks",
            "show a visible image-card limit message when downloaders run out",
            "keep modal-quality image refresh enabled on the 4.3-inch P4 screen",
            "size every image-card tile request to its on-screen bounds",
            "log image-card modal close events",
            "clean up partially-created image card modals",
            "keep image-card modal loading overlay centered",
            "show the cached image-card tile while modal-quality image loads",
            "keep an error state when image-card modals have no preview",
            "clip image card modal content to rounded panel corners",
            "preserve image card rounded corners while pressed",
            "apply image card corner clipping to the pressed state",
            "clear stale image-card tile buffers when card size changes",
            "keep small-display image card tile downloads sized to the tile",
        ),
    )
    expect_image_card_quality_errors(
        "image card modal requests capped image",
        "constexpr int IMAGE_CARD_MAX_CONTEXTS = 6;\n"
        "constexpr int IMAGE_CARD_MODAL_MAX_TARGET_SIDE_PX = 800;\n"
        "constexpr int IMAGE_CARD_CONSTRAINED_MODAL_MAX_TARGET_SIDE_PX = 320;\n"
        "constexpr size_t IMAGE_CARD_MEMORY_HEADROOM_BYTES = 96 * 1024;\n"
        "constexpr size_t IMAGE_CARD_CONSTRAINED_INTERNAL_FREE_BYTES = 40 * 1024;\n"
        "constexpr size_t IMAGE_CARD_CONSTRAINED_INTERNAL_LARGEST_BYTES = 24 * 1024;\n"
        "struct ImageCardModalCache {};\n"
        "inline ImageCardModalCache &image_card_modal_cache();\n"
        "inline bool image_card_retain_modal_cache() { return true; }\n"
        "inline void image_card_release_modal_cache(ArtworkImage *modal_image) { modal_image->release(); }\n"
        "inline lv_style_selector_t image_card_pressed_selector() { return LV_STATE_PRESSED; }\n"
        "inline void image_card_apply_corner_clip(lv_obj_t *obj, lv_coord_t radius) {}\n"
        "inline bool image_card_memory_available(ImageCardCtx *ctx, const char *stage,\n"
        "                                        int width, int height) {\n"
        "  size_t external_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);\n"
        "  image_pipeline_memory_failure(true, 0, 0, 0, external_largest, 0, 0, 0, 0, 0);\n"
        "  return external_largest > 0;\n"
        "}\n"
        "inline bool image_card_modal_refresh_supported() {\n"
        "  return true;\n"
        "}\n"
        "inline void image_card_limit_target_size(lv_coord_t source_width, lv_coord_t source_height,\n"
        "                                         int *target_width, int *target_height) {\n"
        "  image_pipeline_modal_max_target_side(false);\n"
        "}\n"
        "inline void image_card_layout_modal_loading(ImageCardCtx *ctx) {\n"
        "  lv_obj_set_size(ui.loading_widget, width, height);\n"
        "  lv_obj_align(icon, LV_ALIGN_CENTER, 0, -18);\n"
        "  lv_obj_align_to(label, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);\n"
        "}\n"
        "inline void image_card_request_source_url(ImageCardCtx *ctx) {\n"
        "  ctx->image->set_target_size(width, height);\n"
        "  image_card_tile_request_size(width, height, &request_width, &request_height);\n"
        "  ctx->url = image_card_sized_url(ctx->source_url, request_width, request_height);\n"
        "}\n"
        "inline void image_card_tile_request_size(lv_coord_t target_width, lv_coord_t target_height,\n"
        "                                        int *request_width, int *request_height) {\n"
        "  image_card_limit_target_size(target_width, target_height, request_width, request_height);\n"
        "}\n"
        "inline void image_card_refresh_tile_geometry(ImageCardCtx *ctx) {\n"
        "  image_card_schedule_source_refresh(ctx, 1, \"resized tile\");\n"
        "}\n"
        "inline void image_card_reset_resized_tile(ImageCardCtx *ctx) {\n"
        "  ctx->image->release();\n"
        "}\n"
        "inline void image_card_request_modal_source_url(ImageCardCtx *ctx) {\n"
        "  ctx->modal_image->request_update_url(ctx->modal_url, max_source_dim);\n"
        "}\n"
        "inline void image_card_show_modal_download_failure(ImageCardCtx *ctx) {\n"
        "  if (image_card_modal_has_preview(ctx)) {\n"
        "    image_card_hide_modal_loading(ctx);\n"
        "  } else {\n"
        "    image_card_show_modal_loading(ctx, \"Unavailable\");\n"
        "  }\n"
        "}\n"
        "inline void image_card_handle_modal_download_error(ImageCardCtx *ctx) {\n"
        "  image_card_show_modal_download_failure(ctx);\n"
        "}\n"
        "inline void image_card_abort_modal_open(ImageCardCtx *ctx, const char *reason) {\n"
        "  ESP_LOGW(\"image_card\", \"modal shell setup failed\");\n"
        "}\n"
        "inline void image_card_open_modal(ImageCardCtx *ctx) {\n"
        "  ESP_LOGW(\"image_card\", \"modal shell setup failed\");\n"
        "  lv_obj_set_style_clip_corner(ui.panel, true, LV_PART_MAIN);\n"
        "  image_card_show_modal_image(ctx, ctx->image);\n"
        "  image_card_queue_modal_source_request(ctx);\n"
        "  image_card_clear_widget_source(ui.image_widget);\n"
        "  image_card_set_widget_source(ui.image_widget, ctx->modal_image);\n"
        "  if (image_card_modal_active_for(ctx)) {\n"
        "    ESP_LOGD(\"image_card\", \"Deferring image refresh while modal is open for %s\", ctx->entity_id.c_str());\n"
        "  }\n"
        "  ctx->image->cancel_update();\n"
        "  ESP_LOGI(\"image_card\", \"Closing image modal for %s\", ctx->entity_id.c_str());\n"
        "}\n"
        "inline bool bind_image_card(BtnSlot &s, const ParsedCfg &p, const GridConfig &cfg,\n"
        "                            const ThemePalette &palette) {\n"
        "  lv_obj_t *loading = image_card_loading_widget(widget);\n"
        "  image_card_set_loading_state(loading, \"Too many\");\n"
        "  return true;\n"
        "}\n",
        (),
    )
    expect_image_card_startup_errors(
        "image card missing startup reconnect refresh",
        "inline void image_card_request_picture(ImageCardCtx *ctx) {\n"
        "  bool requested = ha_get_attribute(ctx->entity_id, std::string(\"entity_picture\"), callback);\n"
        "}\n"
        "inline void image_card_request_source_url(ImageCardCtx *ctx) {\n"
        "  ctx->url = image_card_cache_bust_url(ctx->source_url);\n"
        "}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        refresh_weather_forecast_cards();\n",
        (
            "refresh image cards when Home Assistant reconnects",
            "retry media cover artwork when Home Assistant reconnects",
            "retry image-card startup quickly after Home Assistant API connects",
            "arm image-card refresh from the Home Assistant API connection",
            "request image-card attributes once the Home Assistant API is connected",
            "refresh image cards when the camera/image entity state changes",
            "ignore stale image-card callbacks after grid rebuild",
            "ignore stale image-card entity_picture callbacks after grid rebuild",
            "request display-sized Home Assistant image card downloads",
            "request bounded Home Assistant image card proxy downloads",
            "recognize Home Assistant camera and image proxy URLs",
            "start image-card refresh when Home Assistant API connects",
            "refresh image cards through Home Assistant connect retries",
        ),
    )
    expect_image_card_startup_errors(
        "image card refreshes on Home Assistant reconnect",
        "constexpr uint32_t IMAGE_CARD_API_RETRY_INTERVAL_MS = 250;\n"
        "inline bool image_card_home_assistant_proxy_url(const std::string &url) {\n"
        "  return url.find(\"/api/camera_proxy/\") != std::string::npos ||\n"
        "         url.find(\"/api/image_proxy/\") != std::string::npos;\n"
        "}\n"
        "inline std::string image_card_entity_proxy_path(const std::string &entity_id) {\n"
        "  if (entity_id.rfind(\"camera.\", 0) == 0) return \"/api/camera_proxy/\" + entity_id;\n"
        "  if (entity_id.rfind(\"image.\", 0) == 0) return \"/api/image_proxy/\" + entity_id;\n"
        "  return \"\";\n"
        "}\n"
        "inline void image_card_handle_picture(ImageCardCtx *ctx) {\n"
        "  ESP_LOGD(\"image_card\", \"Waiting for Home Assistant base URL before loading %s\", ctx->entity_id.c_str());\n"
        "}\n"
        "inline void image_card_request_picture(ImageCardCtx *ctx) {\n"
        "  if (!ha_api_connected()) return;\n"
        "  ha_get_attribute(ctx->entity_id, std::string(\"access_token\"), callback);\n"
        "  image_card_proxy_path_with_token(proxy_path, token);\n"
        "  ha_get_attribute(ctx->entity_id, std::string(\"entity_picture\"), callback);\n"
        "}\n"
        "inline void image_card_request_media_artwork(ImageCardCtx *ctx) {\n"
        "  uint8_t request_mask = artwork_source_request_mask(ctx->media_artwork_retry_mask);\n"
        "  bool remote_queued = ha_get_attribute(ctx->entity_id, std::string(\"entity_picture\"), callback);\n"
        "  bool local_queued = ha_get_attribute(ctx->entity_id, std::string(\"entity_picture_local\"), callback);\n"
        "  ctx->media_artwork_retry_mask = artwork_source_failed_mask(request_mask, remote_queued, local_queued);\n"
        "  if (ctx->media_artwork_retry_mask != 0) image_card_schedule_picture_retry(ctx, 250);\n"
        "}\n"
        "inline void image_card_request_current_picture(ImageCardCtx *ctx) {\n"
        "  if (ctx->media_artwork) {\n"
        "    image_card_request_media_artwork(ctx);\n"
        "  } else {\n"
        "    image_card_request_picture(ctx);\n"
        "  }\n"
        "}\n"
        "inline void image_card_refresh_current_picture(ImageCardCtx *ctx) {\n"
        "  if (ctx->media_artwork) {\n"
        "    ctx->media_artwork_retry_mask = 0;\n"
        "    ctx->media_artwork_sources.clear();\n"
        "  }\n"
        "  image_card_request_current_picture(ctx);\n"
        "}\n"
        "inline void image_card_handle_picture(ImageCardCtx *ctx) {\n"
        "  if (artwork_picture_response_clears_retry(ctx->media_artwork, ctx->media_artwork_retry_mask)) {\n"
        "    ctx->next_picture_retry_ms = 0;\n"
        "  }\n"
        "}\n"
        "inline bool image_card_context_current(ImageCardCtx *ctx,\n"
        "                                       const std::string &entity_id,\n"
        "                                       uint32_t generation) {\n"
        "  return generation == ha_subscription_generation();\n"
        "}\n"
        "inline void subscribe_image_card_entity_state(ImageCardCtx *ctx,\n"
        "                                              const std::string &entity_id) {\n"
        "  ha_subscribe_state(entity_id, callback);\n"
        "}\n"
        "inline bool bind_image_card(BtnSlot &s, const ParsedCfg &p, const GridConfig &cfg) {\n"
        "  const std::string image_card_entity_id = p.entity;\n"
        "  const uint32_t image_card_generation = ha_subscription_generation();\n"
        "  ha_subscribe_attribute(image_card_entity_id, std::string(\"entity_picture\"), callback);\n"
        "  image_card_context_current(ctx, image_card_entity_id, image_card_generation);\n"
        "  return true;\n"
        "}\n"
        "inline void image_card_request_source_url(ImageCardCtx *ctx) {\n"
        "  image_card_tile_request_size(width, height, &request_width, &request_height);\n"
        "  ctx->url = image_card_sized_url(ctx->source_url, request_width, request_height);\n"
        "}\n"
        "inline void image_card_tile_request_size(lv_coord_t target_width, lv_coord_t target_height,\n"
        "                                        int *request_width, int *request_height) {\n"
        "  image_card_limit_target_size(target_width, target_height, request_width, request_height);\n"
        "}\n"
        "inline void refresh_image_cards() {\n"
        "  if (!ha_api_connected()) return;\n"
        "  image_card_refresh_current_picture(ctx);\n"
        "}\n"
        "inline void image_card_refresh_due() {\n"
        "  image_card_request_current_picture(ctx);\n"
        "}\n",
        "api:\n"
        "  on_client_connected:\n"
        "    - lambda: |-\n"
        "        if (is_home_assistant && ha_api_connected()) {\n"
        "        refresh_image_cards();\n"
        "        }\n"
        "    - delay: 2s\n"
        "    - lambda: |-\n"
        "        refresh_image_cards();\n"
        "    - delay: 8s\n"
        "    - lambda: |-\n"
        "        refresh_image_cards();\n"
        "    - delay: 20s\n"
        "    - lambda: |-\n"
        "        refresh_image_cards();\n",
        (),
    )
    valid_shared_wake_guard_widget = (
        "lvgl:\n"
        "  top_layer:\n"
        "    widgets:\n"
        "      - obj:\n"
        "          id: screensaver_wake_touch_guard\n"
        "          width: 100%\n"
        "          height: 100%\n"
        "          clickable: true\n"
        "          hidden: true\n"
    )
    valid_shared_wake_guard_script = (
        "  - id: screensaver_wake_touch_block\n"
        "    then:\n"
        "      - globals.set:\n"
        "          id: screensaver_wake_touch_guard_active\n"
        "          value: 'true'\n"
        "      - lambda: 'screensaver_fill_screen(id(screensaver_wake_touch_guard));'\n"
        "      - lvgl.widget.show: screensaver_wake_touch_guard\n"
        "      - lambda: 'lv_obj_move_foreground(id(screensaver_wake_touch_guard));'\n"
        "      - wait_until:\n"
        "          condition:\n"
        "            lambda: 'return lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED;'\n"
        "          timeout: 150ms\n"
        "      - wait_until:\n"
        "          condition:\n"
        "            lambda: 'return lv_indev_get_state(indev) != LV_INDEV_STATE_PRESSED;'\n"
        "          timeout: 2s\n"
        "      - delay: 250ms\n"
        "      - globals.set:\n"
        "          id: screensaver_wake_touch_guard_active\n"
        "          value: 'false'\n"
        "      - lvgl.widget.hide: screensaver_wake_touch_guard\n"
    )
    valid_shared_wake_flow = (
        "globals:\n"
        "  - id: screensaver_wake_restore_pending\n"
        "    type: bool\n"
        "    initial_value: 'false'\n"
        "script:\n"
        "  - id: screensaver_wake\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(screensaver_wake_restore_pending) =\n"
        "              !id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::ACTIVE);\n"
        "          id(screensaver_wake_touch_guard_skip_once) =\n"
        "              id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "              id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::DISPLAY_OFF);\n"
        "      - script.execute: screensaver_wake_touch_block\n"
        "      - lambda: |-\n"
        "          id(display_mode_controller).clear(espcontrol::DisplayRequestSource::IDLE_TIMER);\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              const bool restore_pending = id(screensaver_wake_restore_pending);\n"
        "              id(screensaver_wake_restore_pending) = false;\n"
        "              return restore_pending ||\n"
        "                  !id(display_mode_controller).current_mode_is(espcontrol::DisplayMode::ACTIVE);\n"
        "          then:\n"
        "            - if:\n"
        "                condition:\n"
        "                  lambda: |-\n"
        "                    bool keep_wake_guard = id(screensaver_wake_touch_guard_skip_once);\n"
        "                    id(screensaver_wake_touch_guard_skip_once) = false;\n"
        "                    return keep_wake_guard;\n"
        "                else:\n"
        "                  - globals.set:\n"
        "                      id: screensaver_wake_touch_guard_active\n"
        "                      value: 'false'\n"
        + valid_shared_wake_guard_script
    )
    expect_screensaver_wake_guard_errors(
        "simple timed global guard is rejected",
        "globals:\n"
        "  - id: screensaver_wake_restore_pending\n"
        "    type: bool\n"
        "    initial_value: 'false'\n"
        "script:\n"
        "  - id: screensaver_wake\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          id(screensaver_wake_restore_pending) =\n"
        "              !id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::ACTIVE);\n"
        "          id(screensaver_wake_touch_guard_skip_once) =\n"
        "              id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) ||\n"
        "              id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::DISPLAY_OFF);\n"
        "      - script.execute: screensaver_wake_touch_block\n"
        "      - lambda: |-\n"
        "          id(display_mode_controller).clear(espcontrol::DisplayRequestSource::IDLE_TIMER);\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              const bool restore_pending = id(screensaver_wake_restore_pending);\n"
        "              id(screensaver_wake_restore_pending) = false;\n"
        "              return restore_pending ||\n"
        "                  !id(display_mode_controller).current_mode_is(espcontrol::DisplayMode::ACTIVE);\n"
        "          then:\n"
        "            - globals.set:\n"
        "                id: screensaver_wake_touch_guard_active\n"
        "                value: 'true'\n"
        "            - script.execute: screensaver_wake_touch_guard_clear\n",
        valid_shared_wake_guard_widget,
        "",
        (
            "do not block the first button tap after normal screensaver wake",
            "do not arm a delayed wake guard clear for normal screensaver wake",
            "missing shared screensaver_wake_touch_block script",
        ),
    )
    expect_screensaver_wake_guard_errors(
        "shared release guard covers Display Off and Cover Art",
        valid_shared_wake_flow,
        valid_shared_wake_guard_widget,
        "script:\n  - id: cover_art_apply_responsive_layout\n",
        (),
    )
    expect_screensaver_wake_guard_errors(
        "Cover Art-only guard does not cover Display Off",
        valid_shared_wake_flow.replace(
            " ||\n              id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::DISPLAY_OFF)",
            "",
        ),
        valid_shared_wake_guard_widget,
        "",
        ("pre-wake Cover Art and Display Off modes",),
    )
    expect_screensaver_wake_guard_errors(
        "release guard accepts the next deliberate tap",
        valid_shared_wake_flow.replace(
            "      - globals.set:\n"
            "          id: screensaver_wake_touch_guard_active\n"
            "          value: 'false'\n"
            "      - lvgl.widget.hide: screensaver_wake_touch_guard\n",
            "",
        ),
        valid_shared_wake_guard_widget,
        "",
        (
            "accept the first deliberate tap after wake-touch release",
            "hide the shared wake guard after touch release",
        ),
    )
    expect_screensaver_wake_guard_errors(
        "stuck touch retains a bounded fallback",
        valid_shared_wake_flow.replace("          timeout: 2s\n", ""),
        valid_shared_wake_guard_widget,
        "",
        ("clear the shared wake guard after a stuck touch timeout",),
    )
    expect_clock_screensaver_overlay_errors(
        "clock screensaver closes active UI before showing",
        "script:\n"
        "  - id: screensaver_sleep_timer\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          media_volume_hide_modal();\n"
        "          climate_control_hide_modal();\n"
        "      - script.execute: hide_cover_art_view\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return screensaver_action_clock_mode(id(screensaver_action).current_option());'\n"
        "          then:\n"
        "            - script.execute: show_clock_view\n"
        "  - id: show_clock_view\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          hide_clock_bar_top_layer_widgets(nullptr, 0, nullptr, nullptr);\n"
        "          lv_obj_clear_flag(id(clock_screensaver), LV_OBJ_FLAG_HIDDEN);\n"
        "          lv_obj_move_foreground(id(clock_screensaver));\n"
        "  - id: clock_screensaver_keep_on_top\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          if (!id(display_mode_controller).current_mode_is(espcontrol::DisplayMode::CLOCK)) return;\n"
        "          hide_clock_bar_top_layer_widgets(nullptr, 0, nullptr, nullptr);\n"
        "          refresh_screensaver_fullscreen(id(clock_screensaver), id(dim_screensaver_touch_guard));\n"
        "          lv_obj_move_foreground(id(clock_screensaver));\n"
        "  - id: show_dimmed_view\n"
        "    then:\n"
        "      - lambda: 'lv_obj_move_foreground(id(dim_screensaver_touch_guard));'\n"
        "interval:\n"
        "  - interval: 1s\n"
        "    then:\n"
        "      - script.execute: clock_screensaver_keep_on_top\n",
        ("overlay the existing UI without closing it",),
    )
    expect_clock_screensaver_overlay_errors(
        "clock screensaver overlays active UI",
        "script:\n"
        "  - id: screensaver_sleep_timer\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return screensaver_action_clock_mode(id(screensaver_action).current_option());'\n"
        "          then:\n"
        "            - script.execute: show_clock_view\n"
        "          else:\n"
        "            - lambda: |-\n"
        "                media_volume_hide_modal();\n"
        "            - script.execute: hide_cover_art_view\n"
        "  - id: show_clock_view\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          hide_clock_bar_top_layer_widgets(nullptr, 0, nullptr, nullptr);\n"
        "          lv_obj_clear_flag(id(clock_screensaver), LV_OBJ_FLAG_HIDDEN);\n"
        "          lv_obj_move_foreground(id(clock_screensaver));\n"
        "  - id: clock_screensaver_keep_on_top\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          if (!id(display_mode_controller).current_mode_is(espcontrol::DisplayMode::CLOCK)) return;\n"
        "          hide_clock_bar_top_layer_widgets(nullptr, 0, nullptr, nullptr);\n"
        "          refresh_screensaver_fullscreen(id(clock_screensaver), id(dim_screensaver_touch_guard));\n"
        "          lv_obj_move_foreground(id(clock_screensaver));\n"
        "  - id: show_dimmed_view\n"
        "    then:\n"
        "      - lambda: 'lv_obj_move_foreground(id(dim_screensaver_touch_guard));'\n"
        "interval:\n"
        "  - interval: 1s\n"
        "    then:\n"
        "      - script.execute: clock_screensaver_keep_on_top\n",
        (),
    )
    expect_clock_screensaver_overlay_errors(
        "clock screensaver stays behind top layer UI",
        "script:\n"
        "  - id: screensaver_sleep_timer\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return screensaver_action_clock_mode(id(screensaver_action).current_option());'\n"
        "          then:\n"
        "            - script.execute: show_clock_view\n"
        "  - id: show_clock_view\n"
        "    then:\n"
        "      - lvgl.widget.show: clock_screensaver\n",
        ("raise the clock screensaver above existing top-layer UI",),
    )
    valid_schedule_screensaver_override = (
        "script:\n"
        "  - id: screensaver_idle_check\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              if (screen_schedule_night_active(\n"
        "                    id(screen_schedule_trigger).state,\n"
        "                    id(schedule_enabled).state,\n"
        "                    id(presence_detected),\n"
        "                    now.is_valid(),\n"
        "                    now.is_valid() ? now.hour : 0,\n"
        "                    (int) id(schedule_on_hour).state,\n"
        "                    (int) id(schedule_off_hour).state)) {\n"
        "                id(screen_schedule_check).execute();\n"
        "                return false;\n"
        "              }\n"
        "              return id(screensaver_mode).state == \"timer\";\n"
        "  - id: screensaver_presence_wake\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              if (screen_schedule_waiting_for_time(\n"
        "                    id(screen_schedule_trigger).state,\n"
        "                    id(schedule_enabled).state,\n"
        "                    now.is_valid()) ||\n"
        "                  screen_schedule_night_active(\n"
        "                    id(screen_schedule_trigger).state,\n"
        "                    id(schedule_enabled).state,\n"
        "                    id(presence_detected),\n"
        "                    now.is_valid(),\n"
        "                    now.is_valid() ? now.hour : 0,\n"
        "                    (int) id(schedule_on_hour).state,\n"
        "                    (int) id(schedule_off_hour).state)) {\n"
        "                id(screen_schedule_check).execute();\n"
        "                return false;\n"
        "              }\n"
        "              return id(screensaver_mode).state == \"sensor\";\n"
        "          then:\n"
        "            - if:\n"
        "                condition:\n"
        "                  lambda: |-\n"
        "                    if (!id(schedule_enabled).state) return true;\n"
        "                    return screen_schedule_normal_active(\n"
        "                      id(screen_schedule_trigger).state,\n"
        "                      id(schedule_enabled).state,\n"
        "                      id(presence_detected),\n"
        "                      now.is_valid(),\n"
        "                      now.is_valid() ? now.hour : 0,\n"
        "                      (int) id(schedule_on_hour).state,\n"
        "                      (int) id(schedule_off_hour).state);\n"
        "                then:\n"
        "            - script.execute: screensaver_wake\n"
        "  - id: screensaver_presence_sleep\n"
        "    then:\n"
        "      - script.execute: screen_schedule_check\n"
        "      - script.wait: screen_schedule_check\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: |-\n"
        "              return id(screensaver_mode).state == \"sensor\" &&\n"
        "                     !screen_schedule_sensor_trigger(\n"
        "                         id(screen_schedule_trigger).state);\n"
        "          then:\n"
        "            - script.execute: screensaver_sleep_sensor\n"
        "  - id: backlight_schedule_display_off\n"
        "    then:\n"
        "      - script.stop: cover_art_delay_timer\n"
        "      - script.execute: hide_cover_art_view\n"
    )
    valid_schedule_sleep_order = (
        "script:\n"
        "  - id: screen_schedule_sleep\n"
        "    then:\n"
        "      - globals.set:\n"
        "          id: screen_schedule_asleep\n"
        "          value: 'true'\n"
        "      - lambda: |-\n"
        "          id(display_mode_controller).request(\n"
        "              espcontrol::DisplayRequestSource::SCREEN_SCHEDULE,\n"
        "              espcontrol::DisplayMode::DISPLAY_OFF);\n"
        "      - script.execute: display_mode_reconcile\n"
    )
    expect_screen_schedule_screensaver_override_errors(
        "night schedule overrides timer and sensor screensaver",
        valid_schedule_screensaver_override,
        (),
        valid_schedule_sleep_order,
    )
    typed_presence_wake = valid_schedule_screensaver_override.replace(
        "            - script.execute: screensaver_wake\n",
        "            - script.execute: screen_schedule_check\n"
        "            - lambda: 'return !screen_schedule_sensor_trigger(id(screen_schedule_trigger).state);'\n"
        "            - lambda: 'return espcontrol::presence_can_wake_display(transition);'\n"
        "            - script.execute: screensaver_wake\n"
        "            - script.execute: display_mode_clear_automatic\n",
        1,
    )
    expect_screen_schedule_screensaver_override_errors(
        "typed presence wake clears hidden automatic sleep",
        typed_presence_wake,
        (),
        valid_schedule_sleep_order,
    )
    expect_screen_schedule_screensaver_override_errors(
        "typed presence wake leaves hidden automatic sleep stale",
        typed_presence_wake.replace(
            "            - script.execute: display_mode_clear_automatic\n", "", 1
        ),
        ("clear stale automatic sleep",),
        valid_schedule_sleep_order,
    )
    migrated_schedule_reconcile = (
        valid_schedule_screensaver_override
        + "  - id: display_mode_reconcile\n"
        "    then:\n"
        "      - lambda: |-\n"
        "          if (schedule_night) {\n"
        "            controller.request(espcontrol::DisplayRequestSource::SCREEN_SCHEDULE,\n"
        "                               espcontrol::DisplayMode::CLOCK);\n"
        "            if (id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART)) {\n"
        "              id(display_mode_controller).target_mode_is(espcontrol::DisplayMode::COVER_ART) = false;\n"
        "              id(hide_cover_art_view).execute();\n"
        "            }\n"
        "          }\n"
    )
    expect_screen_schedule_screensaver_override_errors(
        "migrated schedule clears active cover art",
        migrated_schedule_reconcile,
        (),
        valid_schedule_sleep_order,
    )
    expect_screen_schedule_screensaver_override_errors(
        "migrated schedule leaves active cover art visible",
        migrated_schedule_reconcile.replace(
            "              id(hide_cover_art_view).execute();\n", "", 1
        ),
        ("scheduled night modes should clear active cover art",),
        valid_schedule_sleep_order,
    )
    expect_screen_schedule_screensaver_override_errors(
        "schedule display-off reconciles before publishing its marker",
        valid_schedule_screensaver_override,
        ("set the schedule-asleep marker before reconciling display-off",),
        valid_schedule_sleep_order.replace(
            "      - globals.set:\n"
            "          id: screen_schedule_asleep\n"
            "          value: 'true'\n",
            "",
            1,
        ),
    )
    expect_screen_schedule_screensaver_override_errors(
        "timer screensaver bypasses night schedule",
        valid_schedule_screensaver_override.replace(
            "                id(screen_schedule_check).execute();\n"
            "                return false;\n"
            "              }\n"
            "              return id(screensaver_mode).state == \"timer\";\n",
            "                std::string mode = id(schedule_mode).current_option();\n"
            "                if (screen_schedule_clock_mode(mode)) return false;\n"
            "              }\n"
            "              return id(screensaver_mode).state == \"timer\";\n",
            1,
        ),
        ("override timer screensaver actions",),
    )
    expect_screen_schedule_screensaver_override_errors(
        "sensor screensaver wake bypasses night schedule",
        valid_schedule_screensaver_override.replace(
            "              if (screen_schedule_waiting_for_time(\n"
            "                    id(screen_schedule_trigger).state,\n"
            "                    id(schedule_enabled).state,\n"
            "                    now.is_valid()) ||\n"
            "                  screen_schedule_night_active(\n"
            "                    id(screen_schedule_trigger).state,\n"
            "                    id(schedule_enabled).state,\n"
            "                    id(presence_detected),\n"
            "                    now.is_valid(),\n"
            "                    now.is_valid() ? now.hour : 0,\n"
            "                    (int) id(schedule_on_hour).state,\n"
            "                    (int) id(schedule_off_hour).state)) {\n"
            "                id(screen_schedule_check).execute();\n"
            "                return false;\n"
            "              }\n",
            "",
            1,
        ),
        ("override sensor screensaver wake",),
    )
    expect_screen_schedule_screensaver_override_errors(
        "sensor screensaver wake ignores disabled schedule",
        valid_schedule_screensaver_override.replace(
            "                    if (!id(schedule_enabled).state) return true;\n",
            "",
            1,
        ),
        ("wake when the screen schedule is disabled",),
    )
    expect_artwork_image_auth_errors(
        "local artwork image request uses Basic auth",
        "std::shared_ptr<http_request::HttpContainer> ArtworkImage::get_local_idf_(\n"
        "    const std::string &url, const std::vector<http_request::Header> &headers) {\n"
        "  esp_http_client_config_t config = {};\n"
        "  config.auth_type = HTTP_AUTH_TYPE_BASIC;\n"
        "  if (container->status_code <= 0 && is_ha_media_proxy_url(url)) {\n"
        "    ESP_LOGW(TAG, \"Home Assistant media proxy returned an unknown HTTP status; trying artwork bytes anyway\");\n"
        "    container->status_code = HTTP_CODE_OK;\n"
        "  }\n"
        "}\n",
        (
            "keep local Home Assistant image proxy requests off HTTP Basic auth",
            "explicitly disable HTTP auth for local artwork requests",
        ),
    )
    expect_artwork_image_auth_errors(
        "local artwork image request disables HTTP auth",
        "std::shared_ptr<http_request::HttpContainer> ArtworkImage::get_local_idf_(\n"
        "    const std::string &url, const std::vector<http_request::Header> &headers) {\n"
        "  esp_http_client_config_t config = {};\n"
        "  config.auth_type = HTTP_AUTH_TYPE_NONE;\n"
        "  if (container->status_code <= 0 && is_ha_media_proxy_url(url)) {\n"
        "    ESP_LOGW(TAG, \"Home Assistant media proxy returned an unknown HTTP status; trying artwork bytes anyway\");\n"
        "    container->status_code = HTTP_CODE_OK;\n"
        "  }\n"
        "}\n"
        "void ArtworkImage::end_connection_() {\n"
        "  this->download_buffer_.shrink_to(0);\n"
        "}\n",
        (),
    )
    expect_artwork_image_auth_errors(
        "local artwork image request rejects unknown HA media proxy status",
        "std::shared_ptr<http_request::HttpContainer> ArtworkImage::get_local_idf_(\n"
        "    const std::string &url, const std::vector<http_request::Header> &headers) {\n"
        "  esp_http_client_config_t config = {};\n"
        "  config.auth_type = HTTP_AUTH_TYPE_NONE;\n"
        "}\n",
        ("allow Home Assistant media proxy artwork to fall back to image-byte detection",),
    )
    expect_climate_step_errors(
        "climate ignores whole-number display step",
        "constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 5;\n"
        "inline int climate_round_to_step(ClimateControlCtx *ctx, int value) {\n"
        "  int step = ctx->step_tenths;\n"
        "  int base = ctx->min_tenths;\n"
        "  return value;\n"
        "}\n"
        "inline void climate_control_open_modal(ClimateControlCtx *ctx) {\n"
        "  climate_selected_target(ui.active) - ui.active->step_tenths;\n"
        "  climate_selected_target(ui.active) + ui.active->step_tenths;\n"
        "}\n",
        (
            "keep climate temperature changes at a display-appropriate minimum",
            "round climate targets using the display-appropriate minimum step",
            "round whole-number climate targets to whole-degree boundaries",
            "use the display-appropriate minimum step for the climate minus button",
            "use the display-appropriate minimum step for the climate plus button",
        ),
    )
    expect_climate_step_errors(
        "climate uses configured step increment",
        "constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 5;\n"
        "constexpr int CLIMATE_WHOLE_NUMBER_STEP_TENTHS = 10;\n"
        "int configured_step_tenths = CLIMATE_WHOLE_NUMBER_STEP_TENTHS;\n"
        "inline int climate_effective_step_tenths(ClimateControlCtx *ctx) {\n"
        "  if (!ctx) return CLIMATE_DEFAULT_STEP_TENTHS;\n"
        "  if (ctx->configured_step_tenths == CLIMATE_DEFAULT_STEP_TENTHS ||\n"
        "      ctx->configured_step_tenths == CLIMATE_WHOLE_NUMBER_STEP_TENTHS)\n"
        "    return ctx->configured_step_tenths;\n"
        "  return CLIMATE_WHOLE_NUMBER_STEP_TENTHS;\n"
        "}\n"
        "inline int climate_round_to_step(ClimateControlCtx *ctx, int value) {\n"
        "  int step = climate_effective_step_tenths(ctx);\n"
        "  int base = ctx->precision <= 0 ? 0 : ctx->min_tenths;\n"
        "  return value + step;\n"
        "}\n"
        "inline ClimateControlCtx *create_climate_control_context(const ParsedCfg &p) {\n"
        "  ctx->configured_step_tenths = normalize_climate_temperature_step(\n"
        "    cfg_option_value(p.options, \"temperature_step\")) == \"0.5\"\n"
        "      ? CLIMATE_DEFAULT_STEP_TENTHS\n"
        "      : CLIMATE_WHOLE_NUMBER_STEP_TENTHS;\n"
        "}\n"
        "inline void climate_control_open_modal(ClimateControlCtx *ctx) {\n"
        "  climate_selected_target(ui.active) - climate_effective_step_tenths(ui.active);\n"
        "  climate_selected_target(ui.active) + climate_effective_step_tenths(ui.active);\n"
        "}\n",
        (),
    )
    expect_s3_api_errors(
        "low S3 API queue",
        "api:\n  max_connections: 3\n  max_send_queue: 8\n",
        ("keep the S3 native API send queue high enough",),
    )
    expect_s3_api_errors(
        "low S3 API connections",
        "api:\n  max_connections: 2\n  max_send_queue: 12\n",
        ("keep enough S3 native API slots",),
    )
    expect_s3_api_errors(
        "S3 todo enabled",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_TODO_LITE=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("keep the S3 todo list disabled",),
    )
    expect_todo_disabled_errors(
        "todo enabled on one device",
        {
            "devices/a/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n",
            "devices/b/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_TODO_LITE=1\"\n",
        },
        ("keep the todo list disabled",),
    )
    expect_todo_disabled_errors(
        "todo disabled on all devices",
        {
            "devices/a/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n",
            "devices/b/device/device.yaml": "esphome:\n  platformio_options:\n    build_flags:\n"
            "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n",
        },
        (),
    )
    expect_s3_api_errors(
        "S3 includes navigate API package",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("omit the Home Assistant navigate API action on S3",),
        s3_packages_text="packages:\n  api_navigate: !include ../../common/device/api_navigate.yaml\n",
    )
    expect_s3_api_errors(
        "navigate action left in shared core",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("keep the navigate action out of core_infra",),
        core_text="api:\n  actions:\n    - action: navigate\n",
    )
    expect_s3_api_errors(
        "P4 package missing navigate API package",
        "esphome:\n  platformio_options:\n    build_flags:\n"
        "      - \"-DESPCONTROL_DISABLE_TODO=1\"\n"
        "api:\n  max_connections: 3\n  max_send_queue: 12\n",
        ("include the dedicated Home Assistant navigate API package",),
        extra_packages={"esp32-p4-86": "packages:\n  device: !include device/device.yaml\n"},
    )
    expect_navigation_target_errors(
        "home card navigation targets",
        "struct NavigationHomeTargetEntry {};\n"
        "inline auto navigation_home_targets() {}\n"
        "inline void navigation_find_label_target() { navigation_home_targets(); entry.display_order < best->display_order; }\n"
        "inline void navigation_find_slot_target() { entry.slot == slot; }\n"
        "inline bool navigation_is_voice_target() { return normalized == \"device_volume\"; }\n"
        "inline bool navigation_has_home_label_target() {}\n"
        "inline void navigation_activate_home_target() { navigation_return_home(main_page_obj); handle_button_click(target->config, target->slot, target->button); }\n"
        "inline void espcontrol_navigate() { normalized == \"home\" || normalized == \"main\"; }\n",
        "navigation_clear_home_targets();\n"
        "navigation_register_home_target(idx, pos, p.label, scfg, s.btn);\n"
        "navigation_clear_home_targets();\n"
        "navigation_register_home_target(idx, pos, p.label, s.config->state, s.btn);\n",
        "inline bool navigation_driver_own_subpage() { navigation_register_subpage( }\n",
        "if (navigation_is_voice_target(target) && !navigation_has_home_label_target(target)) { ${navigate_voice_target_code} } else { espcontrol_navigate(target, id(main_page)->obj); }\n",
        {
            "esp32-p4-86": "navigate_voice_target_code: |-\n  if (id(voice_services_enabled).state) { id(open_device_volume_control).execute(); }\n",
            "future-voice-panel": "navigate_voice_target_code: |-\n  if (id(voice_services_enabled).state) { id(open_device_volume_control).execute(); }\n",
            "other-p4": "navigate_voice_target_code: |-\n  ESP_LOGW(\"navigation\", \"Voice volume target is not available on this device\");\n",
        },
        ("esp32-p4-86", "future-voice-panel"),
        (),
    )
    expect_connectivity_api_errors(
        "raw api connected navigation",
        "wifi:\n"
        "  on_connect:\n"
        "    - if:\n"
        "        condition:\n"
        "          api.connected:\n"
        "api:\n"
        "  on_client_connected:\n"
        "    - script.execute: navigate_after_api\n",
        ("wait for Home Assistant state subscription", "only navigate after a Home Assistant state connection"),
    )
    expect_connectivity_api_errors(
        "home assistant state connected navigation",
        "wifi:\n"
        "  on_connect:\n"
        "    - if:\n"
        "        condition:\n"
        "          lambda: 'return ha_api_state_connected();'\n"
        "api:\n"
        "  on_client_connected:\n"
        "    - script.execute: ha_restore_after_api\n"
        "script:\n"
        "  - id: ha_restore_after_api\n"
        "    mode: restart\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return ha_api_connected();'\n"
        "          then:\n"
        "            - if:\n"
        "                condition:\n"
        "                  lambda: 'return lv_scr_act() == id(ha_setup_page)->obj;'\n"
        "                then:\n"
        "                  - script.execute: navigate_after_api\n",
        (),
    )
    expect_connectivity_api_errors(
        "removed home assistant reconnect screen",
        "wifi:\n"
        "  on_connect:\n"
        "    - if:\n"
        "        condition:\n"
        "          lambda: 'return ha_api_state_connected();'\n"
        "api:\n"
        "  on_client_connected:\n"
        "    - script.stop: ha_reconnect_flow\n"
        "    - script.execute: ha_restore_after_api\n"
        "script:\n"
        "  - id: ha_restore_after_api\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return ha_api_connected();'\n"
        "          then:\n"
        "            - if:\n"
        "                condition:\n"
        "                  lambda: 'return lv_scr_act() == id(ha_setup_page)->obj;'\n"
        "                then:\n"
        "                  - script.execute: navigate_after_api\n"
        "  - id: ha_reconnect_flow\n"
        "    then:\n"
        "      - lambda: 'lv_label_set_text(id(ha_setup_title), espcontrol_i18n(\"Connecting to\\nHome Assistant\"));'\n"
        "      - lvgl.page.show: ha_setup_page\n",
        ("keep the current display visible",),
    )
    print("Firmware Home Assistant binding self-tests passed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run guardrail self-tests")
    args = parser.parse_args()
    return run_self_test() if args.self_test else run_scan()


if __name__ == "__main__":
    raise SystemExit(main())
